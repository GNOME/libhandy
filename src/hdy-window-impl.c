/*
 * Copyright (C) 2019 Alexander Mikhaylenko <alexm@gnome.org>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"

#include "hdy-window-impl-private.h"

struct _HdyWindowImpl
{
  GObject parent;

  GtkWindow *window;
  GtkWindowClass *klass;

  GtkWidget *content;
  cairo_surface_t *mask;
  gint last_width;
  gint last_height;
  gint last_border_radius;

  GtkStyleContext *decoration_context;
  GtkStyleContext *overlay_context;
};

G_DEFINE_TYPE (HdyWindowImpl, hdy_window_impl, G_TYPE_OBJECT);

static void
ensure_content (HdyWindowImpl *self)
{
  if (self->content)
    return;

  self->content = gtk_event_box_new ();
  gtk_widget_set_vexpand (self->content, TRUE);
  gtk_widget_show (self->content);

  GTK_CONTAINER_CLASS (self->klass)->add (GTK_CONTAINER (self->window),
                                          self->content);
}

static GtkStyleContext *
create_child_context (HdyWindowImpl *self)
{
  GtkStyleContext *parent, *ret;

  parent = gtk_widget_get_style_context (GTK_WIDGET (self->window));

  ret = gtk_style_context_new ();
  gtk_style_context_set_parent (ret, parent);
  gtk_style_context_set_screen (ret, gtk_style_context_get_screen (parent));
  gtk_style_context_set_frame_clock (ret, gtk_style_context_get_frame_clock (parent));

  g_signal_connect_object (ret,
                           "changed",
                           G_CALLBACK (gtk_widget_queue_draw),
                           self->window,
                           G_CONNECT_SWAPPED);

  return ret;
}

static void
update_child_context (HdyWindowImpl   *self,
                      GtkStyleContext *context,
                      const gchar     *name)
{
  g_autoptr (GtkWidgetPath) path = NULL;
  GtkStyleContext *parent;
  gint pos;

  parent = gtk_widget_get_style_context (GTK_WIDGET (self->window));

  path = gtk_widget_path_new ();
  pos = gtk_widget_path_append_for_widget (path, GTK_WIDGET (self->window));
  pos = gtk_widget_path_append_type (path, GTK_TYPE_WIDGET);
  gtk_widget_path_iter_set_object_name (path, pos, name);

  gtk_style_context_set_path (context, path);
  gtk_style_context_set_state (context, gtk_style_context_get_state (parent));
  gtk_style_context_set_scale (context, gtk_style_context_get_scale (parent));
}

static void
style_changed_cb (HdyWindowImpl *self)
{
  update_child_context (self, self->decoration_context, "decoration");
  update_child_context (self, self->overlay_context, "decoration-overlay");
}

static gboolean
is_fullscreen (HdyWindowImpl *self)
{
  GdkWindow *window = gtk_widget_get_window (GTK_WIDGET (self->window));

  return (gdk_window_get_state (window) & GDK_WINDOW_STATE_FULLSCREEN) > 0;
}

static gboolean
supports_client_shadow (HdyWindowImpl *self)
{
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (GTK_WIDGET (self->window));

  /*
   * GtkWindow adds this when it can't draw proper decorations, e.g. on a
   * non-composited WM on X11. This is documented, so we can rely on this
   * instead of copying the (pretty extensive) check.
   */
  return !gtk_style_context_has_class (context, "solid-csd");
}

static void
max_borders (GtkBorder *one,
             GtkBorder *two)
{
  one->top = MAX (one->top, two->top);
  one->right = MAX (one->right, two->right);
  one->bottom = MAX (one->bottom, two->bottom);
  one->left = MAX (one->left, two->left);
}

static void
get_shadow_width (HdyWindowImpl   *self,
                  GtkStyleContext *context,
                  GtkBorder       *shadow_width)
{
  GtkStateFlags state;
  GtkBorder margin = { 0 };
  GtkAllocation content_alloc, alloc;
  GtkWidget *titlebar;

  *shadow_width = margin;

  if (!gtk_window_get_decorated (self->window))
    return;

  if (gtk_window_is_maximized (self->window) ||
      is_fullscreen (self))
    return;

  if (!gtk_widget_is_toplevel (GTK_WIDGET (self->window)))
    return;

  state = gtk_style_context_get_state (context);

  gtk_style_context_get_margin (context, state, &margin);

  gtk_widget_get_allocation (GTK_WIDGET (self->window), &alloc);
  gtk_widget_get_allocation (self->content, &content_alloc);

  titlebar = gtk_window_get_titlebar (self->window);
  if (titlebar && gtk_widget_get_visible (titlebar)) {
    GtkAllocation titlebar_alloc;

    gtk_widget_get_allocation (titlebar, &titlebar_alloc);

    content_alloc.y = titlebar_alloc.y;
    content_alloc.height += titlebar_alloc.height;
  }

  shadow_width->left = (content_alloc.x - alloc.x);
  shadow_width->right = (alloc.width - content_alloc.width - content_alloc.x);
  shadow_width->top = (content_alloc.y - alloc.y);
  shadow_width->bottom = (alloc.height - content_alloc.height - content_alloc.y);

  max_borders (shadow_width, &margin);
}

static void
rounded_rectangle (cairo_t *cr,
                   gdouble  x,
                   gdouble  y,
                   gdouble  w,
                   gdouble  h,
                   gdouble  r)
{
  const gdouble ARC_0 = 0;
  const gdouble ARC_1 = G_PI * 0.5;
  const gdouble ARC_2 = G_PI;
  const gdouble ARC_3 = G_PI * 1.5;

  r = MAX (r, 0);

  cairo_new_sub_path (cr);
  cairo_arc (cr, x + w - r, y + r,     r, ARC_3, ARC_0);
  cairo_arc (cr, x + w - r, y + h - r, r, ARC_0, ARC_1);
  cairo_arc (cr, x + r,     y + h - r, r, ARC_1, ARC_2);
  cairo_arc (cr, x + r,     y + r,     r, ARC_2, ARC_3);
  cairo_close_path (cr);
}

static cairo_surface_t *
prepare_mask (HdyWindowImpl *self,
              cairo_t       *cr,
              gint           width,
              gint           height,
              gint           border_radius)
{
  cairo_t *mask_cr;
  gint scale_factor;

  scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (self->window));

  if (self->last_width == width * scale_factor &&
      self->last_height == height * scale_factor &&
      self->last_border_radius == border_radius)
    return self->mask;

  self->last_width = width * scale_factor;
  self->last_height = height * scale_factor;
  self->last_border_radius = border_radius;

  g_clear_pointer (&self->mask, cairo_surface_destroy);

  if (border_radius == 0)
      return NULL;

  self->mask = cairo_surface_create_similar_image (cairo_get_target (cr),
                                                   CAIRO_FORMAT_A8,
                                                   width * scale_factor,
                                                   height * scale_factor);

  mask_cr = cairo_create (self->mask);

  cairo_set_source_rgb (mask_cr, 0, 0, 0);
  rounded_rectangle (mask_cr, 0, 0, width, height, border_radius - 0.5);
  cairo_fill (mask_cr);

  cairo_destroy (mask_cr);

  return self->mask;
}

void
hdy_window_impl_add (HdyWindowImpl *self,
                     GtkWidget     *widget)
{
  if (GTK_IS_POPOVER (widget)) {
    GTK_CONTAINER_CLASS (self->klass)->add (GTK_CONTAINER (self->window),
                                            widget);

    return;
  }

  ensure_content (self);

  gtk_container_add (GTK_CONTAINER (self->content), widget);
}

void
hdy_window_impl_remove (HdyWindowImpl *self,
                        GtkWidget     *widget)
{
  if (widget == self->content || GTK_IS_POPOVER (widget))
    GTK_CONTAINER_CLASS (self->klass)->remove (GTK_CONTAINER (self->window),
                                               widget);
  else
    gtk_container_remove (GTK_CONTAINER (self->content), widget);
}

void
hdy_window_impl_forall (HdyWindowImpl *self,
                        gboolean       include_internals,
                        GtkCallback    callback,
                        gpointer       callback_data)
{
  if (include_internals)
    GTK_CONTAINER_CLASS (self->klass)->forall (GTK_CONTAINER (self->window),
                                               include_internals,
                                               callback,
                                               callback_data);
  else {
    gtk_container_forall (GTK_CONTAINER (self->content),
                          callback,
                          callback_data);
  }
}

typedef struct {
  HdyWindowImpl *self;
  cairo_t *cr;
} HdyWindowImplDrawData;

static void
draw_popover_cb (GtkWidget             *child,
                 HdyWindowImplDrawData *data)
{
  HdyWindowImpl *self = data->self;
  GdkWindow *window;
  cairo_t *cr = data->cr;

  if (child == self->content ||
      child == gtk_window_get_titlebar (self->window) ||
      !gtk_widget_get_visible (child) ||
      !gtk_widget_get_child_visible (child))
    return;

  window = gtk_widget_get_window (child);

  if (gtk_widget_get_has_window (child))
    window = gdk_window_get_parent (window);

  if (!gtk_cairo_should_draw_window (cr, window))
      return;

  gtk_container_propagate_draw (GTK_CONTAINER (self->window), child, cr);
}

gboolean
hdy_window_impl_draw (HdyWindowImpl *self,
                      cairo_t       *cr)
{
  HdyWindowImplDrawData data;
  GtkWidget *widget;

  widget = GTK_WIDGET (self->window);

  if (gtk_cairo_should_draw_window (cr, gtk_widget_get_window (widget))) {
    GtkStyleContext *context;
    gboolean can_mask_corners;
    cairo_surface_t *mask = NULL;
    GdkRectangle clip = { 0 };
    gint width, height, x, y, w, h, r;

    can_mask_corners = (gtk_window_get_decorated (self->window) &&
                        !is_fullscreen (self) &&
                        !gtk_window_is_maximized (self->window));

    x = 0;
    y = 0;
    w = width = gtk_widget_get_allocated_width (widget);
    h = height = gtk_widget_get_allocated_height (widget);

    context = gtk_widget_get_style_context (widget);

    if (can_mask_corners) {
      GtkBorder shadow;
      get_shadow_width (self, self->decoration_context, &shadow);

      x += shadow.left;
      y += shadow.top;
      w -= shadow.left + shadow.right;
      h -= shadow.top + shadow.bottom;
    }

    if (!supports_client_shadow (self)) {
      gtk_render_background (self->decoration_context, cr, 0, 0, width, height);
      gtk_render_frame (self->decoration_context, cr, 0, 0, width, height);
    } else {
      gtk_render_background (self->decoration_context, cr, x, y, w, h);
      gtk_render_frame (self->decoration_context, cr, x, y, w, h);
    }

    cairo_save (cr);

    if (gdk_cairo_get_clip_rectangle (cr, &clip)) {
      clip.x -= x;
      clip.y -= y;
    } else {
      clip.x = 0;
      clip.y = 0;
      clip.width = w;
      clip.height = h;
    }

    gtk_style_context_get (self->decoration_context,
                           gtk_style_context_get_state (self->decoration_context),
                           GTK_STYLE_PROPERTY_BORDER_RADIUS, &r,
                           NULL);

    if (can_mask_corners &&
        ((clip.x              <     r && clip.y               <     r) ||
         (clip.x              <     r && clip.y + clip.height > h - r) ||
         (clip.x + clip.width > w - r && clip.y + clip.height > h - r) ||
         (clip.x + clip.width > w - r && clip.y               <     r)))
      mask = prepare_mask (self, cr, w, h, r);

    if (mask)
      cairo_push_group (cr);

    if (!gtk_widget_get_app_paintable (widget)) {
        gtk_render_background (context, cr, x, y, w, h);
        gtk_render_frame (context, cr, x, y, w, h);
    }

    gtk_container_propagate_draw (GTK_CONTAINER (self->window), self->content, cr);

    gtk_container_propagate_draw (GTK_CONTAINER (self->window),
                                  gtk_window_get_titlebar (self->window),
                                  cr);

    gtk_render_background (self->overlay_context, cr, x, y, w, h);
    gtk_render_frame (self->overlay_context, cr, x, y, w, h);

    if (mask) {
        cairo_pop_group_to_source (cr);
        cairo_mask_surface (cr, self->mask, x, y);
    }

    cairo_restore (cr);
  }

  data.self = self;
  data.cr = cr;
  gtk_container_forall (GTK_CONTAINER (self->window),
                        (GtkCallback) draw_popover_cb,
                        &data);

  return GDK_EVENT_PROPAGATE;
}

gboolean
hdy_window_impl_window_state_event (HdyWindowImpl       *self,
                                    GdkEventWindowState *event)
{
  gboolean ret;

  ret = GTK_WIDGET_CLASS (self->klass)->window_state_event (GTK_WIDGET (self->window),
                                                            event);

  style_changed_cb (self);

  return ret;
}

static void
hdy_window_impl_finalize (GObject *object)
{
  HdyWindowImpl *self = (HdyWindowImpl *)object;

  g_clear_pointer (&self->mask, cairo_surface_destroy);
  g_clear_object (&self->decoration_context);
  g_clear_object (&self->overlay_context);

  G_OBJECT_CLASS (hdy_window_impl_parent_class)->finalize (object);
}

static void
hdy_window_impl_class_init (HdyWindowImplClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = hdy_window_impl_finalize;
}

HdyWindowImpl *
hdy_window_impl_new (GtkWindow      *window,
                     GtkWindowClass *klass)
{
  HdyWindowImpl *self;
  GtkStyleContext *context;
  GtkWidget *titlebar;

  g_return_val_if_fail (GTK_IS_WINDOW (window), NULL);
  g_return_val_if_fail (GTK_IS_WINDOW_CLASS (klass), NULL);

  self = g_object_new (HDY_TYPE_WINDOW_IMPL, NULL);

  self->window = window;
  self->klass = klass;

  context = gtk_widget_get_style_context (GTK_WIDGET (window));
  gtk_style_context_add_class (context, "unified");

  /* Trick the window into being CSD */
  titlebar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_window_set_titlebar (window, titlebar);

  g_signal_connect_object (window,
                           "style-updated",
                           G_CALLBACK (style_changed_cb),
                           self,
                           G_CONNECT_SWAPPED);

  self->decoration_context = create_child_context (self);
  self->overlay_context = create_child_context (self);

  style_changed_cb (self);
  ensure_content (self);

  return self;
}

static void
hdy_window_impl_init (HdyWindowImpl *self)
{
}

/*
 * Copyright (C) 2020 Purism SPC
 * Copyright (C) 2020 Felipe Borges
 *
 * Authors:
 * Felipe Borges <felipeborges@gnome.org>
 * Julian Sparber <julian@sparber.net>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 *
 */

#include "config.h"
#include <math.h>

#include "hdy-avatar.h"

#define NUMBER_OF_COLORS 8
/**
 * SECTION:hdy-avatar
 * @short_description: A widget displaying an image, with a generated fallback.
 * @Title: HdyAvatar
 *
 * #HdyAvatar is a widget to display a round avatar.
 * A provided image is made round before displaying, if no image is given this widget generates
 * a round fallback with the initials of the #HdyAvatar:text on top of a colord background.
 * The color is picked based on the hash of the #HdyAvatar:text.
 * If #HdyAvatar:show-initials is set to %FALSE, `avatar-default-symbolic` is shown in place
 * of the initials. Use hdy_avatar_set_image_load_func () to set a custom image.
 *
 * # CSS nodes
 *
 * #HdyAvatar has a single CSS node with name avatar.
 *
 */

typedef struct
{
  gchar *text;
  PangoLayout *layout;
  gboolean show_initials;
  guint color_class;
  gint size;
  cairo_surface_t *round_image;

  HdyAvatarImageLoadFunc load_image_func;
  gpointer load_image_func_target;
  GDestroyNotify load_image_func_target_destroy_notify;
} HdyAvatarPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (HdyAvatar, hdy_avatar, GTK_TYPE_DRAWING_AREA);

enum {
  PROP_0,
  PROP_NAME,
  PROP_SHOW_INITIALS,
  PROP_SIZE,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

static cairo_surface_t *
round_image (GdkPixbuf *pixbuf)
{
  cairo_surface_t *surface;
  cairo_t *cr;
  gdouble size;

  size = (gdouble) gdk_pixbuf_get_width (pixbuf);
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, size, size);
  cr = cairo_create (surface);

  /* Clip a circle */
  cairo_arc (cr, size / 2.0, size / 2.0, size / 2.0, 0, 2 * G_PI);
  cairo_clip (cr);
  cairo_new_path (cr);

  gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
  cairo_paint (cr);

  cairo_destroy (cr);

  return surface;
}

static gchar *
extract_initials_from_text (const gchar *text)
{
  GString *initials;
  g_autofree gchar *p = NULL;
  g_autofree gchar *normalized = NULL;
  gunichar unichar;
  gchar *q = NULL;

  p = g_utf8_strup (text, -1);
  normalized = g_utf8_normalize (g_strstrip (p), -1, G_NORMALIZE_DEFAULT_COMPOSE);
  if (normalized == NULL) {
    return NULL;
  }

  initials = g_string_new ("");

  unichar = g_utf8_get_char (normalized);
  g_string_append_unichar (initials, unichar);

  q = g_utf8_strrchr (normalized, -1, ' ');
  if (q != NULL && g_utf8_next_char (q) != NULL) {
    q = g_utf8_next_char (q);

    unichar = g_utf8_get_char (q);
    g_string_append_unichar (initials, unichar);
  }

  return g_string_free (initials, FALSE);
}

static void
update_custom_image (HdyAvatar *self)
{
  HdyAvatarPrivate *priv;
  g_autoptr (GdkPixbuf) pixbuf = NULL;
  gint scale_factor;
  gint size;
  gboolean was_custom = FALSE;

  priv = hdy_avatar_get_instance_private (HDY_AVATAR (self));

  if (priv->round_image != NULL) {
    g_clear_pointer (&priv->round_image, cairo_surface_destroy);
    was_custom = TRUE;
  }

  if (priv->load_image_func_target != NULL) {
    scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (self));
    size = MIN (gtk_widget_get_allocated_width (GTK_WIDGET (self)),
                gtk_widget_get_allocated_height (GTK_WIDGET (self)));
    pixbuf = priv->load_image_func (size,
                                    scale_factor,
                                    priv->load_image_func_target);
    priv->round_image = round_image (pixbuf);
    cairo_surface_set_device_scale (priv->round_image, scale_factor, scale_factor);
  }

  if (was_custom || priv->round_image != NULL)
    gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
set_css_class (HdyAvatar *self)
{
  HdyAvatarPrivate *priv;
  GtkStyleContext *context;
  g_autofree GRand *rand = NULL;
  g_autofree gchar * new_class = NULL;
  g_autofree gchar * old_class = NULL;

  priv = hdy_avatar_get_instance_private (HDY_AVATAR (self));
  context = gtk_widget_get_style_context (GTK_WIDGET (self));

  old_class = g_strdup_printf ("color%d", priv->color_class);
  gtk_style_context_remove_class (context, old_class);

  if (priv->text == NULL || strlen (priv->text) == 0) {
    /* Use a random color if we don't have a text */
    rand = g_rand_new ();
    priv->color_class = g_rand_int_range (rand, 0, NUMBER_OF_COLORS);
  } else {
    priv->color_class = g_str_hash (priv->text) % NUMBER_OF_COLORS;
  }

  new_class = g_strdup_printf ("color%d", priv->color_class);
  gtk_style_context_add_class (context, new_class);
}

static void
set_font_size (HdyAvatar *self)
{
  HdyAvatarPrivate *priv;
  GtkStyleContext *context;
  PangoFontDescription *font_desc;
  gint size;
  gint new_font_size;

  priv = hdy_avatar_get_instance_private (HDY_AVATAR (self));
  size = gtk_widget_get_allocated_width (GTK_WIDGET (self));

  if (priv->round_image || !priv->layout)
    return;

  if (size < 17)
    new_font_size = 5;
  else if (size < 25)
    new_font_size = 9;
  else if (size < 31)
    new_font_size = 11;
  else if (size < 41)
    new_font_size = 13;
  else if (size < 49)
    new_font_size = 18;
  else /* If the avater is really big just make the font size 1/3 of the overall size */
    new_font_size = size / 3;

  context = gtk_widget_get_style_context (GTK_WIDGET (self));
  gtk_style_context_get (context,
                         gtk_style_context_get_state (context),
                         "font",
                         &font_desc,
                         NULL);

  font_desc = pango_font_description_copy (font_desc);
  pango_font_description_set_size (font_desc, new_font_size * PANGO_SCALE);
  pango_layout_set_font_description (priv->layout, font_desc);
  pango_font_description_free (font_desc);
}

static void
hdy_avatar_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  HdyAvatar *self = HDY_AVATAR (object);

  switch (property_id) {
  case PROP_NAME:
    g_value_set_string (value, hdy_avatar_get_text (self));
    break;

  case PROP_SHOW_INITIALS:
    g_value_set_boolean (value, hdy_avatar_get_show_initials (self));
    break;

  case PROP_SIZE:
    g_value_set_int (value, hdy_avatar_get_size (self));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
hdy_avatar_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  HdyAvatar *self = HDY_AVATAR (object);

  switch (property_id) {
  case PROP_NAME:
    hdy_avatar_set_text (self, g_value_get_string (value));
    break;

  case PROP_SHOW_INITIALS:
    hdy_avatar_set_show_initials (self, g_value_get_boolean (value));
    break;

  case PROP_SIZE:
    hdy_avatar_set_size (self, g_value_get_int (value));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
hdy_avatar_finalize (GObject *object)
{
  HdyAvatarPrivate *priv = hdy_avatar_get_instance_private (HDY_AVATAR (object));

  g_clear_pointer (&priv->text, g_free);
  g_clear_pointer (&priv->round_image, cairo_surface_destroy);

  if (priv->load_image_func_target_destroy_notify != NULL)
    priv->load_image_func_target_destroy_notify (priv->load_image_func_target);

  G_OBJECT_CLASS (hdy_avatar_parent_class)->finalize (object);
}

static gboolean
hdy_avatar_draw (GtkWidget *widget,
                 cairo_t   *cr)
{
  HdyAvatarPrivate *priv = hdy_avatar_get_instance_private (HDY_AVATAR (widget));
  GtkStyleContext *context;
  gint size;
  gint width, height;
  gint scale;
  GdkRGBA color;
  g_autoptr (GtkIconInfo) icon = NULL;
  g_autoptr (GdkPixbuf) pixbuf = NULL;
  cairo_surface_t *surface;

  context = gtk_widget_get_style_context (widget);
  size = MIN (gtk_widget_get_allocated_width (widget),
              gtk_widget_get_allocated_height (widget));
  gtk_render_frame (context, cr, 0, 0, size, size);

  if (priv->round_image) {
    cairo_set_source_surface (cr, priv->round_image, 0, 0);
    cairo_paint (cr);
  } else {
    gtk_render_background (context, cr, 0, 0, size, size);
    if (priv->show_initials) {
      set_font_size (HDY_AVATAR (widget));
      pango_layout_get_size (priv->layout, &width, &height);
      gtk_render_layout (context,
                         cr,
                         ((gdouble)size - ((gdouble)width / PANGO_SCALE)) / 2.0,
                         ((gdouble)size - ((gdouble)height / PANGO_SCALE)) / 2.0,
                         priv->layout);
    } else {
      scale = gtk_widget_get_scale_factor (widget);
      icon = gtk_icon_theme_lookup_icon_for_scale (gtk_icon_theme_get_default (),
                                         "avatar-default-symbolic",
                                         size / 2,
                                         scale,
                                         GTK_ICON_LOOKUP_FORCE_SYMBOLIC);
      gtk_style_context_get_color (context, gtk_style_context_get_state (context), &color);
      pixbuf = gtk_icon_info_load_symbolic (icon, &color, NULL, NULL, NULL, NULL, NULL);
      surface = gdk_cairo_surface_create_from_pixbuf (pixbuf, scale, gtk_widget_get_window (widget));

      width = cairo_image_surface_get_width (surface);
      height = cairo_image_surface_get_height (surface);
      gtk_render_icon_surface (context,
                       cr,
                       surface,
                       ((gdouble)size - ((gdouble)width / (gdouble)scale)) / 2.0,
                       ((gdouble)size - ((gdouble)height / (gdouble)scale)) / 2.0);
      cairo_surface_destroy (surface);
    }
  }

  return FALSE;
}


/* This private method is prefixed by the class name because it will be a
 * virtual method in GTK 4.
 */
static void
hdy_avatar_measure (GtkWidget      *widget,
                    GtkOrientation  orientation,
                    int             for_size,
                    int            *minimum,
                    int            *natural,
                    int            *minimum_baseline,
                    int            *natural_baseline)
{
  HdyAvatarPrivate *priv = hdy_avatar_get_instance_private (HDY_AVATAR (widget));

  if (minimum)
    *minimum = priv->size;
  if (natural)
    *natural = priv->size;
}

static void
hdy_avatar_get_preferred_width (GtkWidget *widget,
                                gint      *minimum,
                                gint      *natural)
{
  hdy_avatar_measure (widget, GTK_ORIENTATION_HORIZONTAL, -1,
                      minimum, natural, NULL, NULL);
}

static void
hdy_avatar_get_preferred_width_for_height (GtkWidget *widget,
                                           gint       height,
                                           gint      *minimum,
                                           gint      *natural)
{
  hdy_avatar_measure (widget, GTK_ORIENTATION_HORIZONTAL, height,
                      minimum, natural, NULL, NULL);
}

static void
hdy_avatar_get_preferred_height (GtkWidget *widget,
                                 gint      *minimum,
                                 gint      *natural)
{
  hdy_avatar_measure (widget, GTK_ORIENTATION_VERTICAL, -1,
                      minimum, natural, NULL, NULL);
}

static void
hdy_avatar_get_preferred_height_for_width (GtkWidget *widget,
                                           gint       width,
                                           gint      *minimum,
                                           gint      *natural)
{
  hdy_avatar_measure (widget, GTK_ORIENTATION_VERTICAL, width,
                      minimum, natural, NULL, NULL);
}

static GtkSizeRequestMode
hdy_avatar_get_request_mode (GtkWidget *widget)
{
  return GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}


static void
hdy_avatar_class_init (HdyAvatarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = hdy_avatar_finalize;

  object_class->set_property = hdy_avatar_set_property;
  object_class->get_property = hdy_avatar_get_property;

  widget_class->draw = hdy_avatar_draw;
  widget_class->get_request_mode = hdy_avatar_get_request_mode;
  widget_class->get_preferred_width = hdy_avatar_get_preferred_width;
  widget_class->get_preferred_height = hdy_avatar_get_preferred_height;
  widget_class->get_preferred_width_for_height = hdy_avatar_get_preferred_width_for_height;
  widget_class->get_preferred_height_for_width = hdy_avatar_get_preferred_height_for_width;

  /**
   * HdyAvatar:size:
   *
   * The avatar size of the avatar.
   */
  props[PROP_SIZE] = g_param_spec_int ("size",
                                       "size",
                                       "The size of the avatar",
                                       -1, INT_MAX, -1,
                                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * HdyAvatar:text:
   *
   * The text used for the initials and for generating the color.
   * If #HdyAvatar:show-initials is %FALSE it's only used to generate the color.
   */
  props[PROP_NAME] = g_param_spec_string ("text",
                                          "text",
                                          "The text used to generate the color and the initials",
                                          NULL,
                                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * HdyAvatar:show_initials:
   *
   * Whether to show the initials or the fallback icon on the generated avatar.
   */
  props[PROP_SHOW_INITIALS] = g_param_spec_boolean ("show-initials",
                                                    "Show initials",
                                                    "Whether to show the initials",
                                                    FALSE,
                                                    G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_css_name (widget_class, "avatar");
}


static void
hdy_avatar_init (HdyAvatar *self)
{
  set_css_class (self);
  g_signal_connect (self, "notify::scale-factor", G_CALLBACK (update_custom_image), NULL);
  g_signal_connect (self, "size-allocate", G_CALLBACK (update_custom_image), NULL);
}


/**
 * hdy_avatar_new:
 * @image: (allow-none): The Avatar to show or %NULL
 * @size: The size of the avatar
 * @text: (allow-none): The text used to generate the color and initials if @show_initials is %TRUE. The color is selected at random if @text is %NULL.
 * @show_initials: whether to show the initials or the fallback icon on top of the color generated based on @text.
 *
 * Creates a new #HdyAvatar.
 *
 * Returns: the newly created #HdyAvatar
 */
GtkWidget *
hdy_avatar_new (GdkPixbuf   *image,
                gint         size,
                const gchar *text,
                gboolean     show_initials)
{
  return g_object_new (HDY_TYPE_AVATAR,
                       "image", image,
                       "size", size,
                       "text", text,
                       "show-initials", show_initials,
                       NULL);
}

/**
 * hdy_avatar_get_text:
 * @self: a #HdyAvatar
 *
 * Get the text used to generate the fallback initials and color
 *
 * Returns: (nullable) (transfer none): returns the text used to generate the fallback initials.
 * This is the internal string used by the #HdyAvatar, and must not be modified.
 */
const gchar *
hdy_avatar_get_text (HdyAvatar *self)
{
  HdyAvatarPrivate *priv;

  g_return_val_if_fail (HDY_IS_AVATAR (self), NULL);

  priv = hdy_avatar_get_instance_private (self);

  return priv->text;
}

/**
 * hdy_avatar_set_text:
 * @self: a #HdyAvatar
 * @text: (allow-none): the text used to get the initials and color
 *
 * Set the text used to generate the fallback initials color
 *
 */
void
hdy_avatar_set_text (HdyAvatar   *self,
                     const gchar *text)
{
  HdyAvatarPrivate *priv;
  g_autofree gchar *initials = NULL;

  g_return_if_fail (HDY_IS_AVATAR (self));

  priv = hdy_avatar_get_instance_private (self);

  if (g_strcmp0 (priv->text, text) == 0)
    return;

  g_clear_pointer (&priv->text, g_free);
  priv->text = g_strdup (text);

  initials = extract_initials_from_text (text);

  if (priv->layout == NULL)
    priv->layout = gtk_widget_create_pango_layout (GTK_WIDGET (self), initials);
  else
    pango_layout_set_text (priv->layout, initials, -1);

  gtk_widget_queue_draw (GTK_WIDGET (self));

  set_css_class (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_NAME]);
}

/**
 * hdy_avatar_get_show_initials:
 * @self: a #HdyAvatar
 *
 * Returns whether initals are used for the fallback or the icon.
 *
 * Returns: %TRUE if the initials are used for the fallback.
 */
gboolean
hdy_avatar_get_show_initials (HdyAvatar *self)
{
  HdyAvatarPrivate *priv;

  g_return_val_if_fail (HDY_IS_AVATAR (self), FALSE);

  priv = hdy_avatar_get_instance_private (self);

  return priv->show_initials;
}

/**
 * hdy_avatar_set_show_initials:
 * @self: a #HdyAvatar
 * @show_initials: whether the initials should be shown on the fallback avatar or the icon.
 *
 * Sets whether the initials should be shown on the fallback avatar or the icon.
 */
void
hdy_avatar_set_show_initials (HdyAvatar *self,
                              gboolean   show_initials)
{
  HdyAvatarPrivate *priv;

  g_return_if_fail (HDY_IS_AVATAR (self));

  priv = hdy_avatar_get_instance_private (self);

  if (priv->show_initials == show_initials)
    return;

  priv->show_initials = show_initials;

  gtk_widget_queue_draw (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SHOW_INITIALS]);
}

/**
 * hdy_avatar_set_image_load_func:
 * @self: a #HdyAvatar
 * @load_image: (closure user_data) (allow-none): callback to set a custom image
 * @user_data: user data passed to @load_image
 * @destroy: destroy notifier for @user_data
 *
 * A callback which is called when the custom image need to be reloaded for some reason 
 * (e.g. scale-factor changes).
 *
 */
void
hdy_avatar_set_image_load_func (HdyAvatar *self,
                                HdyAvatarImageLoadFunc load_image,
                                gpointer user_data,
                                GDestroyNotify destroy)
{
  HdyAvatarPrivate *priv;

  g_return_if_fail (HDY_IS_AVATAR (self));

  priv = hdy_avatar_get_instance_private (self);

  if (priv->load_image_func_target_destroy_notify != NULL)
    priv->load_image_func_target_destroy_notify (priv->load_image_func_target);

  priv->load_image_func = load_image;
  priv->load_image_func_target = user_data;
  priv->load_image_func_target_destroy_notify = destroy;

  update_custom_image (self);
}

/**
 * hdy_avatar_get_size:
 * @self: a #HdyAvatar
 *
 * Returns the size of the avatar.
 *
 * Returns: the size of the avatar.
 */
gint
hdy_avatar_get_size (HdyAvatar *self)
{
  HdyAvatarPrivate *priv;

  g_return_val_if_fail (HDY_IS_AVATAR (self), 0);

  priv = hdy_avatar_get_instance_private (self);

  return priv->size;
}

/**
 * hdy_avatar_set_size:
 * @self: a #HdyAvatar
 * @size: The size to be used for the avatar
 *
 * Sets the size of the avatar.
 */
void
hdy_avatar_set_size (HdyAvatar *self,
                     gint       size)
{
  HdyAvatarPrivate *priv;

  g_return_if_fail (HDY_IS_AVATAR (self));

  priv = hdy_avatar_get_instance_private (self);

  if (priv->size == size)
    return;

  priv->size = size;

  gtk_widget_queue_resize (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SIZE]);
}

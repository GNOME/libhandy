/*
 * Copyright (C) 2020 Purism SPC
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
 * #HdyAvatar is a widget to display a round avatar or to generated a GNOME styled fallback.
 * A provided image is made round and displayed, if no image is given this widget generates
 * a round fallback with the initials of the #HdyAvatar:text on top of a colord background.
 * The color is picked based on the hash of the #HdyAvatar:text.
 * If #HdyAvatar:show-initials is set to %FALSE, `avatar-default-symbolic` is shown in place
 * of the initials.
 *
 * See hdy_avatar_set_image () for a code example.
 *
 */

typedef struct
{
  gchar *text;
  PangoLayout *layout;
  gboolean show_initials;
  guint color_class;
  gint size;
  cairo_surface_t *cache;
} HdyAvatarPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (HdyAvatar, hdy_avatar, GTK_TYPE_DRAWING_AREA);

enum {
  PROP_0,
  PROP_IMAGE,
  PROP_NAME,
  PROP_SHOW_INITIALS,
  PROP_SIZE,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

/*
 * This function was originaly written by Felipe Borges 
 * for gnome-control-center in panels/user-accounts/user-utils.c
 * The copyright in that file is: "Copyright 2009-2010  Red Hat, Inc,"
 * And the License in that file is: "GPL-2.0-or-later"
 * Taken from Commit 7cacd0b233cb5626903c167aea2e4370efc7e9ae
 */
static cairo_surface_t *
round_image (GdkPixbuf *pixbuf)
{
  cairo_surface_t *surface;
  cairo_t *cr;
  gint size;

  size = gdk_pixbuf_get_width (pixbuf);
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, size, size);
  cr = cairo_create (surface);

  /* Clip a circle */
  cairo_arc (cr, size/2, size/2, size/2, 0, 2 * G_PI);
  cairo_clip (cr);
  cairo_new_path (cr);

  gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
  cairo_paint (cr);

  cairo_destroy (cr);

  return surface;
}

/*
 * This function was originaly written by Felipe Borges 
 * for gnome-control-center in panels/user-accounts/user-utils.c
 * The copyright in that file is: "Copyright 2009-2010  Red Hat, Inc,"
 * And the License in that file is: "GPL-2.0-or-later"
 * Taken from Commit 7cacd0b233cb5626903c167aea2e4370efc7e9ae
 */
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
  case PROP_IMAGE:
    hdy_avatar_set_image (self, g_value_get_object (value));
    break;

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
  g_clear_pointer (&priv->cache, cairo_surface_destroy);

  G_OBJECT_CLASS (hdy_avatar_parent_class)->finalize (object);
}

static gboolean
hdy_avatar_draw (GtkWidget *widget,
                 cairo_t   *cr)
{
  HdyAvatarPrivate *priv = hdy_avatar_get_instance_private (HDY_AVATAR (widget));
  GtkStyleContext *context;
  gint width, height;
  gint l_width, l_height;
  gint scale;
  GdkRGBA color;
  g_autoptr (GtkIconInfo) icon = NULL;
  g_autoptr (GdkPixbuf) pixbuf = NULL;
  cairo_surface_t *surface;

  context = gtk_widget_get_style_context (widget);
  width = gtk_widget_get_allocated_height (widget);
  height = gtk_widget_get_allocated_height (widget);
  gtk_render_frame (context, cr, 0, 0, width, height);

  if (priv->cache) {
    cairo_set_source_surface (cr, priv->cache, 0, 0);
    cairo_paint (cr);
  } else {
    gtk_render_background (context, cr, 0, 0, width, height);
    if (priv->show_initials) {
      pango_layout_get_size (priv->layout, &l_width, &l_height);
      gtk_render_layout (context,
                         cr,
                         ((gdouble)width - ((gdouble)l_width / PANGO_SCALE)) / 2.0,
                         ((gdouble)height - ((gdouble)l_height / PANGO_SCALE)) / 2.0,
                         priv->layout);
    } else {
      scale = gtk_widget_get_scale_factor (widget);
      icon = gtk_icon_theme_lookup_icon_for_scale (gtk_icon_theme_get_default (),
                                         "avatar-default-symbolic",
                                         width / 2,
                                         scale,
                                         GTK_ICON_LOOKUP_FORCE_SYMBOLIC);
      gtk_style_context_get_color (context, gtk_style_context_get_state (context), &color);
      pixbuf = gtk_icon_info_load_symbolic (icon, &color, NULL, NULL, NULL, NULL, NULL);
      surface = gdk_cairo_surface_create_from_pixbuf (pixbuf, scale, gtk_widget_get_window (widget));

      l_width = cairo_image_surface_get_width (surface);
      l_height = cairo_image_surface_get_height (surface);
      gtk_render_icon_surface (context,
                       cr,
                       surface,
                       ((gdouble)width - ((gdouble)l_width / (gdouble)scale)) / 2.0,
                       ((gdouble)height - ((gdouble)l_height / (gdouble)scale)) / 2.0);
      cairo_surface_destroy (surface);

    }
  }

  return FALSE;
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

  /**
   * HdyAvatar:image:
   *
   * The avatar to show, when set to %NULL a fallback is generated.
   */
  props[PROP_IMAGE] = g_param_spec_object ("image",
                                           "image",
                                           "The avatar to show",
                                           GDK_TYPE_PIXBUF,
                                           G_PARAM_WRITABLE);
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
 * hdy_avatar_set_image:
 * @self: a #HdyAvatar
 * @image: (allow-none): a #GdkPixbuf, or %NULL
 *
 * Sets the image to be displayed. The @image is required to be the size of the avatar.
 * To consider also HIDPI the size of @image should be 
 * hdy_avatar_get_size () * gtk_widget_get_scale_factor ().
 *
 * |[<!-- language="C" -->
 * GtkWidget *avatar = hdy_avatar_new (NULL, 128, NULL, TRUE);
 * g_autoptr (GdkPixbuf) pixbuf = NULL;
 * gint scale;
 * gint size;
 *
 * scale = gtk_widget_get_scale_factor (avatar);
 * size = hdy_avatar_get_size (self->avatar);
 * pixbuf = gdk_pixbuf_new_from_file_at_scale ("./avatar.png", size * scale, size * scale, TRUE, NULL);
 *
 * hdy_avatar_set_image (HDY_AVATAR (avatar), pixbuf);
 * ]|
 *
 * You probably also want to connect to `notify::scale-factor` to updated the @image
 * when the scale factor changes.
 */
void
hdy_avatar_set_image (HdyAvatar *self,
                      GdkPixbuf *image)
{
  HdyAvatarPrivate *priv;
  gint scale_factor;

  g_return_if_fail (HDY_IS_AVATAR (self));

  priv = hdy_avatar_get_instance_private (self);

  g_clear_pointer (&priv->cache, cairo_surface_destroy);

  if (image != NULL) {
    priv->cache = round_image (image);
    scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (self));
    cairo_surface_set_device_scale (priv->cache, scale_factor, scale_factor);
  }

  gtk_widget_queue_draw (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_IMAGE]);
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

  gtk_widget_set_size_request (GTK_WIDGET (self), size, size);
  gtk_widget_queue_draw (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SIZE]);
}

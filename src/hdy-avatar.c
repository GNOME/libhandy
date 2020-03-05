/*
 * Copyright (C) 2020 Purism SPC
 *   
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *   
 *
 * The code to generate the avatar was originaly written by Felipe Borges 
 * for gnome-control-center in panels/user-accounts/user-utils.c
 * The copyright in that file is: "Copyright 2009-2010  Red Hat, Inc,"
 * And the License in that file is: "GPL-2.0-or-later"
 * Taken from Commit 7cacd0b233cb5626903c167aea2e4370efc7e9ae
 *
 * Authors:
 * Felipe Borges <felipeborges@gnome.org>
 * Julian Sparber <julian@sparber.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "config.h"
#include <math.h>

#include "hdy-avatar.h"

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
  gboolean show_initials;
  gint size;
  /* Whether we have a image or use the fallback icon */
  gboolean image;
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

static void
clear_cache (HdyAvatar *self)
{
  HdyAvatarPrivate *priv;

  priv = hdy_avatar_get_instance_private (self);

  if (!priv->image) {
    g_clear_pointer (&priv->cache, cairo_surface_destroy);
    gtk_widget_queue_draw (GTK_WIDGET (self));
  }
}

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

static GdkRGBA
get_color_for_text (const gchar *text, cairo_pattern_t *pat)
{
  #define RED 0
  #define GREEN 1
  #define BLUE 2
  static gdouble color_palette[][2][3] = {
      {{237, 111, 0}, {255, 190, 111}},
      {{229, 165, 10}, {248, 228, 92}},
      {{138, 62, 163}, {220, 138, 221}},
      {{51, 127, 220}, {153, 193, 241}},
      {{110, 109, 113}, {192, 191, 188}},
      {{41, 174, 113}, {141, 230, 174}},
      {{217, 26, 35}, {246, 115, 101}},
      {{134, 93, 60}, {205, 171, 143}}
  };

  GdkRGBA color;
  guint hash;
  gint number_of_colors;
  gint idx;
  g_autofree GRand *rand = NULL;

  if (text == NULL || strlen (text) == 0) {
    /* Use a random color if we don't have a text */
    rand = g_rand_new ();
    idx = g_rand_int_range (rand, 0, G_N_ELEMENTS (color_palette));
  } else {
    hash = g_str_hash (text);
    number_of_colors = G_N_ELEMENTS (color_palette);
    idx = hash % number_of_colors;
  }

  cairo_pattern_add_color_stop_rgb (pat,
                                    1,
                                    color_palette[idx][0][RED] / 255.0,
                                    color_palette[idx][0][GREEN] / 255.0,
                                    color_palette[idx][0][BLUE] / 255.0);
  cairo_pattern_add_color_stop_rgb (pat,
                                    0,
                                    color_palette[idx][1][RED] / 255.0,
                                    color_palette[idx][1][GREEN] / 255.0,
                                    color_palette[idx][1][BLUE] / 255.0);
  color.red   = (color_palette[idx][1][RED] / 255.0) + 0.3;
  color.green = (color_palette[idx][1][GREEN] / 255.0) + 0.3;
  color.blue  = (color_palette[idx][1][BLUE] / 255.0) + 0.3;
  color.alpha  = 0.85;

  return color;
}

static cairo_surface_t *
generate_user_picture (const gchar *text,
                       gint         size,
                       gboolean     show_initials)
{
  PangoFontDescription *font_desc;
  g_autofree gchar *initials = NULL;
  g_autofree gchar *font = NULL;
  g_autoptr (GtkIconInfo) fallback_icon = NULL;
  g_autoptr (GdkPixbuf) fallback_pixbuf = NULL;
  GdkRGBA color;
  PangoLayout *layout;
  cairo_surface_t *surface;
  gint width, height;
  cairo_t *cr;
  cairo_pattern_t *gradient;

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                        size,
                                        size);
  cr = cairo_create (surface);

  cairo_arc (cr, size/2, size/2, size/2, 0, 2 * G_PI);
  gradient = cairo_pattern_create_linear (0.0, 0.0, 0.0, (gdouble) size);
  color = get_color_for_text (text, gradient);
  cairo_set_source (cr, gradient);
  cairo_pattern_destroy (gradient);
  cairo_fill (cr);

  if (show_initials) {
    /* Draw the initials on top */
    font = g_strdup_printf ("Cantarell Bold %d", (gint)ceil (size / 4));
    initials = extract_initials_from_text (text);
    gdk_cairo_set_source_rgba (cr, &color);
    layout = pango_cairo_create_layout (cr);
    pango_layout_set_text (layout, initials, -1);
    font_desc = pango_font_description_from_string (font);
    pango_layout_set_font_description (layout, font_desc);
    pango_font_description_free (font_desc);

    pango_layout_get_size (layout, &width, &height);
    cairo_translate (cr, size/2, size/2);
    cairo_move_to (cr, - ((gdouble)width / PANGO_SCALE)/2, - ((gdouble)height/PANGO_SCALE)/2);
    pango_cairo_show_layout (cr, layout);
  } else {
    /* Draw fallback icon on top */
    fallback_icon = gtk_icon_theme_lookup_icon (gtk_icon_theme_get_default (),
                                              "avatar-default-symbolic",
                                              size / 3,
                                              GTK_ICON_LOOKUP_FORCE_SYMBOLIC);
    fallback_pixbuf = gtk_icon_info_load_symbolic (fallback_icon, &color, NULL, NULL, NULL, NULL, NULL);
    if (fallback_pixbuf) {
      width = gdk_pixbuf_get_width (fallback_pixbuf);
      height = gdk_pixbuf_get_width (fallback_pixbuf);
      gdk_cairo_set_source_pixbuf (cr,
                                   fallback_pixbuf,
                                   (gdouble) size / 2.0 - (gdouble) width / 2.0,
                                   (gdouble) size / 2.0 - (gdouble) height / 2.0);
      cairo_paint (cr);
    }
  }
  cairo_destroy (cr);

  return surface;
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
  gint scale_factor;

  if (priv->cache == NULL) {
    scale_factor = gtk_widget_get_scale_factor (widget);
    priv->cache = generate_user_picture (priv->text, priv->size * scale_factor, priv->show_initials);
    cairo_surface_set_device_scale (priv->cache, scale_factor, scale_factor);
  }

  cairo_set_source_surface (cr, priv->cache, 0, 0);
  cairo_paint (cr);

  context = gtk_widget_get_style_context (widget);
  width = gtk_widget_get_allocated_height (widget);
  height = gtk_widget_get_allocated_height (widget);
  gtk_render_background (context, cr, 0, 0, width, height);
  gtk_render_frame (context, cr, 0, 0, width, height);

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
}


static void
hdy_avatar_init (HdyAvatar *self)
{
  g_signal_connect (self, "notify::scale-factor", G_CALLBACK (clear_cache), NULL);
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

  g_return_if_fail (HDY_IS_AVATAR (self));

  priv = hdy_avatar_get_instance_private (self);

  if (g_strcmp0 (priv->text, text) == 0)
    return;

  g_clear_pointer (&priv->text, g_free);
  priv->text = g_strdup (text);

  clear_cache (self);
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

  clear_cache (self);
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
    priv->image = TRUE;
  } else {
    priv->image = FALSE;
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
  clear_cache (self);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SIZE]);
}

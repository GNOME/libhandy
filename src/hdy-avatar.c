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
 * a round fallback with the initials of the #HdyAvatar:name on top of a colord background.
 * The color is picked based on the hash of the #HdyAvatar:name. If #HdyAvatar:show-initials is set to %FALSE `avatar-default-symbolic` in place of the initials.
 *
 * See hdy_avatar_set_image () for a code example.
 *
 */

typedef struct
{
  gchar *name;
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
  g_return_if_fail (HDY_IS_AVATAR (self));

  priv = hdy_avatar_get_instance_private (self);

  if (!priv->image) {
    g_clear_pointer (&priv->cache, cairo_surface_destroy);
    gtk_widget_queue_draw (GTK_WIDGET (self));
  }
}

static
cairo_surface_t *
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
extract_initials_from_name (const gchar *name)
{
  GString *initials;
  g_autofree gchar *p = NULL;
  g_autofree gchar *normalized = NULL;
  gunichar unichar;
  gchar *q = NULL;

  p = g_utf8_strup (name, -1);
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
get_color_for_name (const gchar *name)
{
  // https://gitlab.gnome.org/Community/Design/HIG-app-icons/blob/master/GNOME%20HIG.gpl
  static gdouble gnome_color_palette[][3] = {
      {  98, 160, 234 },
      {  53, 132, 228 },
      {  28, 113, 216 },
      {  26,  95, 180 },
      {  87, 227, 137 },
      {  51, 209, 122 },
      {  46, 194, 126 },
      {  38, 162, 105 },
      { 248, 228,  92 },
      { 246, 211,  45 },
      { 245, 194,  17 },
      { 229, 165,  10 },
      { 255, 163,  72 },
      { 255, 120,   0 },
      { 230,  97,   0 },
      { 198,  70,   0 },
      { 237,  51,  59 },
      { 224,  27,  36 },
      { 192,  28,  40 },
      { 165,  29,  45 },
      { 192,  97, 203 },
      { 163,  71, 186 },
      { 129,  61, 156 },
      {  97,  53, 131 },
      { 181, 131,  90 },
      { 152, 106,  68 },
      { 134,  94,  60 },
      {  99,  69,  44 }
  };

  GdkRGBA color = { 255, 255, 255, 1.0 };
  guint hash;
  gint number_of_colors;
  gint idx;
  g_autofree GRand *rand = NULL;

  if (name == NULL || strlen (name) == 0) {
    /* Use a random color if we don't have a name */
    rand = g_rand_new ();
    idx = g_rand_int_range (rand, 0, G_N_ELEMENTS (gnome_color_palette));
  } else {
    hash = g_str_hash (name);
    number_of_colors = G_N_ELEMENTS (gnome_color_palette);
    idx = hash % number_of_colors;
  }

  color.red   = gnome_color_palette[idx][0];
  color.green = gnome_color_palette[idx][1];
  color.blue  = gnome_color_palette[idx][2];

  return color;
}

static cairo_surface_t *
generate_user_picture (const gchar *name, gint size, gboolean show_initials)
{
  PangoFontDescription *font_desc;
  g_autofree gchar *initials = NULL;
  g_autofree gchar *font = NULL;
  g_autoptr (GdkPixbuf) fallback_icon = NULL;
  PangoLayout *layout;
  GdkRGBA color = get_color_for_name (name);
  cairo_surface_t *surface;
  gint width, height;
  cairo_t *cr;

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                        size,
                                        size);
  cr = cairo_create (surface);

  cairo_arc (cr, size/2, size/2, size/2, 0, 2 * G_PI);
  cairo_set_source_rgb (cr, color.red/255.0, color.green/255.0, color.blue/255.0);
  cairo_fill (cr);

  if (show_initials) {
    /* Draw the initials on top */
    font = g_strdup_printf ("Cantarell Ultra-Bold %d", (int)ceil (size / 3));
    initials = extract_initials_from_name (name);
    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
    layout = pango_cairo_create_layout (cr);
    pango_layout_set_text (layout, initials, -1);
    font_desc = pango_font_description_from_string (font);
    pango_layout_set_font_description (layout, font_desc);
    pango_font_description_free (font_desc);

    pango_layout_get_size (layout, &width, &height);
    cairo_translate (cr, size/2, size/2);
    cairo_move_to (cr, - ((double)width / PANGO_SCALE)/2, - ((double)height/PANGO_SCALE)/2);
    pango_cairo_show_layout (cr, layout);
  } else {
    /* Draw fallback icon on top */
    fallback_icon = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                              "avatar-default-symbolic",
                                              size / 2,
                                              GTK_ICON_LOOKUP_FORCE_SYMBOLIC,
                                              NULL);
    if (fallback_icon) {
      width = gdk_pixbuf_get_width (fallback_icon);
      height = gdk_pixbuf_get_width (fallback_icon);
      gdk_cairo_set_source_pixbuf (cr,
                                   fallback_icon,
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
    g_value_set_string (value, hdy_avatar_get_name (self));
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
    hdy_avatar_set_name (self, g_value_get_string (value));
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

  g_clear_pointer (&priv->name, g_free);
  g_clear_pointer (&priv->cache, cairo_surface_destroy);

  G_OBJECT_CLASS (hdy_avatar_parent_class)->finalize (object);
}

static gboolean
hdy_avatar_draw (GtkWidget *widget, cairo_t *cr)
{
  HdyAvatarPrivate *priv = hdy_avatar_get_instance_private (HDY_AVATAR (widget));
  gint scale_factor;

  if (priv->cache == NULL) {
    scale_factor = gtk_widget_get_scale_factor (widget);
    priv->cache = generate_user_picture (priv->name, priv->size * scale_factor, priv->show_initials);
    cairo_surface_set_device_scale (priv->cache, scale_factor, scale_factor);
  }

  cairo_set_source_surface (cr, priv->cache, 0, 0);
  cairo_paint (cr);

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

  props[PROP_IMAGE] = g_param_spec_object ("image",
                                           "image",
                                           "The avatar to show, when set to %NULL a fallback is generated",
                                           GDK_TYPE_PIXBUF,
                                           G_PARAM_WRITABLE);

  props[PROP_SIZE] = g_param_spec_int ("size",
                                          "size",
                                          "The size of the avatar",
                                          -1, INT_MAX, -1,
                                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_NAME] = g_param_spec_string ("name",
                                          "name",
                                          "The name used to generate the color and the initials",
                                          NULL,
                                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_SHOW_INITIALS] = g_param_spec_boolean ("show-initials",
                                                    "Show initials",
                                                    "Whether to show the initials or a fallback icon on the generated avatar",
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
 * @name: (allow-none): The name used to generate the color and initials if @show_initials is %TRUE. The color is selected at random if @name is %NULL.
 * @show_initials: whether to show the initials or the fallback icon on top of the color generated based on @name.
 *
 * Creates a new #HdyAvatar.
 *
 * Returns: the newly created #HdyAvatar
 */
GtkWidget *
hdy_avatar_new (GdkPixbuf *image, gint size, const gchar *name, gboolean show_initials)
{
  return g_object_new (HDY_TYPE_AVATAR,
                       "image", image,
                       "size", size,
                       "name", name,
                       "show-initials", show_initials,
                       NULL);
}

/**
 * hdy_avatar_get_name:
 * @self: a #HdyAvatar
 *
 * Get the name used to generate the fallback initials and color
 *
 * Returns: (nullable) (transfer none): returns the name used to generate the fallback initials.
 * This is the internal string used by the #HdyAvatar, and must not be modified.
 */
const gchar *
hdy_avatar_get_name (HdyAvatar *self)
{
  HdyAvatarPrivate *priv;
  g_return_val_if_fail (HDY_IS_AVATAR (self), NULL);

  priv = hdy_avatar_get_instance_private (self);

  return priv->name;
}

/**
 * hdy_avatar_set_name:
 * @self: a #HdyAvatar
 * @name: (allow-none): the name used to get the initials and color
 *
 * Set the name used to generate the fallback initials color
 *
 */
void
hdy_avatar_set_name (HdyAvatar *self, const gchar *name)
{
  HdyAvatarPrivate *priv;
  g_return_if_fail (HDY_IS_AVATAR (self));

  priv = hdy_avatar_get_instance_private (self);

  if (g_strcmp0 (priv->name, name) == 0)
    return;

  g_clear_pointer (&priv->name, g_free);
  priv->name = g_strdup (name);

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
hdy_avatar_set_show_initials (HdyAvatar *self, gboolean show_initials)
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
hdy_avatar_set_image (HdyAvatar *self, GdkPixbuf *image)
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
hdy_avatar_set_size (HdyAvatar *self, gint size)
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

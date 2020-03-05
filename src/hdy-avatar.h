/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#if !defined(_HANDY_INSIDE) && !defined(HANDY_COMPILATION)
#error "Only <handy.h> can be included directly."
#endif

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define HDY_TYPE_AVATAR (hdy_avatar_get_type())

G_DECLARE_DERIVABLE_TYPE (HdyAvatar, hdy_avatar, HDY, AVATAR, GtkDrawingArea)

/**
 * HdyAvatarClass:
 * @parent_class: The parent class
 */
struct _HdyAvatarClass
{
  GtkDrawingAreaClass parent_class;
};

GtkWidget   *hdy_avatar_new               (GdkPixbuf   *image,
                                           gint         size,
                                           const gchar *name,
                                           gboolean     show_initials);
const gchar *hdy_avatar_get_name          (HdyAvatar *self);
void         hdy_avatar_set_name          (HdyAvatar *self, const gchar *name);
gboolean     hdy_avatar_get_show_initials (HdyAvatar *self);
void         hdy_avatar_set_show_initials (HdyAvatar *self, gboolean show_initials);
GdkPixbuf   *hdy_avatar_get_image         (HdyAvatar *self);
void         hdy_avatar_set_image         (HdyAvatar *self, GdkPixbuf *image);
gint         hdy_avatar_get_size          (HdyAvatar *self);
void         hdy_avatar_set_size          (HdyAvatar *self, gint size);

G_END_DECLS

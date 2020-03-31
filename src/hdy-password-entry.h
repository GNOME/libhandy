/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#if !defined(_HANDY_INSIDE) && !defined(HANDY_COMPILATION)
#error "Only <handy.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define HDY_TYPE_PASSWORD_ENTRY (hdy_password_entry_get_type())

G_DECLARE_DERIVABLE_TYPE (HdyPasswordEntry, hdy_password_entry, HDY, PASSWORD_ENTRY, GtkOverlay)

struct _HdyPasswordEntryClass
{
  GtkOverlayClass parent_class;
};

GtkWidget   *hdy_password_entry_new                   (void);
guint        hdy_password_entry_get_peek_duration     (HdyPasswordEntry *entry);
void         hdy_password_entry_set_peek_duration     (HdyPasswordEntry *entry,
                                                       guint             peek_duration);

gboolean     hdy_password_entry_get_show_peek_icon    (HdyPasswordEntry *entry);
void         hdy_password_entry_set_show_peek_icon    (HdyPasswordEntry *entry,
                                                       gboolean          show_peek_icon);

gboolean     hdy_password_entry_get_show_caps_icon    (HdyPasswordEntry *entry);
void         hdy_password_entry_set_show_caps_icon    (HdyPasswordEntry *entry,
                                                       gboolean          show_caps_icon);


G_END_DECLS

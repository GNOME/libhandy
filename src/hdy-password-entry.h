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

G_DECLARE_DERIVABLE_TYPE (HdyPasswordEntry, hdy_password_entry, HDY, PASSWORD_ENTRY, GtkEntry)

struct _HdyPasswordEntryClass
{
  GtkEntryClass parent_class;
};

GtkWidget   *hdy_password_entry_new               (void);


G_END_DECLS

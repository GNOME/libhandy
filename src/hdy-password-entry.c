/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "hdy-password-entry.h"

typedef struct
{
  gboolean show_characters;
} HdyPasswordEntryPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (HdyPasswordEntry, hdy_password_entry, GTK_TYPE_ENTRY);


static void
hdy_password_entry_class_init (HdyPasswordEntryClass *klass) {

}

static void
hdy_password_entry_init (HdyPasswordEntry *self) {

}

GtkWidget *
hdy_password_entry_new (void)
{
  return GTK_WIDGET (g_object_new (HDY_TYPE_PASSWORD_ENTRY, NULL));
}

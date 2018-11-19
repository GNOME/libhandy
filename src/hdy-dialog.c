/*
 * Copyright © 2018 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include <glib/gi18n.h>

#include "hdy-dialog.h"

/**
 * SECTION:hdy-dialog
 * @short_description: An adaptive dialog
 * @Title: HdyDialog
 */

typedef struct {
} HdyDialogPrivate;

static void hdy_dialog_buildable_init (GtkBuildableIface  *iface);

G_DEFINE_TYPE_WITH_CODE (HdyDialog, hdy_dialog, GTK_TYPE_DIALOG,
                         G_ADD_PRIVATE (HdyDialog)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, hdy_dialog_buildable_init))

static void
hdy_dialog_class_init (HdyDialogClass *klass)
{
}

static void
hdy_dialog_init (HdyDialog *self)
{
  g_object_set (G_OBJECT (self),
                "destroy-with-parent", TRUE,
                NULL);
}

static void
hdy_dialog_buildable_init (GtkBuildableIface *iface)
{
}

GtkWidget *
hdy_dialog_new (GtkWindow *parent)
{
  return g_object_new (HDY_TYPE_DIALOG,
                       "transient-for", parent,
                       NULL);
}

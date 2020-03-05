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
  GtkWidget *icon;

  GtkWidget *entry;
  gboolean show_characters;
} HdyPasswordEntryPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (HdyPasswordEntry, hdy_password_entry, GTK_TYPE_ENTRY);


static void
secondary_icon_clicked_cb (GtkEntry              *entry,
                           GtkEntryIconPosition   icon_pos,
                           GdkEvent              *event,
                           gpointer               user_data)
{
  if (icon_pos == GTK_ENTRY_ICON_SECONDARY)
    g_print ("gotcha!");
}

static void
hdy_password_entry_finalize (GObject *object)
{
  HdyPasswordEntryPrivate *priv = hdy_password_entry_get_instance_private (HDY_PASSWORD_ENTRY (object));

  g_clear_pointer (&priv->entry, gtk_widget_unparent);
  g_clear_pointer (&priv->icon, gtk_widget_unparent);

  if (priv->entry != NULL)
    g_object_unref (priv->entry);
  if (priv->icon != NULL)
    g_object_unref (priv->icon);

  G_OBJECT_CLASS (hdy_password_entry_parent_class)->finalize (object);

}

static void
hdy_password_entry_class_init (HdyPasswordEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = hdy_password_entry_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/handy/ui/hdy-password-entry.ui");

  gtk_widget_class_bind_template_callback (widget_class, secondary_icon_clicked_cb);
}

static void
hdy_password_entry_init (HdyPasswordEntry *entry)
{
  HdyPasswordEntryPrivate *priv = hdy_password_entry_get_instance_private (entry);

  priv->entry = gtk_entry_new ();

  gtk_widget_init_template (GTK_WIDGET (entry));
}

GtkWidget *
hdy_password_entry_new (void)
{
  return GTK_WIDGET (g_object_new (HDY_TYPE_PASSWORD_ENTRY, NULL));
}

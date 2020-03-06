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
  guint timeout_id;
  gboolean show_characters; // not used
} HdyPasswordEntryPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (HdyPasswordEntry, hdy_password_entry, GTK_TYPE_ENTRY);

#define REVEAL_TIMEOUT 2000

static gboolean
timeout_cb (gpointer user_data)
{
  HdyPasswordEntry *entry = user_data;
  HdyPasswordEntryPrivate *priv = hdy_password_entry_get_instance_private (entry);

  if (priv->timeout_id > 0)
    g_source_remove (priv->timeout_id);
  priv->timeout_id = 0;
  gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     "hdy-eye-not-looking-symbolic");

  return TRUE;
}

static void
set_timeout (HdyPasswordEntry *entry)
{
  HdyPasswordEntryPrivate *priv = hdy_password_entry_get_instance_private (entry);

  if (priv->timeout_id > 0)
    g_source_remove (priv->timeout_id);
  priv->timeout_id = g_timeout_add (REVEAL_TIMEOUT,
                                    timeout_cb,
                                    entry);
  g_source_set_name_by_id (priv->timeout_id, "[gtk] hdy_password_entry_reveal_timeout_cb");
}

static void
icon_release_cb (HdyPasswordEntry              *entry,
                           GtkEntryIconPosition           icon_pos,
                           GdkEvent                      *event,
                           gpointer                       user_data)
{
  HdyPasswordEntryPrivate *priv = hdy_password_entry_get_instance_private (entry);

  if (icon_pos == GTK_ENTRY_ICON_SECONDARY)
  {
    if (priv->timeout_id > 0)
    {
      g_source_remove (priv->timeout_id);
      gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
      gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry),
                                         GTK_ENTRY_ICON_SECONDARY,
                                         "hdy-eye-not-looking-symbolic");
      priv->timeout_id = 0;
    }
    else
    {
      gtk_entry_set_visibility (GTK_ENTRY (entry), TRUE);
      gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry),
                                         GTK_ENTRY_ICON_SECONDARY,
                                         "hdy-eye-open-symbolic");
      set_timeout (entry);
    }
  }
}

static void
hdy_password_entry_finalize (GObject *object)
{
  // just for future use

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

  gtk_widget_class_bind_template_callback (widget_class, icon_release_cb);
}

static void
hdy_password_entry_init (HdyPasswordEntry *entry)
{
  gtk_widget_init_template (GTK_WIDGET (entry));
}

GtkWidget *
hdy_password_entry_new (void)
{
  return GTK_WIDGET (g_object_new (HDY_TYPE_PASSWORD_ENTRY, NULL));
}

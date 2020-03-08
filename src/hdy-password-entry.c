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
hdy_password_entry_dispose (GObject *object)
{
  HdyPasswordEntryPrivate *priv = hdy_password_entry_get_instance_private (HDY_PASSWORD_ENTRY (object));

  if (priv->timeout_id > 0)
    g_source_remove (priv->timeout_id);
  g_signal_handlers_disconnect_by_func (object, G_CALLBACK (icon_release_cb), NULL);

  G_OBJECT_CLASS (hdy_password_entry_parent_class)->dispose (object);
}

static void
hdy_password_entry_class_init (HdyPasswordEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = hdy_password_entry_dispose;
}

static void
hdy_password_entry_init (HdyPasswordEntry *entry)
{
  gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
  gtk_entry_set_input_purpose (GTK_ENTRY (entry), GTK_INPUT_PURPOSE_PASSWORD);
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     "hdy-eye-not-looking-symbolic");
  gtk_entry_set_icon_tooltip_text (GTK_ENTRY (entry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     "Clear entry");

  g_signal_connect (GTK_ENTRY (entry),
                    "icon-release",
                    G_CALLBACK (icon_release_cb),
                    NULL);
}

GtkWidget *
hdy_password_entry_new (void)
{
  return GTK_WIDGET (g_object_new (HDY_TYPE_PASSWORD_ENTRY, NULL));
}

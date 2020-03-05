/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "hdy-password-entry.h"

#define DEFAULT_REVEAL_DURATION 2000
#define MIN_REVEAL_DURATION 500

/**
 * SECTION:hdy-password-entry
 * @short_description: A password entry widget.
 * @title: HdyPasswordEntry
 *
 * The #HdyPasswordEntry widget is a password entry widget with toggle
 * functionality to reveal entered characters for a short duration of time.
 *
 * # CSS nodes
 *
 * #HdyPasswordEntry has a single CSS node with name passwordentry.
 *
 * Since: 1.0
 */

typedef struct
{
  guint reveal_duration;
  guint timeout_id;
} HdyPasswordEntryPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (HdyPasswordEntry, hdy_password_entry, GTK_TYPE_ENTRY);

enum {
  PROP_0,
  PROP_REVEAL_DURATION,
  LAST_PROP
};

enum {
  SIGNAL_TIMED_OUT,
  SIGNAL_LAST
};

static GParamSpec *props[LAST_PROP];
static guint signals[SIGNAL_LAST];

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
                                     GTK_ENTRY_ICON_PRIMARY,
                                     "hdy-eye-not-looking-symbolic");

  g_signal_emit (entry, signals[SIGNAL_TIMED_OUT], 0);

  return TRUE;
}

static void
set_timeout (HdyPasswordEntry *entry)
{
  HdyPasswordEntryPrivate *priv = hdy_password_entry_get_instance_private (entry);

  if (priv->timeout_id > 0)
    g_source_remove (priv->timeout_id);
  priv->timeout_id = g_timeout_add (priv->reveal_duration,
                                    timeout_cb,
                                    entry);
  g_source_set_name_by_id (priv->timeout_id, "[hdy] hdy_password_entry_reveal_duration_cb");
}

static void
icon_release_cb (HdyPasswordEntry              *entry,
                 GtkEntryIconPosition           icon_pos,
                 GdkEvent                      *event,
                 gpointer                       user_data)
{
  HdyPasswordEntryPrivate *priv = hdy_password_entry_get_instance_private (entry);

  if (icon_pos == GTK_ENTRY_ICON_PRIMARY)
  {
    if (priv->timeout_id > 0)
    {
      g_source_remove (priv->timeout_id);
      gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
      gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry),
                                         GTK_ENTRY_ICON_PRIMARY,
                                         "hdy-eye-not-looking-symbolic");
      priv->timeout_id = 0;
    }
    else
    {
      gtk_entry_set_visibility (GTK_ENTRY (entry), TRUE);
      gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry),
                                         GTK_ENTRY_ICON_PRIMARY,
                                         "hdy-eye-open-symbolic");
      set_timeout (entry);
    }
  }
}

static void
hdy_password_entry_dispose (GObject *object)
{
  HdyPasswordEntryPrivate *priv = hdy_password_entry_get_instance_private
                                    (HDY_PASSWORD_ENTRY (object));

  if (priv->timeout_id > 0)
    g_source_remove (priv->timeout_id);
  g_signal_handlers_disconnect_by_func (object, G_CALLBACK (icon_release_cb), NULL);

  G_OBJECT_CLASS (hdy_password_entry_parent_class)->dispose (object);
}

static void
hdy_password_entry_get_property  (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  HdyPasswordEntry *self = HDY_PASSWORD_ENTRY (object);

  switch (prop_id) {
  case PROP_REVEAL_DURATION:
    g_value_set_uint (value, hdy_password_entry_get_reveal_duration (self));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
hdy_password_entry_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  HdyPasswordEntry *self = HDY_PASSWORD_ENTRY (object);

  switch (prop_id) {
  case PROP_REVEAL_DURATION:
    hdy_password_entry_set_reveal_duration (self, g_value_get_uint (value));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
hdy_password_entry_class_init (HdyPasswordEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  /**
   * HdyPasswordEntry::timed-out:
   *
   * This signal is emitted when a password reveal timeout happens.
   *
   * Since: 1.0
   */
  signals[SIGNAL_TIMED_OUT] = g_signal_new ("timed-out",
                                           G_TYPE_FROM_CLASS (klass),
                                           G_SIGNAL_RUN_FIRST,
                                           0,
                                           NULL, NULL, NULL,
                                           G_TYPE_NONE,
                                           0);

  object_class->dispose = hdy_password_entry_dispose;
  object_class->get_property = hdy_password_entry_get_property;
  object_class->set_property = hdy_password_entry_set_property;

  /**
   * HdyPasswordEntry:reveal-duration:
   *
   * Amount of duration #HdyPasswordEntry should show password characters
   * if user clicks on reveal icon.
   * #HdyPasswordEntry:reveal-duration is @guint, which has default
   * value of 2000 Milliseconds and a Minimum duration of 500 Milliseconds.
   *
   * Changing this property sends a notify signal.
   *
   * Since: 1.0
   */
  props[PROP_REVEAL_DURATION] = g_param_spec_uint
                                  ("reveal-duration",
                                   _("Reveal duration"),
                                   _("Password reveal duration in milliseconds"),
                                   MIN_REVEAL_DURATION,
                                   G_MAXUINT,
                                   DEFAULT_REVEAL_DURATION,
                                   G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_css_name (widget_class, "passwordentry");
}

static void
hdy_password_entry_init (HdyPasswordEntry *entry)
{
  HdyPasswordEntryPrivate *priv = hdy_password_entry_get_instance_private (entry);

  priv->reveal_duration = DEFAULT_REVEAL_DURATION;
  gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
  gtk_entry_set_input_purpose (GTK_ENTRY (entry), GTK_INPUT_PURPOSE_PASSWORD);
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry),
                                     GTK_ENTRY_ICON_PRIMARY,
                                     "hdy-eye-not-looking-symbolic");
  gtk_entry_set_icon_tooltip_text (GTK_ENTRY (entry),
                                     GTK_ENTRY_ICON_PRIMARY,
                                     "Show password");

  g_signal_connect (GTK_ENTRY (entry),
                    "icon-release",
                    G_CALLBACK (icon_release_cb),
                    NULL);
}

/**
 *hdy_password_entry_get_reveal_duration:
 * @entry: a HdyPasswordEntry
 *
 * Gets reveal duration of password characters.
 *
 * Returns: Reveal duration in milliseconds
 *
 * Since: 1.0
 */

guint
hdy_password_entry_get_reveal_duration (HdyPasswordEntry *entry)
{
  HdyPasswordEntryPrivate *priv = hdy_password_entry_get_instance_private (entry);

  return priv->reveal_duration;
}

/**
 * hdy_password_entry_set_reveal_duration:
 * @entry: a HdyPasswordEntry
 * @reveal_duration: a guint
 *
 * Sets reveal duration of password characters.
 *
 * Since: 1.0
 */
void
hdy_password_entry_set_reveal_duration (HdyPasswordEntry *entry,
                                        guint reveal_duration)
{
  HdyPasswordEntryPrivate *priv = hdy_password_entry_get_instance_private (entry);

  priv->reveal_duration = reveal_duration;
  g_object_notify_by_pspec (G_OBJECT (entry), props[PROP_REVEAL_DURATION]);
}

/**
 * hdy_password_entry_new:
 *
 * Create a new #HdyPasswordEntry widget.
 *
 * Returns: The newly created #HdyPasswordEntry widget
 *
 * Since: 1.0
 */
GtkWidget *
hdy_password_entry_new (void)
{
  return GTK_WIDGET (g_object_new (HDY_TYPE_PASSWORD_ENTRY, NULL));
}


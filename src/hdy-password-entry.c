/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "hdy-password-entry.h"

#define DEFAULT_PEEK_DURATION 2000
#define MIN_PEEK_DURATION 500

/**
 * SECTION:hdy-password-entry
 * @short_description: A password entry widget.
 * @title: HdyPasswordEntry
 *
 * The #HdyPasswordEntry widget is a password entry widget with toggle
 * functionality to show entered characters for a short duration of time.
 *
 * # CSS nodes
 *
 * #HdyPasswordEntry has a single CSS node with name passwordentry.
 *
 * Since: 1.0
 */

typedef struct
{
  GtkWidget *password_entry;
  GtkWidget *peek_icon;
  GtkWidget *peek_icon_event_box;

  GtkCssProvider *css_provider;

  gboolean show_peek_icon;
  guint peek_duration;
  guint timeout_id;
} HdyPasswordEntryPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (HdyPasswordEntry, hdy_password_entry, GTK_TYPE_OVERLAY);

enum {
  PROP_0,
  PROP_PEEK_DURATION,
  PROP_SHOW_PEEK_ICON,
  LAST_PROP
};

enum {
  SIGNAL_TIMED_OUT,
  SIGNAL_LAST
};

static GParamSpec *props[LAST_PROP];
static guint signals[SIGNAL_LAST];

static void hide_text (GtkWidget*, gpointer);
static void show_text (GtkWidget*, gpointer);

static gboolean
timeout_cb (gpointer user_data)
{
  hide_text (NULL, user_data);

  g_signal_emit (HDY_PASSWORD_ENTRY (user_data), signals[SIGNAL_TIMED_OUT], 0);

  return TRUE;
}

static void
set_timeout (HdyPasswordEntry *entry)
{
  HdyPasswordEntryPrivate *priv = hdy_password_entry_get_instance_private (entry);

  if (priv->timeout_id > 0)
    g_source_remove (priv->timeout_id);
  priv->timeout_id = g_timeout_add (priv->peek_duration,
                                    timeout_cb,
                                    entry);
  g_source_set_name_by_id (priv->timeout_id, "[hdy] hdy_password_entry_peek_duration_cb");
}

static void
peek_icon_button_press_event_cb (GtkWidget         *event_box,
                                 GdkEventButton    *event,
                                 HdyPasswordEntry  *entry)
{
  HdyPasswordEntryPrivate *priv = hdy_password_entry_get_instance_private (entry);

  if ((event->type == GDK_BUTTON_PRESS &&
        event->button == 1) ||
       (event->type == GDK_TOUCH_BEGIN))
  {
    if (priv->timeout_id > 0)
      hide_text (NULL, entry);
    else
      show_text (NULL, entry);
  }
}

static void
password_entry_size_allocated_cb (GtkWidget    *widget,
                                  GdkRectangle *allocation,
                                  gpointer      user_data)
{
  GtkStyleContext *style_context;
  GtkStateFlags state_flags;
  GtkBorder border, margin, padding;
  guint spacing;

  style_context = gtk_widget_get_style_context (widget);
  state_flags = gtk_widget_get_state_flags (widget);
  gtk_style_context_get_border (style_context, state_flags, &border);
  gtk_style_context_get_margin (style_context, state_flags, &margin);
  gtk_style_context_get_padding (style_context, state_flags, &padding);

  spacing = margin.right + border.right;

  if (padding.right == padding.left)
    spacing += padding.right;
  else
    spacing += padding.right < padding.left? padding.right: padding.left;
  gtk_widget_set_margin_end (GTK_WIDGET (user_data), spacing);
}

static void
event_box_size_allocated_cb (GtkWidget    *widget,
                             GdkRectangle *allocation,
                             gpointer      user_data)
{
  HdyPasswordEntryPrivate *priv = hdy_password_entry_get_instance_private (HDY_PASSWORD_ENTRY (user_data));
  g_autofree gchar *css;
  GtkBorder padding;
  guint extra_space;

  gtk_style_context_get_padding (gtk_widget_get_style_context (priv->password_entry),
                                 gtk_widget_get_state_flags (priv->password_entry),
                                 &padding);
  extra_space = allocation->width + (padding.right > padding.left ?
                                     padding.left:
                                     padding.right);

  /* Don't add this padding when all the widgets are hidden (in which case
   * the allocated width is 1), because it distorts the appearance of widget.
   */
  if (allocation->width > 6)
    extra_space += 6;

  css = g_strdup_printf (".password_entry:dir(ltr) { padding-right: %dpx; }"\
                         ".password_entry:dir(rtl) { padding-left: %dpx; }",
                         extra_space, extra_space);

  gtk_css_provider_load_from_data (priv->css_provider, css, -1, NULL);
}

static void
hide_text (GtkWidget *widget,
           gpointer   user_data)
{
  HdyPasswordEntryPrivate *priv = hdy_password_entry_get_instance_private (HDY_PASSWORD_ENTRY (user_data));

  if (priv->timeout_id > 0)
    g_source_remove (priv->timeout_id);
  gtk_entry_set_visibility (GTK_ENTRY (priv->password_entry), FALSE);
  gtk_image_set_from_icon_name (GTK_IMAGE (priv->peek_icon),
                                "hdy-eye-not-looking-symbolic",
                                GTK_ICON_SIZE_MENU);
  gtk_widget_set_tooltip_text (priv->peek_icon_event_box, _("Show text"));
  priv->timeout_id = 0;
}

static void
show_text (GtkWidget *widget,
           gpointer user_data)
{
  HdyPasswordEntryPrivate *priv = hdy_password_entry_get_instance_private (HDY_PASSWORD_ENTRY (user_data));

  gtk_entry_set_visibility (GTK_ENTRY (priv->password_entry), TRUE);
  gtk_image_set_from_icon_name (GTK_IMAGE (priv->peek_icon),
                                "hdy-eye-open-symbolic",
                                GTK_ICON_SIZE_MENU);
  gtk_widget_set_tooltip_text (priv->peek_icon_event_box, _("Hide text"));
  set_timeout (HDY_PASSWORD_ENTRY (user_data));
}

static void
populate_popup_cb (GtkEntry   *password_entry,
                   GtkWidget  *widget,
                   gpointer    user_data)
{
  GtkWidget *menuitem;

  g_assert (GTK_IS_MENU (widget));

  if (gtk_entry_get_visibility (password_entry))
    {
      menuitem = gtk_menu_item_new_with_mnemonic (_("_Hide text"));
      g_signal_connect (menuitem, "activate",
                        G_CALLBACK (hide_text), user_data);
    }
  else
    {
      menuitem = gtk_menu_item_new_with_mnemonic (_("_Show text"));
      g_signal_connect (menuitem, "activate",
                        G_CALLBACK (show_text), user_data);
    }

  gtk_widget_set_sensitive (menuitem, TRUE);

  gtk_widget_show (menuitem);
  gtk_menu_shell_append (GTK_MENU_SHELL (widget), menuitem);
  gtk_widget_show (menuitem);
}

static void
hdy_password_entry_dispose (GObject *object)
{
  HdyPasswordEntryPrivate *priv = hdy_password_entry_get_instance_private
                                    (HDY_PASSWORD_ENTRY (object));

  if (priv->timeout_id > 0)
    g_source_remove (priv->timeout_id);

  g_clear_object (&priv->css_provider);

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
  case PROP_PEEK_DURATION:
    g_value_set_uint (value, hdy_password_entry_get_peek_duration (self));
    break;

  case PROP_SHOW_PEEK_ICON:
    g_value_set_boolean (value, hdy_password_entry_get_show_peek_icon (self));
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
  case PROP_PEEK_DURATION:
    hdy_password_entry_set_peek_duration (self, g_value_get_uint (value));
    break;

  case PROP_SHOW_PEEK_ICON:
    hdy_password_entry_set_show_peek_icon (self, g_value_get_boolean (value));
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
   * This signal is emitted when a password peek timeout happens.
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
   * HdyPasswordEntry::peek-duration:
   *
   * It has default value of 2000 Milliseconds and a Minimum
   * duration of 500 Milliseconds.
   *
   * Changing this property sends a notify signal.
   *
   * Since: 1.0
   */
  props[PROP_PEEK_DURATION] = g_param_spec_uint
                                  ("peek-duration",
                                   _("Peek duration"),
                                   _("Password peek duration in milliseconds"),
                                   MIN_PEEK_DURATION,
                                   G_MAXUINT,
                                   DEFAULT_PEEK_DURATION,
                                   G_PARAM_READWRITE);

  /**
   * HdyPasswordEntry::show-peek-icon:
   *
   * Whether to show the peek icon.
   *
   * Changing this property sends a notify signal.
   *
   * Since: 1.0
   */
  props[PROP_SHOW_PEEK_ICON] = g_param_spec_boolean
                                 ("show-peek-icon",
                                  _("Show peek icon"),
                                  _("Whether to show the peek icon"),
                                  TRUE,
                                  G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_css_name (widget_class, "passwordentry");
}

static void
hdy_password_entry_init (HdyPasswordEntry *entry)
{
  HdyPasswordEntryPrivate *priv = hdy_password_entry_get_instance_private (entry);

  priv->peek_duration = DEFAULT_PEEK_DURATION;
  priv->show_peek_icon = TRUE;
  priv->password_entry = gtk_entry_new ();
  priv->peek_icon = gtk_image_new_from_icon_name ("hdy-eye-not-looking-symbolic", GTK_ICON_SIZE_MENU);
  priv->peek_icon_event_box = gtk_event_box_new ();

  priv->css_provider = gtk_css_provider_new ();
  gtk_style_context_add_provider (gtk_widget_get_style_context (priv->password_entry),
                                  GTK_STYLE_PROVIDER (priv->css_provider),
                                  GTK_STYLE_PROVIDER_PRIORITY_USER);

  gtk_widget_set_halign (priv->peek_icon_event_box, GTK_ALIGN_END);
  gtk_widget_set_tooltip_text (priv->peek_icon_event_box, _("Show text"));
  g_signal_connect (G_OBJECT (priv->peek_icon_event_box), "button-press-event",
                    G_CALLBACK (peek_icon_button_press_event_cb), entry);

  gtk_widget_set_valign (priv->peek_icon, GTK_ALIGN_CENTER);

  g_signal_connect (G_OBJECT (priv->peek_icon_event_box), "size-allocate",
                    G_CALLBACK (event_box_size_allocated_cb), entry);
  g_signal_connect (G_OBJECT (priv->password_entry), "size-allocate",
                    G_CALLBACK (password_entry_size_allocated_cb),
                    priv->peek_icon_event_box);
  g_signal_connect (G_OBJECT (priv->password_entry), "populate-popup",
                    G_CALLBACK (populate_popup_cb), entry);

  gtk_style_context_add_class (gtk_widget_get_style_context (priv->password_entry), "password_entry");
  g_object_set (G_OBJECT (priv->password_entry),
                "input-purpose", GTK_INPUT_PURPOSE_PASSWORD,
                "visibility", FALSE,
                "placeholder-text", _("Password"),
                "can-focus", TRUE,
                NULL);

  gtk_overlay_add_overlay (GTK_OVERLAY (entry), priv->peek_icon_event_box);
  gtk_container_add (GTK_CONTAINER (entry), priv->password_entry);
  gtk_container_add (GTK_CONTAINER (priv->peek_icon_event_box), priv->peek_icon);

  gtk_widget_show (priv->password_entry);
  gtk_widget_show (priv->peek_icon_event_box);
  gtk_widget_show (priv->peek_icon);
}

/**
 *hdy_password_entry_get_peek_duration:
 * @entry: a HdyPasswordEntry
 *
 * Gets peek duration of password characters.
 *
 * Returns: Peek duration in milliseconds
 *
 * Since: 1.0
 */

guint
hdy_password_entry_get_peek_duration (HdyPasswordEntry *entry)
{
  HdyPasswordEntryPrivate *priv = hdy_password_entry_get_instance_private (entry);

  return priv->peek_duration;
}

/**
 * hdy_password_entry_set_peek_duration:
 * @entry: a HdyPasswordEntry
 * @peek_duration: a guint
 *
 * Sets peek duration of password characters.
 *
 * Since: 1.0
 */
void
hdy_password_entry_set_peek_duration (HdyPasswordEntry *entry,
                                      guint peek_duration)
{
  HdyPasswordEntryPrivate *priv = hdy_password_entry_get_instance_private (entry);

  priv->peek_duration = peek_duration;
  g_object_notify_by_pspec (G_OBJECT (entry), props[PROP_PEEK_DURATION]);
}

/**
 * hdy_password_entry_get_show_peek_icon:
 * @entry: a HdyPasswordEntry
 *
 * Returns whether the entry is showing a clickable icon
 * to reveal the contents of the entry in clear text.
 *
 * Since: 1.0
 */
gboolean
hdy_password_entry_get_show_peek_icon (HdyPasswordEntry *entry)
{
  HdyPasswordEntryPrivate *priv = hdy_password_entry_get_instance_private (entry);

  return priv->show_peek_icon;
}

/**
 * hdy_password_entry_set_show_peek_icon:
 * @entry: a HdyPasswordEntry
 * @show_peek_icon: whether to show the peek icon
 *
 * Sets whether the entry should have a clickable icon
 * to show the contents of the entry in clear text.
 *
 * Setting this to #FALSE also hides the text again.
 *
 * Since: 1.0
 */
void
hdy_password_entry_set_show_peek_icon (HdyPasswordEntry *entry,
                                       gboolean show_peek_icon)
{
  HdyPasswordEntryPrivate *priv = hdy_password_entry_get_instance_private (entry);

  if (show_peek_icon)
    gtk_widget_show (priv->peek_icon);
  else
    gtk_widget_hide (priv->peek_icon);

  priv->show_peek_icon = show_peek_icon;
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


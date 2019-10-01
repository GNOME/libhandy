/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "hdy-keypad.h"
#include "hdy-keypad-button.h"

/**
 * SECTION:hdy-keypad
 * @short_description: A keypad for dialing numbers
 * @Title: HdyKeypad
 *
 * The #HdyKeypad widget is a keypad for entering numbers such as phone numbers
 * or PIN codes.
 */

typedef struct
{
  GtkWidget *entry;
  GtkGesture *long_press_zero_gesture;
  gboolean only_digits;
  gboolean show_symbols;
} HdyKeypadPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (HdyKeypad, hdy_keypad, GTK_TYPE_GRID)

enum {
  PROP_0,
  PROP_SHOW_SYMBOLS,
  PROP_ONLY_DIGITS,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

static char* SYMBOLS[] = {"0+", "1", "2ABC", "3DEF", "4GHI", "5JKL", "6MNO", "7PQRS", "8TUV", "9WXYZ", "*", "#"};


static void
symbol_clicked (HdyKeypad     *self,
                gchar          symbol)
{
  HdyKeypadPrivate *priv;
  g_autofree gchar *string = g_strdup_printf ("%c", symbol);
  g_return_if_fail (HDY_IS_KEYPAD (self));
  priv = hdy_keypad_get_instance_private (self);
  g_return_if_fail (priv->entry != NULL);
  g_signal_emit_by_name(GTK_ENTRY (priv->entry), "insert-at-cursor", string, NULL);
  gtk_entry_grab_focus_without_selecting (GTK_ENTRY (priv->entry));
}


static void
digit_button_clicked (HdyKeypad       *self,
                      HdyKeypadButton *btn)
{
  gchar digit;

  g_return_if_fail (HDY_IS_KEYPAD (self));
  g_return_if_fail (HDY_IS_KEYPAD_BUTTON (btn));

  digit = hdy_keypad_button_get_digit (btn);
  symbol_clicked (self, digit);
  g_debug ("Button with number %c was pressed", digit);
}


static gboolean
filter_character (HdyKeypad *self, gchar digit)
{
  HdyKeypadPrivate *priv;
  gboolean is_digit = FALSE;

  g_return_val_if_fail (HDY_IS_KEYPAD (self), FALSE);
  priv = hdy_keypad_get_instance_private (self);

  switch (digit) {
  case '0' ... '9':
    is_digit = TRUE;
    break;
  case '#':
  case '*':
  case '+':
    break;
  default:
    return FALSE;
  }

  return !(priv->only_digits && !is_digit);
}


static void
insert_text_cb (HdyKeypad *self,
               gchar       *text,
               gint         length,
               gpointer     position,
               GtkEditable  *editable)
{
  gchar digit = *text;
  g_return_if_fail (HDY_IS_KEYPAD (self));


  if (!filter_character (self, digit))
    g_signal_stop_emission_by_name (editable, "insert-text");
}


static void
map_event_cb (GtkWidget     *widget)
{
  gtk_entry_grab_focus_without_selecting (GTK_ENTRY (widget));
}


static void
long_press_zero_cb (GtkGesture *gesture,
                    gdouble      x,
                    gdouble      y,
                    HdyKeypad   *self)
{
  g_return_if_fail (HDY_IS_KEYPAD (self));
  g_debug ("Long press on zero button");
  symbol_clicked (self, '+');
  gtk_gesture_set_state (GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_CLAIMED);
}


static void
hdy_keypad_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  HdyKeypad *self = HDY_KEYPAD (object);
  HdyKeypadPrivate *priv = hdy_keypad_get_instance_private (self);

  switch (property_id) {
  case PROP_SHOW_SYMBOLS:
    hdy_keypad_show_symbols (self, g_value_get_boolean (value));
    break;

  case PROP_ONLY_DIGITS:
    priv->only_digits = g_value_get_boolean (value);
    if (priv->only_digits)
      priv->show_symbols = FALSE;
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
hdy_keypad_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  HdyKeypad *self = HDY_KEYPAD (object);
  HdyKeypadPrivate *priv = hdy_keypad_get_instance_private (self);

  switch (property_id) {
  case PROP_SHOW_SYMBOLS:
    g_value_set_boolean (value, priv->show_symbols);
    break;

  case PROP_ONLY_DIGITS:
    g_value_set_boolean (value, priv->only_digits);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static GtkWidget *
hdy_keypad_create_num_button (HdyKeypad *self, gint number) {
  GtkWidget *btn = hdy_keypad_button_new (SYMBOLS[number]);
  gtk_widget_set_focus_on_click (btn, FALSE);
  g_signal_connect_object (btn,
                           "clicked",
                           G_CALLBACK (digit_button_clicked),
                           self,
                           G_CONNECT_SWAPPED);
  g_object_bind_property (self, "show-symbols",
                          btn, "show-symbols",
                          G_BINDING_SYNC_CREATE);

  return btn;
}


static void
hdy_keypad_constructed (GObject *object)
{
  HdyKeypad *self = HDY_KEYPAD (object);
  HdyKeypadPrivate *priv = hdy_keypad_get_instance_private (self);
  GtkWidget *btn_zero;
  gint number;

  number = 1;
  // Create buttons from 1 to 9
  for (gint i = 0; i < 3; i++) {
    for (gint j = 0; j < 3; j++) {
      gtk_grid_attach (GTK_GRID (self), hdy_keypad_create_num_button (self, number), j, i, 1, 1);
      number++;
    }
  }

  // Add button 0
  btn_zero = hdy_keypad_create_num_button (self, 0);
  gtk_grid_attach (GTK_GRID (self), btn_zero, 1, 3, 1, 1);

  if (!priv->only_digits) {
    priv->long_press_zero_gesture = gtk_gesture_long_press_new (GTK_WIDGET (btn_zero));
    g_signal_connect (priv->long_press_zero_gesture, "pressed",
                      G_CALLBACK (long_press_zero_cb), self);
    // Add button #
    gtk_grid_attach (GTK_GRID (self), hdy_keypad_create_num_button (self, 10), 0, 3, 1, 1);
    // Add button *
    gtk_grid_attach (GTK_GRID (self), hdy_keypad_create_num_button (self, 11), 2, 3, 1, 1);
  }

  gtk_widget_show_all (GTK_WIDGET (self));

  G_OBJECT_CLASS (hdy_keypad_parent_class)->constructed (object);
}


static void
hdy_keypad_finalize (GObject *object)
{
  HdyKeypadPrivate *priv = hdy_keypad_get_instance_private (HDY_KEYPAD (object));

  if (priv->long_press_zero_gesture != NULL)
    g_object_unref (priv->long_press_zero_gesture);

  G_OBJECT_CLASS (hdy_keypad_parent_class)->finalize (object);
}


static void
hdy_keypad_class_init (HdyKeypadClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = hdy_keypad_finalize;
  object_class->constructed = hdy_keypad_constructed;

  object_class->set_property = hdy_keypad_set_property;
  object_class->get_property = hdy_keypad_get_property;

  props[PROP_SHOW_SYMBOLS] =
    g_param_spec_boolean ("show_symbols",
                         _("Show Symbols"),
                         _("Whether the second line of symbols should be shown or not"),
                         TRUE,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_ONLY_DIGITS] =
    g_param_spec_boolean ("only_digits",
                         _("Only Digits"),
                         _("Whether the keypad should show only digits or also extra buttons for #, *"),
                         FALSE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_accessible_role (widget_class, ATK_ROLE_DIAL);
  gtk_widget_class_set_css_name (widget_class, "hdykeypad");
}


static void
hdy_keypad_init (HdyKeypad *self)
{
  HdyKeypadPrivate *priv = hdy_keypad_get_instance_private (self);
  priv->show_symbols = TRUE;
  priv->only_digits = FALSE;
  gtk_widget_set_events (GTK_WIDGET (self), GDK_KEY_PRESS_MASK);
}


/**
 * hdy_keypad_new:
 * @only_digits: whether we want onyl digits or symbols as well
 *
 * Create a new #HdyKeypad widget.
 *
 * Returns: the newly created #HdyKeypad widget
 *
 */
GtkWidget *hdy_keypad_new (gboolean only_digits)
{
  return g_object_new (HDY_TYPE_KEYPAD,
                       "visible", TRUE,
                       "only-digits", only_digits,
                       "can-focus", TRUE,
                       "row-homogeneous", TRUE,
                       "column-homogeneous", TRUE,
                       NULL);
}


/**
 * hdy_keypad_show_symbols:
 * @self: a #HdyKeypad
 * @visible: whether the second line on #HdyKeypadButton should be shown or not
 *
 * Sets the visibility of symbols (excluding the main digit) on each #HdyKeypadButton in the #HdyKeypad
 *
 */
void
hdy_keypad_show_symbols (HdyKeypad *self, gboolean visible)
{
  HdyKeypadPrivate *priv = hdy_keypad_get_instance_private(self);

  g_return_if_fail (HDY_IS_KEYPAD (self));

  if (visible == priv->show_symbols)
    return;

  priv->show_symbols = visible;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SHOW_SYMBOLS]);
}


/**
 * hdy_keypad_set_entry:
 * @self: a #HdyKeypad
 * @entry: a #GtkEntry
 *
 * Binds a #GtkEntry to the keypad and connects signals
 * so it grabs focus when visible, also it blocks every
 * input which wouldn't be possible with the keypad
 *
 *
 */
void
hdy_keypad_set_entry (HdyKeypad *self, GtkEntry *entry)
{
  HdyKeypadPrivate *priv;

  g_return_if_fail (HDY_IS_KEYPAD (self));
  g_return_if_fail (GTK_IS_ENTRY (entry));

  priv = hdy_keypad_get_instance_private(self);
  if (priv->entry != NULL) {
    g_object_unref (priv->entry);
  }

  if (entry == NULL) {
    priv->entry = NULL;
    return;
  }

  priv->entry = GTK_WIDGET (g_object_ref (entry));

  gtk_widget_show (priv->entry);
  gtk_widget_set_can_focus (priv->entry, TRUE);
  /* Workaround: To keep the osk cloesed
   * https://gitlab.gnome.org/GNOME/gtk/merge_requests/978#note_546576 */
  g_object_set (priv->entry, "im-module", "simple", NULL);

  g_signal_connect_swapped (G_OBJECT (priv->entry),
                            "insert-text",
                            G_CALLBACK (insert_text_cb),
                            self);

  g_signal_connect (G_OBJECT (priv->entry),
                    "map-event",
                    G_CALLBACK (map_event_cb),
                    NULL);

  g_signal_connect (G_OBJECT (priv->entry),
                    "map",
                    G_CALLBACK (map_event_cb),
                    NULL);
}


/**
 * hdy_keypad_get_entry:
 * @self: a #HdyKeypad
 *
 * Get the connected entry. See hdy_keypad_set_entry () for details
 *
 * Returns: (transfer none): the set #GtkEntry or NULL if no widget was set
 *
 */
GtkWidget *
hdy_keypad_get_entry (HdyKeypad *self)
{
  HdyKeypadPrivate *priv;

  g_return_val_if_fail (HDY_IS_KEYPAD (self), NULL);

  priv = hdy_keypad_get_instance_private(self);

  return priv->entry;
}


/**
 * hdy_keypad_set_left_action:
 * @self: a #HdyKeypad
 * @widget: nullable: the widget which should be show in the left lower corner of #HdyKeypad
 *
 * Sets the widget for the left lower corner of #HdyKeypad replacing the existing widget, if NULL it just removes whatever widget is there
 *
 */
void
hdy_keypad_set_left_action (HdyKeypad *self, GtkWidget *widget)
{
  GtkWidget *old_widget;
  g_return_if_fail (HDY_IS_KEYPAD (self));

  old_widget = gtk_grid_get_child_at (GTK_GRID (self), 0, 3);
  if (old_widget != NULL)
    gtk_container_remove (GTK_CONTAINER (self), old_widget);

  if (widget != NULL)
    gtk_grid_attach (GTK_GRID (self), widget, 0, 3, 1, 1);
}


/**
 * hdy_keypad_set_right_action:
 * @self: a #HdyKeypad
 * @widget: nullable: the widget which should be show in the right lower corner of #HdyKeypad
 *
 * Sets the widget for the right lower corner of #HdyKeypad replacing the existing widget, if NULL it just removes whatever widget is there
 *
 */
void
hdy_keypad_set_right_action (HdyKeypad *self, GtkWidget *widget)
{
  GtkWidget *old_widget;
  g_return_if_fail (HDY_IS_KEYPAD (self));

  old_widget = gtk_grid_get_child_at (GTK_GRID (self), 2, 3);
  if (old_widget != NULL)
    gtk_container_remove (GTK_CONTAINER (self), old_widget);

  if (widget != NULL)
    gtk_grid_attach (GTK_GRID (self), widget, 2, 3, 1, 1);
}


/**
 * hdy_keypad_filter_key_press_event:
 * @self: a #HdyKeypad
 * @event: nullable: #GdkEventKey to filter
 *
 * Filter key press events which are possible to enter with the #HdyKeypad
 *
 * Returns: returns TRUE if the key could be entered with the #HdyKeypad
 */
gboolean
hdy_keypad_filter_key_press_event (HdyKeypad   *self,
                                   GdkEventKey *event)
{
  HdyKeypadPrivate *priv;
  gboolean is_digit = FALSE;

  g_return_val_if_fail (HDY_IS_KEYPAD (self), FALSE);
  priv = hdy_keypad_get_instance_private (self);

  switch (event->keyval) {
  case GDK_KEY_KP_0 ... GDK_KEY_KP_9:
  case GDK_KEY_0 ... GDK_KEY_9:
    is_digit = TRUE;
    break;
  case GDK_KEY_numbersign:
  case GDK_KEY_asterisk:
  case GDK_KEY_KP_Multiply:
  case GDK_KEY_plus:
    break;
  default:
    return FALSE;
  }

  return !(priv->only_digits && !is_digit);
}

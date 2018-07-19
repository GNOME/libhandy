/*
 * Copyright (C) 2017 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include <glib/gi18n.h>

#include "hdy-dialer-button.h"

/**
 * SECTION:hdy-dialer-button
 * @short_description: A button on a #HdyDialer keypad
 * @Title: HdyDialerButton
 *
 * The #HdyDialerButton widget is a single button on an #HdyDialer. It
 * can represent a single digit (0-9) plus an arbitrary number of
 * letters that are displayed below the number.
 */

enum {
  PROP_0,
  PROP_DIGIT,
  PROP_LETTERS,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

typedef struct
{
  GtkLabel *label, *secondary_label;
  gint digit;
  gchar *letters;
} HdyDialerButtonPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (HdyDialerButton, hdy_dialer_button, GTK_TYPE_BUTTON)

void
format_label(HdyDialerButton *self)
{
  HdyDialerButtonPrivate *priv = hdy_dialer_button_get_instance_private(self);
  GString *str;
  g_autofree gchar *text;

  str = g_string_new(NULL);
  if (priv->digit >= 0) {
    g_string_sprintf (str, "%d", priv->digit);
  }

  text = g_string_free (str, FALSE);

  gtk_label_set_label (priv->label, text);
  gtk_label_set_label (priv->secondary_label, priv->letters);
}

static void
hdy_dialer_button_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  HdyDialerButton *self = HDY_DIALER_BUTTON (object);
  HdyDialerButtonPrivate *priv = hdy_dialer_button_get_instance_private(self);

  switch (property_id) {
  case PROP_DIGIT:
    priv->digit = g_value_get_int (value);
    format_label(self);
    break;

  case PROP_LETTERS:
    g_free (priv->letters);
    priv->letters = g_value_dup_string (value);
    format_label(self);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
hdy_dialer_button_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  HdyDialerButton *self = HDY_DIALER_BUTTON (object);
  HdyDialerButtonPrivate *priv = hdy_dialer_button_get_instance_private(self);

  switch (property_id) {
  case PROP_DIGIT:
    g_value_set_int (value, priv->digit);
    break;

  case PROP_LETTERS:
    g_value_set_string (value, priv->letters);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
gtk_widget_class_measure (GtkWidgetClass *widget_class,
                          GtkWidget      *widget,
                          GtkOrientation  orientation,
                          int             for_size,
                          int            *minimum_size,
                          int            *natural_size,
                          int            *minimum_baseline,
                          int            *natural_baseline)
{
  switch (orientation) {
  case GTK_ORIENTATION_HORIZONTAL:
    if (for_size < 0)
      widget_class->get_preferred_width (widget, minimum_size, natural_size);
    else
      widget_class->get_preferred_width_for_height (widget, for_size, minimum_size, natural_size);

    break;
  case GTK_ORIENTATION_VERTICAL:
    if (for_size < 0)
      widget_class->get_preferred_height (widget, minimum_size, natural_size);
    else
      widget_class->get_preferred_height_for_width (widget, for_size, minimum_size, natural_size);

    break;
  }
}

static void
hdy_dialer_button_measure (GtkWidget      *widget,
                           GtkOrientation  orientation,
                           int             for_size,
                           int            *minimum_size,
                           int            *natural_size,
                           int            *minimum_baseline,
                           int            *natural_baseline)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (hdy_dialer_button_parent_class);
  gint minimum_width, natural_width;
  gint minimum_height, natural_height;
  gint dummy;

  gtk_widget_class_measure (widget_class,
                            widget, GTK_ORIENTATION_HORIZONTAL,
                            orientation == GTK_ORIENTATION_VERTICAL ? for_size : -1,
                            &minimum_width, &natural_width,
                            &dummy, &dummy);
  gtk_widget_class_measure (widget_class,
                            widget, GTK_ORIENTATION_VERTICAL,
                            orientation == GTK_ORIENTATION_HORIZONTAL ? for_size : -1,
                            &minimum_height, &natural_height,
                            &dummy, &dummy);

  *minimum_size = MAX (minimum_width, minimum_height);
  *natural_size = MAX (natural_width, natural_height);
}

static void
hdy_dialer_button_get_preferred_width (GtkWidget *widget,
                                       gint      *minimum_width,
                                       gint      *natural_width)
{
  int dummy;
  hdy_dialer_button_measure (widget,
                             GTK_ORIENTATION_HORIZONTAL, -1,
                             minimum_width, natural_width,
                             &dummy, &dummy);
}

static void
hdy_dialer_button_get_preferred_height (GtkWidget *widget,
                                        gint      *minimum_height,
                                        gint      *natural_height)
{
  int dummy;
  hdy_dialer_button_measure (widget,
                             GTK_ORIENTATION_VERTICAL, -1,
                             minimum_height, natural_height,
                             &dummy, &dummy);
}

static void
hdy_dialer_button_get_preferred_width_for_height (GtkWidget *widget,
                                                  gint       height,
                                                  gint      *minimum_width,
                                                  gint      *natural_width)
{
  int dummy;
  hdy_dialer_button_measure (widget,
                             GTK_ORIENTATION_HORIZONTAL, height,
                             minimum_width, natural_width,
                             &dummy, &dummy);
}

static void
hdy_dialer_button_get_preferred_height_for_width (GtkWidget *widget,
                                                  gint       width,
                                                  gint      *minimum_height,
                                                  gint      *natural_height)
{
  int dummy;
  hdy_dialer_button_measure (widget,
                             GTK_ORIENTATION_VERTICAL, width,
                             minimum_height, natural_height,
                             &dummy, &dummy);
}


static void
hdy_dialer_button_finalize (GObject *object)
{
  HdyDialerButton *self = HDY_DIALER_BUTTON (object);
  HdyDialerButtonPrivate *priv = hdy_dialer_button_get_instance_private(self);

  g_clear_pointer (&priv->letters, g_free);
  G_OBJECT_CLASS (hdy_dialer_button_parent_class)->finalize (object);
}


static void
hdy_dialer_button_class_init (HdyDialerButtonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = hdy_dialer_button_set_property;
  object_class->get_property = hdy_dialer_button_get_property;

  object_class->finalize = hdy_dialer_button_finalize;

  widget_class->get_preferred_width = hdy_dialer_button_get_preferred_width;
  widget_class->get_preferred_height = hdy_dialer_button_get_preferred_height;
  widget_class->get_preferred_width_for_height = hdy_dialer_button_get_preferred_width_for_height;
  widget_class->get_preferred_height_for_width = hdy_dialer_button_get_preferred_height_for_width;

  props[PROP_DIGIT] =
    g_param_spec_int ("digit",
                      _("Digit"),
                      _("The dialer digit of the button"),
                      -1, INT_MAX, 0,
                      G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_LETTERS] =
    g_param_spec_string ("letters",
                         _("Letters"),
                         _("The dialer letters of the button"),
                         "",
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/handy/dialer/ui/hdy-dialer-button.ui");
  gtk_widget_class_bind_template_child_private (widget_class, HdyDialerButton, label);
  gtk_widget_class_bind_template_child_private (widget_class, HdyDialerButton, secondary_label);
}

/**
 * hdy_dialer_button_new:
 * @digit: the digit displayed on the #HdyDialerButton
 * @letters: (nullable): the letters displayed on the #HdyDialerButton
 *
 * Create a new #HdyDialerButton which displays @digit and
 * @letters. If @digit is negative no number will be displayed. If
 * @letters is %NULL no letters will be displayed.
 *
 * Returns: the newly created #HdyDialerButton widget
 */
GtkWidget *hdy_dialer_button_new (int          digit,
                                  const gchar *letters)
{
  return g_object_new (HDY_TYPE_DIALER_BUTTON, "digit", digit, "letters", letters, NULL);
}

static void
hdy_dialer_button_init (HdyDialerButton *self)
{
  HdyDialerButtonPrivate *priv = hdy_dialer_button_get_instance_private(self);

  gtk_widget_init_template (GTK_WIDGET (self));

  priv->digit = -1;
  priv->letters = NULL;
}

/**
 * hdy_dialer_button_get_digit:
 * @self: a #HdyDialerButton
 *
 * Get the #HdyDialerButton's digit.
 *
 * Returns: the button's digit
 */
gint
hdy_dialer_button_get_digit(HdyDialerButton *self)
{
  HdyDialerButtonPrivate *priv = hdy_dialer_button_get_instance_private(self);

  g_return_val_if_fail (HDY_IS_DIALER_BUTTON (self), -1);

  return priv->digit;
}

/**
 * hdy_dialer_button_get_letters:
 * @self: a #HdyDialerButton
 *
 * Get the #HdyDialerButton's letters.
 *
 * Returns: the button's letters.
 */
const char*
hdy_dialer_button_get_letters(HdyDialerButton *self)
{
  HdyDialerButtonPrivate *priv = hdy_dialer_button_get_instance_private(self);

  g_return_val_if_fail (HDY_IS_DIALER_BUTTON (self), NULL);

  return priv->letters;
}

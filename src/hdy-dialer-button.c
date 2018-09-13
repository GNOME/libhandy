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
 * symbols that are displayed below the number.
 */

enum {
  PROP_0,
  PROP_DIGIT,
  PROP_SYMBOLS,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

typedef struct
{
  GtkLabel *label, *secondary_label;
  gchar *symbols;
} HdyDialerButtonPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (HdyDialerButton, hdy_dialer_button, GTK_TYPE_BUTTON)

static void
format_label(HdyDialerButton *self)
{
  HdyDialerButtonPrivate *priv = hdy_dialer_button_get_instance_private(self);
  gchar *symbols = priv->symbols != NULL ? priv->symbols : "";
  g_autofree gchar *text = NULL;
  gchar *secondary_text = NULL;

  if (*symbols != '\0') {
    secondary_text = g_utf8_find_next_char (symbols, NULL);
    /* Allocate memory for the first character and '\0'. */
    text = g_malloc0 (secondary_text - symbols + 1);
    g_utf8_strncpy (text, symbols, 1);
  }
  else {
    text = g_malloc0 (sizeof (gchar));
    secondary_text = "";
  }

  gtk_label_set_label (priv->label, text);
  gtk_label_set_label (priv->secondary_label, secondary_text);
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
  case PROP_SYMBOLS:
    g_free (priv->symbols);
    priv->symbols = g_value_dup_string (value);
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
    g_value_set_int (value, hdy_dialer_button_get_digit (self));
    break;

  case PROP_SYMBOLS:
    g_value_set_string (value, priv->symbols);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
hdy_dialer_button_get_preferred_width (GtkWidget *widget,
                                       gint      *minimum_width,
                                       gint      *natural_width)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (hdy_dialer_button_parent_class);
  gint min_width, nat_width, min_height, nat_height;

  widget_class->get_preferred_width (widget, &min_width, &nat_width);
  widget_class->get_preferred_height (widget, &min_height, &nat_height);

  *minimum_width = MAX (min_width, min_height);
  *natural_width = MAX (nat_width, nat_height);
}

static void
hdy_dialer_button_get_preferred_height (GtkWidget *widget,
                                        gint      *minimum_height,
                                        gint      *natural_height)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (hdy_dialer_button_parent_class);
  gint min_width, nat_width, min_height, nat_height;

  widget_class->get_preferred_width (widget, &min_width, &nat_width);
  widget_class->get_preferred_height (widget, &min_height, &nat_height);

  *minimum_height = MAX (min_width, min_height);
  *natural_height = MAX (nat_width, nat_height);
}

static void
hdy_dialer_button_get_preferred_width_for_height (GtkWidget *widget,
                                                  gint       height,
                                                  gint      *minimum_width,
                                                  gint      *natural_width)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (hdy_dialer_button_parent_class);
  gint min_width, nat_width;

  widget_class->get_preferred_width_for_height (widget, height, &min_width, &nat_width);

  *minimum_width = MAX (min_width, height);
  *natural_width = MAX (nat_width, height);
}

static void
hdy_dialer_button_get_preferred_height_for_width (GtkWidget *widget,
                                                  gint       width,
                                                  gint      *minimum_height,
                                                  gint      *natural_height)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (hdy_dialer_button_parent_class);
  gint min_height, nat_height;

  widget_class->get_preferred_width_for_height (widget, width, &min_height, &nat_height);

  *minimum_height = MAX (min_height, width);
  *natural_height = MAX (nat_height, width);
}


static void
hdy_dialer_button_finalize (GObject *object)
{
  HdyDialerButton *self = HDY_DIALER_BUTTON (object);
  HdyDialerButtonPrivate *priv = hdy_dialer_button_get_instance_private(self);

  g_clear_pointer (&priv->symbols, g_free);
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
                      G_PARAM_READABLE);

  props[PROP_SYMBOLS] =
    g_param_spec_string ("symbols",
                         _("Symbols"),
                         _("The dialer symbols of the button"),
                         "",
                         G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/handy/dialer/ui/hdy-dialer-button.ui");
  gtk_widget_class_bind_template_child_private (widget_class, HdyDialerButton, label);
  gtk_widget_class_bind_template_child_private (widget_class, HdyDialerButton, secondary_label);
}

/**
 * hdy_dialer_button_new:
 * @symbols: (nullable): the symbols displayed on the #HdyDialerButton
 *
 * Create a new #HdyDialerButton which displays
 * @symbols. If
 * @symbols is %NULL no symbols will be displayed.
 *
 * Returns: the newly created #HdyDialerButton widget
 */
GtkWidget *hdy_dialer_button_new (const gchar *symbols)
{
  return g_object_new (HDY_TYPE_DIALER_BUTTON, "symbols", symbols, NULL);
}

static void
hdy_dialer_button_init (HdyDialerButton *self)
{
  HdyDialerButtonPrivate *priv = hdy_dialer_button_get_instance_private(self);

  gtk_widget_init_template (GTK_WIDGET (self));

  priv->symbols = NULL;
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
hdy_dialer_button_get_digit (HdyDialerButton *self)
{
  HdyDialerButtonPrivate *priv;
  gchar *symbols;

  g_return_val_if_fail (HDY_IS_DIALER_BUTTON (self), -1);

  priv = hdy_dialer_button_get_instance_private(self);
  symbols = priv->symbols;

  g_return_val_if_fail (symbols != NULL, -1);
  g_return_val_if_fail (g_ascii_isdigit (*symbols), -1);

  return *symbols - '0';
}

/**
 * hdy_dialer_button_get_symbols:
 * @self: a #HdyDialerButton
 *
 * Get the #HdyDialerButton's symbols.
 *
 * Returns: the button's symbols.
 */
const char*
hdy_dialer_button_get_symbols (HdyDialerButton *self)
{
  HdyDialerButtonPrivate *priv = hdy_dialer_button_get_instance_private(self);

  g_return_val_if_fail (HDY_IS_DIALER_BUTTON (self), NULL);

  return priv->symbols;
}

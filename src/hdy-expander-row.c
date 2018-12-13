/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "hdy-expander-row.h"

#include <glib/gi18n.h>

/**
 * SECTION:hdy-expander-row
 * @short_description: A #GtkListBox row used to reveal widgets
 * @Title: HdyExpanderRow
 *
 * The #HdyExpanderRow allows the user to reveal of hide widgets below it. It
 * also allows the user to enable the expansion of the row, allowing to disable
 * all that the row contains.
 */

typedef struct
{
  GtkBox *box;
  GtkToggleButton *button;
  GtkSwitch *enable_switch;
  GtkImage *image;
  GtkRevealer *revealer;
  GtkSeparator *separator;

  gboolean enable_expansion;
  gboolean show_enable_switch;
} HdyExpanderRowPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (HdyExpanderRow, hdy_expander_row, HDY_TYPE_ROW)

enum {
  PROP_0,
  PROP_ENABLE_EXPANSION,
  PROP_SHOW_ENABLE_SWITCH,
  LAST_PROP,
};

static GParamSpec *props[LAST_PROP];

static void
button_active_cb (HdyExpanderRow *self)
{
  HdyExpanderRowPrivate *priv = hdy_expander_row_get_instance_private (self);
  gboolean active;

  active = gtk_toggle_button_get_active (priv->button);
  gtk_image_set_from_icon_name (GTK_IMAGE (priv->image),
                                active ? "pan-down-symbolic" : "pan-end-symbolic",
                                GTK_ICON_SIZE_BUTTON);
}

static void
hdy_expander_row_get_property (GObject    *object,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  HdyExpanderRow *self = HDY_EXPANDER_ROW (object);

  switch (prop_id) {
  case PROP_ENABLE_EXPANSION:
    g_value_set_boolean (value, hdy_expander_row_get_enable_expansion (self));
    break;
  case PROP_SHOW_ENABLE_SWITCH:
    g_value_set_boolean (value, hdy_expander_row_get_show_enable_switch (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
hdy_expander_row_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  HdyExpanderRow *self = HDY_EXPANDER_ROW (object);

  switch (prop_id) {
  case PROP_ENABLE_EXPANSION:
    hdy_expander_row_set_enable_expansion (self, g_value_get_boolean (value));
    break;
  case PROP_SHOW_ENABLE_SWITCH:
    hdy_expander_row_set_show_enable_switch (self, g_value_get_boolean (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
hdy_expander_row_add (GtkContainer *container,
                      GtkWidget    *child)
{
  HdyExpanderRow *self = HDY_EXPANDER_ROW (container);
  HdyExpanderRowPrivate *priv = hdy_expander_row_get_instance_private (self);

  /* When constructing the widget, we want the revealer to be added as the child
   * of the HdyExpanderRow, as an implementation detail.
   */
  if (priv->revealer == NULL)
    GTK_CONTAINER_CLASS (hdy_expander_row_parent_class)->add (container, child);
  else
    gtk_container_add (GTK_CONTAINER (priv->box), child);
}

typedef struct {
  HdyExpanderRow *row;
  GtkCallback callback;
  gpointer callback_data;
} ForallData;

static void
for_non_internal_child (GtkWidget *widget,
                        gpointer   callback_data)
{
  ForallData *data = callback_data;
  HdyExpanderRowPrivate *priv = hdy_expander_row_get_instance_private (data->row);

  if (widget != (GtkWidget *) priv->button &&
      widget != (GtkWidget *) priv->enable_switch &&
      widget != (GtkWidget *) priv->revealer &&
      widget != (GtkWidget *) priv->separator)
    data->callback (widget, data->callback_data);
}

static void
hdy_expander_row_forall (GtkContainer *container,
                         gboolean      include_internals,
                         GtkCallback   callback,
                         gpointer      callback_data)
{
  HdyExpanderRow *self = HDY_EXPANDER_ROW (container);
  HdyExpanderRowPrivate *priv = hdy_expander_row_get_instance_private (self);
  ForallData data;

  if (include_internals) {
    GTK_CONTAINER_CLASS (hdy_expander_row_parent_class)->forall (GTK_CONTAINER (self), include_internals, callback, callback_data);

    return;
  }

  data.row = self;
  data.callback = callback;
  data.callback_data = callback_data;

  GTK_CONTAINER_CLASS (hdy_expander_row_parent_class)->forall (GTK_CONTAINER (self), include_internals, for_non_internal_child, &data);
  GTK_CONTAINER_GET_CLASS (priv->box)->forall (GTK_CONTAINER (priv->box), include_internals, callback, callback_data);
}

static void
hdy_expander_row_activate (HdyRow *row)
{
  HdyExpanderRow *self = HDY_EXPANDER_ROW (row);
  HdyExpanderRowPrivate *priv = hdy_expander_row_get_instance_private (self);

  gtk_revealer_set_reveal_child (priv->revealer, priv->enable_expansion);

  HDY_ROW_CLASS (hdy_expander_row_parent_class)->activate (row);
}

static void
hdy_expander_row_class_init (HdyExpanderRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);
  HdyRowClass *row_class = HDY_ROW_CLASS (klass);

  object_class->get_property = hdy_expander_row_get_property;
  object_class->set_property = hdy_expander_row_set_property;

  container_class->add = hdy_expander_row_add;
  container_class->forall = hdy_expander_row_forall;

  row_class->activate = hdy_expander_row_activate;

  /**
   * HdyExpanderRow:enable-expansion:
   *
   * %TRUE if the expansion is enabled.
   */
  props[PROP_ENABLE_EXPANSION] =
    g_param_spec_boolean ("enable-expansion",
                          _("Enable expansion"),
                          _("Whether the expansion is enabled"),
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * HdyExpanderRow:show-enable-switch:
   *
   * %TRUE if the switch enabling the expansion is visible.
   */
  props[PROP_SHOW_ENABLE_SWITCH] =
    g_param_spec_boolean ("show-enable-switch",
                          _("Show enable switch"),
                          _("Whether the switch enabling the expansion is visible"),
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/handy/dialer/ui/hdy-expander-row.ui");
  gtk_widget_class_bind_template_child_private (widget_class, HdyExpanderRow, box);
  gtk_widget_class_bind_template_child_private (widget_class, HdyExpanderRow, button);
  gtk_widget_class_bind_template_child_private (widget_class, HdyExpanderRow, image);
  gtk_widget_class_bind_template_child_private (widget_class, HdyExpanderRow, revealer);
  gtk_widget_class_bind_template_child_private (widget_class, HdyExpanderRow, separator);
  gtk_widget_class_bind_template_child_private (widget_class, HdyExpanderRow, enable_switch);
  gtk_widget_class_bind_template_callback_full (widget_class, "button_active_cb", G_CALLBACK (button_active_cb));
}

static void
hdy_expander_row_init (HdyExpanderRow *self)
{
  HdyExpanderRowPrivate *priv = hdy_expander_row_get_instance_private (self);

  gtk_widget_init_template (GTK_WIDGET (self));

  priv->enable_expansion = TRUE;

  g_object_bind_property (self, "show-enable-switch", priv->separator, "visible", G_BINDING_SYNC_CREATE);
  g_object_bind_property (self, "show-enable-switch", priv->enable_switch, "visible", G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  g_object_bind_property (self, "enable-expansion", priv->enable_switch, "active", G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  g_object_bind_property (self, "enable-expansion", priv->button, "sensitive", G_BINDING_SYNC_CREATE);
  g_object_bind_property (self, "enable-expansion", priv->box, "sensitive", G_BINDING_SYNC_CREATE);

  button_active_cb (self);
}

/**
 * hdy_expander_row_new:
 *
 * Creates a new #HdyExpanderRow.
 *
 * Returns: a new #HdyExpanderRow
 */
HdyExpanderRow *
hdy_expander_row_new (void)
{
  return g_object_new (HDY_TYPE_EXPANDER_ROW, NULL);
}

/**
 * hdy_expander_row_get_enable_expansion:
 * @self: a #HdyExpanderRow
 *
 * Gets whether the expansion of @self is enabled.
 *
 * Returns: whether the expansion of @self is enabled.
 */
gboolean
hdy_expander_row_get_enable_expansion (HdyExpanderRow *self)
{
  HdyExpanderRowPrivate *priv;

  g_return_val_if_fail (HDY_IS_EXPANDER_ROW (self), FALSE);

  priv = hdy_expander_row_get_instance_private (self);

  return priv->enable_expansion;
}

/**
 * hdy_expander_row_set_enable_expansion:
 * @self: a #HdyExpanderRow
 * @enable_expansion: %TRUE to enable the expansion
 *
 * Sets whether the expansion of @self is enabled.
 */
void
hdy_expander_row_set_enable_expansion (HdyExpanderRow *self,
                                       gboolean        enable_expansion)
{
  HdyExpanderRowPrivate *priv;

  g_return_if_fail (HDY_IS_EXPANDER_ROW (self));

  priv = hdy_expander_row_get_instance_private (self);

  if (priv->enable_expansion == !!enable_expansion)
    return;

  priv->enable_expansion = !!enable_expansion;

  gtk_revealer_set_reveal_child (priv->revealer, priv->enable_expansion);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ENABLE_EXPANSION]);
}

/**
 * hdy_expander_row_get_show_enable_switch:
 * @self: a #HdyExpanderRow
 *
 * Gets whether the switch enabling the expansion of @self is visible.
 *
 * Returns: whether the switch enabling the expansion of @self is visible.
 */
gboolean
hdy_expander_row_get_show_enable_switch (HdyExpanderRow *self)
{
  HdyExpanderRowPrivate *priv;

  g_return_val_if_fail (HDY_IS_EXPANDER_ROW (self), FALSE);

  priv = hdy_expander_row_get_instance_private (self);

  return priv->show_enable_switch;
}

/**
 * hdy_expander_row_set_show_enable_switch:
 * @self: a #HdyExpanderRow
 * @show_enable_switch: %TRUE to show the switch enabling the expansion
 *
 * Sets whether the switch enabling the expansion of @self is visible.
 */
void
hdy_expander_row_set_show_enable_switch (HdyExpanderRow *self,
                                         gboolean        show_enable_switch)
{
  HdyExpanderRowPrivate *priv;

  g_return_if_fail (HDY_IS_EXPANDER_ROW (self));

  priv = hdy_expander_row_get_instance_private (self);

  if (priv->show_enable_switch == !!show_enable_switch)
    return;

  priv->show_enable_switch = !!show_enable_switch;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SHOW_ENABLE_SWITCH]);
}

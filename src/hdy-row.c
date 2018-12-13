/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "hdy-row.h"

#include <glib/gi18n.h>

/**
 * SECTION:hdy-row
 * @short_description: A #GtkListBox row used to present actions
 * @Title: HdyRow
 *
 * The #HdyRow widget can have a title, a subtitle and an icon. The row can
 * receive action widgets at its end or widgets below it.
 *
 * It is convenient to present a list of preferences and their related actions.
 */

typedef struct
{
  GtkBox *box;
  GtkBox *header;
  GtkImage *image;
  GtkLabel *subtitle;
  GtkLabel *title;
  GtkBox *title_box;

  GtkWidget *previous_parent;
} HdyRowPrivate;

static void hdy_row_buildable_init (GtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (HdyRow, hdy_row, GTK_TYPE_LIST_BOX_ROW,
                         G_ADD_PRIVATE (HdyRow)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE,
                         hdy_row_buildable_init))

static GtkBuildableIface *parent_buildable_iface;

enum {
  PROP_0,
  PROP_ICON_NAME,
  PROP_SUBTITLE,
  PROP_TITLE,
  LAST_PROP,
};

static GParamSpec *props[LAST_PROP];

static void
row_activated_cb (HdyRow        *self,
                  GtkListBoxRow *row)
{
  if (self == HDY_ROW (row))
    hdy_row_activate (self);
}

static void
parent_cb (HdyRow *self)
{
  HdyRowPrivate *priv = hdy_row_get_instance_private (self);
  GtkWidget *parent = gtk_widget_get_parent (GTK_WIDGET (self));

  if (priv->previous_parent != NULL) {
    g_signal_handlers_disconnect_by_func (priv->previous_parent, G_CALLBACK (row_activated_cb), self);
    priv->previous_parent = NULL;
  }

  if (parent == NULL || !GTK_IS_LIST_BOX (parent))
    return;

  priv->previous_parent = parent;
  g_signal_connect_swapped (parent, "row-activated", G_CALLBACK (row_activated_cb), self);
}

static void
update_subtitle_visibility (HdyRow *self)
{
  HdyRowPrivate *priv = hdy_row_get_instance_private (self);

  gtk_widget_set_visible (GTK_WIDGET (priv->subtitle),
                          gtk_label_get_text (priv->subtitle) != NULL &&
                          g_strcmp0 (gtk_label_get_text (priv->subtitle), "") != 0);
}

static void
hdy_row_get_property (GObject    *object,
                      guint       prop_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
  HdyRow *self = HDY_ROW (object);

  switch (prop_id) {
  case PROP_ICON_NAME:
    g_value_set_string (value, hdy_row_get_icon_name (self));
    break;
  case PROP_SUBTITLE:
    g_value_set_string (value, hdy_row_get_subtitle (self));
    break;
  case PROP_TITLE:
    g_value_set_string (value, hdy_row_get_title (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
hdy_row_set_property (GObject      *object,
                      guint         prop_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
  HdyRow *self = HDY_ROW (object);

  switch (prop_id) {
  case PROP_ICON_NAME:
    hdy_row_set_icon_name (self, g_value_get_string (value));
    break;
  case PROP_SUBTITLE:
    hdy_row_set_subtitle (self, g_value_get_string (value));
    break;
  case PROP_TITLE:
    hdy_row_set_title (self, g_value_get_string (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
hdy_row_dispose (GObject *object)
{
  HdyRow *self = HDY_ROW (object);
  HdyRowPrivate *priv = hdy_row_get_instance_private (self);

  if (priv->previous_parent == NULL)
    return;

  g_signal_handlers_disconnect_by_func (priv->previous_parent, G_CALLBACK (row_activated_cb), self);
  priv->previous_parent = NULL;
}

static void
hdy_row_add (GtkContainer *container,
             GtkWidget    *child)
{
  HdyRow *self = HDY_ROW (container);
  HdyRowPrivate *priv = hdy_row_get_instance_private (self);

  /* When constructing the widget, we want the box to be added as the child of
   * the GtkListBoxRow, as an implementation detail.
   */
  if (priv->box == NULL)
    GTK_CONTAINER_CLASS (hdy_row_parent_class)->add (container, child);
  else
    gtk_container_add (GTK_CONTAINER (priv->box), child);
}

typedef struct {
  HdyRow *row;
  GtkCallback callback;
  gpointer callback_data;
} ForallData;

static void
for_non_internal_child (GtkWidget *widget,
                        gpointer   callback_data)
{
  ForallData *data = callback_data;
  HdyRowPrivate *priv = hdy_row_get_instance_private (data->row);

  if (widget != (GtkWidget *) priv->box &&
      widget != (GtkWidget *) priv->image &&
      widget != (GtkWidget *) priv->title_box)
    data->callback (widget, data->callback_data);
}

static void
hdy_row_forall (GtkContainer *container,
                gboolean      include_internals,
                GtkCallback   callback,
                gpointer      callback_data)
{
  HdyRow *self = HDY_ROW (container);
  HdyRowPrivate *priv = hdy_row_get_instance_private (self);
  ForallData data;

  if (include_internals) {
    GTK_CONTAINER_CLASS (hdy_row_parent_class)->forall (GTK_CONTAINER (self), include_internals, callback, callback_data);

    return;
  }

  data.row = self;
  data.callback = callback;
  data.callback_data = callback_data;

  GTK_CONTAINER_GET_CLASS (priv->box)->forall (GTK_CONTAINER (priv->box), include_internals, for_non_internal_child, &data);
  GTK_CONTAINER_GET_CLASS (priv->header)->forall (GTK_CONTAINER (priv->header), include_internals, for_non_internal_child, &data);
}

static void
hdy_row_activate_real (HdyRow *self)
{
}

static void
hdy_row_class_init (HdyRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->get_property = hdy_row_get_property;
  object_class->set_property = hdy_row_set_property;
  object_class->dispose = hdy_row_dispose;

  container_class->add = hdy_row_add;
  container_class->forall = hdy_row_forall;

  klass->activate = hdy_row_activate_real;

  /**
   * HdyRow:icon-name:
   *
   * The icon name for this row.
   */
  props[PROP_ICON_NAME] =
    g_param_spec_string ("icon-name",
                         _("Icon name"),
                         _("Icon name"),
                         "",
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * HdyRow:subtitle:
   *
   * The subtitle for this row.
   */
  props[PROP_SUBTITLE] =
    g_param_spec_string ("subtitle",
                         _("Subtitle"),
                         _("Subtitle"),
                         "",
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * HdyRow:title:
   *
   * The title for this row.
   */
  props[PROP_TITLE] =
    g_param_spec_string ("title",
                         _("Title"),
                         _("Title"),
                         "",
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/handy/dialer/ui/hdy-row.ui");
  gtk_widget_class_bind_template_child_private (widget_class, HdyRow, box);
  gtk_widget_class_bind_template_child_private (widget_class, HdyRow, header);
  gtk_widget_class_bind_template_child_private (widget_class, HdyRow, image);
  gtk_widget_class_bind_template_child_private (widget_class, HdyRow, subtitle);
  gtk_widget_class_bind_template_child_private (widget_class, HdyRow, title);
  gtk_widget_class_bind_template_child_private (widget_class, HdyRow, title_box);
}

static void
hdy_row_init (HdyRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  update_subtitle_visibility (self);

  g_signal_connect (self, "notify::parent", G_CALLBACK (parent_cb), NULL);

}

static void
hdy_row_buildable_add_child (GtkBuildable *buildable,
                                           GtkBuilder   *builder,
                                           GObject      *child,
                                           const gchar  *type)
{
  if (type && strcmp (type, "action") == 0)
    hdy_row_add_action (HDY_ROW (buildable), GTK_WIDGET (child));
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
hdy_row_buildable_init (GtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->add_child = hdy_row_buildable_add_child;
}

/**
 * hdy_row_new:
 *
 * Creates a new #HdyRow.
 *
 * Returns: a new #HdyRow
 */
HdyRow *
hdy_row_new (void)
{
  return g_object_new (HDY_TYPE_ROW, NULL);
}

/**
 * hdy_row_get_title:
 * @self: a #HdyRow
 *
 * Gets the title for @self.
 *
 * Returns: the title for @self.
 */
const gchar *
hdy_row_get_title (HdyRow *self)
{
  HdyRowPrivate *priv;

  g_return_val_if_fail (HDY_IS_ROW (self), FALSE);

  priv = hdy_row_get_instance_private (self);

  return gtk_label_get_text (priv->title);
}

/**
 * hdy_row_set_title:
 * @self: a #HdyRow
 * @title: the title
 *
 * Sets the title for @self.
 */
void
hdy_row_set_title (HdyRow      *self,
                   const gchar *title)
{
  HdyRowPrivate *priv = hdy_row_get_instance_private (self);

  g_return_if_fail (HDY_IS_ROW (self));

  if (g_strcmp0 (gtk_label_get_text (priv->title), title) == 0)
    return;

  gtk_label_set_text (priv->title, title);
  gtk_widget_set_visible (GTK_WIDGET (priv->title),
                          title != NULL && g_strcmp0 (title, "") != 0);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TITLE]);
}

/**
 * hdy_row_get_subtitle:
 * @self: a #HdyRow
 *
 * Gets the subtitle for @self.
 *
 * Returns: the subtitle for @self.
 */
const gchar *
hdy_row_get_subtitle (HdyRow *self)
{
  HdyRowPrivate *priv;

  g_return_val_if_fail (HDY_IS_ROW (self), FALSE);

  priv = hdy_row_get_instance_private (self);

  return gtk_label_get_text (priv->subtitle);
}

/**
 * hdy_row_set_subtitle:
 * @self: a #HdyRow
 * @subtitle: the subtitle
 *
 * Sets the subtitle for @self.
 */
void
hdy_row_set_subtitle (HdyRow      *self,
                      const gchar *subtitle)
{
  HdyRowPrivate *priv = hdy_row_get_instance_private (self);

  g_return_if_fail (HDY_IS_ROW (self));

  if (g_strcmp0 (gtk_label_get_text (priv->subtitle), subtitle) == 0)
    return;

  gtk_label_set_text (priv->subtitle, subtitle);
  gtk_widget_set_visible (GTK_WIDGET (priv->subtitle),
                          subtitle != NULL && g_strcmp0 (subtitle, "") != 0);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SUBTITLE]);
}

/**
 * hdy_row_get_icon_name:
 * @self: a #HdyRow
 *
 * Gets the icon name for @self.
 *
 * Returns: the icon name for @self.
 */
const gchar *
hdy_row_get_icon_name (HdyRow *self)
{
  HdyRowPrivate *priv;
  const gchar *icon_name;

  g_return_val_if_fail (HDY_IS_ROW (self), FALSE);

  priv = hdy_row_get_instance_private (self);

  gtk_image_get_icon_name (priv->image, &icon_name, NULL);

  return icon_name;
}

/**
 * hdy_row_set_icon_name:
 * @self: a #HdyRow
 * @icon_name: the icon name
 *
 * Sets the icon name for @self.
 */
void
hdy_row_set_icon_name (HdyRow      *self,
                       const gchar *icon_name)
{
  HdyRowPrivate *priv = hdy_row_get_instance_private (self);
  const gchar *old_icon_name;

  g_return_if_fail (HDY_IS_ROW (self));

  gtk_image_get_icon_name (priv->image, &old_icon_name, NULL);
  if (g_strcmp0 (old_icon_name, icon_name) == 0)
    return;

  gtk_image_set_from_icon_name (priv->image, icon_name, GTK_ICON_SIZE_INVALID);
  gtk_widget_set_visible (GTK_WIDGET (priv->image),
                          icon_name != NULL && g_strcmp0 (icon_name, "") != 0);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ICON_NAME]);
}

/**
 * hdy_row_add_action:
 * @self: a #HdyRow
 * @widget: (allow-none): the action widget
 *
 * Adds an action widget to @self.
 */
void
hdy_row_add_action (HdyRow    *self,
                    GtkWidget *widget)
{
  HdyRowPrivate *priv = hdy_row_get_instance_private (self);

  gtk_box_pack_end (priv->header, widget, FALSE, TRUE, 0);
}

void
hdy_row_activate (HdyRow *self)
{
  g_return_if_fail (HDY_IS_ROW (self));

  HDY_ROW_GET_CLASS (self)->activate (self);
}

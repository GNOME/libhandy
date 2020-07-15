/*
 * Copyright (C) 2020 Ujjwal Kumar <ujjwalkumar0501@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "hdy-grid.h"

/**
 * SECTION:hdy-grid
 * @short_description: An adaptive grid widget.
 * @Title: HdyGrid
 *
 * The #HdyGrid widget repositions children of a row when enough width is
 * unavailable.
 *
 * # CSS nodes
 *
 * #HdyGrid has a single CSS node with name grid.
 *
 * Since: 1.0
 */

typedef struct _HdyGridChild        HdyGridChild;

enum {
  CHILD_PROP_0,
  CHILD_PROP_WEIGHT,
  CHILD_PROP_POSITION,
  LAST_CHILD_PROP
};

typedef struct
{
  GList          *children;
} HdyGridPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (HdyGrid, hdy_grid, GTK_TYPE_CONTAINER)

struct _HdyGridChild
{
  GtkWidget *widget;

  guint      weight;
  guint      position;
};

static GParamSpec *child_props[LAST_CHILD_PROP];

static gint
widget_weight_comparator (HdyGridChild *a,
                          HdyGridChild *b)
{
  if (a->weight < b->weight)
    return -1;
  else if (a->weight == b->weight)
    return 0;
  else
    return 1;
}

static gint
widget_position_comparator (GtkRequestedSize *a,
                            GtkRequestedSize *b)
{
  HdyGridChild *child_a = a->data, *child_b = b->data;

  if (child_a->position < child_b->position)
    return -1;
  else if (child_a->position == child_b->position)
    return 0;
  else
    return 1;
}

static gint
search_widget_comparator (HdyGridChild *a,
                          GtkWidget    *b)
{
  if (a->widget == b)
    return 0;
  else
    return -1;
}

static void
reassign_children_position (GList *l)
{
  for (gint i = 0; l; l = l->next, i++) {
    HdyGridChild *child = l->data;
    child->position = i;
  }
}

static void
compute_height_for_width (HdyGrid *widget,
                          gint     width,
                          gint    *minimum_height,
                          gint    *natural_height)
{
  HdyGridPrivate *priv = hdy_grid_get_instance_private (widget);
  HdyGridChild *child;
  GList *list;
  GtkRequestedSize *sizes;
  gint min, nat, i, j = 0, sum = 0, minimum = 0, natural = 0, child_count;

  child_count = g_list_length (priv->children);
  list = g_list_copy (priv->children);
  list = g_list_sort (list, (GCompareFunc) widget_weight_comparator);

  sizes = g_newa (GtkRequestedSize, child_count);
  memset (sizes, 0, child_count * sizeof (GtkRequestedSize));

  for (i = 0; list; list = list->next, i++) {
    child = list->data;
    gtk_widget_get_preferred_width (child->widget, &sizes[i].minimum_size, &sizes[i].natural_size);
    sizes[i].data = list->data;
  }

  for (i = 0; i < child_count; i++) {
    if (sum + sizes[i].minimum_size > width) {
      gtk_distribute_natural_allocation (width - sum, i - j, &sizes[j]);
      min = 0;
      nat = 0;

      for (gint child_min, child_nat; j < i; j++) {
        child = sizes[j].data;
        gtk_widget_get_preferred_height_for_width (child->widget,
                                                   sizes[j].minimum_size,
                                                   &child_min, &child_nat);
        min = MAX (min, child_min);
        nat = MAX (nat, child_nat);
      }

      minimum += min;
      natural += nat;
      sum = sizes[i].minimum_size;
    } else {
      sum += sizes[i].minimum_size;
    }
  }

  gtk_distribute_natural_allocation (width - sum, i - j, &sizes[j]);
  min = 0;
  nat = 0;

  for (gint child_min, child_nat; j < i; j++) {
    child = sizes[j].data;
    gtk_widget_get_preferred_height_for_width (child->widget,
                                               sizes[j].minimum_size,
                                               &child_min, &child_nat);
    min = MAX (min, child_min);
    nat = MAX (nat, child_nat);
  }

  minimum += min;
  natural += nat;

  if (minimum_height)
    *minimum_height = minimum;
  if (natural_height)
    *natural_height = natural;
}

/* This private method is prefixed by the call name because it will be a virtual
 * method in GTK 4.
 */
static void
hdy_grid_measure (GtkWidget      *widget,
                  GtkOrientation  orientation,
                  int             for_size,
                  int            *minimum,
                  int            *natural,
                  int            *minimum_baseline,
                  int            *natural_baseline)
{
  HdyGridPrivate *priv = hdy_grid_get_instance_private (HDY_GRID (widget));
  HdyGridChild *child;
  GList *list;
  gint min, nat, child_min, child_nat;

  min = 0;
  nat = 0;
  list = g_list_copy (priv->children);
  list = g_list_sort (list, (GCompareFunc) widget_weight_comparator);

  if (for_size < 1) {
    for (gint weight_width = 0, prev_weight = 0; list; list = list->next) {
      child = list->data;

      if (orientation == GTK_ORIENTATION_HORIZONTAL)
        gtk_widget_get_preferred_width (child->widget, &child_min, &child_nat);
      else
        gtk_widget_get_preferred_height (child->widget, &child_min, &child_nat);

      if (prev_weight == child->weight)
        weight_width += child_min;
      else {
        weight_width = child_min;
        prev_weight = child->weight;
      }

      min = MAX (min, weight_width);
      nat += child_nat;
    }
  } else {
    if (orientation == GTK_ORIENTATION_VERTICAL)
      compute_height_for_width (HDY_GRID (widget), for_size, &min, &nat);
  }

  if (minimum)
    *minimum = min;
  if (natural)
    *natural = nat;

  if (minimum_baseline)
    *natural_baseline = -1;
  if (natural_baseline)
    *minimum_baseline = -1;
}

static void
hdy_grid_get_preferred_width (GtkWidget *widget,
                              gint      *minimum_width,
                              gint      *natural_width)
{
  hdy_grid_measure (widget, GTK_ORIENTATION_HORIZONTAL, -1,
                    minimum_width, natural_width, NULL, NULL);
}

static void
hdy_grid_get_preferred_height (GtkWidget *widget,
                               gint      *minimum_height,
                               gint      *natural_height)
{
  hdy_grid_measure (widget, GTK_ORIENTATION_VERTICAL, -1,
                    minimum_height, natural_height, NULL, NULL);
}

static void
hdy_grid_get_preferred_width_for_height (GtkWidget *widget,
                                         gint       height,
                                         gint      *minimum_width,
                                         gint      *natural_width)
{
  hdy_grid_measure (widget, GTK_ORIENTATION_HORIZONTAL, -1,
                    minimum_width, natural_width, NULL, NULL);
}

static void
hdy_grid_get_preferred_height_for_width (GtkWidget *widget,
                                         gint       width,
                                         gint      *minimum_height,
                                         gint      *natural_height)
{
  hdy_grid_measure (widget, GTK_ORIENTATION_VERTICAL, width,
                    minimum_height, natural_height, NULL, NULL);
}

static void
hdy_grid_size_allocate (GtkWidget     *widget,
                        GtkAllocation *allocation)
{
  HdyGridPrivate *priv = hdy_grid_get_instance_private (HDY_GRID (widget));
  HdyGridChild *child;
  GList *list, *groups;
  GtkRequestedSize *sizes;
  GtkAllocation child_allocation;
  gint child_count, i, j = 0, sum = 0;

  child_count = g_list_length (priv->children);
  list = g_list_copy (priv->children);
  list = g_list_sort (list, (GCompareFunc) widget_weight_comparator);

  sizes = g_newa (GtkRequestedSize, child_count);
  memset (sizes, 0, child_count * sizeof (GtkRequestedSize));

  for (i = 0; list; list = list->next, i++) {
    child = list->data;
    gtk_widget_get_preferred_width (child->widget,
                                    &sizes[i].minimum_size,
                                    &sizes[i].natural_size);
    sizes[i].data = list->data;
  }

  list = NULL;
  groups = NULL;
  for (i = 0; i < child_count; i++) {
    if (sum + sizes[i].minimum_size > allocation->width) {
      gtk_distribute_natural_allocation (allocation->width - sum, i - j, &sizes[j]);

      groups = g_list_append (groups, list);
      list = g_list_append (NULL, &sizes[i]);
      sum = sizes[i].minimum_size;
      j = i;
    } else {
      list = g_list_append (list, &sizes[i]);
      sum += sizes[i].minimum_size;
    }
  }

  gtk_distribute_natural_allocation (allocation->width - sum, i - j, &sizes[j]);
  groups = g_list_append (groups, list);

  /* allocate positions to widgets */
  child_allocation.y = allocation->y;
  list = groups;
  for (; list; list = list->next) {
    GList *group = list->data, *group_member;
    GtkRequestedSize *child_request;
    gint line_height = 0;

    group = g_list_sort (group, (GCompareFunc) widget_position_comparator);
    list->data = group;

    child_allocation.x = allocation->x;
    group_member = g_list_first (group);
    while (group_member) {
      child_request = group_member->data;
      child = child_request->data;
      child_allocation.width = child_request->minimum_size;

      gtk_widget_get_preferred_height_for_width (child->widget,
                                                 child_allocation.width,
                                                 &child_allocation.height,
                                                 NULL);

      gtk_widget_size_allocate_with_baseline (child->widget, &child_allocation, -1);

      line_height = MAX (child_allocation.height, line_height);
      child_allocation.x += child_allocation.width;
      group_member = group_member->next;
    }

    child_allocation.y += line_height;
  }

  GTK_WIDGET_CLASS (hdy_grid_parent_class)->size_allocate (widget, allocation);
  gtk_widget_set_clip (widget, allocation);
}

static void
hdy_grid_forall (GtkContainer *container,
                 gboolean      include_internals,
                 GtkCallback   callback,
                 gpointer      callback_data)
{
  HdyGridPrivate *priv = hdy_grid_get_instance_private (HDY_GRID (container));
  HdyGridChild *child;
  GList *l;

  for (l = priv->children; l; l = l->next) {
    child = l->data;
    (* callback) (child->widget, callback_data);
  }
}

static void
hdy_grid_add (GtkContainer *container,
              GtkWidget    *widget)
{
  HdyGridPrivate *priv = hdy_grid_get_instance_private (HDY_GRID (container));
  HdyGridChild *child;

  child = g_slice_new (HdyGridChild);
  child->widget = widget;
  child->weight = 0;
  child->position = g_list_length (priv->children);

  priv->children = g_list_append (priv->children, child);

  gtk_widget_set_parent (widget, GTK_WIDGET (container));
}

static void
hdy_grid_remove (GtkContainer *self,
                 GtkWidget    *child)
{
  HdyGridPrivate *priv;
  GList *l;

  HDY_IS_GRID (self);
  GTK_IS_WIDGET (child);

  priv = hdy_grid_get_instance_private (HDY_GRID (self));
  l = g_list_find_custom (priv->children, child, (GCompareFunc) search_widget_comparator);

  priv->children = g_list_delete_link (priv->children, l);
  reassign_children_position (priv->children);
}

static void
hdy_grid_get_child_property (GtkContainer *object,
                             GtkWidget    *child,
                             guint         prop_id,
                             GValue       *value,
                             GParamSpec   *pspec)
{
  switch (prop_id) {
  case CHILD_PROP_WEIGHT:
    g_value_set_uint (value, hdy_grid_get_child_weight (HDY_GRID (object), child));
    break;
  case CHILD_PROP_POSITION:
    g_value_set_uint (value, hdy_grid_get_child_position (HDY_GRID (object), child));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
hdy_grid_set_child_property (GtkContainer *object,
                             GtkWidget    *child,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  switch (prop_id) {
  case CHILD_PROP_WEIGHT:
    hdy_grid_set_child_weight (HDY_GRID (object), child, g_value_get_uint (value));
    break;
  case CHILD_PROP_POSITION:
    hdy_grid_set_child_position (HDY_GRID (object), child, g_value_get_uint (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
hdy_grid_class_init (HdyGridClass *klass)
{
  GtkWidgetClass *widget_class = (GtkWidgetClass*) klass;
  GtkContainerClass *container_class = (GtkContainerClass*) klass;

  widget_class->get_preferred_width = hdy_grid_get_preferred_width;
  widget_class->get_preferred_height = hdy_grid_get_preferred_height;
  widget_class->get_preferred_width_for_height = hdy_grid_get_preferred_width_for_height;
  widget_class->get_preferred_height_for_width = hdy_grid_get_preferred_height_for_width;
  widget_class->size_allocate = hdy_grid_size_allocate;

  container_class->add = hdy_grid_add;
  container_class->remove = hdy_grid_remove;
  container_class->forall = hdy_grid_forall;
  container_class->set_child_property = hdy_grid_set_child_property;
  container_class->get_child_property = hdy_grid_get_child_property;

  child_props[CHILD_PROP_WEIGHT] =
        g_param_spec_uint ("weight",
                          _("Widget weight"),
                          _("Weight of widget determines which widget is relocated when enough width is unavailable."),
                          0,
                          G_MAXUINT,
                          0,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  child_props[CHILD_PROP_POSITION] =
        g_param_spec_uint ("position",
                          _("Widget position"),
                          _("Position of the widget in a row."),
                          0,
                          G_MAXUINT,
                          0,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  gtk_container_class_install_child_properties (container_class, LAST_CHILD_PROP, child_props);
  gtk_widget_class_set_css_name (widget_class, "grid");
}

static void
hdy_grid_init (HdyGrid *self)
{
  gtk_widget_set_has_window (GTK_WIDGET (self), FALSE);
}

GtkWidget *
hdy_grid_new (void)
{
  return g_object_new (HDY_TYPE_GRID, NULL);
}

/**
 * hdy_grid_get_child_weight:
 * @self: a #HdyGrid
 * @child: a child
 *
 * Gets the weight of child.
 * See hdy_grid_set_child_weight().
 *
 * Returns: the weight of child, -1 if child is not found.
 *
 * Since: 1.0
 */
guint
hdy_grid_get_child_weight (HdyGrid   *self,
                           GtkWidget *child)
{
  HdyGridPrivate *priv;
  GList *l;
  HdyGridChild *child_info;

  HDY_IS_GRID (self);
  g_return_val_if_fail (GTK_IS_WIDGET (child), -1);

  priv = hdy_grid_get_instance_private (self);
  l = g_list_find_custom (priv->children, child, (GCompareFunc) search_widget_comparator);

  if (l) {
    child_info = l->data;
    return child_info->weight;
  } else {
    return -1;
  }
}

/**
 * hdy_grid_set_child_weight:
 * @self: a #HdyGrid
 * @child: a child
 * @position: the new weight for child
 *
 * Sets the weight for the child.
 * See hdy_grid_get_child_weight().
 *
 * Since: 1.0
 */
void
hdy_grid_set_child_weight (HdyGrid   *self,
                           GtkWidget *child,
                           guint      weight)
{
  HdyGridPrivate *priv;
  GList *l;
  HdyGridChild *child_info;

  HDY_IS_GRID (self);
  GTK_IS_WIDGET (child);

  priv = hdy_grid_get_instance_private (self);
  l = g_list_find_custom (priv->children, child, (GCompareFunc) search_widget_comparator);

  if (l) {
    child_info = l->data;
    child_info->weight = weight;
    g_object_notify_by_pspec (G_OBJECT (child), child_props[CHILD_PROP_WEIGHT]);
  } else g_warning ("child not found");
}

/**
 * hdy_grid_get_child_position:
 * @self: a #HdyGrid
 * @child: a child
 *
 * Gets the position of child.
 * See hdy_grid_set_child_position().
 *
 * Returns: the position of child, -1 if child is not found.
 *
 * Since: 1.0
 */
guint
hdy_grid_get_child_position (HdyGrid   *self,
                             GtkWidget *child)
{
  HdyGridPrivate *priv;
  GList *l;
  HdyGridChild *child_info;

  HDY_IS_GRID (self);
  g_return_val_if_fail (GTK_IS_WIDGET (child), -1);

  priv = hdy_grid_get_instance_private (self);
  l = g_list_find_custom (priv->children, child, (GCompareFunc) search_widget_comparator);

  if (l) {
    child_info = l->data;
    return child_info->position;
  } else {
    return -1;
  }
}

/**
 * hdy_grid_set_child_position:
 * @self: a #HdyGrid
 * @child: a child
 * @position: the new position for child
 *
 * Sets the position for the child.
 * See hdy_grid_get_child_position().
 *
 * Since: 1.0
 */
void
hdy_grid_set_child_position (HdyGrid   *self,
                             GtkWidget *child,
                             guint      position)
{
  HdyGridPrivate *priv;
  GList *l;

  HDY_IS_GRID (self);
  GTK_IS_WIDGET (child);

  priv = hdy_grid_get_instance_private (self);
  l = g_list_find_custom (priv->children, child, (GCompareFunc) search_widget_comparator);

  if (l) {
    priv->children = g_list_remove_link (priv->children, l);
    priv->children = g_list_insert (priv->children, l, position);
    reassign_children_position (priv->children);
    g_object_notify_by_pspec (G_OBJECT (child), child_props[CHILD_PROP_POSITION]);
  }
}

/*
 * Copyright (C) 2020 Ujjwal Kumar <ujjwalkumar0501@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "hdy-grid.h"

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
widget_weight_comparator (gconstpointer a,
                          gconstpointer b)
{
  if (((HdyGridChild*) a)->weight < ((HdyGridChild*) b)->weight)
    return -1;
  else if (((HdyGridChild*) a)->weight == ((HdyGridChild*) b)->weight)
    return 0;
  else
    return 1;
}

static gint
widget_position_comparator (gconstpointer a,
                            gconstpointer b)
{
  HdyGridChild *child_a = ((GtkRequestedSize*) a)->data,
               *child_b = ((GtkRequestedSize*) b)->data;

  if (child_a->position < child_b->position)
    return -1;
  else if (child_a->position == child_b->position)
    return 0;
  else
    return 1;
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
  GList *list = priv->children;
  gint min, nat, child_min, child_nat, child_count = g_list_length (list);
  gint *width_of_weights;

  min = 0;
  nat = 0;

  width_of_weights = g_malloc (child_count * sizeof (guint));
  memset (width_of_weights, 0, child_count * sizeof (guint));

  for (gint i = 0; list; list = list->next, i++) {
    child = list->data;

    if (child->weight >= child_count)
      g_error ("Invalid weight asigned to child");

    if (orientation == GTK_ORIENTATION_HORIZONTAL)
      gtk_widget_get_preferred_width (child->widget, &child_min, &child_nat);
    else
      gtk_widget_get_preferred_height (child->widget, &child_min, &child_nat);

    width_of_weights[child->weight] += child_min;

    min = MAX (min, width_of_weights[child->weight]);
    nat += child_nat;
  }

  if (minimum != NULL) *minimum = min;
  if (natural != NULL) *natural = nat;

  if (minimum_baseline != NULL) *natural_baseline = -1;
  if (natural_baseline != NULL) *minimum_baseline = -1;
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
  HdyGridPrivate *priv = hdy_grid_get_instance_private (HDY_GRID (widget));
  HdyGridChild *child;
  GList *children_copy, *list;
  GtkRequestedSize *sizes;
  gint min, nat, child_min, child_nat,
       child_count = g_list_length (priv->children),
       i, j, sum;

  children_copy = g_list_copy (priv->children);
  children_copy = g_list_sort (children_copy, (GCompareFunc) widget_weight_comparator);

  sizes = g_newa (GtkRequestedSize, child_count * sizeof (GtkRequestedSize));
  memset (sizes, 0, child_count * sizeof (GtkRequestedSize));

  list = children_copy;
  for (i = 0; list; list = list->next, i++) {
    child = list->data;
    gtk_widget_get_preferred_width (child->widget, &sizes[i].minimum_size, &sizes[i].natural_size);
    sizes[i].data = list->data;
  }

  for (i = 0, sum = 0, j = 0; i < child_count; i++) {
    if (sum + sizes[i].minimum_size > width)
    {
      gtk_distribute_natural_allocation (width - sum, i - j, &sizes[j]);
      min = 0;
      nat = 0;

      for (; j < i; j++) {
        child = sizes[j].data;
        gtk_widget_get_preferred_height_for_width (child->widget,
                                                   sizes[j].minimum_size,
                                                   &child_min, &child_nat);
        min = MAX (min, child_min);
        nat = MAX (nat, child_nat);
      }

      *minimum_height += min;
      *natural_height += nat;
      sum = sizes[i].minimum_size;
    }
    else
    {
      sum += sizes[i].minimum_size;
    }
  }

  gtk_distribute_natural_allocation (width - sum, i - j, &sizes[j]);
  min = 0;
  nat = 0;

  for (; j < i; j++) {
    child = sizes[j].data;
    gtk_widget_get_preferred_height_for_width (child->widget,
                                               sizes[j].minimum_size,
                                               &child_min, &child_nat);
    min = MAX (min, child_min);
    nat = MAX (nat, child_nat);
  }

  *minimum_height += min;
  *natural_height += nat;
}

static void
hdy_grid_size_allocate (GtkWidget     *widget,
                        GtkAllocation *allocation)
{
  HdyGridPrivate *priv = hdy_grid_get_instance_private (HDY_GRID (widget));
  HdyGridChild *child;
  GList *children_copy, *list, *groups;
  GtkRequestedSize *sizes;
  GtkAllocation child_allocation;
  gint child_count = g_list_length (priv->children),
       i, j, sum;

  children_copy = g_list_copy (priv->children);
  children_copy = g_list_sort (children_copy, (GCompareFunc) widget_weight_comparator);

  sizes = g_newa (GtkRequestedSize, child_count * sizeof (GtkRequestedSize));
  memset (sizes, 0, child_count * sizeof (GtkRequestedSize));

  list = children_copy;
  for (i = 0; list; list = list->next, i++) {
    child = list->data;
    gtk_widget_get_preferred_width (child->widget, &sizes[i].minimum_size, &sizes[i].natural_size);
    sizes[i].data = list->data;
  }

  list = NULL; groups = NULL;
  for (i = 0, sum = 0, j = 0; i < child_count; i++) {
    if (sum + sizes[i].minimum_size > allocation->width)
    {
      gtk_distribute_natural_allocation (allocation->width - sum, i - j, &sizes[j]);

      groups = g_list_append (groups, list);
      list = g_list_append (NULL, &sizes[i]);
      sum = sizes[i].minimum_size;
      j = i;
    }
    else
    {
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
  HdyGrid *grid = HDY_GRID (container);
  HdyGridPrivate *priv = hdy_grid_get_instance_private (grid);
  HdyGridChild *child;
  GList *list;

  list = g_list_first (priv->children);
  while (list)
    {
      child = list->data;
      list  = list->next;

      (* callback) (child->widget, callback_data);
    }
}

static void
hdy_grid_add (GtkContainer *container,
              GtkWidget    *widget)
{
  HdyGrid *grid = HDY_GRID (container);
  HdyGridPrivate *priv = hdy_grid_get_instance_private (grid);
  HdyGridChild *child;

  child = g_slice_new (HdyGridChild);
  child->widget = widget;
  child->weight = 0;
  child->position = g_list_length (priv->children);

  priv->children = g_list_append (priv->children, child);

  gtk_widget_set_parent (widget, GTK_WIDGET (grid));
}

static void
hdy_grid_remove (GtkContainer *container,
                 GtkWidget    *widget)
{
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
    g_value_set_uint (value, hdy_grid_get_weight (object, child));
    break;
  case CHILD_PROP_POSITION:
    g_value_set_uint (value, hdy_grid_get_position (object, child));
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
    hdy_grid_set_weight (GTK_CONTAINER (object), child, g_value_get_uint (value));
    break;
  case CHILD_PROP_POSITION:
    hdy_grid_set_position (GTK_CONTAINER (object), child, g_value_get_uint (value));
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
                          _("Weight of widget determine the priority in order which it will be shifted."),
                          0,
                          G_MAXUINT,
                          0,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  child_props[CHILD_PROP_POSITION] =
        g_param_spec_uint ("position",
                          _("Widget position"),
                          _("Weight of widget determine the priority in order which it will be shifted."),
                          0,
                          G_MAXUINT,
                          0,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  gtk_container_class_install_child_properties (container_class, LAST_CHILD_PROP, child_props);

  gtk_container_class_handle_border_width (container_class);
}

static void
hdy_grid_init (HdyGrid *self)
{
  HdyGridPrivate *priv = hdy_grid_get_instance_private (self);

  priv->children = NULL;

  gtk_widget_set_has_window (GTK_WIDGET (self), FALSE);
}

GtkWidget *
hdy_grid_new (void)
{
  return g_object_new (HDY_TYPE_GRID, NULL);
}

guint
hdy_grid_get_weight (GtkContainer *container,
                     GtkWidget    *child)
{
  HdyGridPrivate *priv = hdy_grid_get_instance_private (HDY_GRID (container));
  GList *list = priv->children;
  HdyGridChild *child_info = NULL;

  g_return_val_if_fail (child != NULL, -1);
  g_return_val_if_fail (list != NULL, -1);

  while (list) {
    child_info = list->data;
    if (child_info->widget == child)
      break;

    list = list->next;
  }

  return child_info->weight;
}

void
hdy_grid_set_weight (GtkContainer *container,
                     GtkWidget    *child,
                     guint         weight)
{
  HdyGridPrivate *priv = hdy_grid_get_instance_private (HDY_GRID (container));
  GList *list = priv->children;
  HdyGridChild *child_info = NULL;

  g_return_if_fail (child != NULL);
  g_return_if_fail (list != NULL);

  while (list) {
    child_info = list->data;
    if (child_info->widget == child)
      break;

    list = list->next;
  }

  child_info->weight = weight;

  g_object_notify_by_pspec (G_OBJECT (child), child_props[CHILD_PROP_WEIGHT]);
}

guint
hdy_grid_get_position (GtkContainer *container,
                       GtkWidget    *child)
{
  HdyGridPrivate *priv = hdy_grid_get_instance_private (HDY_GRID (container));
  GList *list = priv->children;
  HdyGridChild *child_info = NULL;

  g_return_val_if_fail (child != NULL, -1);
  g_return_val_if_fail (list != NULL, -1);

  while (list) {
    child_info = list->data;
    if (child_info->widget == child)
      break;

    list = list->next;
  }

  return child_info->position;
}

void
hdy_grid_set_position (GtkContainer *container,
                       GtkWidget    *child,
                       guint         position)
{
  HdyGridPrivate *priv = hdy_grid_get_instance_private (HDY_GRID (container));
  GList *list = priv->children;
  HdyGridChild *child_info = NULL;

  g_return_if_fail (child != NULL);
  g_return_if_fail (list != NULL);

  while (list) {
    child_info = list->data;
    if (child_info->widget == child)
      break;

    list = list->next;
  }

  child_info->position = position;

  g_object_notify_by_pspec (G_OBJECT (child), child_props[CHILD_PROP_POSITION]);
}

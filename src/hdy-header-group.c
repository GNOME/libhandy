/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "hdy-header-group.h"

struct _HdyHeaderGroupChild
{
  GObject parent_instance;

  HdyHeaderGroupChildType type;
  GObject *object;
};

G_DEFINE_TYPE (HdyHeaderGroupChild, hdy_header_group_child, G_TYPE_OBJECT)

struct _HdyHeaderGroup
{
  GObject parent_instance;

  GSList *children;
  gboolean decorate_all;
  HdyHeaderGroup *parent;
  gchar *layout;
};

static void hdy_header_group_buildable_init (GtkBuildableIface *iface);
static gboolean hdy_header_group_buildable_custom_tag_start (GtkBuildable  *buildable,
                                                             GtkBuilder    *builder,
                                                             GObject       *child,
                                                             const gchar   *tagname,
                                                             GMarkupParser *parser,
                                                             gpointer      *data);
static void hdy_header_group_buildable_custom_finished (GtkBuildable *buildable,
                                                        GtkBuilder   *builder,
                                                        GObject      *child,
                                                        const gchar  *tagname,
                                                        gpointer      user_data);

G_DEFINE_TYPE_WITH_CODE (HdyHeaderGroup, hdy_header_group, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE,
                         hdy_header_group_buildable_init))

enum {
  PROP_0,
  PROP_DECORATE_ALL,
  N_PROPS
};

static GParamSpec *props [N_PROPS];

static void update_decoration_layouts (HdyHeaderGroup *self);

static void
hdy_header_group_child_finalize (GObject *object)
{
  HdyHeaderGroupChild *self = (HdyHeaderGroupChild *)object;

  g_clear_object (&self->object);

  G_OBJECT_CLASS (hdy_header_group_child_parent_class)->finalize (object);
}

static void
hdy_header_group_child_class_init (HdyHeaderGroupChildClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = hdy_header_group_child_finalize;
}

static void
hdy_header_group_child_init (HdyHeaderGroupChild *self)
{
}

static void
hdy_header_group_child_set_decoration_layout (HdyHeaderGroupChild *self,
                                              const gchar         *layout)
{
  g_assert (HDY_IS_HEADER_GROUP_CHILD (self));

  switch (self->type) {
  case HDY_HEADER_GROUP_CHILD_TYPE_HEADER_BAR:
    hdy_header_bar_set_decoration_layout (HDY_HEADER_BAR (self->object), layout);
    break;
  case HDY_HEADER_GROUP_CHILD_TYPE_GTK_HEADER_BAR:
    gtk_header_bar_set_decoration_layout (GTK_HEADER_BAR (self->object), layout);
    break;
  case HDY_HEADER_GROUP_CHILD_TYPE_HEADER_GROUP:
    {
      HdyHeaderGroup *group = HDY_HEADER_GROUP (self->object);

      g_free (group->layout);
      group->layout = g_strdup (layout);

      update_decoration_layouts (group);
    }
    break;
  case HDY_HEADER_GROUP_CHILD_TYPE_INVALID:
  default:
    g_assert_not_reached ();
  }
}

static gboolean
hdy_header_group_child_get_mapped (HdyHeaderGroupChild *self)
{
  g_assert (HDY_IS_HEADER_GROUP_CHILD (self));

  switch (self->type) {
  case HDY_HEADER_GROUP_CHILD_TYPE_HEADER_BAR:
  case HDY_HEADER_GROUP_CHILD_TYPE_GTK_HEADER_BAR:
    return gtk_widget_get_mapped (GTK_WIDGET (self->object));
  case HDY_HEADER_GROUP_CHILD_TYPE_HEADER_GROUP:
    for (GSList *children = HDY_HEADER_GROUP (self->object)->children;
         children != NULL;
         children = children->next)
      if (hdy_header_group_child_get_mapped (HDY_HEADER_GROUP_CHILD (children->data)))
          return TRUE;

    return FALSE;
  case HDY_HEADER_GROUP_CHILD_TYPE_INVALID:
  default:
    g_assert_not_reached ();
  }
}

static HdyHeaderGroupChild *
get_child_for_object (HdyHeaderGroup *self,
                      gpointer        object)
{
  GSList *children;

  for (children = self->children; children != NULL; children = children->next) {
    HdyHeaderGroupChild *child = HDY_HEADER_GROUP_CHILD (children->data);

    g_assert (child);

    if (child->object == object)
      return child;
  }

  return NULL;
}

static void
update_decoration_layouts (HdyHeaderGroup *self)
{
  GSList *children;
  GtkSettings *settings;
  HdyHeaderGroupChild *start_child = NULL, *end_child = NULL;
  g_autofree gchar *layout = NULL;
  g_autofree gchar *start_layout = NULL;
  g_autofree gchar *end_layout = NULL;
  g_auto(GStrv) ends = NULL;

  g_return_if_fail (HDY_IS_HEADER_GROUP (self));

  children = self->children;

  if (children == NULL)
    return;

  settings = gtk_settings_get_default ();
  if (self->layout)
    layout = g_strdup (self->layout);
  else
    g_object_get (G_OBJECT (settings), "gtk-decoration-layout", &layout, NULL);
  if (layout == NULL)
    layout = g_strdup (":");

  if (self->decorate_all) {
    for (; children != NULL; children = children->next)
      hdy_header_group_child_set_decoration_layout (HDY_HEADER_GROUP_CHILD (children->data), layout);

    return;
  }

  for (; children != NULL; children = children->next) {
    HdyHeaderGroupChild *child = HDY_HEADER_GROUP_CHILD (children->data);

    hdy_header_group_child_set_decoration_layout (child, ":");

    if (!hdy_header_group_child_get_mapped (child))
      continue;

    /* The headerbars are in reverse order in the list. */
    start_child = child;
    if (end_child == NULL)
      end_child = child;
  }

  if (start_child == NULL || end_child == NULL)
    return;

  if (start_child == end_child) {
    hdy_header_group_child_set_decoration_layout (start_child, layout);

    return;
  }

  ends = g_strsplit (layout, ":", 2);
  if (g_strv_length (ends) >= 2) {
    start_layout = g_strdup_printf ("%s:", ends[0]);
    end_layout = g_strdup_printf (":%s", ends[1]);
  } else {
    start_layout = g_strdup (":");
    end_layout = g_strdup (":");
  }
  hdy_header_group_child_set_decoration_layout (start_child, start_layout);
  hdy_header_group_child_set_decoration_layout (end_child, end_layout);
}

static void
header_bar_destroyed (HdyHeaderGroup *self,
                      GtkHeaderBar   *header_bar)
{
  g_autoptr (HdyHeaderGroupChild) child = NULL;

  g_return_if_fail (HDY_IS_HEADER_GROUP (self));
  g_return_if_fail (GTK_IS_HEADER_BAR (header_bar));

  child = get_child_for_object (self, header_bar);

  g_return_if_fail (child != NULL);

  self->children = g_slist_remove (self->children, child);

  g_object_unref (self);
}

HdyHeaderGroup *
hdy_header_group_new (void)
{
  return g_object_new (HDY_TYPE_HEADER_GROUP, NULL);
}

/**
 * hdy_header_group_add_header_bar:
 * @self: a #HdyHeaderGroup
 * @header_bar: the #HdyHeaderBar to add
 *
 * Adds @header_bar to @self.
 * When the widget is destroyed or no longer referenced elsewhere, it will
 * be removed from the header group.
 *
 * Since: 1.0
 */
void
hdy_header_group_add_header_bar (HdyHeaderGroup *self,
                                 HdyHeaderBar   *header_bar)
{
  HdyHeaderGroupChild *child;

  g_return_if_fail (HDY_IS_HEADER_GROUP (self));
  g_return_if_fail (HDY_IS_HEADER_BAR (header_bar));
  g_return_if_fail (get_child_for_object (self, header_bar) == NULL);

  g_signal_connect_swapped (header_bar, "map", G_CALLBACK (update_decoration_layouts), self);
  g_signal_connect_swapped (header_bar, "unmap", G_CALLBACK (update_decoration_layouts), self);

  child = hdy_header_group_child_new_for_header_bar (header_bar);
  self->children = g_slist_prepend (self->children, child);

  g_object_ref (self);

  g_signal_connect_swapped (header_bar, "destroy", G_CALLBACK (header_bar_destroyed), self);

  update_decoration_layouts (self);
}

/**
 * hdy_header_group_add_gtk_header_bar:
 * @self: a #HdyHeaderGroup
 * @header_bar: the #GtkHeaderBar to add
 *
 * Adds @header_bar to @self.
 * When the widget is destroyed or no longer referenced elsewhere, it will
 * be removed from the header group.
 *
 * Since: 1.0
 */
void
hdy_header_group_add_gtk_header_bar (HdyHeaderGroup *self,
                                     GtkHeaderBar   *header_bar)
{
  HdyHeaderGroupChild *child;

  g_return_if_fail (HDY_IS_HEADER_GROUP (self));
  g_return_if_fail (GTK_IS_HEADER_BAR (header_bar));
  g_return_if_fail (get_child_for_object (self, header_bar) == NULL);

  g_signal_connect_swapped (header_bar, "map", G_CALLBACK (update_decoration_layouts), self);
  g_signal_connect_swapped (header_bar, "unmap", G_CALLBACK (update_decoration_layouts), self);

  child = hdy_header_group_child_new_for_gtk_header_bar (header_bar);
  self->children = g_slist_prepend (self->children, child);

  g_object_ref (self);

  g_signal_connect_swapped (header_bar, "destroy", G_CALLBACK (header_bar_destroyed), self);

  update_decoration_layouts (self);
}

/**
 * hdy_header_group_add_header_group:
 * @self: a #HdyHeaderGroup
 * @header_group: (transfer full): the #HdyHeaderGroup to add
 *
 * Adds @header_group to @self.
 * When the nested group is no longer referenced elsewhere, it will be removed
 * from the header group.
 *
 * Since: 1.0
 */
void
hdy_header_group_add_header_group (HdyHeaderGroup *self,
                                   HdyHeaderGroup *header_group)
{
  HdyHeaderGroupChild *child;

  g_return_if_fail (HDY_IS_HEADER_GROUP (self));
  g_return_if_fail (HDY_IS_HEADER_GROUP (header_group));
  g_return_if_fail (get_child_for_object (self, header_group) == NULL);

  child = hdy_header_group_child_new_for_header_group (header_group);
  self->children = g_slist_prepend (self->children, child);

  g_object_ref (self);

  /* FIXME */
  g_signal_connect_swapped (header_group, "destroy", G_CALLBACK (header_bar_destroyed), self);

  update_decoration_layouts (self);
}

typedef struct {
  gchar *name;
  gint line;
  gint col;
} ItemData;

static void
item_data_free (gpointer data)
{
  ItemData *item_data = data;

  g_free (item_data->name);
  g_free (item_data);
}

typedef struct {
  GObject *object;
  GtkBuilder *builder;
  GSList *items;
} GSListSubParserData;

static void
hdy_header_group_dispose (GObject *object)
{
  HdyHeaderGroup *self = (HdyHeaderGroup *)object;

  if (self->parent) {
    hdy_header_group_remove_header_group (self->parent, self);
    self->parent = NULL;
  }

  g_slist_free_full (self->children, (GDestroyNotify) g_object_unref);
  self->children = NULL;

  G_OBJECT_CLASS (hdy_header_group_parent_class)->dispose (object);
}

static void
hdy_header_group_finalize (GObject *object)
{
  HdyHeaderGroup *self = (HdyHeaderGroup *) object;

  g_free (self->layout);

  G_OBJECT_CLASS (hdy_header_group_parent_class)->finalize (object);
}

static void
hdy_header_group_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  HdyHeaderGroup *self = HDY_HEADER_GROUP (object);

  switch (prop_id) {
  case PROP_DECORATE_ALL:
    g_value_set_boolean (value, hdy_header_group_get_decorate_all (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
hdy_header_group_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  HdyHeaderGroup *self = HDY_HEADER_GROUP (object);

  switch (prop_id) {
  case PROP_DECORATE_ALL:
    hdy_header_group_set_decorate_all (self, g_value_get_boolean (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

/*< private >
 * @builder: a #GtkBuilder
 * @context: the #GMarkupParseContext
 * @parent_name: the name of the expected parent element
 * @error: return location for an error
 *
 * Checks that the parent element of the currently handled
 * start tag is @parent_name and set @error if it isn't.
 *
 * This is intended to be called in start_element vfuncs to
 * ensure that element nesting is as intended.
 *
 * Returns: %TRUE if @parent_name is the parent element
 */
/* This has been copied and modified from gtkbuilder.c. */
static gboolean
_gtk_builder_check_parent (GtkBuilder           *builder,
                           GMarkupParseContext  *context,
                           const gchar          *parent_name,
                           GError              **error)
{
  const GSList *stack;
  gint line, col;
  const gchar *parent;
  const gchar *element;

  stack = g_markup_parse_context_get_element_stack (context);

  element = (const gchar *)stack->data;
  parent = stack->next ? (const gchar *)stack->next->data : "";

  if (g_str_equal (parent_name, parent) ||
      (g_str_equal (parent_name, "object") && g_str_equal (parent, "template")))
    return TRUE;

  g_markup_parse_context_get_position (context, &line, &col);
  g_set_error (error,
               GTK_BUILDER_ERROR,
               GTK_BUILDER_ERROR_INVALID_TAG,
               ".:%d:%d Can't use <%s> here",
               line, col, element);

  return FALSE;
}

/*< private >
 * _gtk_builder_prefix_error:
 * @builder: a #GtkBuilder
 * @context: the #GMarkupParseContext
 * @error: an error
 *
 * Calls g_prefix_error() to prepend a filename:line:column marker
 * to the given error. The filename is taken from @builder, and
 * the line and column are obtained by calling
 * g_markup_parse_context_get_position().
 *
 * This is intended to be called on errors returned by
 * g_markup_collect_attributes() in a start_element vfunc.
 */
/* This has been copied and modified from gtkbuilder.c. */
static void
_gtk_builder_prefix_error (GtkBuilder           *builder,
                           GMarkupParseContext  *context,
                           GError              **error)
{
  gint line, col;

  g_markup_parse_context_get_position (context, &line, &col);
  g_prefix_error (error, ".:%d:%d ", line, col);
}

/*< private >
 * _gtk_builder_error_unhandled_tag:
 * @builder: a #GtkBuilder
 * @context: the #GMarkupParseContext
 * @object: name of the object that is being handled
 * @element_name: name of the element whose start tag is being handled
 * @error: return location for the error
 *
 * Sets @error to a suitable error indicating that an @element_name
 * tag is not expected in the custom markup for @object.
 *
 * This is intended to be called in a start_element vfunc.
 */
/* This has been copied and modified from gtkbuilder.c. */
static void
_gtk_builder_error_unhandled_tag (GtkBuilder           *builder,
                                  GMarkupParseContext  *context,
                                  const gchar          *object,
                                  const gchar          *element_name,
                                  GError              **error)
{
  gint line, col;

  g_markup_parse_context_get_position (context, &line, &col);
  g_set_error (error,
               GTK_BUILDER_ERROR,
               GTK_BUILDER_ERROR_UNHANDLED_TAG,
               ".:%d:%d Unsupported tag for %s: <%s>",
               line, col,
               object, element_name);
}

/* This has been copied and modified from gtksizegroup.c. */
static void
header_group_start_element (GMarkupParseContext  *context,
                          const gchar          *element_name,
                          const gchar         **names,
                          const gchar         **values,
                          gpointer              user_data,
                          GError              **error)
{
  GSListSubParserData *data = (GSListSubParserData*)user_data;

  if (strcmp (element_name, "headerbar") == 0)
    {
      const gchar *name;
      ItemData *item_data;

      if (!_gtk_builder_check_parent (data->builder, context, "headerbars", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "name", &name,
                                        G_MARKUP_COLLECT_INVALID))
        {
          _gtk_builder_prefix_error (data->builder, context, error);
          return;
        }

      item_data = g_new (ItemData, 1);
      item_data->name = g_strdup (name);
      g_markup_parse_context_get_position (context, &item_data->line, &item_data->col);
      data->items = g_slist_prepend (data->items, item_data);
    }
  else if (strcmp (element_name, "headerbars") == 0)
    {
      if (!_gtk_builder_check_parent (data->builder, context, "object", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_INVALID, NULL, NULL,
                                        G_MARKUP_COLLECT_INVALID))
        _gtk_builder_prefix_error (data->builder, context, error);
    }
  else
    {
      _gtk_builder_error_unhandled_tag (data->builder, context,
                                        "HdyHeaderGroup", element_name,
                                        error);
    }
}


/* This has been copied and modified from gtksizegroup.c. */
static const GMarkupParser header_group_parser =
  {
    header_group_start_element
  };

/* This has been copied and modified from gtksizegroup.c. */
static gboolean
hdy_header_group_buildable_custom_tag_start (GtkBuildable  *buildable,
                                             GtkBuilder    *builder,
                                             GObject       *child,
                                             const gchar   *tagname,
                                             GMarkupParser *parser,
                                             gpointer      *parser_data)
{
  GSListSubParserData *data;

  if (child)
    return FALSE;

  if (strcmp (tagname, "headerbars") == 0)
    {
      data = g_slice_new0 (GSListSubParserData);
      data->items = NULL;
      data->object = G_OBJECT (buildable);
      data->builder = builder;

      *parser = header_group_parser;
      *parser_data = data;

      return TRUE;
    }

  return FALSE;
}

/* This has been copied and modified from gtksizegroup.c. */
static void
hdy_header_group_buildable_custom_finished (GtkBuildable *buildable,
                                            GtkBuilder   *builder,
                                            GObject      *child,
                                            const gchar  *tagname,
                                            gpointer      user_data)
{
  GSList *l;
  GSListSubParserData *data;

  if (strcmp (tagname, "headerbars") != 0)
    return;

  data = (GSListSubParserData*)user_data;
  data->items = g_slist_reverse (data->items);

  for (l = data->items; l; l = l->next) {
    ItemData *item_data = l->data;
    GObject *object = gtk_builder_get_object (builder, item_data->name);

    if (!object)
      continue;

    if (GTK_IS_HEADER_BAR (object))
      hdy_header_group_add_gtk_header_bar (HDY_HEADER_GROUP (data->object),
                                           GTK_HEADER_BAR (object));
    else if (HDY_IS_HEADER_BAR (object))
      hdy_header_group_add_header_bar (HDY_HEADER_GROUP (data->object),
                                       HDY_HEADER_BAR (object));
    else if (HDY_IS_HEADER_GROUP (object))
      hdy_header_group_add_header_group (HDY_HEADER_GROUP (data->object),
                                         HDY_HEADER_GROUP (object));
  }

  g_slist_free_full (data->items, item_data_free);
  g_slice_free (GSListSubParserData, data);
}

static void
hdy_header_group_class_init (HdyHeaderGroupClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = hdy_header_group_dispose;
  object_class->finalize = hdy_header_group_finalize;
  object_class->get_property = hdy_header_group_get_property;
  object_class->set_property = hdy_header_group_set_property;

  /**
   * HdyHeaderGroup:decorate-all:
   *
   * Whether the elements of the group should all receive the full decoration.
   * This is useful in conjunction with #HdyLeaflet:folded when the leaflet
   * contains the header bars of the group, as you want them all to display the
   * complete decoration when the leaflet is folded.
   *
   * Since: 1.0
   */
  props[PROP_DECORATE_ALL] =
    g_param_spec_boolean ("decorate-all",
                          _("Decorate all"),
                          _("Whether the elements of the group should all receive the full decoration"),
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
hdy_header_group_init (HdyHeaderGroup *self)
{
  GtkSettings *settings = gtk_settings_get_default ();

  g_signal_connect_swapped (settings, "notify::gtk-decoration-layout", G_CALLBACK (update_decoration_layouts), self);
}

static void
hdy_header_group_buildable_init (GtkBuildableIface *iface)
{
  iface->custom_tag_start = hdy_header_group_buildable_custom_tag_start;
  iface->custom_finished = hdy_header_group_buildable_custom_finished;
}

/**
 * hdy_header_group_child_new:
 *
 * Creates a new #HdyHeaderGroupChild.
 *
 * Returns: (transfer full): a newly created #HdyHeaderGroupChild
 *
 * Since: 1.0
 */
HdyHeaderGroupChild *
hdy_header_group_child_new (void)
{
  return g_object_new (HDY_TYPE_HEADER_GROUP_CHILD, NULL);
}

/**
 * hdy_header_group_child_new_for_header_bar:
 * @header_bar: a #HdyHeaderBar
 *
 * Creates a new #HdyHeaderGroupChild for @header_bar.
 *
 * Returns: (transfer full): a newly created #HdyHeaderGroupChild
 *
 * Since: 1.0
 */
HdyHeaderGroupChild *
hdy_header_group_child_new_for_header_bar (HdyHeaderBar *header_bar)
{
  HdyHeaderGroupChild *self;

  g_return_val_if_fail (HDY_IS_HEADER_BAR (header_bar), NULL);

  self = g_object_new (HDY_TYPE_HEADER_GROUP_CHILD, NULL);
  self->type = HDY_HEADER_GROUP_CHILD_TYPE_HEADER_BAR;
  self->object = G_OBJECT (header_bar);

  return self;
}

/**
 * hdy_header_group_child_new_for_gtk_header_bar:
 * @header_bar: a #GtkHeaderBar
 *
 * Creates a new #HdyHeaderGroupChild for @header_bar.
 *
 * Returns: (transfer full): a newly created #HdyHeaderGroupChild
 *
 * Since: 1.0
 */
HdyHeaderGroupChild *
hdy_header_group_child_new_for_gtk_header_bar (GtkHeaderBar *header_bar)
{
  HdyHeaderGroupChild *self;

  g_return_val_if_fail (GTK_IS_HEADER_BAR (header_bar), NULL);

  self = g_object_new (HDY_TYPE_HEADER_GROUP_CHILD, NULL);
  self->type = HDY_HEADER_GROUP_CHILD_TYPE_GTK_HEADER_BAR;
  self->object = G_OBJECT (header_bar);

  return self;
}

/**
 * hdy_header_group_child_new_for_header_group:
 * @header_group: (transfer full): a #HdyHeaderGroup
 *
 * Creates a new #HdyHeaderGroupChild for @header_group.
 *
 * Returns: (transfer full): a newly created #HdyHeaderGroupChild
 *
 * Since: 1.0
 */
HdyHeaderGroupChild *
hdy_header_group_child_new_for_header_group (HdyHeaderGroup *header_group)
{
  HdyHeaderGroupChild *self;

  g_return_val_if_fail (HDY_IS_HEADER_GROUP (header_group), NULL);

  self = g_object_new (HDY_TYPE_HEADER_GROUP_CHILD, NULL);
  self->type = HDY_HEADER_GROUP_CHILD_TYPE_HEADER_GROUP;
  self->object = G_OBJECT (header_group);

  return self;
}

/**
 * hdy_header_group_child_get_header_bar:
 * @self: a #HdyHeaderGroupChild
 *
 * Gets the child #HdyHeaderBar, or %NULL in case of error.
 * Use hdy_header_group_child_get_child_type() to check the child type.
 *
 * Returns: (transfer none): the child #HdyHeaderBar, or %NULL in case of error.
 *
 * Since: 1.0
 */
HdyHeaderBar *
hdy_header_group_child_get_header_bar (HdyHeaderGroupChild *self)
{
  g_return_val_if_fail (HDY_IS_HEADER_GROUP_CHILD (self), NULL);
  g_return_val_if_fail (self->type == HDY_HEADER_GROUP_CHILD_TYPE_HEADER_BAR, NULL);

  return HDY_HEADER_BAR (self->object);
}

/**
 * hdy_header_group_child_get_gtk_header_bar:
 * @self: a #HdyHeaderGroupChild
 *
 * Gets the child #GtkHeaderBar, or %NULL in case of error.
 * Use hdy_header_group_child_get_child_type() to check the child type.
 *
 * Returns: (transfer none): the child #GtkHeaderBar, or %NULL in case of error.
 *
 * Since: 1.0
 */
GtkHeaderBar *
hdy_header_group_child_get_gtk_header_bar (HdyHeaderGroupChild *self)
{
  g_return_val_if_fail (HDY_IS_HEADER_GROUP_CHILD (self), NULL);
  g_return_val_if_fail (self->type == HDY_HEADER_GROUP_CHILD_TYPE_GTK_HEADER_BAR, NULL);

  return GTK_HEADER_BAR (self->object);
}

/**
 * hdy_header_group_child_get_header_group:
 * @self: a #HdyHeaderGroupChild
 *
 * Gets the child #HdyHeaderGroup, or %NULL in case of error.
 * Use hdy_header_group_child_get_child_type() to check the child type.
 *
 * Returns: (transfer none): the child #HdyHeaderGroup, or %NULL in case of error.
 *
 * Since: 1.0
 */
HdyHeaderGroup *
hdy_header_group_child_get_header_group (HdyHeaderGroupChild *self)
{
  g_return_val_if_fail (HDY_IS_HEADER_GROUP_CHILD (self), NULL);
  g_return_val_if_fail (self->type == HDY_HEADER_GROUP_CHILD_TYPE_HEADER_GROUP, NULL);

  return HDY_HEADER_GROUP (self->object);
}

/**
 * hdy_header_group_child_get_child_type:
 * @self: a #HdyHeaderGroupChild
 *
 * Gets the child type.
 *
 * Returns: the child type.
 *
 * Since: 1.0
 */
HdyHeaderGroupChildType
hdy_header_group_child_get_child_type (HdyHeaderGroupChild *self)
{
  g_return_val_if_fail (HDY_IS_HEADER_GROUP_CHILD (self), HDY_HEADER_GROUP_CHILD_TYPE_INVALID);

  return self->type;
}

/**
 * hdy_header_group_get_children:
 * @self: a #HdyHeaderGroup
 *
 * Returns the list of children associated with @self.
 *
 * Returns: (element-type HdyHeaderGroupChild) (transfer none): the #GSList of
 *   children. The list is owned by libhandy and should not be modified.
 *
 * Since: 1.0
 */
GSList *
hdy_header_group_get_children (HdyHeaderGroup *self)
{
  g_return_val_if_fail (HDY_IS_HEADER_GROUP (self), NULL);

  return self->children;
}

/**
 * hdy_header_group_remove_header_bar:
 * @self: a #HdyHeaderGroup
 * @header_bar: the #HdyHeaderBar to remove
 *
 * Removes @header_bar from @self.
 *
 * Since: 1.0
 */
void
hdy_header_group_remove_header_bar (HdyHeaderGroup *self,
                                    HdyHeaderBar   *header_bar)
{
  g_autoptr (HdyHeaderGroupChild) child = NULL;

  g_return_if_fail (HDY_IS_HEADER_GROUP (self));
  g_return_if_fail (HDY_IS_HEADER_BAR (header_bar));

  child = get_child_for_object (self, header_bar);

  g_return_if_fail (child != NULL);

  self->children = g_slist_remove (self->children, child);

  g_signal_handlers_disconnect_by_data (header_bar, self);

  g_object_unref (self);
}

/**
 * hdy_header_group_remove_gtk_header_bar:
 * @self: a #HdyHeaderGroup
 * @header_bar: the #GtkHeaderBar to remove
 *
 * Removes a #GtkHeaderBar from a #HdyHeaderGroup
 *
 * Since: 1.0
 */
void
hdy_header_group_remove_gtk_header_bar (HdyHeaderGroup *self,
                                        GtkHeaderBar   *header_bar)
{
  g_autoptr (HdyHeaderGroupChild) child = NULL;

  g_return_if_fail (HDY_IS_HEADER_GROUP (self));
  g_return_if_fail (GTK_IS_HEADER_BAR (header_bar));

  child = get_child_for_object (self, header_bar);

  g_return_if_fail (child != NULL);

  self->children = g_slist_remove (self->children, child);

  g_signal_handlers_disconnect_by_data (header_bar, self);

  g_object_unref (self);
}

/**
 * hdy_header_group_remove_header_group:
 * @self: a #HdyHeaderGroup
 * @header_group: the #HdyHeaderGroup to remove
 *
 * Removes a nested #HdyHeaderGroup from a #HdyHeaderGroup
 *
 * Since: 1.0
 */
void
hdy_header_group_remove_header_group (HdyHeaderGroup *self,
                                      HdyHeaderGroup *header_group)
{
  g_autoptr (HdyHeaderGroupChild) child = NULL;

  g_return_if_fail (HDY_IS_HEADER_GROUP (self));
  g_return_if_fail (GTK_IS_HEADER_BAR (header_group));

  child = get_child_for_object (self, header_group);

  g_return_if_fail (child != NULL);

  self->children = g_slist_remove (self->children, child);

  g_signal_handlers_disconnect_by_data (header_group, self);

  g_object_unref (self);
}

/**
 * hdy_header_group_set_decorate_all:
 * @self: a #HdyHeaderGroup
 * @decorate_all: whether the elements of the group should all receive the full decoration
 *
 * Sets whether the elements of the group should all receive the full decoration.
 *
 * Since: 1.0
 */
void
hdy_header_group_set_decorate_all (HdyHeaderGroup *self,
                                   gboolean        decorate_all)
{
  g_return_if_fail (HDY_IS_HEADER_GROUP (self));

  decorate_all = !!decorate_all;

  if (self->decorate_all == decorate_all)
    return;

  self->decorate_all = decorate_all;

  update_decoration_layouts (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_DECORATE_ALL]);
}

/**
 * hdy_header_group_get_decorate_all:
 * @self: a #HdyHeaderGroup
 *
 * Gets whether the elements of the group should all receive the full decoration.
 *
 * Returns: %TRUE if the elements of the group should all receive the full
 *   decoration, %FALSE otherwise.
 *
 * Since: 1.0
 */
gboolean
hdy_header_group_get_decorate_all (HdyHeaderGroup *self)
{
  g_return_val_if_fail (HDY_IS_HEADER_GROUP (self), FALSE);

  return self->decorate_all;
}

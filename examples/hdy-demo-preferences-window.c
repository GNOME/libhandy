#include "hdy-demo-preferences-window.h"

struct _HdyDemoPreferencesWindow
{
  HdyPreferencesWindow parent_instance;

  GListStore *combo_list;
  HdyComboRow *combo_row;
  GListModel *enum_combo_list;
  HdyComboRow *enum_combo_row;
};

G_DEFINE_TYPE (HdyDemoPreferencesWindow, hdy_demo_preferences_window, HDY_TYPE_PREFERENCES_WINDOW)

static gchar *
combo_get_name (gpointer item,
                gpointer user_data)
{
  return g_strdup (gtk_label_get_text (GTK_LABEL (item)));
}

HdyDemoPreferencesWindow *
hdy_demo_preferences_window_new (void)
{
  return g_object_new (HDY_TYPE_DEMO_PREFERENCES_WINDOW, NULL);
}

static void
hdy_demo_preferences_window_class_init (HdyDemoPreferencesWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/sm/puri/handy/demo/ui/hdy-demo-preferences-window.ui");
  gtk_widget_class_bind_template_child (widget_class, HdyDemoPreferencesWindow, combo_row);
  gtk_widget_class_bind_template_child (widget_class, HdyDemoPreferencesWindow, enum_combo_row);
}

static void
hdy_demo_preferences_window_init (HdyDemoPreferencesWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->combo_list = g_list_store_new (GTK_TYPE_LABEL);

  g_list_store_insert (self->combo_list, 0,
                       g_object_new (GTK_TYPE_LABEL,
                                     "ellipsize", PANGO_ELLIPSIZE_END,
                                     "label", "Foo",
                                     "margin", 12,
                                     "max-width-chars", 20,
                                     "visible", TRUE,
                                     "width-chars", 20,
                                     "xalign", 0.0,
                                      NULL));

  g_list_store_insert (self->combo_list, 1,
                       g_object_new (GTK_TYPE_LABEL,
                                     "ellipsize", PANGO_ELLIPSIZE_END,
                                     "label", "Bar",
                                     "margin", 12,
                                     "max-width-chars", 20,
                                     "visible", TRUE,
                                     "width-chars", 20,
                                     "xalign", 0.0,
                                      NULL));

  g_list_store_insert (self->combo_list, 2,
                       g_object_new (GTK_TYPE_LABEL,
                                     "ellipsize", PANGO_ELLIPSIZE_END,
                                     "label", "Baz",
                                     "margin", 12,
                                     "max-width-chars", 20,
                                     "visible", TRUE,
                                     "width-chars", 20,
                                     "xalign", 0.0,
                                      NULL));

  hdy_combo_row_bind_name_model (self->combo_row, G_LIST_MODEL (self->combo_list), combo_get_name, NULL, NULL);

  hdy_combo_row_set_for_enum (self->enum_combo_row, GTK_TYPE_ORIENTATION, hdy_enum_value_row_name, NULL, NULL);
}

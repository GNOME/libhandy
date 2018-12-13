#include "hdy-demo-preferences-window.h"

struct _HdyDemoPreferencesWindow
{
  HdyPreferencesWindow parent_instance;

  GListStore *combo_list;
  GListModel *enum_combo_list;
};

G_DEFINE_TYPE (HdyDemoPreferencesWindow, hdy_demo_preferences_window, HDY_TYPE_PREFERENCES_WINDOW)

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
}

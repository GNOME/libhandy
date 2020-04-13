#pragma once

#include <gladeui/glade.h>

#define HANDY_USE_UNSTABLE_API
#include <handy.h>

void glade_hdy_window_post_create (GladeWidgetAdaptor *adaptor,
                                   GObject            *object,
                                   GladeCreateReason   reason);

void glade_hdy_window_add_child (GladeWidgetAdaptor *adaptor,
                                 GObject            *object,
                                 GObject            *child);

void glade_hdy_window_remove_child (GladeWidgetAdaptor *adaptor,
                                    GObject            *object,
                                    GObject            *child);

void glade_hdy_window_replace_child (GladeWidgetAdaptor *adaptor,
                                     GtkWidget          *object,
                                     GtkWidget          *current,
                                     GtkWidget          *new_widget);

GList *glade_hdy_window_get_children (GladeWidgetAdaptor *adaptor,
                                      GObject            *object);

gboolean glade_hdy_window_add_verify (GladeWidgetAdaptor *adaptor,
                                      GtkWidget          *object,
                                      GtkWidget          *child,
                                      gboolean            user_feedback);

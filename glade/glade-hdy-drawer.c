/*
 * Copyright (C) 2019 Alexander Mikhaylenko <exalm7659@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 *
 * Based on
 * glade-gtk-overlay.c - GladeWidgetAdaptor for GtkOverlay
 * Copyright (C) 2013 Juan Pablo Ugarte
 */

#include "glade-hdy-drawer.h"

#include <config.h>
#include <glib/gi18n-lib.h>
#include <gladeui/glade.h>

static void
selection_changed_cb (GladeProject *project,
                      GladeWidget  *gwidget)
{
  GList *list;
  GtkWidget *child, *sel_widget;
  HdyDrawer *drawer;

  list = glade_project_selection_get (project);
  if (!list || g_list_length (list) != 1)
    return;

  sel_widget = list->data;

  drawer = HDY_DRAWER (glade_widget_get_object (gwidget));

  if (!GTK_IS_WIDGET (sel_widget) ||
      !gtk_widget_is_ancestor (sel_widget, GTK_WIDGET (drawer)))
    return;

  child = gtk_bin_get_child (GTK_BIN (drawer));
  if (child && (sel_widget == child || gtk_widget_is_ancestor (sel_widget, child))) {
    hdy_drawer_close (drawer);
    return;
  }

  child = hdy_drawer_get_overlay (drawer);
  if (child && (sel_widget == child || gtk_widget_is_ancestor (sel_widget, child)))
    hdy_drawer_open (drawer, child);
}

static void
project_changed_cb (GladeWidget *gwidget,
                    GParamSpec  *pspec,
                    gpointer     user_data)
{
  GladeProject *project, *old_project;

  project = glade_widget_get_project (gwidget);
  old_project = g_object_get_data (G_OBJECT (gwidget), "paginator-project-ptr");

  if (old_project)
    g_signal_handlers_disconnect_by_func (G_OBJECT (old_project),
                                          G_CALLBACK (selection_changed_cb),
                                          gwidget);

  if (project)
    g_signal_connect (G_OBJECT (project), "selection-changed",
                      G_CALLBACK (selection_changed_cb), gwidget);
}

void
glade_hdy_drawer_post_create (GladeWidgetAdaptor *adaptor,
                              GObject            *container,
                              GladeCreateReason   reason)
{
  GladeWidget *widget = glade_widget_get_from_gobject (container);

  if (reason == GLADE_CREATE_USER)
    gtk_container_add (GTK_CONTAINER (container), glade_placeholder_new ());

  g_signal_connect (G_OBJECT (widget), "notify::project",
                    G_CALLBACK (project_changed_cb), NULL);

  project_changed_cb (widget, NULL, NULL);
}

#define TOO_MANY_CHILDREN_MSG _("Objects of type %s can only contain 2 children.")
gboolean
glade_hdy_drawer_add_child_verify (GladeWidgetAdaptor *adaptor,
                                   GObject            *container,
                                   GObject            *widget,
                                   gboolean            user_feedback)
{
  GtkWidget *child, *overlay;

  if (!GWA_GET_CLASS (GTK_TYPE_CONTAINER)->add_verify (adaptor, container,
                                                       widget, user_feedback))
    return FALSE;

  child = gtk_bin_get_child (GTK_BIN (container));
  overlay = hdy_drawer_get_overlay (HDY_DRAWER (container));

  if (child && !GLADE_IS_PLACEHOLDER (child) && overlay) {
    if (user_feedback)
      glade_util_ui_message (glade_app_get_window (),
                             GLADE_UI_INFO, NULL,
                             TOO_MANY_CHILDREN_MSG,
                             glade_widget_adaptor_get_title (adaptor));

    return FALSE;
  }

  return TRUE;
}

void
glade_hdy_drawer_add_child (GladeWidgetAdaptor *adaptor,
                            GObject            *object,
                            GObject            *child)
{
  gchar *special_type = g_object_get_data (child, "special-child-type");
  GtkWidget *bin_child;

  if ((special_type && !strcmp (special_type, "overlay")) ||
      ((bin_child = gtk_bin_get_child (GTK_BIN (object))) &&
       !GLADE_IS_PLACEHOLDER (bin_child))) {
    g_object_set_data (child, "special-child-type", "overlay");
    hdy_drawer_set_overlay (HDY_DRAWER (object), GTK_WIDGET (child));
    hdy_drawer_open (HDY_DRAWER (object), GTK_WIDGET (child));
  }
  else {
    GWA_GET_CLASS (GTK_TYPE_CONTAINER)->add (adaptor, object, child);
  }
}

void
glade_hdy_drawer_remove_child  (GladeWidgetAdaptor *adaptor,
                                GObject            *object,
                                GObject            *child)
{
  gchar *special_type = g_object_get_data (child, "special-child-type");

  if (special_type && !strcmp (special_type, "overlay")) {
    g_object_set_data (child, "special-child-type", NULL);
    gtk_widget_show (GTK_WIDGET (child));
  }

  gtk_container_remove (GTK_CONTAINER (object), GTK_WIDGET (child));

  if (!gtk_bin_get_child (GTK_BIN (object)))
    gtk_container_add (GTK_CONTAINER (object), glade_placeholder_new ());
}

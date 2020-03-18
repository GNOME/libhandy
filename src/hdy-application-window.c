/*
 * Copyright (C) 2019 Alexander Mikhaylenko <alexm@gnome.org>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"

#include "hdy-application-window.h"
#include "hdy-window-impl-private.h"

typedef struct
{
  HdyWindowImpl *impl;
} HdyApplicationWindowPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (HdyApplicationWindow, hdy_application_window, GTK_TYPE_APPLICATION_WINDOW)

#define HDY_GET_IMPL(obj) (((HdyApplicationWindowPrivate *) hdy_application_window_get_instance_private (HDY_APPLICATION_WINDOW (obj)))->impl)

static void
hdy_application_window_add (GtkContainer *container,
                            GtkWidget    *widget)
{
  hdy_window_impl_add (HDY_GET_IMPL (container), widget);
}

static void
hdy_application_window_remove (GtkContainer *container,
                               GtkWidget    *widget)
{
  hdy_window_impl_remove (HDY_GET_IMPL (container), widget);
}

static void
hdy_application_window_forall (GtkContainer *container,
                               gboolean      include_internals,
                               GtkCallback   callback,
                               gpointer      callback_data)
{
  hdy_window_impl_forall (HDY_GET_IMPL (container),
                          include_internals,
                          callback,
                          callback_data);
}

static gboolean
hdy_application_window_draw (GtkWidget *widget,
                             cairo_t   *cr)
{
  return hdy_window_impl_draw (HDY_GET_IMPL (widget), cr);
}

static void
hdy_application_window_finalize (GObject *object)
{
  HdyApplicationWindow *self = (HdyApplicationWindow *)object;
  HdyApplicationWindowPrivate *priv = hdy_application_window_get_instance_private (self);

  g_clear_object (&priv->impl);

  G_OBJECT_CLASS (hdy_application_window_parent_class)->finalize (object);
}

static void
hdy_application_window_class_init (HdyApplicationWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->finalize = hdy_application_window_finalize;
  widget_class->draw = hdy_application_window_draw;
  container_class->add = hdy_application_window_add;
  container_class->remove = hdy_application_window_remove;
  container_class->forall = hdy_application_window_forall;
}

static void
hdy_application_window_init (HdyApplicationWindow *self)
{
  HdyApplicationWindowPrivate *priv = hdy_application_window_get_instance_private (self);

  priv->impl = hdy_window_impl_new (GTK_WINDOW (self),
                                    GTK_WINDOW_CLASS (hdy_application_window_parent_class));

  gtk_application_window_set_show_menubar (GTK_APPLICATION_WINDOW (self), FALSE);
}

/**
 * hdy_application_window_new:
 *
 * Create a new #HdyApplicationWindow.
 *
 * Returns: (transfer full): a newly created #HdyApplicationWindow
 */
HdyApplicationWindow *
hdy_application_window_new (void)
{
  return g_object_new (HDY_TYPE_APPLICATION_WINDOW,
                       NULL);
}

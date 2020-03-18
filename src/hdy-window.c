/*
 * Copyright (C) 2019 Alexander Mikhaylenko <alexm@gnome.org>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"

#include "hdy-window.h"
#include "hdy-window-impl-private.h"

typedef struct
{
  HdyWindowImpl *impl;
} HdyWindowPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (HdyWindow, hdy_window, GTK_TYPE_WINDOW)

#define HDY_GET_IMPL(obj) (((HdyWindowPrivate *) hdy_window_get_instance_private (HDY_WINDOW (obj)))->impl)

static void
hdy_window_add (GtkContainer *container,
                GtkWidget    *widget)
{
  hdy_window_impl_add (HDY_GET_IMPL (container), widget);
}

static void
hdy_window_remove (GtkContainer *container,
                   GtkWidget    *widget)
{
  hdy_window_impl_remove (HDY_GET_IMPL (container), widget);
}

static void
hdy_window_forall (GtkContainer *container,
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
hdy_window_draw (GtkWidget *widget,
                 cairo_t   *cr)
{
  return hdy_window_impl_draw (HDY_GET_IMPL (widget), cr);
}

static gboolean
hdy_window_window_state_event (GtkWidget           *widget,
                               GdkEventWindowState *event)
{
  return hdy_window_impl_window_state_event (HDY_GET_IMPL (widget), event);
}

static void
hdy_window_finalize (GObject *object)
{
  HdyWindow *self = (HdyWindow *)object;
  HdyWindowPrivate *priv = hdy_window_get_instance_private (self);

  g_clear_object (&priv->impl);

  G_OBJECT_CLASS (hdy_window_parent_class)->finalize (object);
}

static void
hdy_window_class_init (HdyWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->finalize = hdy_window_finalize;
  widget_class->draw = hdy_window_draw;
  widget_class->window_state_event = hdy_window_window_state_event;
  container_class->add = hdy_window_add;
  container_class->remove = hdy_window_remove;
  container_class->forall = hdy_window_forall;
}

static void
hdy_window_init (HdyWindow *self)
{
  HdyWindowPrivate *priv = hdy_window_get_instance_private (self);

  priv->impl = hdy_window_impl_new (GTK_WINDOW (self),
                                    GTK_WINDOW_CLASS (hdy_window_parent_class));
}

/**
 * hdy_window_new:
 *
 * Create a new #HdyWindow.
 *
 * Returns: (transfer full): a newly created #HdyWindow
 */
HdyWindow *
hdy_window_new (void)
{
  return g_object_new (HDY_TYPE_WINDOW,
                       "type", GTK_WINDOW_TOPLEVEL,
                       NULL);
}

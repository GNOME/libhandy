/*
 * Copyright (C) 2019 Alexander Mikhaylenko <alexm@gnome.org>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"

#include "hdy-window-handle.h"
#include "hdy-window-handle-controller-private.h"

typedef struct
{
  HdyWindowHandleController *controller;
} HdyWindowHandlePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (HdyWindowHandle, hdy_window_handle, GTK_TYPE_EVENT_BOX)

static void
hdy_window_handle_dispose (GObject *object)
{
  HdyWindowHandle *self = (HdyWindowHandle *)object;
  HdyWindowHandlePrivate *priv = hdy_window_handle_get_instance_private (self);

  g_clear_object (&priv->controller);

  G_OBJECT_CLASS (hdy_window_handle_parent_class)->dispose (object);
}

static void
hdy_window_handle_class_init (HdyWindowHandleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = hdy_window_handle_dispose;
}

static void
hdy_window_handle_init (HdyWindowHandle *self)
{
  HdyWindowHandlePrivate *priv = hdy_window_handle_get_instance_private (self);

  priv->controller = hdy_window_handle_controller_new (GTK_WIDGET (self));
}

/**
 * hdy_window_handle_new:
 *
 * Create a new #HdyWindowHandle.
 *
 * Returns: (transfer full): a newly created #HdyWindowHandle
 */
HdyWindowHandle *
hdy_window_handle_new (void)
{
  return g_object_new (HDY_TYPE_WINDOW_HANDLE, NULL);
}

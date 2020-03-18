/*
 * Copyright (C) 2020 Alexander Mikhaylenko <alexm@gnome.org>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"

#include "hdy-window.h"
#include "hdy-window-mixin-private.h"

/**
 * SECTION:hdy-window
 * @short_description: A window that masks corners.
 * @title: HdyWindow
 * @See_also: #HdyApplicationWindow, #HdyHeaderBar, #HdyWindowHandle
 *
 * The HdyWindow widget is a subclass of #GtkWindow which provides rounded
 * corners on all sides and ensures they can never be overlapped by the content.
 *
 * This means that headerbars in a #HdyWindow can be safely animated without
 * #HdyTitleBar.
 *
 * This allows #HdyWindow to be used without a separate titlebar by putting an
 * empty widget into the titlebar area, and using #HdyHeaderBar or
 * #HdyWindowHandle in the main window area:
 *
 * |[
 * <object class="HdyWindow"/>
 *   <child type="titlebar">
 *     <object class="GtkBox"/>
 *   </child>
 *   <child>
 *     <object class="GtkBox">
 *       <property name="visible">True</property>
 *       <property name="orientation">vertical</property>
 *       <child>
 *         <object class="HdyHeaderBar">
 *           <property name="visible">True</property>
 *           <property name="show-close-button">True</property>
 *         </object>
 *       </child>
 *     </object>
 *   </child>
 * </object>
 * ]|
 *
 * #HdyWindow allows to easily implement titlebar autohiding by putting the
 * headerbar inside a #GtkRevealer, and to show titlebar above content by
 * putting it into a #GtkOverlay instead of #GtkBox.
 *
 * Depending on the content, it may bring slight performance regresssion when
 * the window is not fullscreen, tiled or maximized.
 *
 * If no titlebar is set, the window behaves the same way as #GtkWindow.
 *
 * # CSS nodes
 *
 * #HdyWindow has a main CSS node with the name window and style classes
 * .background.
 *
 * Style classes that are typically used with the main CSS node are .csd (when
 * client-side decorations are in use), .solid-csd (for client-side decorations
 * without invisible borders), .ssd (used by mutter when rendering server-side
 * decorations). #HdyWindow also represents window states with the following
 * style classes on the main node: .tiled, .maximized, .fullscreen. When
 * #HdyWindow doesn't have default titlebar, the main node also has .unified
 * style class.
 *
 * It contains the subnodes decoration for window shadow and/or border,
 * decoration-overlay for the sheen on top of the window, and deck, which
 * contains the child inside the window.
 *
 * if a separate titlebar is used, #HdyWindow has the titlebar child CSS node
 * as a subnode, and adds .titlebar and .default-decoration style classes to it.
 *
 * Since: 1.0
 */

typedef struct
{
  HdyWindowMixin *mixin;
} HdyWindowPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (HdyWindow, hdy_window, GTK_TYPE_WINDOW)

#define HDY_GET_WINDOW_MIXIN(obj) (((HdyWindowPrivate *) hdy_window_get_instance_private (HDY_WINDOW (obj)))->mixin)

static void
hdy_window_add (GtkContainer *container,
                GtkWidget    *widget)
{
  hdy_window_mixin_add (HDY_GET_WINDOW_MIXIN (container), widget);
}

static void
hdy_window_remove (GtkContainer *container,
                   GtkWidget    *widget)
{
  hdy_window_mixin_remove (HDY_GET_WINDOW_MIXIN (container), widget);
}

static void
hdy_window_forall (GtkContainer *container,
                   gboolean      include_internals,
                   GtkCallback   callback,
                   gpointer      callback_data)
{
  hdy_window_mixin_forall (HDY_GET_WINDOW_MIXIN (container),
                           include_internals,
                           callback,
                           callback_data);
}

static gboolean
hdy_window_draw (GtkWidget *widget,
                 cairo_t   *cr)
{
  return hdy_window_mixin_draw (HDY_GET_WINDOW_MIXIN (widget), cr);
}

static void
hdy_window_size_allocate (GtkWidget     *widget,
                          GtkAllocation *alloc)
{
  hdy_window_mixin_size_allocate (HDY_GET_WINDOW_MIXIN (widget), alloc);
}

static gboolean
hdy_window_window_state_event (GtkWidget           *widget,
                               GdkEventWindowState *event)
{
  return hdy_window_mixin_window_state_event (HDY_GET_WINDOW_MIXIN (widget),
                                              event);
}

static void
hdy_window_finalize (GObject *object)
{
  HdyWindow *self = (HdyWindow *)object;
  HdyWindowPrivate *priv = hdy_window_get_instance_private (self);

  g_clear_object (&priv->mixin);

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
  widget_class->size_allocate = hdy_window_size_allocate;
  widget_class->window_state_event = hdy_window_window_state_event;
  container_class->add = hdy_window_add;
  container_class->remove = hdy_window_remove;
  container_class->forall = hdy_window_forall;
}

static void
hdy_window_init (HdyWindow *self)
{
  HdyWindowPrivate *priv = hdy_window_get_instance_private (self);

  priv->mixin = hdy_window_mixin_new (GTK_WINDOW (self),
                                      GTK_WINDOW_CLASS (hdy_window_parent_class));
}

/**
 * hdy_window_new:
 *
 * Create a new #HdyWindow.
 *
 * Returns: (transfer full): a newly created #HdyWindow
 *
 * Since: 1.0
 */
HdyWindow *
hdy_window_new (void)
{
  return g_object_new (HDY_TYPE_WINDOW,
                       "type", GTK_WINDOW_TOPLEVEL,
                       NULL);
}

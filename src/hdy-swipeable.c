/*
 * Copyright (C) 2019 Alexander Mikhaylenko <exalm7659@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "hdy-swipeable-private.h"

G_DEFINE_INTERFACE (HdySwipeable, hdy_swipeable, GTK_TYPE_WIDGET)

enum {
  SIGNAL_SWITCH_CHILD,
  SIGNAL_BEGIN_SWIPE,
  SIGNAL_UPDATE_SWIPE,
  SIGNAL_END_SWIPE,
  SIGNAL_LAST_SIGNAL,
};

static guint signals[SIGNAL_LAST_SIGNAL];

static void
hdy_swipeable_default_init (HdySwipeableInterface *iface)
{
  signals[SIGNAL_SWITCH_CHILD] =
    g_signal_new ("switch-child",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE,
                  2,
                  G_TYPE_UINT, G_TYPE_INT64);

  signals[SIGNAL_BEGIN_SWIPE] =
    g_signal_new ("begin-swipe",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE,
                  0);

  signals[SIGNAL_UPDATE_SWIPE] =
    g_signal_new ("update-swipe",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_DOUBLE);

  signals[SIGNAL_END_SWIPE] =
    g_signal_new ("end-swipe",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE,
                  2,
                  G_TYPE_INT64, G_TYPE_DOUBLE);
}

void
hdy_swipeable_switch_child (HdySwipeable *self,
                            guint         index,
                            gint64        duration)
{
  HdySwipeableInterface *iface;

  g_return_if_fail (HDY_IS_SWIPEABLE (self));

  iface = HDY_SWIPEABLE_GET_IFACE (self);
  g_return_if_fail (iface->switch_child != NULL);

  (* iface->switch_child) (self, index, duration);
}

void
hdy_swipeable_begin_swipe (HdySwipeable *self)
{
  HdySwipeableInterface *iface;

  g_return_if_fail (HDY_IS_SWIPEABLE (self));

  iface = HDY_SWIPEABLE_GET_IFACE (self);
  g_return_if_fail (iface->begin_swipe != NULL);

  (* iface->begin_swipe) (self);
}

void
hdy_swipeable_update_swipe (HdySwipeable *self,
                            gdouble       value)
{
  HdySwipeableInterface *iface;

  g_return_if_fail (HDY_IS_SWIPEABLE (self));

  iface = HDY_SWIPEABLE_GET_IFACE (self);
  g_return_if_fail (iface->update_swipe != NULL);

  (* iface->update_swipe) (self, value);
}

void
hdy_swipeable_end_swipe (HdySwipeable *self,
                         gint64        duration,
                         gdouble       to)
{
  HdySwipeableInterface *iface;

  g_return_if_fail (HDY_IS_SWIPEABLE (self));

  iface = HDY_SWIPEABLE_GET_IFACE (self);
  g_return_if_fail (iface->end_swipe != NULL);

  (* iface->end_swipe) (self, duration, to);
}

void
hdy_swipeable_emit_switch_child (HdySwipeable *self,
                                 guint         index,
                                 gint64        duration)
{
  g_return_if_fail (HDY_IS_SWIPEABLE (self));

  g_signal_emit (self, signals[SIGNAL_SWITCH_CHILD], 0, index, duration);
}

void
hdy_swipeable_emit_begin_swipe (HdySwipeable *self)
{
  g_return_if_fail (HDY_IS_SWIPEABLE (self));

  g_signal_emit (self, signals[SIGNAL_BEGIN_SWIPE], 0);
}

void
hdy_swipeable_emit_update_swipe (HdySwipeable *self,
                                 gdouble       value)
{
  g_return_if_fail (HDY_IS_SWIPEABLE (self));

  g_signal_emit (self, signals[SIGNAL_UPDATE_SWIPE], 0, value);
}

void
hdy_swipeable_emit_end_swipe (HdySwipeable *self,
                              gint64        duration,
                              gdouble       to)
{
  g_return_if_fail (HDY_IS_SWIPEABLE (self));

  g_signal_emit (self, signals[SIGNAL_END_SWIPE], 0, duration, to);
}

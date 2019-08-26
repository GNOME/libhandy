/*
 * Copyright (C) 2019 Alexander Mikhaylenko <exalm7659@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define HDY_TYPE_SWIPEABLE (hdy_swipeable_get_type ())

G_DECLARE_INTERFACE (HdySwipeable, hdy_swipeable, HDY, SWIPEABLE, GtkWidget)

struct _HdySwipeableInterface
{
  GTypeInterface parent;

  void (*switch_child) (HdySwipeable *self,
                        guint         index,
                        gint64        duration);
  void (*begin_swipe)  (HdySwipeable  *self);
  void (*update_swipe) (HdySwipeable *self,
                        gdouble       value);
  void (*end_swipe)    (HdySwipeable *self,
                        gint64        duration,
                        gdouble       to);
};

G_END_DECLS

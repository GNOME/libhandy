/*
 * Copyright (C) 2019 Alexander Mikhaylenko <exalm7659@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#if !defined(_HANDY_INSIDE) && !defined(HANDY_COMPILATION)
#error "Only <handy.h> can be included directly."
#endif

#include <gtk/gtk.h>
#include "hdy-swipeable-private.h"

G_BEGIN_DECLS

#define HDY_TYPE_SWIPE_TRACKER (hdy_swipe_tracker_get_type())

G_DECLARE_FINAL_TYPE (HdySwipeTracker, hdy_swipe_tracker, HDY, SWIPE_TRACKER, GObject)

HdySwipeTracker *hdy_swipe_tracker_new (HdySwipeable *swipeable);

gboolean         hdy_swipe_tracker_get_enabled (HdySwipeTracker *self);
void             hdy_swipe_tracker_set_enabled (HdySwipeTracker *self,
                                                gboolean         enabled);

gboolean         hdy_swipe_tracker_get_reversed (HdySwipeTracker *self);
void             hdy_swipe_tracker_set_reversed (HdySwipeTracker *self,
                                                 gboolean         reversed);

gboolean         hdy_swipe_tracker_get_allow_mouse_drag (HdySwipeTracker *self);
void             hdy_swipe_tracker_set_allow_mouse_drag (HdySwipeTracker *self,
                                                         gboolean         allow_mouse_drag);

gboolean         hdy_swipe_tracker_captured_event (HdySwipeTracker *self,
                                                   GdkEvent        *event);

void             hdy_swipe_tracker_confirm_swipe (HdySwipeTracker *self,
                                                  gdouble          distance,
                                                  gdouble         *snap_points,
                                                  gint             n_snap_points,
                                                  gdouble          current_progress,
                                                  gdouble          cancel_progress);

G_END_DECLS

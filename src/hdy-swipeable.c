/*
 * Copyright (C) 2019 Alexander Mikhaylenko <exalm7659@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "hdy-swipeable-private.h"

/**
 * SECTION:hdy-swipeable
 * @short_description: An interface for swipeable widgets.
 * @title: HdySwipeable
 * @See_also: #HdyCarousel, #HdyDeck, #HdyLeaflet, #HdySwipeGroup
 *
 * The #HdySwipeable interface is implemented by all swipeable widgets. They
 * can be synced using #HdySwipeGroup.
 *
 * #HdySwipeable is only meant to be used by libhandy widgets and is currently
 * implemented by #HdyCarousel, #HdyDeck and #HdyLeaflet. It should not be
 * implemented by applications.
 *
 * Since: 0.0.12
 */

G_DEFINE_INTERFACE (HdySwipeable, hdy_swipeable, GTK_TYPE_WIDGET)

static void
hdy_swipeable_default_init (HdySwipeableInterface *iface)
{
}

/**
 * hdy_swipeable_get_distance:
 * @self: a #HdySwipeable
 *
 * Gets the swipe distance of @self. This corresponds to how many pixels
 * 1 unit represents.
 *
 * Returns: the swipe distance in pixels
 *
 * Since: 1.0
 */
gdouble
hdy_swipeable_get_distance (HdySwipeable *self)
{
  HdySwipeableInterface *iface;

  g_return_val_if_fail (HDY_IS_SWIPEABLE (self), 0);

  iface = HDY_SWIPEABLE_GET_IFACE (self);
  g_return_val_if_fail (iface->get_distance != NULL, 0);

  return (* iface->get_distance) (self);
}

/**
 * hdy_swipeable_get_range:
 * @self: a #HdySwipeable
 * @lower: (out) (allow-none): location to store the minimum possible value, or %NULL
 * @upper: (out) (allow-none): location to store the maximum possible value, or %NULL
 *
 * Gets the range of possible progress values.
 *
 * Since: 1.0
 */
void
hdy_swipeable_get_range (HdySwipeable *self,
                         gdouble      *lower,
                         gdouble      *upper)
{
  HdySwipeableInterface *iface;

  g_return_if_fail (HDY_IS_SWIPEABLE (self));

  iface = HDY_SWIPEABLE_GET_IFACE (self);
  g_return_if_fail (iface->get_range != NULL);

  (* iface->get_range) (self, lower, upper);
}

static gdouble *
get_snap_points_from_range (HdySwipeable *self,
                            gint         *n_snap_points)
{
  gint n;
  gdouble *points, lower, upper;

  hdy_swipeable_get_range (self, &lower, &upper);

  n = (lower != upper) ? 2 : 1;

  points = g_new0 (gdouble, n);
  points[0] = lower;
  points[n - 1] = upper;

  if (n_snap_points)
    *n_snap_points = n;

  return points;
}

/**
 * hdy_swipeable_get_snap_points: (virtual get_snap_points)
 * @self: a #HdySwipeable
 * @n_snap_points: (out): location to return the number of the snap points
 *
 * Gets the snap points of @self. Each snap point represents a progress value
 * that is considered acceptable to end the swipe on.
 *
 * If not implemented, the default implementation returns one snap point for
 * each end of the range, or just one snap point if they are equal.
 *
 * Returns: (array length=n_snap_points) (transfer full): the snap points of
 *     @self. The array must be freed with g_free().
 *
 * Since: 1.0
 */
gdouble *
hdy_swipeable_get_snap_points (HdySwipeable *self,
                               gint         *n_snap_points)
{
  HdySwipeableInterface *iface;

  g_return_val_if_fail (HDY_IS_SWIPEABLE (self), NULL);

  iface = HDY_SWIPEABLE_GET_IFACE (self);

  if (iface->get_snap_points)
    return (* iface->get_snap_points) (self, n_snap_points);

  return get_snap_points_from_range (self, n_snap_points);
}

/**
 * hdy_swipeable_get_progress:
 * @self: a #HdySwipeable
 *
 * Gets the current progress of @self
 *
 * Returns: the current progress, unitless
 *
 * Since: 1.0
 */
gdouble
hdy_swipeable_get_progress (HdySwipeable *self)
{
  HdySwipeableInterface *iface;

  g_return_val_if_fail (HDY_IS_SWIPEABLE (self), 0);

  iface = HDY_SWIPEABLE_GET_IFACE (self);
  g_return_val_if_fail (iface->get_progress != NULL, 0);

  return (* iface->get_progress) (self);
}

/**
 * hdy_swipeable_get_cancel_progress:
 * @self: a #HdySwipeable
 *
 * Gets the progress @self will snap back to after the gesture is canceled.
 *
 * Returns: the cancel progress, unitless
 *
 * Since: 1.0
 */
gdouble
hdy_swipeable_get_cancel_progress (HdySwipeable *self)
{
  HdySwipeableInterface *iface;

  g_return_val_if_fail (HDY_IS_SWIPEABLE (self), 0);

  iface = HDY_SWIPEABLE_GET_IFACE (self);
  g_return_val_if_fail (iface->get_cancel_progress != NULL, 0);

  return (* iface->get_cancel_progress) (self);
}

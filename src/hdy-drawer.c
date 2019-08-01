/*
 * Copyright (C) 2019 Alexander Mikhaylenko <exalm7659@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "hdy-drawer.h"

#include "hdy-swipe-tracker-private.h"

#include <math.h>
#include <stdlib.h>

#define EDGE_THRESHOLD 16
#define DEFAULT_DURATION 250

/**
 * SECTION:hdy-drawer
 * @short_description: A sliding drawer widget.
 * @title: HdyDrawer
 *
 * The #HdyDrawer widget can be used to create sliding drawers.
 *
 * Since: 0.0.11
 */

struct _HdyDrawer
{
  GtkEventBox parent_instance;

  struct {
    uint tick_cb_id;
    int64_t start_time;
    int64_t end_time;
    double start_position;
    double end_position;
  } animation_data;

  HdySwipeTracker *tracker;
  GList *anchors;
  GtkWidget *overlay;
  double *snap_points;
  int n_snap_points;

  double progress;
  gboolean can_hide_overlay;
  GtkPanDirection direction;
  uint animation_duration;
};

static void hdy_drawer_buildable_init (GtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (HdyDrawer, hdy_drawer, GTK_TYPE_EVENT_BOX,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE,
                                                hdy_drawer_buildable_init))

enum {
  PROP_0,
  PROP_INTERACTIVE,
  PROP_PROGRESS,
  PROP_CAN_HIDE_OVERLAY,
  PROP_DIRECTION,
  PROP_ANIMATION_DURATION,
  LAST_PROP,
};

static GParamSpec *props[LAST_PROP];

static int
get_widget_width (GtkWidget *widget) {
  return gtk_widget_get_allocated_width (widget) +
         gtk_widget_get_margin_start (widget) +
         gtk_widget_get_margin_end (widget);
}

static int
get_widget_height (GtkWidget *widget) {
  return gtk_widget_get_allocated_height (widget) +
         gtk_widget_get_margin_top (widget) +
         gtk_widget_get_margin_bottom (widget);
}

static double
get_anchor_value (HdyDrawer *self,
                  GtkWidget *widget)
{
  switch (self->direction) {
  case GTK_PAN_DIRECTION_UP:
  case GTK_PAN_DIRECTION_DOWN:
    return (double) get_widget_height (widget) /
                    get_widget_height (self->overlay);

  case GTK_PAN_DIRECTION_LEFT:
  case GTK_PAN_DIRECTION_RIGHT:
    return (double) get_widget_width (widget) /
                    get_widget_width (self->overlay);

  default:
    g_assert_not_reached ();
  }
}

static int
double_cmp_func (const void *p1,
                 const void *p2)
{
  double a = *(double *) p1;
  double b = *(double *) p2;
  return (int) (a > b) - (int) (a < b);
}

static void
update_snap_points (HdyDrawer *self)
{
  int n_snap_points, i;
  GList *anchors;

  n_snap_points = 0;
  if (self->can_hide_overlay)
    n_snap_points++;
  if (self->overlay)
    n_snap_points += 1 + g_list_length (self->anchors);

  if (n_snap_points != self->n_snap_points) {
    if (!self->snap_points)
      self->snap_points = g_new (double, n_snap_points);
    else
      self->snap_points = g_renew (double, self->snap_points, n_snap_points);
    self->n_snap_points = n_snap_points;
  }

  i = 0;
  if (self->can_hide_overlay)
    self->snap_points[i++] = 0;
  if (self->overlay) {
    for (anchors = self->anchors; anchors; anchors = anchors->next)
      self->snap_points[i++] = get_anchor_value (self, anchors->data);

    self->snap_points[i] = 1;
  }

  qsort (self->snap_points, n_snap_points, sizeof (double), double_cmp_func);
}

static void
set_progress (HdyDrawer *self,
              double     progress)
{
  self->progress = CLAMP (progress, 0, 1 + g_list_length (self->anchors));

  gtk_widget_queue_resize (GTK_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PROGRESS]);
}

#define LERP(a, b, t) ((a) + (((b) - (a)) * (t)))

static double
get_position (HdyDrawer *self)
{
  double upper_point, lower_point, t, i;

  if (self->n_snap_points == 0)
    return 0;

  upper_point = self->snap_points[(int) ceil (self->progress)];
  lower_point = self->snap_points[(int) floor (self->progress)];
  t = modf (self->progress, &i);

  return LERP (lower_point, upper_point, t);
}

static void
set_position (HdyDrawer *self,
              double     position)
{
  int upper_index, lower_index, i, n;
  double upper, lower, t;

  if (self->n_snap_points == 0) {
    set_progress (self, 0);
    return;
  }

  n = self->n_snap_points;
  position = CLAMP (position, self->snap_points[0], self->snap_points[n - 1]);

  upper_index = 0;
  lower_index = self->n_snap_points - 1;

  for (i = 0; i < n; i++) {
    if (self->snap_points[i] >= position) {
      upper_index = i;
      break;
    }
  }

  for (i = n - 1; i >= 0; i--) {
    if (self->snap_points[i] <= position) {
      lower_index = i;
      break;
    }
  }

  lower = self->snap_points[lower_index];
  upper = self->snap_points[upper_index];

  if (upper_index == lower_index) {
    set_progress (self, upper_index);
    return;
  }

  t = (position - lower) / (upper - lower);
  set_progress (self, LERP (lower_index, upper_index, t));
}

static GtkPanDirection
get_real_direction (HdyDrawer *self)
{
  if (gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL) {
    if (self->direction == GTK_PAN_DIRECTION_LEFT)
      return GTK_PAN_DIRECTION_RIGHT;
    if (self->direction == GTK_PAN_DIRECTION_RIGHT)
      return GTK_PAN_DIRECTION_LEFT;
  }

  return self->direction;
}

static void
update_tracker_direction (HdyDrawer *self)
{
  GtkPanDirection direction;
  GtkOrientation orientation;
  gboolean horiz, reversed;

  if (!self->tracker)
    return;

  direction = get_real_direction (self);
  horiz = (direction == GTK_PAN_DIRECTION_LEFT || direction == GTK_PAN_DIRECTION_RIGHT);
  orientation = (horiz ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL);
  reversed = (direction == GTK_PAN_DIRECTION_DOWN || direction == GTK_PAN_DIRECTION_RIGHT);

  g_object_set (self->tracker,
                "orientation", orientation,
                "reversed", reversed,
                NULL);
}

static gboolean
can_swipe_from (HdyDrawer *self,
                double     x,
                double     y)
{
  double distance;
  int width, height;

  distance = MAX (hdy_drawer_get_offset (self), EDGE_THRESHOLD);

  switch (get_real_direction (self)) {
  case GTK_PAN_DIRECTION_LEFT:
    width = gtk_widget_get_allocated_width (GTK_WIDGET (self));
    return (x >= width - distance);

  case GTK_PAN_DIRECTION_RIGHT:
    return (x <= distance);

  case GTK_PAN_DIRECTION_UP:
    height = gtk_widget_get_allocated_height (GTK_WIDGET (self));
    return (y >= height - distance);

  case GTK_PAN_DIRECTION_DOWN:
    return (y <= distance);

  default:
    g_assert_not_reached ();
  }
}

static int
get_distance (HdyDrawer *self)
{
  GtkRequisition min, nat;
  int width, height;

  if (!self->overlay)
    return 0;

  gtk_widget_get_preferred_size (self->overlay, &min, &nat);
  width = gtk_widget_get_allocated_width (GTK_WIDGET (self));
  height = gtk_widget_get_allocated_height (GTK_WIDGET (self));

  switch (self->direction) {
  case GTK_PAN_DIRECTION_UP:
  case GTK_PAN_DIRECTION_DOWN:
    if (gtk_widget_get_valign (self->overlay) == GTK_ALIGN_FILL)
      return height;
    return nat.height;

  case GTK_PAN_DIRECTION_LEFT:
  case GTK_PAN_DIRECTION_RIGHT:
    if (gtk_widget_get_halign (self->overlay) == GTK_ALIGN_FILL)
      return width;
    return nat.width;

  default:
    g_assert_not_reached ();
  }
}

static void
stop_animation (HdyDrawer *self)
{
  if (self->animation_data.tick_cb_id == 0)
    return;

  gtk_widget_remove_tick_callback (GTK_WIDGET (self),
                                   self->animation_data.tick_cb_id);
  self->animation_data.tick_cb_id = 0;
}

static double
ease_out_cubic (double t)
{
  double p = t - 1;
  return p * p * p + 1;
}

static gboolean
animation_cb (GtkWidget     *widget,
              GdkFrameClock *frame_clock,
              gpointer       user_data)
{
  HdyDrawer *self = HDY_DRAWER (widget);
  int64_t frame_time, duration;
  double position;
  double t;

  g_assert (self->animation_data.tick_cb_id != 0);

  frame_time = gdk_frame_clock_get_frame_time (frame_clock) / 1000;
  frame_time = MIN (frame_time, self->animation_data.end_time);

  duration = self->animation_data.end_time - self->animation_data.start_time;
  position = (double) (frame_time - self->animation_data.start_time) / duration;

  t = ease_out_cubic (position);
  set_position (self, LERP (self->animation_data.start_position,
                            self->animation_data.end_position, t));

  if (frame_time == self->animation_data.end_time) {
    self->animation_data.tick_cb_id = 0;
    return G_SOURCE_REMOVE;
  }

  return G_SOURCE_CONTINUE;
}

static void
animate (HdyDrawer *self,
         double     to,
         int64_t    duration)
{
  GdkFrameClock *frame_clock;
  int64_t frame_time;

  stop_animation (self);

  if (duration <= 0) {
    set_position (self, to);
    return;
  }

  frame_clock = gtk_widget_get_frame_clock (GTK_WIDGET (self));
  frame_time = gdk_frame_clock_get_frame_time (frame_clock);

  self->animation_data.start_position = get_position (self);
  self->animation_data.end_position = to;

  self->animation_data.start_time = frame_time / 1000;
  self->animation_data.end_time = self->animation_data.start_time + duration;
  self->animation_data.tick_cb_id =
    gtk_widget_add_tick_callback (GTK_WIDGET (self), animation_cb, self, NULL);
}

static void
swipe_begin_cb (HdyDrawer       *self,
                double           x,
                double           y,
                HdySwipeTracker *tracker)
{
  double *points;
  double distance, position, closest_point;

  if (!hdy_drawer_get_interactive (self))
    return;

  if (!can_swipe_from (self, x, y))
    return;

  stop_animation (self);

  distance = get_distance (self);
  points = g_memdup (self->snap_points, sizeof (double) * self->n_snap_points);
  position = get_position (self);
  closest_point = points[(int) round (self->progress)];

  hdy_swipe_tracker_confirm_swipe (self->tracker, distance, points,
                                   self->n_snap_points, position,
                                   closest_point);
}

static void
swipe_update_cb (HdyDrawer       *self,
                 double           value,
                 HdySwipeTracker *tracker)
{
  set_position (self, value);
}

static void
swipe_end_cb (HdyDrawer       *self,
              gint64           duration,
              double           to,
              HdySwipeTracker *tracker)
{
  if (duration == 0) {
    set_position (self, to);
    return;
  }

  animate (self, to, duration);
}

static gboolean
captured_event_cb (HdyDrawer *self,
                   GdkEvent  *event)
{
  return hdy_swipe_tracker_captured_event (self->tracker, event);
}

static GtkAllocation
get_overlay_allocation (HdyDrawer     *self,
                        GtkAllocation *allocation)
{
  GtkRequisition min, nat;
  GtkAllocation out;
  GtkAlign halign, valign;
  gboolean hexpand, vexpand;
  GtkPanDirection direction;
  int offset;

  g_assert (self->overlay);

  gtk_widget_get_preferred_size (self->overlay, &min, &nat);

  g_object_get (self->overlay,
                "halign", &halign,
                "valign", &valign,
                "hexpand", &hexpand,
                "vexpand", &vexpand,
                NULL);

  out.x = 0;
  out.y = 0;
  out.width = hexpand ? allocation->width : nat.width;
  out.height = vexpand ? allocation->height : nat.height;

  direction = get_real_direction (self);

  if (direction == GTK_PAN_DIRECTION_UP || direction == GTK_PAN_DIRECTION_DOWN) {
    switch (halign) {
    case GTK_ALIGN_START:
      /* Nothing to do, skip it. */
      break;

    case GTK_ALIGN_CENTER:
      out.x = (allocation->width - out.width) / 2;
      break;

    case GTK_ALIGN_END:
      out.x = allocation->width - out.width;
      break;

    case GTK_ALIGN_FILL:
    case GTK_ALIGN_BASELINE:
      out.width = allocation->width;
      break;

    default:
      g_assert_not_reached ();
    }

    if (direction == GTK_PAN_DIRECTION_UP)
      out.y = allocation->height - out.height;
  }

  if (direction == GTK_PAN_DIRECTION_LEFT || direction == GTK_PAN_DIRECTION_RIGHT) {
    switch (valign) {
    case GTK_ALIGN_START:
      /* Nothing to do, skip it. */
      break;

    case GTK_ALIGN_CENTER:
      out.y = (allocation->height - out.height) / 2;
      break;

    case GTK_ALIGN_END:
      out.y = allocation->height - out.height;
      break;

    case GTK_ALIGN_FILL:
    case GTK_ALIGN_BASELINE:
      out.height = allocation->height;
      break;

    default:
      g_assert_not_reached ();
    }

    if (direction == GTK_PAN_DIRECTION_LEFT)
      out.x = allocation->width - out.width;
  }

  offset = get_distance (self) - hdy_drawer_get_offset (self);

  switch (direction) {
  case GTK_PAN_DIRECTION_LEFT:
    out.x += offset;
    break;

  case GTK_PAN_DIRECTION_RIGHT:
    out.x -= offset;
    break;

  case GTK_PAN_DIRECTION_UP:
    out.y += offset;
    break;

  case GTK_PAN_DIRECTION_DOWN:
    out.y -= offset;
    break;

  default:
    g_assert_not_reached ();
  }
  return out;
}

static void
measure (GtkWidget      *widget,
         GtkOrientation  orientation,
         int             for_size,
         int            *minimum,
         int            *natural,
         int            *minimum_baseline,
         int            *natural_baseline)
{
  HdyDrawer *self = HDY_DRAWER (widget);
  GtkWidget *child;
  int child_min, child_nat, overlay_min, overlay_nat;

  if (minimum_baseline)
    *minimum_baseline = -1;
  if (natural_baseline)
    *natural_baseline = -1;

  child = gtk_bin_get_child (GTK_BIN (widget));

  if (child && gtk_widget_get_visible (child)) {
    if (orientation == GTK_ORIENTATION_VERTICAL)
      if (for_size < 0)
        gtk_widget_get_preferred_height (child, &child_min, &child_nat);
      else
        gtk_widget_get_preferred_height_for_width (child, for_size,
                                                   &child_min, &child_nat);
    else
      if (for_size < 0)
        gtk_widget_get_preferred_width (child, &child_min, &child_nat);
      else
        gtk_widget_get_preferred_width_for_height (child, for_size,
                                                   &child_min, &child_nat);
  } else {
    child_min = 0;
    child_nat = 0;
  }

  if (self->overlay && gtk_widget_get_visible (self->overlay)) {
    if (orientation == GTK_ORIENTATION_VERTICAL)
      if (for_size < 0)
        gtk_widget_get_preferred_height (self->overlay,
                                         &overlay_min, &overlay_nat);
      else
        gtk_widget_get_preferred_height_for_width (self->overlay, for_size,
                                                   &overlay_min, &overlay_nat);
    else
      if (for_size < 0)
        gtk_widget_get_preferred_width (self->overlay,
                                        &overlay_min, &overlay_nat);
      else
        gtk_widget_get_preferred_width_for_height (self->overlay, for_size,
                                                   &overlay_min, &overlay_nat);
  } else {
    overlay_min = 0;
    overlay_nat = 0;
  }

  if (minimum)
    *minimum = MAX (child_min, overlay_min);
  if (natural)
    *natural = MAX (child_nat, overlay_nat);
}

static void
hdy_drawer_direction_changed (GtkWidget        *widget,
                              GtkTextDirection  previous_direction)
{
  HdyDrawer *self = HDY_DRAWER (widget);

  update_tracker_direction (self);
}

static void
hdy_drawer_get_preferred_width (GtkWidget *widget,
                                int       *minimum_width,
                                int       *natural_width)
{
  measure (widget, GTK_ORIENTATION_HORIZONTAL, -1,
           minimum_width, natural_width, NULL, NULL);
}

static void
hdy_drawer_get_preferred_height (GtkWidget *widget,
                                 int       *minimum_height,
                                 int       *natural_height)
{
  measure (widget, GTK_ORIENTATION_VERTICAL, -1,
           minimum_height, natural_height, NULL, NULL);
}

static void
hdy_drawer_get_preferred_width_for_height (GtkWidget *widget,
                                           int        for_height,
                                           int       *minimum_width,
                                           int       *natural_width)
{
  measure (widget, GTK_ORIENTATION_HORIZONTAL, for_height,
           minimum_width, natural_width, NULL, NULL);
}

static void
hdy_drawer_get_preferred_height_for_width (GtkWidget *widget,
                                           int        for_width,
                                           int       *minimum_height,
                                           int       *natural_height)
{
  measure (widget, GTK_ORIENTATION_VERTICAL, for_width,
           minimum_height, natural_height, NULL, NULL);
}

static void
hdy_drawer_size_allocate (GtkWidget     *widget,
                          GtkAllocation *allocation)
{
  HdyDrawer *self = HDY_DRAWER (widget);
  gboolean need_reallocation;

  GTK_WIDGET_CLASS (hdy_drawer_parent_class)->size_allocate (widget, allocation);

  if (self->overlay) {
    GtkAllocation alloc;

    alloc = get_overlay_allocation (self, allocation);
    gtk_widget_size_allocate (self->overlay, &alloc);
  }

  gtk_widget_set_clip (widget, allocation);

  /* To create snap points for the first time, we need to allocate the children.
   * To allocate the overlay properly, we need the snap points. To break this
   * cycle, allocate  the second time if no snap points exited. This is not
   * perfect, and could have been avoided if we could use a moving window. */
  need_reallocation = (self->n_snap_points == 0);
  update_snap_points (self);
  if (need_reallocation)
    hdy_drawer_size_allocate (widget, allocation);
}

static void
hdy_drawer_remove (GtkContainer *container,
                   GtkWidget    *widget)
{
  HdyDrawer *self = HDY_DRAWER (container);

  if (widget == self->overlay) {
    hdy_drawer_set_overlay (self, NULL);
    return;
  }

  GTK_CONTAINER_CLASS (hdy_drawer_parent_class)->remove (container, widget);
}

static void
hdy_drawer_forall (GtkContainer *container,
                   gboolean      include_internals,
                   GtkCallback   callback,
                   gpointer      callback_data)
{
  HdyDrawer *self = HDY_DRAWER (container);

  GTK_CONTAINER_CLASS (hdy_drawer_parent_class)->forall (container,
                                                         include_internals,
                                                         callback,
                                                         callback_data);

  if (self->overlay)
    (* callback) (self->overlay, callback_data);
}

static void
hdy_drawer_buildable_add_child (GtkBuildable *buildable,
                                GtkBuilder   *builder,
                                GObject      *child,
                                const gchar  *type)
{
  if (g_strcmp0 (type, "overlay") == 0)
    hdy_drawer_set_overlay (HDY_DRAWER (buildable), GTK_WIDGET (child));
  else if (!type)
    gtk_container_add (GTK_CONTAINER (buildable), GTK_WIDGET (child));
  else
    GTK_BUILDER_WARN_INVALID_CHILD_TYPE (buildable, type);
}

static void
hdy_drawer_constructed (GObject *object)
{
  HdyDrawer *self = (HdyDrawer *)object;

  self->tracker = hdy_swipe_tracker_new (GTK_WIDGET (self));
  g_signal_connect_swapped (self->tracker, "begin", G_CALLBACK (swipe_begin_cb), self);
  g_signal_connect_swapped (self->tracker, "update", G_CALLBACK (swipe_update_cb), self);
  g_signal_connect_swapped (self->tracker, "end", G_CALLBACK (swipe_end_cb), self);

  update_tracker_direction (self);

  /*
   * HACK: GTK3 has no other way to get events on capture phase.
   * This is a reimplementation of _gtk_widget_set_captured_event_handler(),
   * which is private. In GTK4 it can be replaced with GtkEventControllerLegacy
   * with capture propagation phase
   */
  g_object_set_data (object, "captured-event-handler", captured_event_cb);
}

static void
hdy_drawer_finalize (GObject *object)
{
  HdyDrawer *self = (HdyDrawer *)object;

  g_signal_handlers_disconnect_by_data (self->tracker, self);
  g_object_unref (self->tracker);

  g_object_set_data (object, "captured-event-handler", NULL);

  g_list_free_full (self->anchors, g_object_unref);

  g_free (self->snap_points);

  G_OBJECT_CLASS (hdy_drawer_parent_class)->finalize (object);
}

static void
hdy_drawer_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  HdyDrawer *self = HDY_DRAWER (object);

  switch (prop_id) {
  case PROP_INTERACTIVE:
    g_value_set_boolean (value, hdy_drawer_get_interactive (self));
    break;

  case PROP_PROGRESS:
    g_value_set_double (value, hdy_drawer_get_progress (self));
    break;

  case PROP_CAN_HIDE_OVERLAY:
    g_value_set_boolean (value, hdy_drawer_get_can_hide_overlay (self));
    break;

  case PROP_DIRECTION:
    g_value_set_enum (value, hdy_drawer_get_direction (self));
    break;

  case PROP_ANIMATION_DURATION:
    g_value_set_uint (value, hdy_drawer_get_animation_duration (self));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
hdy_drawer_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  HdyDrawer *self = HDY_DRAWER (object);

  switch (prop_id) {
  case PROP_INTERACTIVE:
    hdy_drawer_set_interactive (self, g_value_get_boolean (value));
    break;

  case PROP_CAN_HIDE_OVERLAY:
    hdy_drawer_set_can_hide_overlay (self, g_value_get_boolean (value));
    break;

  case PROP_DIRECTION:
    hdy_drawer_set_direction (self, g_value_get_enum (value));
    break;

  case PROP_ANIMATION_DURATION:
    hdy_drawer_set_animation_duration (self, g_value_get_uint (value));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
hdy_drawer_buildable_init (GtkBuildableIface *iface)
{
  iface->add_child = hdy_drawer_buildable_add_child;
}

static void
hdy_drawer_class_init (HdyDrawerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->constructed = hdy_drawer_constructed;
  object_class->finalize = hdy_drawer_finalize;
  object_class->get_property = hdy_drawer_get_property;
  object_class->set_property = hdy_drawer_set_property;
  widget_class->direction_changed = hdy_drawer_direction_changed;
  widget_class->get_preferred_width = hdy_drawer_get_preferred_width;
  widget_class->get_preferred_height = hdy_drawer_get_preferred_height;
  widget_class->get_preferred_width_for_height = hdy_drawer_get_preferred_width_for_height;
  widget_class->get_preferred_height_for_width = hdy_drawer_get_preferred_height_for_width;
  widget_class->size_allocate = hdy_drawer_size_allocate;
  container_class->remove = hdy_drawer_remove;
  container_class->forall = hdy_drawer_forall;

  /**
   * HdyDrawer:interactive:
   *
   * Whether @self can be swiped. This can be used to temporarily disable
   * a #HdyDrawer to only allow swiping in a certain state.
   *
   * Since: 0.0.11
   */
  props[PROP_INTERACTIVE] =
    g_param_spec_boolean ("interactive",
                          _("Interactive"),
                          _("Whether the widget can be swiped"),
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * HdyDrawer:position:
   *
   * The current progress value. It is unitless. If there are no anchors,
   * the value changes from 0 to 1. If there are anchors, the distance between
   * each anchor is 1 even when the distance in pixels is different.
   *
   * Since: 0.0.11
   */
  props[PROP_PROGRESS] =
    g_param_spec_double ("progress",
                         _("Progress"),
                         _("Reveal progress"),
                         0,
                         G_MAXDOUBLE,
                         0,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * HdyDrawer:can-hide-overlay:
   *
   * Whether overlay widget can be completely hidden. If %FALSE, the smallest
   * anchor will be used as the minimum point. This can be useful if a part of
   * the overlay (for example, a drag handle) must be on screen at all times.
   *
   * Since: 0.0.11
   */
  props[PROP_CAN_HIDE_OVERLAY] =
    g_param_spec_boolean ("can-hide-overlay",
                          _("Can hide overlay"),
                          _("Whether overlay widget can be completely hidden"),
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * HdyDrawer:direction:
   *
   * Swipe direction. For example, @GTK_PAN_DIRECTION_UP means a bottom to top
   * swipe opens the drawer and a top to bottom swipe closes it.
   *
   * Since: 0.0.11
   */
  props[PROP_DIRECTION] =
    g_param_spec_enum ("direction",
                       _("Direction"),
                       _("Swipe direction"),
                       GTK_TYPE_PAN_DIRECTION,
                       GTK_PAN_DIRECTION_UP,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * HdyDrawer:animation-duration:
   *
   * Animation duration in milliseconds, used by hdy_drawer_open() and
   * hdy_drawer_close().
   *
   * Since: 0.0.11
   */
  props[PROP_ANIMATION_DURATION] =
    g_param_spec_uint ("animation-duration",
                       _("Animation duration"),
                       _("Animation duration"),
                       0, G_MAXUINT, DEFAULT_DURATION,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_css_name (widget_class, "hdydrawer");
}

static void
hdy_drawer_init (HdyDrawer *self)
{
  self->direction = GTK_PAN_DIRECTION_UP;
  self->progress = 0;
  self->can_hide_overlay = TRUE;
  self->n_snap_points = 0;
  self->animation_duration = DEFAULT_DURATION;
}

/**
 * hdy_drawer_new:
 *
 * Create a new #HdyDrawer widget.
 *
 * Returns: The newly created #HdyDrawer widget
 *
 * Since: 0.0.11
 */
HdyDrawer *
hdy_drawer_new (void)
{
  return g_object_new (HDY_TYPE_DRAWER, NULL);
}

/**
 * hdy_drawer_open:
 * @self: a #HdyDrawer
 * @widget: the overlay or an anchor widget
 *
 * Opens the drawer with an animation. If @widget is the overlay, it will open
 * the drawer completely, if @widget is an anchor, it will only reveal it far
 * enough to show the anchor completely.
 *
 * #HdyDrawer:animation-duration property can be used for controlling the
 * duration.
 *
 * Since: 0.0.11
 */
void
hdy_drawer_open (HdyDrawer *self,
                 GtkWidget *widget)
{
  double value;

  g_return_if_fail (HDY_IS_DRAWER (self));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (widget == self->overlay) {
    value = self->overlay ? 1 : 0;
  } else {
    if (!g_list_find (self->anchors, widget))
      return;

    value = get_anchor_value (self, widget);
  }

  animate (self, value, self->animation_duration);
}

/**
 * hdy_drawer_close:
 * @self: a #HdyDrawer
 *
 * Closes the drawer with an animation. If #HdyDrawer:can-hide-overlay is %TRUE,
 * it will be closed completely, otherwise the smallest anchor will be shown.
 *
 * #HdyDrawer:animation-duration property can be used for controlling the
 * duration.
 *
 * Since: 0.0.11
 */
void
hdy_drawer_close (HdyDrawer *self)
{
  double pos;

  g_return_if_fail (HDY_IS_DRAWER (self));

  pos = 0;
  if (self->n_snap_points != 0)
    pos = self->snap_points[0];

  animate (self, pos, self->animation_duration);
}

/**
 * hdy_drawer_get_overlay:
 * @self: a #HdyDrawer
 *
 * Gets the drawer overlay of @self, or %NULL if it's not set.
 *
 * Returns: (transfer none) (nullable): the drawer overlay, or %NULL
 *
 * Since: 0.0.11
 */
GtkWidget *
hdy_drawer_get_overlay (HdyDrawer *self)
{
  g_return_val_if_fail (HDY_IS_DRAWER (self), NULL);

  return self->overlay;
}

/**
 * hdy_drawer_set_overlay:
 * @self: a #HdyDrawer
 * @widget: (transfer full) (nullable): a widget or %NULL
 *
 * Sets the drawer overlay to @widget, or unsets it if @widget is %NULL.
 *
 * Since: 0.0.11
 */
void
hdy_drawer_set_overlay (HdyDrawer *self,
                        GtkWidget *widget)
{
  g_return_if_fail (HDY_IS_DRAWER (self));

  if (self->overlay)
    gtk_widget_unparent (self->overlay);

  self->overlay = widget;

  if (self->overlay)
    gtk_widget_set_parent (self->overlay, GTK_WIDGET (self));
}

/**
 * hdy_drawer_add_anchor:
 * @self: a #HdyDrawer
 * @widget: (transfer none): a widget to add
 *
 * Adds @widget as an anchor. The drawer will use this widget's width or height
 * (depending on #HdyDrawer:direction) as another snap point.
 *
 * @widget must be a child of the overlay
 *
 * Since: 0.0.11
 */
void
hdy_drawer_add_anchor (HdyDrawer *self,
                       GtkWidget *widget)
{
  g_return_if_fail (HDY_IS_DRAWER (self));
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (GTK_IS_WIDGET (self->overlay));
  g_return_if_fail (gtk_widget_is_ancestor (widget, GTK_WIDGET (self->overlay)));
  g_return_if_fail (g_list_index (self->anchors, widget) < 0);

  self->anchors = g_list_prepend (self->anchors, g_object_ref (widget));

  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

/**
 * hdy_drawer_remove_anchor:
 * @self: a #HdyDrawer
 * @widget: (transfer none): a widget to remove
 *
 * Removes the widget from anchors list.
 *
 * Since: 0.0.11
 */
void
hdy_drawer_remove_anchor (HdyDrawer *self,
                          GtkWidget *widget)
{
  g_return_if_fail (HDY_IS_DRAWER (self));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  self->anchors = g_list_remove (self->anchors, widget);
  g_object_unref (widget);

  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

/**
 * hdy_drawer_get_offset:
 * @self: a #HdyDrawer
 *
 * Gets the current position in pixels.
 *
 * Returns: the current position
 *
 * Since: 0.0.11
 */
int
hdy_drawer_get_offset (HdyDrawer *self)
{
  g_return_val_if_fail (HDY_IS_DRAWER (self), 0);

  return (int) (get_distance (self) * get_position (self));
}

/**
 * hdy_drawer_get_interactive
 * @self: a #HdyDrawer
 *
 * Gets whether @self can be swiped.
 *
 * Returns: %TRUE if @self can be swiped
 *
 * Since: 0.0.11
 */
gboolean
hdy_drawer_get_interactive (HdyDrawer *self)
{
  g_return_val_if_fail (HDY_IS_DRAWER (self), FALSE);

  return hdy_swipe_tracker_get_enabled (self->tracker);
}

/**
 * hdy_drawer_set_interactive
 * @self: a #HdyDrawer
 * @interactive: whether @self can be swiped.
 *
 * Sets whether @self can be swiped. This can be used to temporarily disable
 * a #HdyDrawer to only allow swiping in a certain state.
 *
 * Since: 0.0.11
 */
void
hdy_drawer_set_interactive (HdyDrawer *self,
                            gboolean   interactive)
{
  g_return_if_fail (HDY_IS_DRAWER (self));

  hdy_swipe_tracker_set_enabled (self->tracker, interactive);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_INTERACTIVE]);
}

/**
 * hdy_drawer_get_progress
 * @self: a #HdyDrawer
 *
 * Gets the current progress value. It is unitless. If there are no anchors,
 * the value changes from 0 to 1. If there are anchors, the distance between
 * each anchor is 1 even when the distance in pixels is different.
 *
 * Returns: the progress value
 *
 * Since: 0.0.11
 */
double
hdy_drawer_get_progress (HdyDrawer *self)
{
  g_return_val_if_fail (HDY_IS_DRAWER (self), FALSE);

  return self->progress;
}

/**
 * hdy_drawer_get_can_hide_overlay
 * @self: a #HdyDrawer
 *
 * Gets whether the overlay can be completely hidden.
 *
 * Returns: %TRUE if overlay can be completely hidden
 *
 * Since: 0.0.11
 */
gboolean
hdy_drawer_get_can_hide_overlay (HdyDrawer *self)
{
  g_return_val_if_fail (HDY_IS_DRAWER (self), FALSE);

  return self->can_hide_overlay;
}

/**
 * hdy_drawer_set_can_hide_overlay
 * @self: a #HdyDrawer
 * @can_hide_overlay: if %TRUE, the overlay can be hidden
 *
 * Set whether the overlay can be hidden. If %FALSE, the smallest anchor will
 * be used as the minimum point. This can be useful if a part of the overlay
 * (for example, a drag handle) must be on screen at all times.
 *
 * Since: 0.0.11
 */
void
hdy_drawer_set_can_hide_overlay (HdyDrawer *self,
                                 gboolean   can_hide_overlay)
{
  g_return_if_fail (HDY_IS_DRAWER (self));

  can_hide_overlay = !!can_hide_overlay;

  if (self->can_hide_overlay == can_hide_overlay)
    return;

  self->can_hide_overlay = can_hide_overlay;

  update_snap_points (self);

  if (can_hide_overlay)
    set_progress (self, self->progress + 1);
  else
    set_progress (self, self->progress - 1);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CAN_HIDE_OVERLAY]);
}

/**
 * hdy_drawer_get_direction
 * @self: a #HdyDrawer
 *
 * Get swiping direction of @self.
 *
 * Returns: the drawer direction
 *
 * Since: 0.0.11
 */
GtkPanDirection
hdy_drawer_get_direction (HdyDrawer *self)
{
  g_return_val_if_fail (HDY_IS_DRAWER (self), GTK_PAN_DIRECTION_UP);

  return self->direction;
}

/**
 * hdy_drawer_set_direction
 * @self: a #HdyDrawer
 * @direction: the swiping direction
 *
 * Set swiping direction of @self. For example, @GTK_PAN_DIRECTION_UP means a
 * bottom to top swipe opens the drawer and a top to bottom swipe closes it.
 *
 * Since: 0.0.11
 */
void
hdy_drawer_set_direction (HdyDrawer       *self,
                          GtkPanDirection  direction)
{
  g_return_if_fail (HDY_IS_DRAWER (self));

  if (self->direction == direction)
    return;

  self->direction = direction;

  gtk_widget_queue_resize (GTK_WIDGET (self));
  update_tracker_direction (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_DIRECTION]);
}

/**
 * hdy_drawer_get_animation_duration:
 * @self: a #HdyDrawer
 *
 * Gets animation duration used by hdy_drawer_open() and hdy_drawer_close().
 *
 * Returns: Animation duration in milliseconds
 *
 * Since: 0.0.11
 */
uint
hdy_drawer_get_animation_duration (HdyDrawer *self)
{
  g_return_val_if_fail (HDY_IS_DRAWER (self), 0);

  return self->animation_duration;
}

/**
 * hdy_drawer_set_animation_duration:
 * @self: a #HdyDrawer
 * @duration: animation duration in milliseconds
 *
 * Sets animation duration used by hdy_drawer_open() and hdy_drawer_close().
 *
 * Since: 0.0.11
 */
void
hdy_drawer_set_animation_duration (HdyDrawer *self,
                                   uint       duration)
{
  g_return_if_fail (HDY_IS_DRAWER (self));

  if (self->animation_duration == duration)
    return;

  self->animation_duration = duration;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ANIMATION_DURATION]);
}

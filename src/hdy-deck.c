/*
 * Copyright (C) 2018 Purism SPC
 * Copyright (C) 2019 Alexander Mikhaylenko <exalm7659@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "gtkprogresstrackerprivate.h"
#include "hdy-animation-private.h"
#include "hdy-deck.h"
#include "hdy-shadow-helper-private.h"
#include "hdy-swipeable-private.h"
#include "hdy-swipe-tracker-private.h"

/**
 * SECTION:hdy-deck
 * @short_description: An adaptive container acting like a box or a stack. FIXME
 * @Title: HdyDeck
 *
 * The #HdyDeck widget displays one of the visible children, similar to a
 * #GtkStack. The children are strictly ordered and can be navigated using
 * swipe gestures.
 */

/**
 * HdyDeckTransitionType:
 * @HDY_DECK_TRANSITION_TYPE_NONE: No transition
 * @HDY_DECK_TRANSITION_TYPE_SLIDE: Slide from left, right, up or down according to the orientation, text direction and the children order
 * @HDY_DECK_TRANSITION_TYPE_OVER: Cover the old page or uncover the new page, sliding from or towards the end according to orientation, text direction and children order
 * @HDY_DECK_TRANSITION_TYPE_UNDER: Uncover the new page or cover the old page, sliding from or towards the start according to orientation, text direction and children order
 *
 * This enumeration value describes the possible transitions between modes and
 * children in a #HdyDeck widget.
 *
 * New values may be added to this enumeration over time.
 *
 * Since: 0.0.12
 */

enum {
  PROP_0,
  PROP_HHOMOGENEOUS,
  PROP_VHOMOGENEOUS,
  PROP_VISIBLE_CHILD,
  PROP_VISIBLE_CHILD_NAME,
  PROP_TRANSITION_TYPE,
  PROP_TRANSITION_DURATION,
  PROP_TRANSITION_RUNNING,
  PROP_INTERPOLATE_SIZE,
  PROP_CAN_SWIPE_BACK,
  PROP_CAN_SWIPE_FORWARD,

  /* orientable */
  PROP_ORIENTATION,
  LAST_PROP = PROP_ORIENTATION,
};

enum {
  CHILD_PROP_0,
  CHILD_PROP_NAME,
  LAST_CHILD_PROP,
};

#define GTK_ORIENTATION_MAX 2

typedef struct _HdyDeckChildInfo HdyDeckChildInfo;

struct _HdyDeckChildInfo
{
  GtkWidget *widget;
  gchar *name;

  /* Convenience storage for per-child temporary frequently computed values. */
  GtkAllocation alloc;
  GtkRequisition min;
  GtkRequisition nat;
  gboolean visible;
};

typedef struct
{
  GList *children;
  /* It is probably cheaper to store and maintain a reversed copy of the
   * children list that to reverse the list every time we need to allocate or
   * draw children for RTL languages on a horizontal deck.
   */
  GList *children_reversed;
  HdyDeckChildInfo *visible_child;
  HdyDeckChildInfo *last_visible_child;

  GdkWindow* bin_window;
  GdkWindow* view_window;

  gboolean homogeneous[GTK_ORIENTATION_MAX];

  GtkOrientation orientation;

  gboolean move_bin_window_request;

  HdyDeckTransitionType transition_type;

  HdySwipeTracker *tracker;

  /* Transition variables. */
  struct {
    HdyDeckTransitionType type;
    guint duration;

    gdouble progress;
    gdouble start_progress;
    gdouble end_progress;

    gboolean is_gesture_active;
    gboolean is_cancelled;

    cairo_surface_t *last_visible_surface;
    GtkAllocation last_visible_surface_allocation;
    guint tick_id;
    GtkProgressTracker tracker;
    gboolean first_frame_skipped;

    gint last_visible_widget_width;
    gint last_visible_widget_height;

    gboolean interpolate_size;
    gboolean can_swipe_back;
    gboolean can_swipe_forward;

    HdyDeckTransitionType active_type;
    GtkPanDirection active_direction;
  } transition;

  HdyShadowHelper *shadow_helper;
} HdyDeckPrivate;

static GParamSpec *props[LAST_PROP];
static GParamSpec *child_props[LAST_CHILD_PROP];

static gint HOMOGENEOUS_PROP[GTK_ORIENTATION_MAX] = {
  PROP_HHOMOGENEOUS, PROP_VHOMOGENEOUS,
};

static void hdy_deck_buildable_init (GtkBuildableIface  *iface);
static void hdy_deck_swipeable_init (HdySwipeableInterface *iface);

G_DEFINE_TYPE_WITH_CODE (HdyDeck, hdy_deck, GTK_TYPE_CONTAINER,
                         G_ADD_PRIVATE (HdyDeck)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, hdy_deck_buildable_init)
                         G_IMPLEMENT_INTERFACE (HDY_TYPE_SWIPEABLE, hdy_deck_swipeable_init))

static void
free_child_info (HdyDeckChildInfo *child_info)
{
  g_free (child_info->name);
  g_free (child_info);
}

static HdyDeckChildInfo *
find_child_info_for_widget (HdyDeck   *self,
                            GtkWidget *widget)
{
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);
  GList *children;
  HdyDeckChildInfo *child_info;

  for (children = priv->children; children; children = children->next) {
    child_info = children->data;

    if (child_info->widget == widget)
      return child_info;
  }

  return NULL;
}

static HdyDeckChildInfo *
find_child_info_for_name (HdyDeck     *self,
                          const gchar *name)
{
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);
  GList *children;
  HdyDeckChildInfo *child_info;

  for (children = priv->children; children; children = children->next) {
    child_info = children->data;

    if (g_strcmp0 (child_info->name, name) == 0)
      return child_info;
  }

  return NULL;
}

static GList *
get_directed_children (HdyDeck *self)
{
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);

  return priv->orientation == GTK_ORIENTATION_HORIZONTAL &&
         gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL ?
         priv->children_reversed : priv->children;
}

/* Transitions that cause the bin window to move */
static inline gboolean
is_window_moving_transition (HdyDeck *self)
{
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);
  GtkPanDirection direction;
  gboolean is_rtl;
  GtkPanDirection left_or_right, right_or_left;

  direction = priv->transition.active_direction;
  is_rtl = gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL;
  left_or_right = is_rtl ? GTK_PAN_DIRECTION_RIGHT : GTK_PAN_DIRECTION_LEFT;
  right_or_left = is_rtl ? GTK_PAN_DIRECTION_LEFT : GTK_PAN_DIRECTION_RIGHT;

  switch (priv->transition.active_type) {
  case HDY_DECK_TRANSITION_TYPE_NONE:
    return FALSE;
  case HDY_DECK_TRANSITION_TYPE_SLIDE:
    return TRUE;
  case HDY_DECK_TRANSITION_TYPE_OVER:
    return direction == GTK_PAN_DIRECTION_UP || direction == left_or_right;
  case HDY_DECK_TRANSITION_TYPE_UNDER:
    return direction == GTK_PAN_DIRECTION_DOWN || direction == right_or_left;
  default:
    g_assert_not_reached ();
  }
}

/* Transitions that change direction depending on the relative order of the
old and new child */
static inline gboolean
is_direction_dependent_transition (HdyDeckTransitionType transition_type)
{
  return (transition_type == HDY_DECK_TRANSITION_TYPE_SLIDE ||
          transition_type == HDY_DECK_TRANSITION_TYPE_OVER ||
          transition_type == HDY_DECK_TRANSITION_TYPE_UNDER);
}

static GtkPanDirection
get_pan_direction (HdyDeck  *self,
                   gboolean  new_child_first)
{
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL) {
    if (gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL)
      return new_child_first ? GTK_PAN_DIRECTION_LEFT : GTK_PAN_DIRECTION_RIGHT;
    else
      return new_child_first ? GTK_PAN_DIRECTION_RIGHT : GTK_PAN_DIRECTION_LEFT;
  }
  else
    return new_child_first ? GTK_PAN_DIRECTION_DOWN : GTK_PAN_DIRECTION_UP;
}

static gint
get_bin_window_x (HdyDeck             *self,
                  const GtkAllocation *allocation)
{
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);
  int x = 0;

  if (priv->transition.is_gesture_active ||
      gtk_progress_tracker_get_state (&priv->transition.tracker) != GTK_PROGRESS_STATE_AFTER) {
    if (priv->transition.active_direction == GTK_PAN_DIRECTION_LEFT)
      x = allocation->width * (1 - priv->transition.progress);
    if (priv->transition.active_direction == GTK_PAN_DIRECTION_RIGHT)
      x = -allocation->width * (1 - priv->transition.progress);
  }

  return x;
}

static gint
get_bin_window_y (HdyDeck             *self,
                  const GtkAllocation *allocation)
{
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);
  int y = 0;

  if (priv->transition.is_gesture_active ||
      gtk_progress_tracker_get_state (&priv->transition.tracker) != GTK_PROGRESS_STATE_AFTER) {
    if (priv->transition.active_direction == GTK_PAN_DIRECTION_UP)
      y = allocation->height * (1 - priv->transition.progress);
    if (priv->transition.active_direction == GTK_PAN_DIRECTION_DOWN)
      y = -allocation->height * (1 - priv->transition.progress);
  }

  return y;
}

static void
move_resize_bin_window (HdyDeck       *self,
                        GtkAllocation *allocation,
                        gboolean       resize)
{
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);
  GtkAllocation alloc;
  gboolean move;

  if (priv->bin_window == NULL)
    return;

  if (allocation == NULL) {
    gtk_widget_get_allocation (GTK_WIDGET (self), &alloc);
    allocation = &alloc;
  }

  move = priv->move_bin_window_request || is_window_moving_transition (self);

  if (move && resize)
    gdk_window_move_resize (priv->bin_window,
                            get_bin_window_x (self, allocation), get_bin_window_y (self, allocation),
                            allocation->width, allocation->height);
  else if (move)
    gdk_window_move (priv->bin_window,
                     get_bin_window_x (self, allocation), get_bin_window_y (self, allocation));
  else if (resize)
    gdk_window_resize (priv->bin_window,
                       allocation->width, allocation->height);

  priv->move_bin_window_request = FALSE;
}

static void
hdy_deck_child_progress_updated (HdyDeck *self)
{
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);

  gtk_widget_queue_draw (GTK_WIDGET (self));

  if (!priv->homogeneous[GTK_ORIENTATION_VERTICAL] ||
      !priv->homogeneous[GTK_ORIENTATION_HORIZONTAL])
    gtk_widget_queue_resize (GTK_WIDGET (self));

  move_resize_bin_window (self, NULL, FALSE);

  if (!priv->transition.is_gesture_active &&
      gtk_progress_tracker_get_state (&priv->transition.tracker) == GTK_PROGRESS_STATE_AFTER) {
    if (priv->transition.last_visible_surface != NULL) {
      cairo_surface_destroy (priv->transition.last_visible_surface);
      priv->transition.last_visible_surface = NULL;
    }

    if (priv->transition.is_cancelled) {
      if (priv->last_visible_child != NULL) {
        gtk_widget_set_child_visible (priv->last_visible_child->widget, TRUE);
        gtk_widget_set_child_visible (priv->visible_child->widget, FALSE);
        priv->visible_child = priv->last_visible_child;
        priv->last_visible_child = NULL;
      }

      g_object_freeze_notify (G_OBJECT (self));
      g_object_notify_by_pspec (G_OBJECT (self), props[PROP_VISIBLE_CHILD]);
      g_object_notify_by_pspec (G_OBJECT (self), props[PROP_VISIBLE_CHILD_NAME]);
      g_object_thaw_notify (G_OBJECT (self));
    } else {
      if (priv->last_visible_child != NULL) {
        gtk_widget_set_child_visible (priv->last_visible_child->widget, FALSE);
        priv->last_visible_child = NULL;
      }
    }

    gtk_widget_queue_allocate (GTK_WIDGET (self));
    hdy_shadow_helper_clear_cache (priv->shadow_helper);
  }
}

static gboolean
hdy_deck_transition_cb (GtkWidget     *widget,
                        GdkFrameClock *frame_clock,
                        gpointer       user_data)
{
  HdyDeck *self = HDY_DECK (widget);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);
  gdouble progress;

  if (priv->transition.first_frame_skipped) {
    gtk_progress_tracker_advance_frame (&priv->transition.tracker,
                                        gdk_frame_clock_get_frame_time (frame_clock));
    progress = gtk_progress_tracker_get_ease_out_cubic (&priv->transition.tracker, FALSE);
    priv->transition.progress =
      hdy_lerp (priv->transition.end_progress,
                priv->transition.start_progress, progress);
  } else
    priv->transition.first_frame_skipped = TRUE;

  /* Finish animation early if not mapped anymore */
  if (!gtk_widget_get_mapped (widget))
    gtk_progress_tracker_finish (&priv->transition.tracker);

  hdy_deck_child_progress_updated (self);

  if (gtk_progress_tracker_get_state (&priv->transition.tracker) == GTK_PROGRESS_STATE_AFTER) {
    priv->transition.tick_id = 0;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TRANSITION_RUNNING]);

    return FALSE;
  }

  return TRUE;
}

static void
hdy_deck_schedule_child_ticks (HdyDeck *self)
{
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);

  if (priv->transition.tick_id == 0) {
    priv->transition.tick_id =
      gtk_widget_add_tick_callback (GTK_WIDGET (self), hdy_deck_transition_cb, self, NULL);
    if (!priv->transition.is_gesture_active)
      g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TRANSITION_RUNNING]);
  }
}

static void
hdy_deck_unschedule_child_ticks (HdyDeck *self)
{
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);

  if (priv->transition.tick_id != 0) {
    gtk_widget_remove_tick_callback (GTK_WIDGET (self), priv->transition.tick_id);
    priv->transition.tick_id = 0;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TRANSITION_RUNNING]);
  }
}

static void
hdy_deck_start_transition (HdyDeck               *self,
                           HdyDeckTransitionType  transition_type,
                           guint                  transition_duration,
                           GtkPanDirection        transition_direction)
{
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);
  GtkWidget *widget = GTK_WIDGET (self);

  if (gtk_widget_get_mapped (widget) &&
      (hdy_get_enable_animations (widget) || priv->transition.is_gesture_active) &&
      transition_type != HDY_DECK_TRANSITION_TYPE_NONE &&
      transition_duration != 0 &&
      priv->last_visible_child != NULL) {
    priv->transition.active_type = transition_type;
    priv->transition.active_direction = transition_direction;
    priv->transition.first_frame_skipped = FALSE;
    priv->transition.start_progress = 0;
    priv->transition.end_progress = 1;
    priv->transition.progress = 0;
    priv->transition.is_cancelled = FALSE;

    if (!priv->transition.is_gesture_active) {
      hdy_deck_schedule_child_ticks (self);
      gtk_progress_tracker_start (&priv->transition.tracker,
                                  transition_duration * 1000,
                                  0,
                                  1.0);
    }
  }
  else {
    hdy_deck_unschedule_child_ticks (self);
    priv->transition.active_type = HDY_DECK_TRANSITION_TYPE_NONE;
    gtk_progress_tracker_finish (&priv->transition.tracker);
  }

  hdy_deck_child_progress_updated (self);
}

static void
set_visible_child_info (HdyDeck               *self,
                        HdyDeckChildInfo      *new_visible_child,
                        HdyDeckTransitionType  transition_type,
                        guint                  transition_duration,
                        gboolean               emit_switch_child)
{
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);
  GtkWidget *widget = GTK_WIDGET (self);
  GList *children;
  HdyDeckChildInfo *child_info;
  GtkPanDirection transition_direction = GTK_PAN_DIRECTION_LEFT;

  /* If we are being destroyed, do not bother with transitions and   *
   * notifications.
   */
  if (gtk_widget_in_destruction (widget))
    return;

  /* If none, pick first visible. */
  if (new_visible_child == NULL) {
    for (children = priv->children; children; children = children->next) {
      child_info = children->data;

      if (gtk_widget_get_visible (child_info->widget)) {
        new_visible_child = child_info;

        break;
      }
    }
  }

  if (new_visible_child == priv->visible_child)
    return;

  /* FIXME Probably copied from GtkStack, should check whether it's needed. */
  /* toplevel = gtk_widget_get_toplevel (widget); */
  /* if (GTK_IS_WINDOW (toplevel)) { */
  /*   focus = gtk_window_get_focus (GTK_WINDOW (toplevel)); */
  /*   if (focus && */
  /*       priv->visible_child && */
  /*       priv->visible_child->widget && */
  /*       gtk_widget_is_ancestor (focus, priv->visible_child->widget)) { */
  /*     contains_focus = TRUE; */

  /*     if (priv->visible_child->last_focus) */
  /*       g_object_remove_weak_pointer (G_OBJECT (priv->visible_child->last_focus), */
  /*                                     (gpointer *)&priv->visible_child->last_focus); */
  /*     priv->visible_child->last_focus = focus; */
  /*     g_object_add_weak_pointer (G_OBJECT (priv->visible_child->last_focus), */
  /*                                (gpointer *)&priv->visible_child->last_focus); */
  /*   } */
  /* } */

  if (priv->last_visible_child)
    gtk_widget_set_child_visible (priv->last_visible_child->widget, FALSE);
  priv->last_visible_child = NULL;

  if (priv->transition.last_visible_surface != NULL)
    cairo_surface_destroy (priv->transition.last_visible_surface);
  priv->transition.last_visible_surface = NULL;

  hdy_shadow_helper_clear_cache (priv->shadow_helper);

  if (priv->visible_child && priv->visible_child->widget) {
    if (gtk_widget_is_visible (widget)) {
      GtkAllocation allocation;

      priv->last_visible_child = priv->visible_child;
      gtk_widget_get_allocated_size (priv->last_visible_child->widget, &allocation, NULL);
      priv->transition.last_visible_widget_width = allocation.width;
      priv->transition.last_visible_widget_height = allocation.height;
    }
    else
      gtk_widget_set_child_visible (priv->visible_child->widget, FALSE);
  }

  /* FIXME This comes from GtkStack and should be adapted. */
  /* hdy_deck_accessible_update_visible_child (stack, */
  /*                                              priv->visible_child ? priv->visible_child->widget : NULL, */
  /*                                              new_visible_child ? new_visible_child->widget : NULL); */

  priv->visible_child = new_visible_child;

  if (new_visible_child) {
    gtk_widget_set_child_visible (new_visible_child->widget, TRUE);

    /* FIXME This comes from GtkStack and should be adapted. */
    /* if (contains_focus) { */
    /*   if (new_visible_child->last_focus) */
    /*     gtk_widget_grab_focus (new_visible_child->last_focus); */
    /*   else */
    /*     gtk_widget_child_focus (new_visible_child->widget, GTK_DIR_TAB_FORWARD); */
    /* } */
  }

  if ((new_visible_child == NULL || priv->last_visible_child == NULL) &&
      is_direction_dependent_transition (transition_type))
    transition_type = HDY_DECK_TRANSITION_TYPE_NONE;
  else if (is_direction_dependent_transition (transition_type)) {
    gboolean new_first = FALSE;
    for (children = priv->children; children; children = children->next) {
      if (new_visible_child == children->data) {
        new_first = TRUE;

        break;
      }
      if (priv->last_visible_child == children->data)
        break;
    }

    transition_direction = get_pan_direction (self, new_first);
  }

  if (priv->homogeneous[GTK_ORIENTATION_HORIZONTAL] &&
      priv->homogeneous[GTK_ORIENTATION_VERTICAL])
    gtk_widget_queue_allocate (widget);
  else
    gtk_widget_queue_resize (widget);

  hdy_deck_start_transition (self, transition_type, transition_duration, transition_direction);

  if (emit_switch_child) {
    gint n;

    children = gtk_container_get_children (GTK_CONTAINER (self));
    n = g_list_index (children, new_visible_child->widget);
    g_list_free (children);

    hdy_swipeable_emit_switch_child (HDY_SWIPEABLE (self), n, transition_duration);
  }

  g_object_freeze_notify (G_OBJECT (self));
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_VISIBLE_CHILD]);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_VISIBLE_CHILD_NAME]);
  g_object_thaw_notify (G_OBJECT (self));
}

static void
get_padding (GtkWidget *widget,
             GtkBorder *padding)
{
  GtkStyleContext *context;
  GtkStateFlags state;

  context = gtk_widget_get_style_context (widget);
  state = gtk_style_context_get_state (context);

  gtk_style_context_get_padding (context, state, padding);
}

/**
 * hdy_deck_set_homogeneous:
 * @self: a #HdyDeck
 * @orientation: the orientation
 * @homogeneous: %TRUE to make @self homogeneous
 *
 * Sets the #HdyDeck to be homogeneous or not for the given orientation.
 * If it is homogeneous, the #HdyDeck will request the same
 * width or height for all its children depending on the orientation.
 * If it isn't, the stack may change width or height when a different child
 * becomes visible.
 */
void
hdy_deck_set_homogeneous (HdyDeck        *self,
                          GtkOrientation  orientation,
                          gboolean        homogeneous)
{
  HdyDeckPrivate *priv;

  g_return_if_fail (HDY_IS_DECK (self));

  priv = hdy_deck_get_instance_private (self);

  homogeneous = !!homogeneous;

  if (priv->homogeneous[orientation] == homogeneous)
    return;

  priv->homogeneous[orientation] = homogeneous;

  if (gtk_widget_get_visible (GTK_WIDGET (self)))
    gtk_widget_queue_resize (GTK_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[HOMOGENEOUS_PROP[orientation]]);
}

/**
 * hdy_deck_get_homogeneous:
 * @self: a #HdyDeck
 * @orientation: the orientation
 *
 * Gets whether @self is homogeneous for the given orientation.
 * See hdy_deck_set_homogeneous().
 *
 * Returns: whether @self is homogeneous for the given orientation.
 */
gboolean
hdy_deck_get_homogeneous (HdyDeck        *self,
                          GtkOrientation  orientation)
{
  HdyDeckPrivate *priv;

  g_return_val_if_fail (HDY_IS_DECK (self), FALSE);

  priv = hdy_deck_get_instance_private (self);

  return priv->homogeneous[orientation];
}

/**
 * hdy_deck_get_transition_type:
 * @self: a #HdyDeck
 *
 * Gets the type of animation that will be used
 * for transitions between modes and children in @self.
 *
 * Returns: the current transition type of @self
 *
 * Since: 0.0.12
 */
HdyDeckTransitionType
hdy_deck_get_transition_type (HdyDeck *self)
{
  HdyDeckPrivate *priv;

  g_return_val_if_fail (HDY_IS_DECK (self), HDY_DECK_TRANSITION_TYPE_NONE);

  priv = hdy_deck_get_instance_private (self);

  return priv->transition_type;
}

/**
 * hdy_deck_set_transition_type:
 * @self: a #HdyDeck
 * @transition: the new transition type
 *
 * Sets the type of animation that will be used for transitions between modes
 * and children in @self.
 *
 * The transition type can be changed without problems at runtime, so it is
 * possible to change the animation based on the mode or child that is about to
 * become current.
 *
 * Since: 0.0.12
 */
void
hdy_deck_set_transition_type (HdyDeck               *self,
                              HdyDeckTransitionType  transition)
{
  HdyDeckPrivate *priv;

  g_return_if_fail (HDY_IS_DECK (self));

  priv = hdy_deck_get_instance_private (self);

  if (priv->transition_type == transition)
    return;

  priv->transition_type = transition;
  g_object_notify_by_pspec (G_OBJECT (self),
                            props[PROP_TRANSITION_TYPE]);
}

/**
 * hdy_deck_get_transition_duration:
 * @self: a #HdyDeck
 *
 * Returns the amount of time (in milliseconds) that
 * transitions between children in @self will take.
 *
 * Returns: the mode transition duration
 */
guint
hdy_deck_get_transition_duration (HdyDeck *self)
{
  HdyDeckPrivate *priv;

  g_return_val_if_fail (HDY_IS_DECK (self), 0);

  priv = hdy_deck_get_instance_private (self);

  return priv->transition.duration;
}

/**
 * hdy_deck_set_transition_duration:
 * @self: a #HdyDeck
 * @duration: the new duration, in milliseconds
 *
 * Sets the duration that transitions between children in @self
 * will take.
 */
void
hdy_deck_set_transition_duration (HdyDeck *self,
                                  guint    duration)
{
  HdyDeckPrivate *priv;

  g_return_if_fail (HDY_IS_DECK (self));

  priv = hdy_deck_get_instance_private (self);

  if (priv->transition.duration == duration)
    return;

  priv->transition.duration = duration;
  g_object_notify_by_pspec (G_OBJECT (self),
                            props[PROP_TRANSITION_DURATION]);
}

/**
 * hdy_deck_get_visible_child:
 * @self: a #HdyDeck
 *
 * Get the visible child widget.
 *
 * Returns: (transfer none): the visible child widget
 */
GtkWidget *
hdy_deck_get_visible_child (HdyDeck *self)
{
  HdyDeckPrivate *priv;

  g_return_val_if_fail (HDY_IS_DECK (self), NULL);

  priv = hdy_deck_get_instance_private (self);

  if (priv->visible_child == NULL)
    return NULL;

  return priv->visible_child->widget;
}

void
hdy_deck_set_visible_child (HdyDeck   *self,
                            GtkWidget *visible_child)
{
  HdyDeckPrivate *priv;
  HdyDeckChildInfo *child_info;
  gboolean contains_child;

  g_return_if_fail (HDY_IS_DECK (self));
  g_return_if_fail (GTK_IS_WIDGET (visible_child));

  priv = hdy_deck_get_instance_private (self);

  child_info = find_child_info_for_widget (self, visible_child);
  contains_child = child_info != NULL;

  g_return_if_fail (contains_child);

  set_visible_child_info (self, child_info, priv->transition_type,
                          priv->transition.duration, TRUE);
}

const gchar *
hdy_deck_get_visible_child_name (HdyDeck *self)
{
  HdyDeckPrivate *priv;

  g_return_val_if_fail (HDY_IS_DECK (self), NULL);

  priv = hdy_deck_get_instance_private (self);

  if (priv->visible_child == NULL)
    return NULL;

  return priv->visible_child->name;
}

void
hdy_deck_set_visible_child_name (HdyDeck     *self,
                                 const gchar *name)
{
  HdyDeckPrivate *priv;
  HdyDeckChildInfo *child_info;
  gboolean contains_child;

  g_return_if_fail (HDY_IS_DECK (self));
  g_return_if_fail (name != NULL);

  priv = hdy_deck_get_instance_private (self);

  child_info = find_child_info_for_name (self, name);
  contains_child = child_info != NULL;

  g_return_if_fail (contains_child);

  set_visible_child_info (self, child_info, priv->transition_type,
                          priv->transition.duration, TRUE);
}

/**
 * hdy_deck_get_transition_running:
 * @self: a #HdyDeck
 *
 * Returns whether @self is currently in a transition from one page to
 * another.
 *
 * Returns: %TRUE if the transition is currently running, %FALSE otherwise.
 */
gboolean
hdy_deck_get_transition_running (HdyDeck *self)
{
  HdyDeckPrivate *priv;

  g_return_val_if_fail (HDY_IS_DECK (self), FALSE);

  priv = hdy_deck_get_instance_private (self);

  return (priv->transition.tick_id != 0 ||
          priv->transition.is_gesture_active);
}

/**
 * hdy_deck_set_interpolate_size:
 * @self: a #HdyDeck
 * @interpolate_size: the new value
 *
 * Sets whether or not @self will interpolate its size when
 * changing the visible child. If the #HdyDeck:interpolate-size
 * property is set to %TRUE, @stack will interpolate its size between
 * the current one and the one it'll take after changing the
 * visible child, according to the set transition duration.
 */
void
hdy_deck_set_interpolate_size (HdyDeck  *self,
                               gboolean  interpolate_size)
{
  HdyDeckPrivate *priv;

  g_return_if_fail (HDY_IS_DECK (self));

  priv = hdy_deck_get_instance_private (self);

  interpolate_size = !!interpolate_size;

  if (priv->transition.interpolate_size == interpolate_size)
    return;

  priv->transition.interpolate_size = interpolate_size;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_INTERPOLATE_SIZE]);
}

/**
 * hdy_deck_get_interpolate_size:
 * @self: a #HdyDeck
 *
 * Returns wether the #HdyDeck is set up to interpolate between
 * the sizes of children on page switch.
 *
 * Returns: %TRUE if child sizes are interpolated
 */
gboolean
hdy_deck_get_interpolate_size (HdyDeck *self)
{
  HdyDeckPrivate *priv;

  g_return_val_if_fail (HDY_IS_DECK (self), FALSE);

  priv = hdy_deck_get_instance_private (self);

  return priv->transition.interpolate_size;
}

/**
 * hdy_deck_set_can_swipe_back:
 * @self: a #HdyDeck
 * @can_swipe_back: the new value
 *
 * Sets whether or not @self allows switching to the previous child that has
 * 'allow-visible' child property set to %TRUE via a swipe gesture
 *
 * Since: 0.0.12
 */
void
hdy_deck_set_can_swipe_back (HdyDeck  *self,
                             gboolean  can_swipe_back)
{
  HdyDeckPrivate *priv;

  g_return_if_fail (HDY_IS_DECK (self));

  priv = hdy_deck_get_instance_private (self);

  can_swipe_back = !!can_swipe_back;

  if (priv->transition.can_swipe_back == can_swipe_back)
    return;

  priv->transition.can_swipe_back = can_swipe_back;
  hdy_swipe_tracker_set_enabled (priv->tracker, can_swipe_back || priv->transition.can_swipe_forward);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CAN_SWIPE_BACK]);
}

/**
 * hdy_deck_get_can_swipe_back
 * @self: a #HdyDeck
 *
 * Returns whether the #HdyDeck allows swiping to the previous child.
 *
 * Returns: %TRUE if back swipe is enabled.
 *
 * Since: 0.0.12
 */
gboolean
hdy_deck_get_can_swipe_back (HdyDeck *self)
{
  HdyDeckPrivate *priv;

  g_return_val_if_fail (HDY_IS_DECK (self), FALSE);

  priv = hdy_deck_get_instance_private (self);

  return priv->transition.can_swipe_back;
}

/**
 * hdy_deck_set_can_swipe_forward:
 * @self: a #HdyDeck
 * @can_swipe_forward: the new value
 *
 * Sets whether or not @self allows switching to the next child that has
 * 'allow-visible' child property set to %TRUE via a swipe gesture.
 *
 * Since: 0.0.12
 */
void
hdy_deck_set_can_swipe_forward (HdyDeck  *self,
                                gboolean  can_swipe_forward)
{
  HdyDeckPrivate *priv;

  g_return_if_fail (HDY_IS_DECK (self));

  priv = hdy_deck_get_instance_private (self);

  can_swipe_forward = !!can_swipe_forward;

  if (priv->transition.can_swipe_forward == can_swipe_forward)
    return;

  priv->transition.can_swipe_forward = can_swipe_forward;
  hdy_swipe_tracker_set_enabled (priv->tracker, priv->transition.can_swipe_back || can_swipe_forward);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CAN_SWIPE_FORWARD]);
}

/**
 * hdy_deck_get_can_swipe_forward
 * @self: a #HdyDeck
 *
 * Returns whether the #HdyDeck allows swiping to the next child.
 *
 * Returns: %TRUE if back swipe is enabled.
 *
 * Since: 0.0.12
 */
gboolean
hdy_deck_get_can_swipe_forward (HdyDeck *self)
{
  HdyDeckPrivate *priv;

  g_return_val_if_fail (HDY_IS_DECK (self), FALSE);

  priv = hdy_deck_get_instance_private (self);

  return priv->transition.can_swipe_forward;
}

static void
get_preferred_size (gint     *min,
                    gint     *nat,
                    gboolean  same_orientation,
                    gboolean  homogeneous,
                    gint      visible_children,
                    gdouble   visible_child_progress,
                    gint      sum_nat,
                    gint      max_min,
                    gint      max_nat,
                    gint      visible_min,
                    gint      last_visible_min)
{
  if (same_orientation) {
    *min = homogeneous ?
             max_min :
             hdy_lerp (visible_min, last_visible_min, visible_child_progress);
    *nat = max_nat;
  }
  else {
    *min = homogeneous ?
             max_min :
             hdy_lerp (visible_min, last_visible_min, visible_child_progress);
    *nat = max_nat;
  }
}

/* This private method is prefixed by the call name because it will be a virtual
 * method in GTK 4.
 */
static void
hdy_deck_measure (GtkWidget      *widget,
                  GtkOrientation  orientation,
                  int             for_size,
                  int            *minimum,
                  int            *natural,
                  int            *minimum_baseline,
                  int            *natural_baseline)
{
  HdyDeck *self = HDY_DECK (widget);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);
  GList *children;
  HdyDeckChildInfo *child_info;
  gint visible_children;
  gdouble visible_child_progress;
  gint child_min, max_min, visible_min, last_visible_min;
  gint child_nat, max_nat, sum_nat;
  void (*get_preferred_size_static) (GtkWidget *widget,
                                     gint      *minimum_width,
                                     gint      *natural_width);
  void (*get_preferred_size_for_size) (GtkWidget *widget,
                                       gint       height,
                                       gint      *minimum_width,
                                       gint      *natural_width);

  get_preferred_size_static = orientation == GTK_ORIENTATION_HORIZONTAL ?
    gtk_widget_get_preferred_width :
    gtk_widget_get_preferred_height;
  get_preferred_size_for_size = orientation == GTK_ORIENTATION_HORIZONTAL ?
    gtk_widget_get_preferred_width_for_height :
    gtk_widget_get_preferred_height_for_width;

  visible_children = 0;
  child_min = max_min = visible_min = last_visible_min = 0;
  child_nat = max_nat = sum_nat = 0;
  for (children = priv->children; children; children = children->next) {
    child_info = children->data;

    if (child_info->widget == NULL || !gtk_widget_get_visible (child_info->widget))
      continue;

    visible_children++;
    if (for_size < 0)
      get_preferred_size_static (child_info->widget,
                                 &child_min, &child_nat);
    else
      get_preferred_size_for_size (child_info->widget, for_size,
                                   &child_min, &child_nat);

    max_min = MAX (max_min, child_min);
    max_nat = MAX (max_nat, child_nat);
    sum_nat += child_nat;
  }

  if (priv->visible_child != NULL) {
    if (for_size < 0)
      get_preferred_size_static (priv->visible_child->widget,
                                 &visible_min, NULL);
    else
      get_preferred_size_for_size (priv->visible_child->widget, for_size,
                                   &visible_min, NULL);
  }

  if (priv->last_visible_child != NULL) {
    if (for_size < 0)
      get_preferred_size_static (priv->last_visible_child->widget,
                                 &last_visible_min, NULL);
    else
      get_preferred_size_for_size (priv->last_visible_child->widget, for_size,
                                   &last_visible_min, NULL);
  }

  visible_child_progress = priv->transition.interpolate_size ? priv->transition.progress : 1.0;

  get_preferred_size (minimum, natural,
                      gtk_orientable_get_orientation (GTK_ORIENTABLE (widget)) == orientation,
                      priv->homogeneous[orientation],
                      visible_children, visible_child_progress,
                      sum_nat, max_min, max_nat, visible_min, last_visible_min);
}

static void
hdy_deck_get_preferred_width (GtkWidget *widget,
                              gint      *minimum_width,
                              gint      *natural_width)
{
  hdy_deck_measure (widget, GTK_ORIENTATION_HORIZONTAL, -1,
                    minimum_width, natural_width, NULL, NULL);
}

static void
hdy_deck_get_preferred_height (GtkWidget *widget,
                               gint      *minimum_height,
                               gint      *natural_height)
{
  hdy_deck_measure (widget, GTK_ORIENTATION_VERTICAL, -1,
                    minimum_height, natural_height, NULL, NULL);
}

static void
hdy_deck_get_preferred_width_for_height (GtkWidget *widget,
                                         gint       height,
                                         gint      *minimum_width,
                                         gint      *natural_width)
{
  hdy_deck_measure (widget, GTK_ORIENTATION_HORIZONTAL, height,
                    minimum_width, natural_width, NULL, NULL);
}

static void
hdy_deck_get_preferred_height_for_width (GtkWidget *widget,
                                         gint       width,
                                         gint      *minimum_height,
                                         gint      *natural_height)
{
  hdy_deck_measure (widget, GTK_ORIENTATION_VERTICAL, width,
                    minimum_height, natural_height, NULL, NULL);
}

static void
hdy_deck_size_allocate_children (GtkWidget     *widget,
                                 GtkAllocation *allocation)
{
  HdyDeck *self = HDY_DECK (widget);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);
  GList *directed_children, *children;
  HdyDeckChildInfo *child_info, *visible_child;

  directed_children = get_directed_children (self);
  visible_child = priv->visible_child;

  for (children = directed_children; children; children = children->next) {
    child_info = children->data;

    if (!child_info->widget)
      continue;

    if (child_info->widget == visible_child->widget)
      continue;

    if (priv->last_visible_child &&
        child_info->widget == priv->last_visible_child->widget)
      continue;

    gtk_widget_set_child_visible (child_info->widget, FALSE);
  }

  if (visible_child->widget == NULL)
    return;

  /* FIXME is this needed? */
  if (!gtk_widget_get_visible (visible_child->widget)) {
    gtk_widget_set_child_visible (visible_child->widget, FALSE);

    return;
  }

  gtk_widget_set_child_visible (visible_child->widget, TRUE);

  for (children = directed_children; children; children = children->next) {
    child_info = children->data;

    if (child_info != visible_child &&
        child_info != priv->last_visible_child) {
      child_info->visible = FALSE;

      continue;
    }

    child_info->alloc.x = 0;
    child_info->alloc.y = 0;
    child_info->alloc.width = allocation->width;
    child_info->alloc.height = allocation->height;
    child_info->visible = TRUE;
  }
}

static void
hdy_deck_size_allocate (GtkWidget     *widget,
                        GtkAllocation *allocation)
{
  HdyDeck *self = HDY_DECK (widget);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);
  GList *directed_children, *children;
  HdyDeckChildInfo *child_info;

  directed_children = get_directed_children (self);

  gtk_widget_set_allocation (widget, allocation);

  if (gtk_widget_get_realized (widget)) {
    gdk_window_move_resize (priv->view_window,
                            allocation->x, allocation->y,
                            allocation->width, allocation->height);
    move_resize_bin_window (self, allocation, TRUE);
  }

  /* Prepare children information. */
  for (children = directed_children; children; children = children->next) {
    child_info = children->data;

    gtk_widget_get_preferred_size (child_info->widget, &child_info->min, &child_info->nat);
    child_info->alloc.x = child_info->alloc.y = child_info->alloc.width = child_info->alloc.height = 0;
    child_info->visible = FALSE;
  }

  /* Allocate size to the children. */
  hdy_deck_size_allocate_children (widget, allocation);

  /* Apply visibility and allocation. */
  for (children = directed_children; children; children = children->next) {
    child_info = children->data;

    gtk_widget_set_child_visible (child_info->widget, child_info->visible);
    if (!child_info->visible)
      continue;

    gtk_widget_size_allocate (child_info->widget, &child_info->alloc);
    if (gtk_widget_get_realized (widget))
      gtk_widget_show (child_info->widget);
  }
}

static void
hdy_deck_draw_under (GtkWidget *widget,
                     cairo_t   *cr)
{
  HdyDeck *self = HDY_DECK (widget);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);
  GtkAllocation allocation;
  int x, y;

  gtk_widget_get_allocation (widget, &allocation);

  x = get_bin_window_x (self, &allocation);
  y = get_bin_window_y (self, &allocation);

  if (gtk_cairo_should_draw_window (cr, priv->bin_window)) {
    gint clip_x, clip_y, clip_w, clip_h;
    gdouble progress;

    clip_x = 0;
    clip_y = 0;
    clip_w = allocation.width;
    clip_h = allocation.height;

    switch (priv->transition.active_direction) {
    case GTK_PAN_DIRECTION_LEFT:
      clip_x = x;
      clip_w -= x;
      break;
    case GTK_PAN_DIRECTION_RIGHT:
      clip_w += x;
      break;
    case GTK_PAN_DIRECTION_UP:
      clip_y = y;
      clip_h -= y;
      break;
    case GTK_PAN_DIRECTION_DOWN:
      clip_h += y;
      break;
    default:
      g_assert_not_reached ();
      break;
    }

    progress = priv->transition.progress;

    cairo_save (cr);
    cairo_rectangle (cr, clip_x, clip_y, clip_w, clip_h);
    cairo_clip (cr);
    gtk_container_propagate_draw (GTK_CONTAINER (self),
                                  priv->visible_child->widget,
                                  cr);
    cairo_translate (cr, x, y);
    hdy_shadow_helper_draw_shadow (priv->shadow_helper, cr, allocation.width,
                                   allocation.height, progress,
                                   priv->transition.active_direction);
    cairo_restore (cr);
  }

  if (priv->transition.last_visible_surface &&
      gtk_cairo_should_draw_window (cr, priv->view_window)) {
    switch (priv->transition.active_direction) {
    case GTK_PAN_DIRECTION_LEFT:
      x -= allocation.width;
      break;
    case GTK_PAN_DIRECTION_RIGHT:
      x += allocation.width;
      break;
    case GTK_PAN_DIRECTION_UP:
      y -= allocation.height;
      break;
    case GTK_PAN_DIRECTION_DOWN:
      y += allocation.height;
      break;
    default:
      g_assert_not_reached ();
      break;
    }

    x += priv->transition.last_visible_surface_allocation.x;
    y += priv->transition.last_visible_surface_allocation.y;

    if (gtk_widget_get_valign (priv->last_visible_child->widget) == GTK_ALIGN_END &&
        priv->transition.last_visible_widget_height > allocation.height)
      y -= priv->transition.last_visible_widget_height - allocation.height;
    else if (gtk_widget_get_valign (priv->last_visible_child->widget) == GTK_ALIGN_CENTER)
      y -= (priv->transition.last_visible_widget_height - allocation.height) / 2;

    cairo_save (cr);
    cairo_set_source_surface (cr, priv->transition.last_visible_surface, x, y);
    cairo_paint (cr);
    cairo_restore (cr);
  }
}

static void
hdy_deck_draw_over (GtkWidget *widget,
                    cairo_t   *cr)
{
  HdyDeck *self = HDY_DECK (widget);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);

  if (priv->transition.last_visible_surface &&
      gtk_cairo_should_draw_window (cr, priv->view_window)) {
    GtkAllocation allocation;
    gint x, y, clip_x, clip_y, clip_w, clip_h, shadow_x, shadow_y;
    gdouble progress;
    GtkPanDirection direction;

    gtk_widget_get_allocation (widget, &allocation);

    x = get_bin_window_x (self, &allocation);
    y = get_bin_window_y (self, &allocation);

    clip_x = 0;
    clip_y = 0;
    clip_w = allocation.width;
    clip_h = allocation.height;
    shadow_x = 0;
    shadow_y = 0;

    switch (priv->transition.active_direction) {
    case GTK_PAN_DIRECTION_LEFT:
      shadow_x = x - allocation.width;
      clip_w = x;
      x = 0;
      direction = GTK_PAN_DIRECTION_RIGHT;
      break;
    case GTK_PAN_DIRECTION_RIGHT:
      clip_x = shadow_x = x + allocation.width;
      clip_w = -x;
      x = 0;
      direction = GTK_PAN_DIRECTION_LEFT;
      break;
    case GTK_PAN_DIRECTION_UP:
      shadow_y = y - allocation.height;
      clip_h = y;
      y = 0;
      direction = GTK_PAN_DIRECTION_DOWN;
      break;
    case GTK_PAN_DIRECTION_DOWN:
      clip_y = shadow_y = y + allocation.height;
      clip_h = -y;
      y = 0;
      direction = GTK_PAN_DIRECTION_UP;
      break;
    default:
      g_assert_not_reached ();
      break;
    }

    x += priv->transition.last_visible_surface_allocation.x;
    y += priv->transition.last_visible_surface_allocation.y;

    if (gtk_widget_get_valign (priv->last_visible_child->widget) == GTK_ALIGN_END &&
        priv->transition.last_visible_widget_height > allocation.height)
      y -= priv->transition.last_visible_widget_height - allocation.height;
    else if (gtk_widget_get_valign (priv->last_visible_child->widget) == GTK_ALIGN_CENTER)
      y -= (priv->transition.last_visible_widget_height - allocation.height) / 2;

    progress = 1 - priv->transition.progress;

    cairo_save (cr);
    cairo_rectangle (cr, clip_x, clip_y, clip_w, clip_h);
    cairo_clip (cr);
    cairo_set_source_surface (cr, priv->transition.last_visible_surface, x, y);
    cairo_paint (cr);
    cairo_translate (cr, shadow_x, shadow_y);
    hdy_shadow_helper_draw_shadow (priv->shadow_helper, cr, allocation.width,
                                   allocation.height, progress, direction);
    cairo_restore (cr);
  }

  if (gtk_cairo_should_draw_window (cr, priv->bin_window))
    gtk_container_propagate_draw (GTK_CONTAINER (self),
                                  priv->visible_child->widget,
                                  cr);
}

static void
hdy_deck_draw_slide (GtkWidget *widget,
                     cairo_t   *cr)
{
  HdyDeck *self = HDY_DECK (widget);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);

  if (priv->transition.last_visible_surface &&
      gtk_cairo_should_draw_window (cr, priv->view_window)) {
    GtkAllocation allocation;
    int x, y;

    gtk_widget_get_allocation (widget, &allocation);

    x = get_bin_window_x (self, &allocation);
    y = get_bin_window_y (self, &allocation);

    switch (priv->transition.active_direction) {
    case GTK_PAN_DIRECTION_LEFT:
      x -= allocation.width;
      break;
    case GTK_PAN_DIRECTION_RIGHT:
      x += allocation.width;
      break;
    case GTK_PAN_DIRECTION_UP:
      y -= allocation.height;
      break;
    case GTK_PAN_DIRECTION_DOWN:
      y += allocation.height;
      break;
    default:
      g_assert_not_reached ();
      break;
    }

    x += priv->transition.last_visible_surface_allocation.x;
    y += priv->transition.last_visible_surface_allocation.y;

    if (gtk_widget_get_valign (priv->last_visible_child->widget) == GTK_ALIGN_END &&
        priv->transition.last_visible_widget_height > allocation.height)
      y -= priv->transition.last_visible_widget_height - allocation.height;
    else if (gtk_widget_get_valign (priv->last_visible_child->widget) == GTK_ALIGN_CENTER)
      y -= (priv->transition.last_visible_widget_height - allocation.height) / 2;

    cairo_save (cr);
    cairo_set_source_surface (cr, priv->transition.last_visible_surface, x, y);
    cairo_paint (cr);
    cairo_restore (cr);
  }

  if (gtk_cairo_should_draw_window (cr, priv->bin_window))
    gtk_container_propagate_draw (GTK_CONTAINER (self),
                                  priv->visible_child->widget,
                                  cr);
}

static void
hdy_deck_draw_over_or_under (GtkWidget *widget,
                             cairo_t   *cr)
{
  HdyDeck *self = HDY_DECK (widget);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);
  gboolean is_rtl;
  GtkPanDirection direction, left_or_right, right_or_left;

  direction = priv->transition.active_direction;

  is_rtl = gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL;
  left_or_right = is_rtl ? GTK_PAN_DIRECTION_RIGHT : GTK_PAN_DIRECTION_LEFT;
  right_or_left = is_rtl ? GTK_PAN_DIRECTION_LEFT : GTK_PAN_DIRECTION_RIGHT;

  switch (priv->transition.active_type) {
  case HDY_DECK_TRANSITION_TYPE_OVER:
    if (direction == GTK_PAN_DIRECTION_UP || direction == left_or_right)
      hdy_deck_draw_over (widget, cr);
    else if (direction == GTK_PAN_DIRECTION_DOWN || direction == right_or_left)
      hdy_deck_draw_under (widget, cr);
    else
      g_assert_not_reached ();
    break;
  case HDY_DECK_TRANSITION_TYPE_UNDER:
    if (direction == GTK_PAN_DIRECTION_UP || direction == left_or_right)
      hdy_deck_draw_under (widget, cr);
    else if (direction == GTK_PAN_DIRECTION_DOWN || direction == right_or_left)
      hdy_deck_draw_over (widget, cr);
    else
      g_assert_not_reached ();
    break;
  case HDY_DECK_TRANSITION_TYPE_NONE:
  case HDY_DECK_TRANSITION_TYPE_SLIDE:
  default:
    g_assert_not_reached ();
  }
}

static gboolean
hdy_deck_draw (GtkWidget *widget,
               cairo_t   *cr)
{
  HdyDeck *self = HDY_DECK (widget);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);
  cairo_t *pattern_cr;

  if (gtk_cairo_should_draw_window (cr, priv->view_window)) {
    GtkStyleContext *context;

    context = gtk_widget_get_style_context (widget);
    gtk_render_background (context,
                           cr,
                           0, 0,
                           gtk_widget_get_allocated_width (widget),
                           gtk_widget_get_allocated_height (widget));
  }

  if (priv->visible_child) {
    if ((priv->transition.is_gesture_active &&
        priv->transition_type != HDY_DECK_TRANSITION_TYPE_NONE) ||
        gtk_progress_tracker_get_state (&priv->transition.tracker) != GTK_PROGRESS_STATE_AFTER) {
      if (priv->transition.last_visible_surface == NULL &&
          priv->last_visible_child != NULL) {
        gtk_widget_get_allocation (priv->last_visible_child->widget,
                                   &priv->transition.last_visible_surface_allocation);
        priv->transition.last_visible_surface =
          gdk_window_create_similar_surface (gtk_widget_get_window (widget),
                                             CAIRO_CONTENT_COLOR_ALPHA,
                                             priv->transition.last_visible_surface_allocation.width,
                                             priv->transition.last_visible_surface_allocation.height);
        pattern_cr = cairo_create (priv->transition.last_visible_surface);
        /* We don't use propagate_draw here, because we don't want to apply
         * the bin_window offset
         */
        gtk_widget_draw (priv->last_visible_child->widget, pattern_cr);
        cairo_destroy (pattern_cr);
      }

      cairo_rectangle (cr,
                       0, 0,
                       gtk_widget_get_allocated_width (widget),
                       gtk_widget_get_allocated_height (widget));
      cairo_clip (cr);

      switch (priv->transition.active_type) {
      case HDY_DECK_TRANSITION_TYPE_SLIDE:
        hdy_deck_draw_slide (widget, cr);
        break;
      case HDY_DECK_TRANSITION_TYPE_OVER:
      case HDY_DECK_TRANSITION_TYPE_UNDER:
        hdy_deck_draw_over_or_under (widget, cr);
        break;
      case HDY_DECK_TRANSITION_TYPE_NONE:
      default:
        g_assert_not_reached ();
      }
    }
    else if (gtk_cairo_should_draw_window (cr, priv->bin_window))
      gtk_container_propagate_draw (GTK_CONTAINER (self),
                                    priv->visible_child->widget,
                                    cr);
  }

  return FALSE;
}

static void
update_tracker_orientation (HdyDeck *self)
{
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);
  gboolean reverse;

  reverse = (priv->orientation == GTK_ORIENTATION_HORIZONTAL &&
             gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL);

  g_object_set (priv->tracker,
                "orientation", priv->orientation,
                "reversed", reverse,
                NULL);
}

static void
hdy_deck_direction_changed (GtkWidget        *widget,
                            GtkTextDirection  previous_direction)
{
  HdyDeck *self = HDY_DECK (widget);

  update_tracker_orientation (self);
}

static void
hdy_deck_child_visibility_notify_cb (GObject    *obj,
                                     GParamSpec *pspec,
                                     gpointer    user_data)
{
  HdyDeck *self = HDY_DECK (user_data);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);
  GtkWidget *widget = GTK_WIDGET (obj);
  HdyDeckChildInfo *child_info;

  child_info = find_child_info_for_widget (self, widget);

  if (priv->visible_child == NULL && gtk_widget_get_visible (widget))
    set_visible_child_info (self, child_info, priv->transition_type,
                            priv->transition.duration, TRUE);
  else if (priv->visible_child == child_info && !gtk_widget_get_visible (widget))
    set_visible_child_info (self, NULL, priv->transition_type,
                            priv->transition.duration, TRUE);
}

static void
hdy_deck_add (GtkContainer *container,
              GtkWidget    *widget)
{
  HdyDeck *self;
  HdyDeckPrivate *priv;
  HdyDeckChildInfo *child_info;

  g_return_if_fail (gtk_widget_get_parent (widget) == NULL);

  self = HDY_DECK (container);
  priv = hdy_deck_get_instance_private (self);

  gtk_widget_set_child_visible (widget, FALSE);
  gtk_widget_set_parent_window (widget, priv->bin_window);
  gtk_widget_set_parent (widget, GTK_WIDGET (self));

  child_info = g_new0 (HdyDeckChildInfo, 1);
  child_info->widget = widget;

  priv->children = g_list_append (priv->children, child_info);
  priv->children_reversed = g_list_prepend (priv->children_reversed, child_info);

  if (priv->bin_window)
    gdk_window_set_events (priv->bin_window,
                           gdk_window_get_events (priv->bin_window) |
                           gtk_widget_get_events (widget));

  g_signal_connect (widget, "notify::visible",
                    G_CALLBACK (hdy_deck_child_visibility_notify_cb), self);

  if (hdy_deck_get_visible_child (self) == NULL &&
      gtk_widget_get_visible (widget)) {
    set_visible_child_info (self, child_info, priv->transition_type,
                            priv->transition.duration, FALSE);
  }

  if ((priv->homogeneous[GTK_ORIENTATION_HORIZONTAL] ||
       priv->homogeneous[GTK_ORIENTATION_VERTICAL] ||
       priv->visible_child == child_info))
    gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
hdy_deck_remove (GtkContainer *container,
                 GtkWidget    *widget)
{
  HdyDeck *self = HDY_DECK (container);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);
  HdyDeckChildInfo *child_info;
  gboolean contains_child;

  child_info = find_child_info_for_widget (self, widget);
  contains_child = child_info != NULL;

  g_return_if_fail (contains_child);

  priv->children = g_list_remove (priv->children, child_info);
  priv->children_reversed = g_list_remove (priv->children_reversed, child_info);
  free_child_info (child_info);

  if (hdy_deck_get_visible_child (self) == widget)
    set_visible_child_info (self, NULL, priv->transition_type,
                            priv->transition.duration, TRUE);

  if (gtk_widget_get_visible (widget))
    gtk_widget_queue_resize (GTK_WIDGET (container));

  gtk_widget_unparent (widget);
}

static void
hdy_deck_forall (GtkContainer *container,
                 gboolean      include_internals,
                 GtkCallback   callback,
                 gpointer      callback_data)
{
  HdyDeck *self = HDY_DECK (container);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);
  /* This shallow copy is needed when the callback changes the list while we are
   * looping through it, for example by calling hdy_deck_remove() on all
   * children when destroying the HdyDeck_private_offset.
   */
  g_autoptr (GList) children_copy = g_list_copy (priv->children);
  GList *children;
  HdyDeckChildInfo *child_info;

  for (children = children_copy; children; children = children->next) {
    child_info = children->data;

    (* callback) (child_info->widget, callback_data);
  }

  g_list_free (priv->children_reversed);
  priv->children_reversed = g_list_copy (priv->children);
  priv->children_reversed = g_list_reverse (priv->children_reversed);
}

static void
hdy_deck_get_property (GObject    *object,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  HdyDeck *self = HDY_DECK (object);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);

  switch (prop_id) {
  case PROP_HHOMOGENEOUS:
    g_value_set_boolean (value, hdy_deck_get_homogeneous (self, GTK_ORIENTATION_HORIZONTAL));
    break;
  case PROP_VHOMOGENEOUS:
    g_value_set_boolean (value, hdy_deck_get_homogeneous (self, GTK_ORIENTATION_VERTICAL));
    break;
  case PROP_VISIBLE_CHILD:
    g_value_set_object (value, hdy_deck_get_visible_child (self));
    break;
  case PROP_VISIBLE_CHILD_NAME:
    g_value_set_string (value, hdy_deck_get_visible_child_name (self));
    break;
  case PROP_TRANSITION_TYPE:
    g_value_set_enum (value, hdy_deck_get_transition_type (self));
    break;
  case PROP_TRANSITION_DURATION:
    g_value_set_uint (value, hdy_deck_get_transition_duration (self));
    break;
  case PROP_TRANSITION_RUNNING:
    g_value_set_boolean (value, hdy_deck_get_transition_running (self));
    break;
  case PROP_INTERPOLATE_SIZE:
    g_value_set_boolean (value, hdy_deck_get_interpolate_size (self));
    break;
  case PROP_CAN_SWIPE_BACK:
    g_value_set_boolean (value, hdy_deck_get_can_swipe_back (self));
    break;
  case PROP_CAN_SWIPE_FORWARD:
    g_value_set_boolean (value, hdy_deck_get_can_swipe_forward (self));
    break;
  case PROP_ORIENTATION:
    g_value_set_enum (value, priv->orientation);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
hdy_deck_set_property (GObject      *object,
                       guint         prop_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  HdyDeck *self = HDY_DECK (object);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);

  switch (prop_id) {
  case PROP_HHOMOGENEOUS:
    hdy_deck_set_homogeneous (self, GTK_ORIENTATION_HORIZONTAL, g_value_get_boolean (value));
    break;
  case PROP_VHOMOGENEOUS:
    hdy_deck_set_homogeneous (self, GTK_ORIENTATION_VERTICAL, g_value_get_boolean (value));
    break;
  case PROP_VISIBLE_CHILD:
    hdy_deck_set_visible_child (self, g_value_get_object (value));
    break;
  case PROP_VISIBLE_CHILD_NAME:
    hdy_deck_set_visible_child_name (self, g_value_get_string (value));
    break;
  case PROP_TRANSITION_TYPE:
    hdy_deck_set_transition_type (self, g_value_get_enum (value));
    break;
  case PROP_TRANSITION_DURATION:
    hdy_deck_set_transition_duration (self, g_value_get_uint (value));
    break;
  case PROP_INTERPOLATE_SIZE:
    hdy_deck_set_interpolate_size (self, g_value_get_boolean (value));
    break;
  case PROP_CAN_SWIPE_BACK:
    hdy_deck_set_can_swipe_back (self, g_value_get_boolean (value));
    break;
  case PROP_CAN_SWIPE_FORWARD:
    hdy_deck_set_can_swipe_forward (self, g_value_get_boolean (value));
    break;
  case PROP_ORIENTATION:
    {
      GtkOrientation orientation = g_value_get_enum (value);
      if (priv->orientation != orientation) {
        priv->orientation = orientation;
        update_tracker_orientation (self);
        gtk_widget_queue_resize (GTK_WIDGET (self));
        g_object_notify (object, "orientation");
      }
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
hdy_deck_dispose (GObject *object)
{
  HdyDeck *self = HDY_DECK (object);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);

  priv->visible_child = NULL;

  if (priv->shadow_helper)
    g_clear_object (&priv->shadow_helper);

  G_OBJECT_CLASS (hdy_deck_parent_class)->dispose (object);
}

static void
hdy_deck_finalize (GObject *object)
{
  HdyDeck *self = HDY_DECK (object);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);

  hdy_deck_unschedule_child_ticks (self);

  if (priv->transition.last_visible_surface != NULL)
    cairo_surface_destroy (priv->transition.last_visible_surface);

  g_object_set_data (object, "captured-event-handler", NULL);

  G_OBJECT_CLASS (hdy_deck_parent_class)->finalize (object);
}

static void
hdy_deck_get_child_property (GtkContainer *container,
                             GtkWidget    *widget,
                             guint         property_id,
                             GValue       *value,
                             GParamSpec   *pspec)
{
  HdyDeck *self = HDY_DECK (container);
  HdyDeckChildInfo *child_info;

  child_info = find_child_info_for_widget (self, widget);
  if (child_info == NULL) {
    GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
    return;
  }

  switch (property_id) {
  case CHILD_PROP_NAME:
    g_value_set_string (value, child_info->name);
    break;

  default:
    GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
    break;
  }
}

static void
hdy_deck_set_child_property (GtkContainer *container,
                             GtkWidget    *widget,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  HdyDeck *self = HDY_DECK (container);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);
  HdyDeckChildInfo *child_info;
  HdyDeckChildInfo *child_info2;
  gchar *name;
  GList *children;

  child_info = find_child_info_for_widget (self, widget);
  if (child_info == NULL) {
    GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
    return;
  }

  switch (property_id) {
  case CHILD_PROP_NAME:
    name = g_value_dup_string (value);
    for (children = priv->children; children; children = children->next) {
      child_info2 = children->data;

      if (child_info == child_info2)
        continue;
      if (g_strcmp0 (child_info2->name, name) == 0) {
        g_warning ("Duplicate child name in HdyDeck: %s", name);

        break;
      }
    }

    g_free (child_info->name);
    child_info->name = name;

    gtk_container_child_notify_by_pspec (container, widget, pspec);

    if (priv->visible_child == child_info)
      g_object_notify_by_pspec (G_OBJECT (self),
                                props[PROP_VISIBLE_CHILD_NAME]);

    break;

  default:
    GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
    break;
  }
}

static void
hdy_deck_realize (GtkWidget *widget)
{
  HdyDeck *self = HDY_DECK (widget);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);
  GtkAllocation allocation;
  GdkWindowAttr attributes = { 0 };
  GdkWindowAttributesType attributes_mask;
  GList *children;
  HdyDeckChildInfo *child_info;
  GtkBorder padding;

  gtk_widget_set_realized (widget, TRUE);
  gtk_widget_set_window (widget, g_object_ref (gtk_widget_get_parent_window (widget)));

  gtk_widget_get_allocation (widget, &allocation);

  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes_mask = (GDK_WA_X | GDK_WA_Y) | GDK_WA_VISUAL;

  priv->view_window = gdk_window_new (gtk_widget_get_window (widget),
                                      &attributes, attributes_mask);
  gtk_widget_register_window (widget, priv->view_window);

  get_padding (widget, &padding);
  attributes.x = padding.left;
  attributes.y = padding.top;
  attributes.width = allocation.width;
  attributes.height = allocation.height;

  for (children = priv->children; children != NULL; children = children->next) {
    child_info = children->data;
    attributes.event_mask |= gtk_widget_get_events (child_info->widget);
  }

  priv->bin_window = gdk_window_new (priv->view_window, &attributes, attributes_mask);
  gtk_widget_register_window (widget, priv->bin_window);

  for (children = priv->children; children != NULL; children = children->next) {
    child_info = children->data;

    gtk_widget_set_parent_window (child_info->widget, priv->bin_window);
  }

  gdk_window_show (priv->bin_window);
}

static void
hdy_deck_unrealize (GtkWidget *widget)
{
  HdyDeck *self = HDY_DECK (widget);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);

  gtk_widget_unregister_window (widget, priv->bin_window);
  gdk_window_destroy (priv->bin_window);
  priv->bin_window = NULL;
  gtk_widget_unregister_window (widget, priv->view_window);
  gdk_window_destroy (priv->view_window);
  priv->view_window = NULL;

  GTK_WIDGET_CLASS (hdy_deck_parent_class)->unrealize (widget);
}

static void
hdy_deck_map (GtkWidget *widget)
{
  HdyDeck *self = HDY_DECK (widget);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);

  GTK_WIDGET_CLASS (hdy_deck_parent_class)->map (widget);

  gdk_window_show (priv->view_window);
}

static void
hdy_deck_unmap (GtkWidget *widget)
{
  HdyDeck *self = HDY_DECK (widget);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);

  gdk_window_hide (priv->view_window);

  GTK_WIDGET_CLASS (hdy_deck_parent_class)->unmap (widget);
}

static void
hdy_deck_switch_child (HdySwipeable *swipeable,
                       guint         index,
                       gint64        duration)
{
  HdyDeck *self = HDY_DECK (swipeable);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);
  HdyDeckChildInfo *child_info;

  child_info = g_list_nth_data (priv->children, index);

  set_visible_child_info (self, child_info, priv->transition_type,
                          duration, FALSE);
}

static HdyDeckChildInfo *
find_swipeable_child (HdyDeck *self,
                      gint     direction)
{
  HdyDeckPrivate *priv;
  GList *children;
  HdyDeckChildInfo *child = NULL;

  priv = hdy_deck_get_instance_private (self);

  children = g_list_find (priv->children, priv->visible_child);

  children = (direction < 0) ? children->prev : children->next;
  if (children != NULL)
    child = children->data;

  return child;
}

static double
get_current_progress (HdyDeck *self)
{
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);
  gboolean new_first = FALSE;
  GList *children;

  if (!priv->transition.is_gesture_active &&
      gtk_progress_tracker_get_state (&priv->transition.tracker) == GTK_PROGRESS_STATE_AFTER)
    return 0;

  for (children = priv->children; children; children = children->next) {
    if (priv->last_visible_child == children->data) {
      new_first = TRUE;

      break;
    }
    if (priv->visible_child == children->data)
      break;
  }

  return priv->transition.progress * (new_first ? 1 : -1);
}

static void
hdy_deck_begin_swipe (HdySwipeable *swipeable,
                      gint          direction,
                      gboolean      direct)
{
  HdyDeck *self = HDY_DECK (swipeable);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);
  gint n;
  gdouble *points, distance, progress;

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    distance = gtk_widget_get_allocated_width (GTK_WIDGET (self));
  else
    distance = gtk_widget_get_allocated_height (GTK_WIDGET (self));

  if (priv->transition.tick_id > 0) {
    gint current_direction;
    gboolean is_rtl;

    is_rtl = (gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL);

    switch (priv->transition.active_direction) {
    case GTK_PAN_DIRECTION_UP:
      current_direction = 1;
      break;
    case GTK_PAN_DIRECTION_DOWN:
      current_direction = -1;
      break;
    case GTK_PAN_DIRECTION_LEFT:
      current_direction = is_rtl ? -1 : 1;
      break;
    case GTK_PAN_DIRECTION_RIGHT:
      current_direction = is_rtl ? 1 : -1;
      break;
    default:
      g_assert_not_reached ();
    }

    n = 2;
    points = g_new0 (gdouble, n);
    points[current_direction > 0 ? 1 : 0] = current_direction;

    progress = get_current_progress (self);

    gtk_widget_remove_tick_callback (GTK_WIDGET (self), priv->transition.tick_id);
    priv->transition.tick_id = 0;
    priv->transition.is_gesture_active = TRUE;
    priv->transition.is_cancelled = FALSE;
  } else {
    HdyDeckChildInfo *child;

    if (((direction < 0 && priv->transition.can_swipe_back) ||
        (direction > 0 && priv->transition.can_swipe_forward) ||
         !direct))
      child = find_swipeable_child (self, direction);
    else
      child = NULL;

    if (child) {
      priv->transition.is_gesture_active = TRUE;
      set_visible_child_info (self, child, priv->transition_type,
                              priv->transition.duration, FALSE);

      g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TRANSITION_RUNNING]);
    }

    progress = 0;

    n = child ? 2 : 1;
    points = g_new0 (gdouble, n);
    if (child)
      points[direction > 0 ? 1 : 0] = direction;
  }

  hdy_swipe_tracker_confirm_swipe (priv->tracker, distance, points, n, progress, 0);
}

static void
hdy_deck_update_swipe (HdySwipeable *swipeable,
                       gdouble       value)
{
  HdyDeck *self = HDY_DECK (swipeable);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);

  priv->transition.progress = ABS (value);
  hdy_deck_child_progress_updated (self);
}

static void
hdy_deck_end_swipe (HdySwipeable *swipeable,
                    gint64        duration,
                    gdouble       to)
{
  HdyDeck *self = HDY_DECK (swipeable);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);

 if (!priv->transition.is_gesture_active)
    return;

  priv->transition.start_progress = priv->transition.progress;
  priv->transition.end_progress = ABS (to);
  priv->transition.is_cancelled = (to == 0);
  priv->transition.first_frame_skipped = TRUE;

  hdy_deck_schedule_child_ticks (self);
  if (hdy_get_enable_animations (GTK_WIDGET (self)) &&
      duration != 0 &&
      priv->transition_type != HDY_DECK_TRANSITION_TYPE_NONE) {
    gtk_progress_tracker_start (&priv->transition.tracker,
                                duration * 1000,
                                0,
                                1.0);
  } else {
    priv->transition.progress = priv->transition.end_progress;
    gtk_progress_tracker_finish (&priv->transition.tracker);
  }

  priv->transition.is_gesture_active = FALSE;
  hdy_deck_child_progress_updated (self);

  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static gboolean
captured_event_cb (HdyDeck  *self,
                   GdkEvent *event)
{
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);

  return hdy_swipe_tracker_captured_event (priv->tracker, event);
}

static void
hdy_deck_class_init (HdyDeckClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = (GtkWidgetClass*) klass;
  GtkContainerClass *container_class = (GtkContainerClass*) klass;

  object_class->get_property = hdy_deck_get_property;
  object_class->set_property = hdy_deck_set_property;
  object_class->dispose = hdy_deck_dispose;
  object_class->finalize = hdy_deck_finalize;

  widget_class->realize = hdy_deck_realize;
  widget_class->unrealize = hdy_deck_unrealize;
  widget_class->map = hdy_deck_map;
  widget_class->unmap = hdy_deck_unmap;
  widget_class->get_preferred_width = hdy_deck_get_preferred_width;
  widget_class->get_preferred_height = hdy_deck_get_preferred_height;
  widget_class->get_preferred_width_for_height = hdy_deck_get_preferred_width_for_height;
  widget_class->get_preferred_height_for_width = hdy_deck_get_preferred_height_for_width;
  widget_class->size_allocate = hdy_deck_size_allocate;
  widget_class->draw = hdy_deck_draw;
  widget_class->direction_changed = hdy_deck_direction_changed;

  container_class->add = hdy_deck_add;
  container_class->remove = hdy_deck_remove;
  container_class->forall = hdy_deck_forall;
  container_class->set_child_property = hdy_deck_set_child_property;
  container_class->get_child_property = hdy_deck_get_child_property;
  gtk_container_class_handle_border_width (container_class);

  g_object_class_override_property (object_class,
                                    PROP_ORIENTATION,
                                    "orientation");

  props[PROP_HHOMOGENEOUS] =
    g_param_spec_boolean ("hhomogeneous",
                          _("Horizontally homogeneous"),
                          _("Horizontally homogeneous sizing"),
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_VHOMOGENEOUS] =
    g_param_spec_boolean ("vhomogeneous",
                          _("Vertically homogeneous"),
                          _("Vertically homogeneous sizing"),
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_VISIBLE_CHILD] =
    g_param_spec_object ("visible-child",
                         _("Visible child"),
                         _("The widget currently visible"),
                         GTK_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_VISIBLE_CHILD_NAME] =
    g_param_spec_string ("visible-child-name",
                         _("Name of visible child"),
                         _("The name of the widget currently visible"),
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * HdyDeck:transition-type:
   *
   * The type of animation that will be used for transitions between modes and
   * children.
   *
   * The transition type can be changed without problems at runtime, so it is
   * possible to change the animation based on the mode or child that is about
   * to become current.
   *
   * Since: 0.0.12
   */
  props[PROP_TRANSITION_TYPE] =
    g_param_spec_enum ("transition-type",
                       _("Transition type"),
                       _("The type of animation used to transition between modes and children"),
                       HDY_TYPE_DECK_TRANSITION_TYPE, HDY_DECK_TRANSITION_TYPE_NONE,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_TRANSITION_DURATION] =
    g_param_spec_uint ("transition-duration",
                       _("Transition duration"),
                       _("The transition animation duration, in milliseconds"),
                       0, G_MAXUINT, 200,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_TRANSITION_RUNNING] =
      g_param_spec_boolean ("transition-running",
                            _("Transition running"),
                            _("Whether or not the transition is currently running"),
                            FALSE,
                            G_PARAM_READABLE);

  props[PROP_INTERPOLATE_SIZE] =
      g_param_spec_boolean ("interpolate-size",
                            _("Interpolate size"),
                            _("Whether or not the size should smoothly change when changing between differently sized children"),
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * HdyDeck:can-swipe-back:
   *
   * Whether or not @self allows switching to the previous child that has
   * 'allow-visible' child property set to %TRUE via a swipe gesture.
   *
   * Since: 0.0.12
   */
  props[PROP_CAN_SWIPE_BACK] =
      g_param_spec_boolean ("can-swipe-back",
                            _("Can swipe back"),
                            _("Whether or not swipe gesture can be used to switch to the previous child"),
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * HdyDeck:can-swipe-forward:
   *
   * Whether or not @self allows switching to the next child that has
   * 'allow-visible' child property set to %TRUE via a swipe gesture.
   *
   * Since: 0.0.12
   */
  props[PROP_CAN_SWIPE_FORWARD] =
      g_param_spec_boolean ("can-swipe-forward",
                            _("Can swipe forward"),
                            _("Whether or not swipe gesture can be used to switch to the next child"),
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  child_props[CHILD_PROP_NAME] =
    g_param_spec_string ("name",
                         _("Name"),
                         _("The name of the child page"),
                         NULL,
                         G_PARAM_READWRITE);

  gtk_container_class_install_child_properties (container_class, LAST_CHILD_PROP, child_props);

  gtk_widget_class_set_accessible_role (widget_class, ATK_ROLE_PANEL);
  gtk_widget_class_set_css_name (widget_class, "hdydeck");
}

GtkWidget *
hdy_deck_new (void)
{
  return g_object_new (HDY_TYPE_DECK, NULL);
}

static void
hdy_deck_init (HdyDeck *self)
{
  GtkWidget *widget = GTK_WIDGET (self);
  HdyDeckPrivate *priv = hdy_deck_get_instance_private (self);

  priv->children = NULL;
  priv->children_reversed = NULL;
  priv->visible_child = NULL;
  priv->homogeneous[GTK_ORIENTATION_HORIZONTAL] = TRUE;
  priv->homogeneous[GTK_ORIENTATION_VERTICAL] = TRUE;
  priv->transition_type = HDY_DECK_TRANSITION_TYPE_NONE;
  priv->transition.duration = 200;

  priv->tracker = hdy_swipe_tracker_new (HDY_SWIPEABLE (self));
  g_object_set (priv->tracker, "orientation", priv->orientation, "enabled", FALSE, NULL);

  priv->shadow_helper = hdy_shadow_helper_new (widget);

  gtk_widget_set_has_window (widget, FALSE);
  gtk_widget_set_can_focus (widget, FALSE);
  gtk_widget_set_redraw_on_allocate (widget, FALSE);

  /*
   * HACK: GTK3 has no other way to get events on capture phase.
   * This is a reimplementation of _gtk_widget_set_captured_event_handler(),
   * which is private. In GTK4 it can be replaced with GtkEventControllerLegacy
   * with capture propagation phase
   */
  g_object_set_data (G_OBJECT (self), "captured-event-handler", captured_event_cb);
}

static void
hdy_deck_buildable_init (GtkBuildableIface *iface)
{
}

static void
hdy_deck_swipeable_init (HdySwipeableInterface *iface)
{
  iface->switch_child = hdy_deck_switch_child;
  iface->begin_swipe = hdy_deck_begin_swipe;
  iface->update_swipe = hdy_deck_update_swipe;
  iface->end_swipe = hdy_deck_end_swipe;
}

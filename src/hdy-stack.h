/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#if !defined(_HANDY_INSIDE) && !defined(HANDY_COMPILATION)
#error "Only <handy.h> can be included directly."
#endif

#include <gtk/gtk.h>
#include "hdy-enums.h"

G_BEGIN_DECLS

#define HDY_TYPE_STACK (hdy_stack_get_type())

G_DECLARE_DERIVABLE_TYPE (HdyStack, hdy_stack, HDY, STACK, GtkContainer)

typedef enum {
  HDY_STACK_TRANSITION_TYPE_NONE,
  HDY_STACK_TRANSITION_TYPE_SLIDE,
  HDY_STACK_TRANSITION_TYPE_OVER,
  HDY_STACK_TRANSITION_TYPE_UNDER,
} HdyStackTransitionType;

/**
 * HdyStackClass
 * @parent_class: The parent class
 */
struct _HdyStackClass
{
  GtkContainerClass parent_class;

  /*< private >*/

  /* Signals
   */
  void (*todo) (HdyStack *self);
};

GtkWidget       *hdy_stack_new (void);
GtkWidget       *hdy_stack_get_visible_child (HdyStack *self);
void             hdy_stack_set_visible_child (HdyStack *self,
                                                GtkWidget  *visible_child);
const gchar     *hdy_stack_get_visible_child_name (HdyStack *self);
void             hdy_stack_set_visible_child_name (HdyStack  *self,
                                                     const gchar *name);
gboolean         hdy_stack_get_homogeneous (HdyStack       *self,
                                            GtkOrientation  orientation);
void             hdy_stack_set_homogeneous (HdyStack       *self,
                                            GtkOrientation  orientation,
                                            gboolean        homogeneous);
HdyStackTransitionType hdy_stack_get_transition_type (HdyStack *self);
void             hdy_stack_set_transition_type (HdyStack               *self,
                                                  HdyStackTransitionType  transition);

guint            hdy_stack_get_transition_duration (HdyStack *self);
void             hdy_stack_set_transition_duration (HdyStack *self,
                                                    guint     duration);
gboolean         hdy_stack_get_transition_running (HdyStack *self);
gboolean         hdy_stack_get_interpolate_size (HdyStack *self);
void             hdy_stack_set_interpolate_size (HdyStack *self,
                                                   gboolean    interpolate_size);
gboolean         hdy_stack_get_can_swipe_back (HdyStack *self);
void             hdy_stack_set_can_swipe_back (HdyStack *self,
                                                 gboolean    can_swipe_back);
gboolean         hdy_stack_get_can_swipe_forward (HdyStack *self);
void             hdy_stack_set_can_swipe_forward (HdyStack *self,
                                                    gboolean    can_swipe_forward);

G_END_DECLS

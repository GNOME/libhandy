/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#if !defined(_HANDY_INSIDE) && !defined(HANDY_COMPILATION)
#error "Only <handy.h> can be included directly."
#endif

#include "hdy-version.h"

#include <gtk/gtk.h>
#include "hdy-enums.h"

G_BEGIN_DECLS

#define HDY_TYPE_SQUEEZER (hdy_squeezer_get_type ())

HDY_AVAILABLE_IN_ALL
G_DECLARE_DERIVABLE_TYPE (HdySqueezer, hdy_squeezer, HDY, SQUEEZER, GtkContainer)

typedef enum {
  HDY_SQUEEZER_TRANSITION_TYPE_NONE,
  HDY_SQUEEZER_TRANSITION_TYPE_CROSSFADE,
} HdySqueezerTransitionType;

/**
 * HdySqueezerClass
 * @parent_class: The parent class
 */
struct _HdySqueezerClass
{
  GtkContainerClass parent_class;
};

HDY_AVAILABLE_IN_ALL
GtkWidget *hdy_squeezer_new (void);

HDY_AVAILABLE_IN_ALL
gboolean hdy_squeezer_get_homogeneous (HdySqueezer *self);
HDY_AVAILABLE_IN_ALL
void     hdy_squeezer_set_homogeneous (HdySqueezer *self,
                                       gboolean     homogeneous);

HDY_AVAILABLE_IN_ALL
guint hdy_squeezer_get_transition_duration (HdySqueezer *self);
HDY_AVAILABLE_IN_ALL
void  hdy_squeezer_set_transition_duration (HdySqueezer *self,
                                            guint        duration);

HDY_AVAILABLE_IN_ALL
HdySqueezerTransitionType hdy_squeezer_get_transition_type (HdySqueezer *self);
HDY_AVAILABLE_IN_ALL
void                      hdy_squeezer_set_transition_type (HdySqueezer               *self,
                                                            HdySqueezerTransitionType  transition);

HDY_AVAILABLE_IN_ALL
gboolean hdy_squeezer_get_transition_running (HdySqueezer *self);

HDY_AVAILABLE_IN_ALL
gboolean hdy_squeezer_get_interpolate_size (HdySqueezer *self);
HDY_AVAILABLE_IN_ALL
void     hdy_squeezer_set_interpolate_size (HdySqueezer *self,
                                            gboolean     interpolate_size);

HDY_AVAILABLE_IN_ALL
GtkWidget *hdy_squeezer_get_visible_child (HdySqueezer *self);

HDY_AVAILABLE_IN_ALL
gboolean hdy_squeezer_get_child_enabled (HdySqueezer *self,
                                         GtkWidget   *child);
HDY_AVAILABLE_IN_ALL
void     hdy_squeezer_set_child_enabled (HdySqueezer *self,
                                         GtkWidget   *child,
                                         gboolean     enabled);

G_END_DECLS

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

G_BEGIN_DECLS

#define HDY_TYPE_DRAWER (hdy_drawer_get_type())

G_DECLARE_FINAL_TYPE (HdyDrawer, hdy_drawer, HDY, DRAWER, GtkEventBox)

HdyDrawer     *hdy_drawer_new (void);

void           hdy_drawer_open (HdyDrawer *self,
                                GtkWidget *widget);
void           hdy_drawer_close (HdyDrawer *self);

GtkWidget     *hdy_drawer_get_overlay (HdyDrawer *self);
void           hdy_drawer_set_overlay (HdyDrawer *self,
                                       GtkWidget *widget);

void           hdy_drawer_add_anchor (HdyDrawer *self,
                                      GtkWidget *widget);
void           hdy_drawer_remove_anchor (HdyDrawer *self,
                                         GtkWidget *widget);

int            hdy_drawer_get_offset (HdyDrawer *self);

gboolean       hdy_drawer_get_interactive (HdyDrawer *self);
void           hdy_drawer_set_interactive (HdyDrawer *self,
                                           gboolean   interactive);

double         hdy_drawer_get_progress (HdyDrawer *self);

gboolean       hdy_drawer_get_can_hide_overlay (HdyDrawer *self);
void           hdy_drawer_set_can_hide_overlay (HdyDrawer *self,
                                                gboolean   can_hide_overlay);

GtkPanDirection hdy_drawer_get_direction (HdyDrawer *self);
void           hdy_drawer_set_direction (HdyDrawer       *self,
                                         GtkPanDirection  direction);

uint           hdy_drawer_get_animation_duration (HdyDrawer *self);
void           hdy_drawer_set_animation_duration (HdyDrawer *self,
                                                  uint       duration);

G_END_DECLS

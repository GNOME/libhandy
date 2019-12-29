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
#include "hdy-deprecation-macros.h"

G_BEGIN_DECLS

#define HDY_TYPE_PAGINATOR_BOX (hdy_paginator_box_get_type())

G_DECLARE_FINAL_TYPE (HdyPaginatorBox, hdy_paginator_box, HDY, PAGINATOR_BOX, GtkContainer)

HdyPaginatorBox *hdy_paginator_box_new (void);

void             hdy_paginator_box_insert (HdyPaginatorBox *self,
                                           GtkWidget       *widget,
                                           gint             position);
void             hdy_paginator_box_reorder (HdyPaginatorBox *self,
                                            GtkWidget       *widget,
                                            gint             position);

_HDY_DEPRECATED
void             hdy_paginator_box_animate (HdyPaginatorBox *self,
                                            gdouble          position,
                                            gint64           duration);
gboolean         hdy_paginator_box_is_animating (HdyPaginatorBox *self);
void             hdy_paginator_box_stop_animation (HdyPaginatorBox *self);

void             hdy_paginator_box_scroll_to (HdyPaginatorBox *self,
                                              GtkWidget       *widget,
                                              gint64           duration);

guint            hdy_paginator_box_get_n_pages (HdyPaginatorBox *self);
gdouble          hdy_paginator_box_get_distance (HdyPaginatorBox *self);

gdouble          hdy_paginator_box_get_position (HdyPaginatorBox *self);
void             hdy_paginator_box_set_position (HdyPaginatorBox *self,
                                                 gdouble          position);

guint            hdy_paginator_box_get_spacing (HdyPaginatorBox *self);
void             hdy_paginator_box_set_spacing (HdyPaginatorBox *self,
                                                guint            spacing);

guint            hdy_paginator_box_get_reveal_duration (HdyPaginatorBox *self);
void             hdy_paginator_box_set_reveal_duration (HdyPaginatorBox *self,
                                                        guint            reveal_duration);

GtkWidget       *hdy_paginator_box_get_nth_child (HdyPaginatorBox *self,
                                                  guint            n);

gdouble         *hdy_paginator_box_get_snap_points        (HdyPaginatorBox *self,
                                                           gint            *n_snap_points);
void             hdy_paginator_box_get_range              (HdyPaginatorBox *self,
                                                           gdouble         *lower,
                                                           gdouble         *upper);
gdouble          hdy_paginator_box_get_closest_snap_point (HdyPaginatorBox *self);
GtkWidget       *hdy_paginator_box_get_page_at_position   (HdyPaginatorBox *self,
                                                           gdouble          position);
gint             hdy_paginator_box_get_current_page_index (HdyPaginatorBox *self);

G_END_DECLS

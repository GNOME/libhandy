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

#define HDY_TYPE_PAGINATOR_BOX (hdy_paginator_box_get_type())

G_DECLARE_FINAL_TYPE (HdyPaginatorBox, hdy_paginator_box, HDY, PAGINATOR_BOX, GtkContainer)

HdyPaginatorBox *hdy_paginator_box_new (void);

void             hdy_paginator_box_insert (HdyPaginatorBox *self,
                                           GtkWidget       *child,
                                           int              position);
void             hdy_paginator_box_reorder (HdyPaginatorBox *self,
                                            GtkWidget       *child,
                                            int              position);

void             hdy_paginator_box_animate (HdyPaginatorBox *self,
                                            double           position,
                                            int64_t          duration);
gboolean         hdy_paginator_box_is_animating (HdyPaginatorBox *self);
void             hdy_paginator_box_stop_animation (HdyPaginatorBox *self);

void             hdy_paginator_box_scroll_to (HdyPaginatorBox *self,
                                              GtkWidget       *widget,
                                              int64_t          duration);

int              hdy_paginator_box_get_n_pages (HdyPaginatorBox *self);
double           hdy_paginator_box_get_distance (HdyPaginatorBox *self);

double           hdy_paginator_box_get_position (HdyPaginatorBox *self);
void             hdy_paginator_box_set_position (HdyPaginatorBox *self,
                                                 double           position);

int              hdy_paginator_box_get_spacing (HdyPaginatorBox *self);
void             hdy_paginator_box_set_spacing (HdyPaginatorBox *self,
                                                int              spacing);

G_END_DECLS

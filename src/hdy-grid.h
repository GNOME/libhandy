/*
 * Copyright (C) 2020 Ujjwal Kumar <ujjwalkumar0501@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#if !defined(_HANDY_INSIDE) && !defined(HANDY_COMPILATION)
#error "Only <handy.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define HDY_TYPE_GRID (hdy_grid_get_type())

G_DECLARE_DERIVABLE_TYPE (HdyGrid, hdy_grid, HDY, GRID, GtkContainer)

/**
 * HdyGridClass
 * @parent_class: The parent class
 */
struct _HdyGridClass
{
  GtkContainerClass parent_class;
};

GtkWidget       *hdy_grid_new (void);
guint            hdy_grid_get_position (GtkContainer *, GtkWidget *);
void             hdy_grid_set_position (GtkContainer *, GtkWidget *, guint);
guint            hdy_grid_get_weight (GtkContainer *, GtkWidget *);
void             hdy_grid_set_weight (GtkContainer *, GtkWidget *, guint);


G_END_DECLS

/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#if !defined(_HANDY_INSIDE) && !defined(HANDY_COMPILATION)
#error "Only <handy.h> can be included directly."
#endif

#include "hdy-version.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define HDY_TYPE_COLUMN (hdy_column_get_type())

HDY_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (HdyColumn, hdy_column, HDY, COLUMN, GtkBin)

HDY_AVAILABLE_IN_ALL
GtkWidget *hdy_column_new (void);
HDY_AVAILABLE_IN_ALL
gint hdy_column_get_maximum_width (HdyColumn *self);
HDY_AVAILABLE_IN_ALL
void hdy_column_set_maximum_width (HdyColumn *self,
                                   gint       maximum_width);
HDY_AVAILABLE_IN_ALL
gint hdy_column_get_linear_growth_width (HdyColumn *self);
HDY_AVAILABLE_IN_ALL
void hdy_column_set_linear_growth_width (HdyColumn *self,
                                         gint       linear_growth_width);

G_END_DECLS

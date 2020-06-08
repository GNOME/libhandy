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

#define HDY_TYPE_HEADER_GROUP (hdy_header_group_get_type())

HDY_AVAILABLE_IN_ALL
G_DECLARE_DERIVABLE_TYPE (HdyHeaderGroup, hdy_header_group, HDY, HEADER_GROUP, GObject)

/**
 * HdyHeaderGroupClass
 * @parent_class: The parent class
 */
struct _HdyHeaderGroupClass
{
  GObjectClass parent_class;
};

HDY_AVAILABLE_IN_ALL
HdyHeaderGroup *hdy_header_group_new (void);

HDY_AVAILABLE_IN_ALL
void hdy_header_group_add_header_bar (HdyHeaderGroup *self,
                                      GtkHeaderBar   *header_bar);

HDY_AVAILABLE_IN_ALL
GtkHeaderBar *hdy_header_group_get_focus         (HdyHeaderGroup *self);
HDY_AVAILABLE_IN_ALL
void          hdy_header_group_set_focus         (HdyHeaderGroup *self,
                                                  GtkHeaderBar   *header_bar);
HDY_AVAILABLE_IN_ALL
GSList *      hdy_header_group_get_header_bars   (HdyHeaderGroup *self);
HDY_AVAILABLE_IN_ALL
void          hdy_header_group_remove_header_bar (HdyHeaderGroup *self,
                                                  GtkHeaderBar   *header_bar);


G_END_DECLS

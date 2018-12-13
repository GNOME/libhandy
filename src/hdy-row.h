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

G_BEGIN_DECLS

#define HDY_TYPE_ROW (hdy_row_get_type())

G_DECLARE_DERIVABLE_TYPE (HdyRow, hdy_row, HDY, ROW, GtkListBoxRow)

/**
 * HdyRowClass
 * @parent_class: The parent class
 * @activate: Activates the row to trigger its main action.
 */
struct _HdyRowClass
{
  GtkListBoxRowClass parent_class;

  void (*activate) (HdyRow *self);
};

HdyRow *hdy_row_new (void);

const gchar *hdy_row_get_title (HdyRow *self);
void         hdy_row_set_title (HdyRow      *self,
                                const gchar *title);

const gchar *hdy_row_get_subtitle (HdyRow *self);
void         hdy_row_set_subtitle (HdyRow      *self,
                                   const gchar *subtitle);

const gchar *hdy_row_get_icon_name (HdyRow *self);
void         hdy_row_set_icon_name (HdyRow      *self,
                                    const gchar *icon_name);

void hdy_row_add_action (HdyRow    *self,
                         GtkWidget *widget);

void hdy_row_activate (HdyRow *self);

G_END_DECLS

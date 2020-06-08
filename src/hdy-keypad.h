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

G_BEGIN_DECLS

#define HDY_TYPE_KEYPAD (hdy_keypad_get_type())

HDY_AVAILABLE_IN_ALL
G_DECLARE_DERIVABLE_TYPE (HdyKeypad, hdy_keypad, HDY, KEYPAD, GtkBin)

/**
 * HdyKeypadClass:
 * @parent_class: The parent class
 */
struct _HdyKeypadClass
{
  GtkBinClass parent_class;
};

HDY_AVAILABLE_IN_ALL
GtkWidget       *hdy_keypad_new                     (gboolean only_digits,
                                                     gboolean show_symbols);
HDY_AVAILABLE_IN_ALL
void             hdy_keypad_set_row_spacing         (HdyKeypad *self,
                                                     guint      spacing);
HDY_AVAILABLE_IN_ALL
guint            hdy_keypad_get_row_spacing         (HdyKeypad *self);
HDY_AVAILABLE_IN_ALL
void             hdy_keypad_set_column_spacing      (HdyKeypad *self,
                                                     guint      spacing);
HDY_AVAILABLE_IN_ALL
guint            hdy_keypad_get_column_spacing      (HdyKeypad *self);
HDY_AVAILABLE_IN_ALL
void             hdy_keypad_show_symbols            (HdyKeypad *self,
                                                     gboolean   visible);
HDY_AVAILABLE_IN_ALL
void             hdy_keypad_set_entry               (HdyKeypad *self,
                                                     GtkEntry  *entry);
HDY_AVAILABLE_IN_ALL
GtkWidget       *hdy_keypad_get_entry               (HdyKeypad *self);
HDY_AVAILABLE_IN_ALL
void             hdy_keypad_set_left_action         (HdyKeypad *self,
                                                     GtkWidget *widget);
HDY_AVAILABLE_IN_ALL
void             hdy_keypad_set_right_action        (HdyKeypad *self,
                                                     GtkWidget *widget);


G_END_DECLS

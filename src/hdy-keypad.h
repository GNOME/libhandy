/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#if !defined(_HANDY_INSIDE) && !defined(HANDY_COMPILATION)
#error "Only <handy.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define HDY_TYPE_KEYPAD (hdy_keypad_get_type())

G_DECLARE_DERIVABLE_TYPE (HdyKeypad, hdy_keypad, HDY, KEYPAD, GtkGrid)

/**
 * HdyKeypadClass:
 * @parent_class: The parent class
 */
struct _HdyKeypadClass
{
  GtkGridClass parent_class;
};

GtkWidget       *hdy_keypad_new                     (gboolean only_digits);

gboolean         hdy_keypad_get_show_action_buttons (HdyKeypad  *self);
void             hdy_keypad_set_show_action_buttons (HdyKeypad  *self,
                                                     gboolean    show);
void             hdy_keypad_show_symbols            (HdyKeypad *self,
                                                     gboolean visible);
gboolean         hdy_keypad_filter_key_press_event  (HdyKeypad   *self,
                                                    GdkEventKey *event);

GtkWidget       *hdy_keypad_get_entry               (HdyKeypad *self);
void             hdy_keypad_set_left_action         (HdyKeypad *self,
                                                     GtkWidget *widget);
void             hdy_keypad_set_right_action        (HdyKeypad *self,
                                                     GtkWidget *widget);


G_END_DECLS

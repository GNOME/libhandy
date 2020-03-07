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

#define HDY_TYPE_PREFERENCES_WINDOW (hdy_preferences_window_get_type())

G_DECLARE_DERIVABLE_TYPE (HdyPreferencesWindow, hdy_preferences_window, HDY, PREFERENCES_WINDOW, GtkWindow)

/**
 * HdyPreferencesWindowClass
 * @parent_class: The parent class
 */
struct _HdyPreferencesWindowClass
{
  GtkWindowClass parent_class;
};

GtkWidget *hdy_preferences_window_new (void);

gboolean hdy_preferences_window_get_search_enabled (HdyPreferencesWindow *self);
void     hdy_preferences_window_set_search_enabled (HdyPreferencesWindow *self,
                                                    gboolean              search_enabled);

G_END_DECLS

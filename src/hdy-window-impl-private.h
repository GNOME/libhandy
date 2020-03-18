/*
 * Copyright (C) 2019 Alexander Mikhaylenko <alexm@gnome.org>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#if !defined(_HANDY_INSIDE) && !defined(HANDY_COMPILATION)
#error "Only <handy.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define HDY_TYPE_WINDOW_IMPL (hdy_window_impl_get_type())

G_DECLARE_FINAL_TYPE (HdyWindowImpl, hdy_window_impl, HDY, WINDOW_IMPL, GObject)

HdyWindowImpl *hdy_window_impl_new (GtkWindow      *window,
                                    GtkWindowClass *klass);
void           hdy_window_impl_add (HdyWindowImpl *self,
                                    GtkWidget     *widget);
void           hdy_window_impl_remove (HdyWindowImpl *self,
                                       GtkWidget     *widget);
void           hdy_window_impl_forall (HdyWindowImpl *self,
                                       gboolean       include_internals,
                                       GtkCallback    callback,
                                       gpointer       callback_data);

gboolean       hdy_window_impl_draw (HdyWindowImpl *self,
                                     cairo_t       *cr);

G_END_DECLS

/*
 * Copyright (C) 2019 Alexander Mikhaylenko <exalm7659@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#include <gladeui/glade.h>

#define HANDY_USE_UNSTABLE_API
#include <handy.h>


void glade_hdy_drawer_post_create (GladeWidgetAdaptor *adaptor,
                                   GObject            *container,
                                   GladeCreateReason   reason);

gboolean glade_hdy_drawer_add_child_verify (GladeWidgetAdaptor *adaptor,
                                            GObject            *container,
                                            GObject            *child,
                                            gboolean            user_feedback);

void glade_hdy_drawer_add_child     (GladeWidgetAdaptor *adaptor,
                                     GObject            *container,
                                     GObject            *child);
void glade_hdy_drawer_remove_child  (GladeWidgetAdaptor *adaptor,
                                     GObject            *container,
                                     GObject            *child);

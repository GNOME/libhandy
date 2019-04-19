/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

void gtk_container_get_children_clip (GtkContainer  *container,
                                      GtkAllocation *out_clip);

G_END_DECLS

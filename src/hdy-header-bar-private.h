/*
 * Copyright (c) 2013 Red Hat, Inc.
 * Copyright (C) 2019 Purism SPC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#include "hdy-header-bar.h"

G_BEGIN_DECLS

gboolean     _hdy_header_bar_shows_app_menu        (HdyHeaderBar *bar);
void         _hdy_header_bar_update_window_buttons (HdyHeaderBar *bar);
gboolean     _hdy_header_bar_update_window_icon    (HdyHeaderBar *bar,
                                                    GtkWindow    *window);

G_END_DECLS

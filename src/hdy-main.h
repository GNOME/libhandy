/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */
#pragma once

#if !defined(_HANDY_INSIDE) && !defined(HANDY_COMPILATION)
#error "Only <handy.h> can be included directly."
#endif

#include <glib.h>
#include "hdy-version.h"

G_BEGIN_DECLS

#ifndef HANDY_DISABLE_DEPRECATED

HDY_DEPRECATED_IN_0_0_8
gboolean hdy_init(int *argc, char ***argv);

#endif

#ifdef HDY_IS_STATIC
void hdy_init_static (void);
#endif

G_END_DECLS

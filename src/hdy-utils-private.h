/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#if !defined(_HANDY_INSIDE) && !defined(HANDY_COMPILATION)
#error "Only <handy.h> can be included directly."
#endif

#include <glib.h>

G_BEGIN_DECLS

#define HDY_AS_BOOLEAN(val) ((gboolean) !!val)
#define HDY_ENSURE_BOOLEAN(var) (var = HDY_AS_BOOLEAN(var))

G_END_DECLS

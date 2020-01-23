/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#if !defined(_HANDY_INSIDE) && !defined(HANDY_COMPILATION)
#error "Only <handy.h> can be included directly."
#endif

#include <glib-object.h>
#include "hdy-enums.h"

G_BEGIN_DECLS

typedef enum {
  HDY_TRANSITION_TYPE_NONE,
  HDY_TRANSITION_TYPE_SLIDE,
  HDY_TRANSITION_TYPE_OVER,
  HDY_TRANSITION_TYPE_UNDER,
} HdyTransitionType;

G_END_DECLS

/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"
#include "hdy-transition-type.h"

/**
 * SECTION:hdy-transition-type
 * @short_description: TransitionTypes for #HdyLeaflet.
 * @title: HdyTransitionType
 */

/**
 * HdyTransitionType:
 * @HDY_TRANSITION_TYPE_NONE: No transition
 * @HDY_TRANSITION_TYPE_SLIDE: Slide from left, right, up or down according to the orientation, text direction and the children order
 * @HDY_TRANSITION_TYPE_OVER: Cover the old page or uncover the new page, sliding from or towards the end according to orientation, text direction and children order
 * @HDY_TRANSITION_TYPE_UNDER: Uncover the new page or cover the old page, sliding from or towards the start according to orientation, text direction and children order
 *
 * This enumeration value describes the possible transitions between modes and
 * children in a #HdyLeaflet widget.
 *
 * New values may be added to this enumeration over time.
 *
 * Since: 1.0
 */

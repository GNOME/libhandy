/*
 * Copyright (C) 2020 Alexander Mikhaylenko <alexm@gnome.org>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#if !defined(_HANDY_INSIDE) && !defined(HANDY_COMPILATION)
#error "Only <handy.h> can be included directly."
#endif

#include <gtk/gtk.h>
#include "hdy-carousel.h"

G_BEGIN_DECLS

#define HDY_TYPE_CAROUSEL_PAGE_DOTS (hdy_carousel_page_dots_get_type())

G_DECLARE_FINAL_TYPE (HdyCarouselPageDots, hdy_carousel_page_dots, HDY, CAROUSEL_PAGE_DOTS, GtkDrawingArea)

GtkWidget   *hdy_carousel_page_dots_new (void);

HdyCarousel *hdy_carousel_page_dots_get_carousel (HdyCarouselPageDots *self);
void         hdy_carousel_page_dots_set_carousel (HdyCarouselPageDots *self,
                                                  HdyCarousel         *carousel);

G_END_DECLS

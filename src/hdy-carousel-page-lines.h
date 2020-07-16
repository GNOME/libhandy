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

#define HDY_TYPE_CAROUSEL_PAGE_LINES (hdy_carousel_page_lines_get_type())

G_DECLARE_FINAL_TYPE (HdyCarouselPageLines, hdy_carousel_page_lines, HDY, CAROUSEL_PAGE_LINES, GtkDrawingArea)

GtkWidget   *hdy_carousel_page_lines_new (void);

HdyCarousel *hdy_carousel_page_lines_get_carousel (HdyCarouselPageLines *self);
void         hdy_carousel_page_lines_set_carousel (HdyCarouselPageLines *self,
                                                   HdyCarousel          *carousel);

G_END_DECLS

/*
 * Copyright (C) 2020 Alexander Mikhaylenko <alexm@gnome.org>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#define HANDY_USE_UNSTABLE_API
#include <handy.h>

gint notified;

static void
notify_cb (GtkWidget *widget, gpointer data)
{
  notified++;
}

static void
test_hdy_carousel_page_lines_carousel (void)
{
  g_autoptr (HdyCarouselPageLines) lines = NULL;
  HdyCarousel *carousel;

  lines = g_object_ref_sink (HDY_CAROUSEL_PAGE_LINES (hdy_carousel_page_lines_new ()));
  g_assert_nonnull (lines);

  notified = 0;
  g_signal_connect (lines, "notify::carousel", G_CALLBACK (notify_cb), NULL);

  carousel = HDY_CAROUSEL (hdy_carousel_new ());
  g_assert_nonnull (carousel);

  g_assert_null (hdy_carousel_page_lines_get_carousel (lines));
  g_assert_cmpint (notified, ==, 0);

  hdy_carousel_page_lines_set_carousel (lines, carousel);
  g_assert (hdy_carousel_page_lines_get_carousel (lines) == carousel);
  g_assert_cmpint (notified, ==, 1);

  hdy_carousel_page_lines_set_carousel (lines, NULL);
  g_assert_null (hdy_carousel_page_lines_get_carousel (lines));
  g_assert_cmpint (notified, ==, 2);
}

gint
main (gint argc,
      gchar *argv[])
{
  gtk_test_init (&argc, &argv, NULL);
  hdy_init ();

  g_test_add_func("/Handy/CarouselPageLines/carousel", test_hdy_carousel_page_lines_carousel);
  return g_test_run();
}

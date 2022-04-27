/*
 * Copyright (C) 2020 Alexander Mikhaylenko <alexm@gnome.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <handy.h>


static void
test_hdy_window_new (void)
{
  GtkWidget *window;

  window = g_object_ref_sink (hdy_window_new ());
  g_assert_nonnull (window);

  g_object_unref (window);
}


gint
main (gint argc,
      gchar *argv[])
{
  gtk_test_init (&argc, &argv, NULL);
  hdy_init ();

  g_test_add_func("/Handy/Window/new", test_hdy_window_new);

  return g_test_run();
}

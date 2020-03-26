/*
 * Copyright (C) 2020 Alexander Mikhaylenko <alexm@gnome.org>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#define HANDY_USE_UNSTABLE_API
#include <handy.h>


static void
test_hdy_window_styles (void)
{
  g_autoptr (GtkWidget) window = NULL;
  GtkWidget *box;
  GtkStyleContext *context;
  GtkAllocation alloc = { 0 };

  window = g_object_ref_sink (hdy_window_new ());
  g_assert_nonnull (window);

  context = gtk_widget_get_style_context (window);
  g_assert_nonnull (context);

  g_assert_false (gtk_style_context_has_class (context, "unified"));

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_window_set_titlebar (GTK_WINDOW (window), box);

  alloc.width = 300;
  alloc.height = 600;
  gtk_widget_size_allocate (window, &alloc);

  g_assert_true (gtk_style_context_has_class (context, "unified"));
}


gint
main (gint argc,
      gchar *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func("/Handy/Window/add", test_hdy_window_styles);

  return g_test_run();
}

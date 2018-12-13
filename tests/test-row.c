/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#define HANDY_USE_UNSTABLE_API
#include <handy.h>


static void
test_hdy_row_add (void)
{
  g_autoptr (HdyRow) row = NULL;
  GtkWidget *sw;

  row = g_object_ref_sink (HDY_ROW (hdy_row_new ()));
  g_assert_nonnull (row);

  sw = gtk_switch_new ();
  g_assert_nonnull (sw);

  gtk_container_add (GTK_CONTAINER (row), sw);
}


static void
test_hdy_row_add_action (void)
{
  g_autoptr (HdyRow) row = NULL;
  GtkWidget *sw;

  row = g_object_ref_sink (HDY_ROW (hdy_row_new ()));
  g_assert_nonnull (row);

  sw = gtk_switch_new ();
  g_assert_nonnull (sw);

  hdy_row_add_action (row, sw);
}


static void
test_hdy_row_title (void)
{
  g_autoptr (HdyRow) row = NULL;

  row = g_object_ref_sink (HDY_ROW (hdy_row_new ()));
  g_assert_nonnull (row);

  g_assert_cmpstr (hdy_row_get_title (row), ==, "");

  hdy_row_set_title (row, "Dummy title");
  g_assert_cmpstr (hdy_row_get_title (row), ==, "Dummy title");
}


static void
test_hdy_row_subtitle (void)
{
  g_autoptr (HdyRow) row = NULL;

  row = g_object_ref_sink (HDY_ROW (hdy_row_new ()));
  g_assert_nonnull (row);

  g_assert_cmpstr (hdy_row_get_subtitle (row), ==, "");

  hdy_row_set_subtitle (row, "Dummy subtitle");
  g_assert_cmpstr (hdy_row_get_subtitle (row), ==, "Dummy subtitle");
}


static void
test_hdy_row_icon_name (void)
{
  g_autoptr (HdyRow) row = NULL;

  row = g_object_ref_sink (HDY_ROW (hdy_row_new ()));
  g_assert_nonnull (row);

  g_assert_null (hdy_row_get_icon_name (row));

  hdy_row_set_icon_name (row, "dummy-icon-name");
  g_assert_cmpstr (hdy_row_get_icon_name (row), ==, "dummy-icon-name");
}


gint
main (gint argc,
      gchar *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func("/Handy/Row/add", test_hdy_row_add);
  g_test_add_func("/Handy/Row/add_action", test_hdy_row_add_action);
  g_test_add_func("/Handy/Row/title", test_hdy_row_title);
  g_test_add_func("/Handy/Row/subtitle", test_hdy_row_subtitle);
  g_test_add_func("/Handy/Row/icon_name", test_hdy_row_icon_name);

  return g_test_run();
}

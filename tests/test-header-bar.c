/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <handy.h>


static void
test_hdy_header_bar_pack (void)
{
  HdyHeaderBar *bar;
  GtkWidget *widget;

  bar = g_object_ref_sink (HDY_HEADER_BAR (hdy_header_bar_new ()));
  g_assert_nonnull (bar);

  widget = gtk_switch_new ();
  g_assert_nonnull (widget);

  hdy_header_bar_pack_start (bar, widget);

  widget = gtk_switch_new ();
  g_assert_nonnull (widget);

  hdy_header_bar_pack_end (bar, widget);

  g_object_unref (bar);
}


static void
test_hdy_header_bar_title (void)
{
  HdyHeaderBar *bar;

  bar = g_object_ref_sink (HDY_HEADER_BAR (hdy_header_bar_new ()));
  g_assert_nonnull (bar);

  g_assert_null (hdy_header_bar_get_title (bar));

  hdy_header_bar_set_title (bar, "Dummy title");
  g_assert_cmpstr (hdy_header_bar_get_title (bar), ==, "Dummy title");

  hdy_header_bar_set_title (bar, NULL);
  g_assert_null (hdy_header_bar_get_title (bar));

  g_object_unref (bar);
}


static void
test_hdy_header_bar_subtitle (void)
{
  HdyHeaderBar *bar;

  bar = g_object_ref_sink (HDY_HEADER_BAR (hdy_header_bar_new ()));
  g_assert_nonnull (bar);

  g_assert_null (hdy_header_bar_get_subtitle (bar));

  hdy_header_bar_set_subtitle (bar, "Dummy subtitle");
  g_assert_cmpstr (hdy_header_bar_get_subtitle (bar), ==, "Dummy subtitle");

  hdy_header_bar_set_subtitle (bar, NULL);
  g_assert_null (hdy_header_bar_get_subtitle (bar));

  g_object_unref (bar);
}


static void
test_hdy_header_bar_custom_title (void)
{
  HdyHeaderBar *bar;
  GtkWidget *widget;

  bar = g_object_ref_sink (HDY_HEADER_BAR (hdy_header_bar_new ()));
  g_assert_nonnull (bar);

  g_assert_null (hdy_header_bar_get_custom_title (bar));

  widget = gtk_switch_new ();
  g_assert_nonnull (widget);
  hdy_header_bar_set_custom_title (bar, widget);
  g_assert (hdy_header_bar_get_custom_title (bar) == widget);

  hdy_header_bar_set_custom_title (bar, NULL);
  g_assert_null (hdy_header_bar_get_custom_title (bar));

  g_object_unref (bar);
}


static void
test_hdy_header_bar_show_close_button (void)
{
  HdyHeaderBar *bar;

  bar = g_object_ref_sink (HDY_HEADER_BAR (hdy_header_bar_new ()));
  g_assert_nonnull (bar);

  g_assert_false (hdy_header_bar_get_show_close_button (bar));

  hdy_header_bar_set_show_close_button (bar, TRUE);
  g_assert_true (hdy_header_bar_get_show_close_button (bar));

  hdy_header_bar_set_show_close_button (bar, FALSE);
  g_assert_false (hdy_header_bar_get_show_close_button (bar));

  g_object_unref (bar);
}


static void
test_hdy_header_bar_has_subtitle (void)
{
  HdyHeaderBar *bar;

  bar = g_object_ref_sink (HDY_HEADER_BAR (hdy_header_bar_new ()));
  g_assert_nonnull (bar);

  g_assert_true (hdy_header_bar_get_has_subtitle (bar));

  hdy_header_bar_set_has_subtitle (bar, FALSE);
  g_assert_false (hdy_header_bar_get_has_subtitle (bar));

  hdy_header_bar_set_has_subtitle (bar, TRUE);
  g_assert_true (hdy_header_bar_get_has_subtitle (bar));

  g_object_unref (bar);
}


static void
test_hdy_header_bar_decoration_layout (void)
{
  HdyHeaderBar *bar;

  bar = g_object_ref_sink (HDY_HEADER_BAR (hdy_header_bar_new ()));
  g_assert_nonnull (bar);

  g_assert_null (hdy_header_bar_get_decoration_layout (bar));

  hdy_header_bar_set_decoration_layout (bar, ":");
  g_assert_cmpstr (hdy_header_bar_get_decoration_layout (bar), ==, ":");

  hdy_header_bar_set_decoration_layout (bar, NULL);
  g_assert_null (hdy_header_bar_get_decoration_layout (bar));

  g_object_unref (bar);
}


static void
test_hdy_header_bar_centering_policy (void)
{
  HdyHeaderBar *bar;

  bar = g_object_ref_sink (HDY_HEADER_BAR (hdy_header_bar_new ()));
  g_assert_nonnull (bar);

  g_assert_cmpint (hdy_header_bar_get_centering_policy (bar), ==, HDY_CENTERING_POLICY_LOOSE);

  hdy_header_bar_set_centering_policy (bar, HDY_CENTERING_POLICY_STRICT);
  g_assert_cmpint (hdy_header_bar_get_centering_policy (bar), ==, HDY_CENTERING_POLICY_STRICT);

  hdy_header_bar_set_centering_policy (bar, HDY_CENTERING_POLICY_LOOSE);
  g_assert_cmpint (hdy_header_bar_get_centering_policy (bar), ==, HDY_CENTERING_POLICY_LOOSE);

  g_object_unref (bar);
}


static void
test_hdy_header_bar_transition_duration (void)
{
  HdyHeaderBar *bar;

  bar = g_object_ref_sink (HDY_HEADER_BAR (hdy_header_bar_new ()));
  g_assert_nonnull (bar);

  g_assert_cmpuint (hdy_header_bar_get_transition_duration (bar), ==, 200);

  hdy_header_bar_set_transition_duration (bar, 0);
  g_assert_cmpuint (hdy_header_bar_get_transition_duration (bar), ==, 0);

  hdy_header_bar_set_transition_duration (bar, 1000);
  g_assert_cmpuint (hdy_header_bar_get_transition_duration (bar), ==, 1000);

  g_object_unref (bar);
}


static void
test_hdy_header_bar_interpolate_size (void)
{
  HdyHeaderBar *bar;

  bar = g_object_ref_sink (HDY_HEADER_BAR (hdy_header_bar_new ()));
  g_assert_nonnull (bar);

  g_assert_false (hdy_header_bar_get_interpolate_size (bar));

  hdy_header_bar_set_interpolate_size (bar, TRUE);
  g_assert_true (hdy_header_bar_get_interpolate_size (bar));

  hdy_header_bar_set_interpolate_size (bar, FALSE);
  g_assert_false (hdy_header_bar_get_interpolate_size (bar));

  g_object_unref (bar);
}


gint
main (gint argc,
      gchar *argv[])
{
  gtk_test_init (&argc, &argv, NULL);
  hdy_init ();

  g_test_add_func("/Handy/HeaderBar/pack", test_hdy_header_bar_pack);
  g_test_add_func("/Handy/HeaderBar/title", test_hdy_header_bar_title);
  g_test_add_func("/Handy/HeaderBar/subtitle", test_hdy_header_bar_subtitle);
  g_test_add_func("/Handy/HeaderBar/custom_title", test_hdy_header_bar_custom_title);
  g_test_add_func("/Handy/HeaderBar/show_close_button", test_hdy_header_bar_show_close_button);
  g_test_add_func("/Handy/HeaderBar/has_subtitle", test_hdy_header_bar_has_subtitle);
  g_test_add_func("/Handy/HeaderBar/decoration_layout", test_hdy_header_bar_decoration_layout);
  g_test_add_func("/Handy/HeaderBar/centering_policy", test_hdy_header_bar_centering_policy);
  g_test_add_func("/Handy/HeaderBar/transition_duration", test_hdy_header_bar_transition_duration);
  g_test_add_func("/Handy/HeaderBar/interpolate_size", test_hdy_header_bar_interpolate_size);

  return g_test_run();
}

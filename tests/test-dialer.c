/*
 * Copyright (C) 2017 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#define HANDY_USE_UNSTABLE_API
#include <handy.h>

gint notified;


static void
notify_cb(GtkWidget *widget, gpointer data)
{
  notified++;
}


static void
test_hdy_dialer_setnumber(void)
{
  GtkWidget *dialer;

  notified = 0;
  dialer = hdy_dialer_new();
  g_signal_connect (dialer, "notify::number", G_CALLBACK (notify_cb), NULL);

  g_assert_cmpstr("", ==, hdy_dialer_get_number(HDY_DIALER (dialer)));

  hdy_dialer_set_number(HDY_DIALER (dialer), "#1234");
  g_assert_cmpstr("#1234", ==, hdy_dialer_get_number(HDY_DIALER (dialer)));
  g_assert_cmpint(1, ==, notified);

  /* Check that we're assigning to the string and not overwriting */
  hdy_dialer_set_number(HDY_DIALER (dialer), "#123");
  g_assert_cmpstr("#1234", !=, hdy_dialer_get_number(HDY_DIALER (dialer)));
  g_assert_cmpint(2, ==, notified);

  /* Do the same using the GObject property */
  g_object_set(G_OBJECT (dialer), "number", "#12", NULL);
  g_assert_cmpstr("#123", !=, hdy_dialer_get_number(HDY_DIALER (dialer)));
  g_assert_cmpstr("#12", ==, hdy_dialer_get_number(HDY_DIALER (dialer)));
  g_assert_cmpint(3, ==, notified);
}


static void
test_hdy_dialer_clear_number(void)
{
  GtkWidget *dialer;

  notified = 0;
  dialer = hdy_dialer_new();
  g_signal_connect (dialer, "notify::number", G_CALLBACK (notify_cb), NULL);

  g_assert_cmpstr("", ==, hdy_dialer_get_number(HDY_DIALER (dialer)));
  hdy_dialer_clear_number (HDY_DIALER (dialer));
  g_assert_cmpint(0, ==, notified);

  hdy_dialer_set_number(HDY_DIALER (dialer), "#1234");
  g_assert_cmpstr("#1234", ==, hdy_dialer_get_number(HDY_DIALER (dialer)));
  g_assert_cmpint(1, ==, notified);
  hdy_dialer_clear_number (HDY_DIALER (dialer));
  g_assert_cmpint(2, ==, notified);
  hdy_dialer_clear_number (HDY_DIALER (dialer));
  g_assert_cmpint(2, ==, notified);
}


static void
test_hdy_dialer_action_buttions(void)
{
  HdyDialer *dialer = HDY_DIALER (hdy_dialer_new());
  gboolean val;

  notified = 0;
  g_signal_connect (dialer, "notify::show-action-buttons", G_CALLBACK (notify_cb), NULL);

  /* Getters/setters */
  g_assert_false(hdy_dialer_get_show_action_buttons (dialer));
  hdy_dialer_set_show_action_buttons (dialer, TRUE);
  g_assert_true(hdy_dialer_get_show_action_buttons (dialer));
  hdy_dialer_set_show_action_buttons (dialer, FALSE);
  g_assert_false(hdy_dialer_get_show_action_buttons (dialer));
  g_assert_cmpint(2, ==, notified);

  /* Property */
  g_object_set (dialer, "show-action-buttons", TRUE, NULL);
  g_object_get (dialer, "show-action-buttons", &val, NULL);
  g_assert_true(val);
  g_assert_cmpint(3, ==, notified);

  /* Setting the same value should not notify */
  hdy_dialer_set_show_action_buttons (dialer, TRUE);
  g_assert_cmpint(3, ==, notified);
}


gint
main (gint argc,
      gchar *argv[])
{
  gtk_test_init (&argc, &argv, NULL);
  hdy_init (&argc, &argv);

  g_test_add_func("/Handy/Dialer/setnumber", test_hdy_dialer_setnumber);
  g_test_add_func("/Handy/Dialer/clear_number", test_hdy_dialer_clear_number);
  g_test_add_func("/Handy/Dialer/action_buttons", test_hdy_dialer_action_buttions);
  g_test_add_func("/Handy/Dialer/relief", test_hdy_dialer_relief);
  return g_test_run();
}

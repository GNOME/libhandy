/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#define HANDY_USE_UNSTABLE_API
#include <handy.h>

static void
test_hdy_keypad_show_symbols (void)
{
  HdyKeypad *keypad = HDY_KEYPAD (hdy_keypad_new (TRUE));
  GList *l;
  GList *list;

  list = gtk_container_get_children (GTK_CONTAINER (keypad));

  for (l = list; l != NULL; l = l->next)
  {
    if (HDY_IS_KEYPAD_BUTTON(l->data)) {
      gboolean value;
      g_object_get (l->data, "show-symbols", &value, NULL);
      g_assert_false (value);
    }
  }
  g_list_free (list);
}


static void
test_hdy_keypad_set_actions (void)
{
  HdyKeypad *keypad = HDY_KEYPAD (hdy_keypad_new (FALSE));
  GtkWidget *button_right = gtk_button_new ();
  GtkWidget *button_left = gtk_button_new ();

  // Right extra button
  g_assert (gtk_grid_get_child_at (GTK_GRID (keypad), 2, 3) != NULL);
  // Left extra button
  g_assert (gtk_grid_get_child_at (GTK_GRID (keypad), 0, 3) != NULL);

  hdy_keypad_set_right_action (keypad, button_right);
  hdy_keypad_set_left_action (keypad, button_left);
  g_assert (button_right == gtk_grid_get_child_at (GTK_GRID (keypad), 2, 3));
  g_assert (button_left == gtk_grid_get_child_at (GTK_GRID (keypad), 0, 3));

  hdy_keypad_set_right_action (keypad, NULL);
  g_assert (gtk_grid_get_child_at (GTK_GRID (keypad), 2, 3) == NULL);

  hdy_keypad_set_left_action (keypad, NULL);
  g_assert (gtk_grid_get_child_at (GTK_GRID (keypad), 0, 3) == NULL);
}


gint
main (gint argc,
      gchar *argv[])
{
  gtk_test_init (&argc, &argv, NULL);
  hdy_init (&argc, &argv);

  g_test_add_func ("/Handy/Keypad/show_symbols", test_hdy_keypad_show_symbols);
  g_test_add_func ("/Handy/Keypad/set_actions", test_hdy_keypad_set_actions);

  return g_test_run ();
}

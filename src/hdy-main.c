/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */
#include "config.h"
#include "hdy-main.h"
#include <glib/gi18n.h>

static gint hdy_initialized = FALSE;

/**
 * SECTION:hdy-main
 * @Short_description: Library initialization
 * @Title: hdy-main
 *
 * Before using the Handy libarary you should initialize it. This makes
 * sure translations for the Handy library are set up properly.
 */

/**
 * hdy_init:
 *
 * Call this function before using any other Handy functions in your
 * GUI applications.
 *
 * Returns: %TRUE if initialization was successful, %FALSE otherwise.
 */
gboolean
hdy_init (void)
{
  if (hdy_initialized)
    return TRUE;

  textdomain (GETTEXT_PACKAGE);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);

  hdy_initialized = TRUE;

  return TRUE;
}

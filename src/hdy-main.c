/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */
#include "config.h"
#include "hdy-arrows.h"
#include "hdy-column.h"
#include "hdy-dialer-button.h"
#include "hdy-dialer-cycle-button.h"
#include "hdy-dialer.h"
#include "hdy-enums.h"
#include "hdy-header-group.h"
#include "hdy-leaflet.h"
#include "hdy-main.h"
#include "hdy-title-bar.h"
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
 * @argc: (inout): Address of the <parameter>argc</parameter> parameter of
 *     your main() function (or 0 if @argv is %NULL). This will be changed if
 *     any arguments were handled.
 * @argv: (array length=argc) (inout) (allow-none): Address of the
 *     <parameter>argv</parameter> parameter of main(), or %NULL. Any options
 *     understood by Handy are stripped before return.
 *
 * Call this function before using any other Handy functions in your
 * GUI applications.
 *
 * Returns: %TRUE if initialization was successful, %FALSE otherwise.
 */
gboolean
hdy_init (int *argc, char ***argv)
{
  if (hdy_initialized)
    return TRUE;

  textdomain (GETTEXT_PACKAGE);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);

  g_type_ensure (HDY_TYPE_ARROWS);
  g_type_ensure (HDY_TYPE_ARROWS_DIRECTION);
  g_type_ensure (HDY_TYPE_COLUMN);
  g_type_ensure (HDY_TYPE_DIALER);
  g_type_ensure (HDY_TYPE_DIALER_BUTTON);
  g_type_ensure (HDY_TYPE_DIALER_CYCLE_BUTTON);
  g_type_ensure (HDY_TYPE_FOLD);
  g_type_ensure (HDY_TYPE_HEADER_GROUP);
  g_type_ensure (HDY_TYPE_LEAFLET);
  g_type_ensure (HDY_TYPE_LEAFLET_CHILD_TRANSITION_TYPE);
  g_type_ensure (HDY_TYPE_LEAFLET_MODE_TRANSITION_TYPE);
  g_type_ensure (HDY_TYPE_TITLE_BAR);

  hdy_initialized = TRUE;

  return TRUE;
}

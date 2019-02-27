/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */
#include "config.h"
#include "hdy-main-private.h"
#include <glib/gi18n.h>
#include "gconstructor.h"

#if defined (G_HAS_CONSTRUCTORS)

#ifdef G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA
#pragma G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(hdy_init)
#endif
G_DEFINE_CONSTRUCTOR(hdy_init)

static gint hdy_initialized = FALSE;

static void
hdy_init (void)
{
  if (hdy_initialized)
    return;

  textdomain (GETTEXT_PACKAGE);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  hdy_init_public_types ();

  hdy_initialized = TRUE;
}

#else
# error Your platform/compiler is missing constructor support
#endif

/*
 * Copyright (C) 2018-2020 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */
#include "config.h"
#include "hdy-main-private.h"
#include <gio/gio.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include "gconstructorprivate.h"

/**
 * PRIVATE:hdy-main
 * @short_description: Library initialization.
 * @Title: hdy-main
 * @stability: Private
 *
 * Before using the Handy libarary you should initialize it. This makes
 * sure translations for the Handy library are set up properly.
 */

#if defined (G_HAS_CONSTRUCTORS)

#ifdef G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA
#pragma G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(hdy_constructor)
#endif
G_DEFINE_CONSTRUCTOR(hdy_constructor)

/* The style provider priority to use for libhandy widgets custom styling. It is
 * higher than themes and settings, allowing to override theme defaults, but
 * lower than applications and user provided styles, so application developers
 * can nonetheless apply custom styling on top of it. */
#define HDY_STYLE_PROVIDER_PRIORITY_OVERRIDE (GTK_STYLE_PROVIDER_PRIORITY_SETTINGS + 1)

#define HDY_THEMES_PATH "/sm/puri/handy/themes/"

static guint queued_update;

static inline gboolean
hdy_resource_exists (const gchar *resource_path)
{
  return g_resources_get_info (resource_path, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL, NULL, NULL);
}

static gchar *
hdy_themes_get_theme_name (gboolean *prefer_dark_theme)
{
  gchar *theme_name = NULL;
  char *p;

  g_assert (prefer_dark_theme);

  theme_name = g_strdup (g_getenv ("GTK_THEME"));

  if (theme_name == NULL) {
    g_object_get (gtk_settings_get_default (),
                  "gtk-theme-name", &theme_name,
                  "gtk-application-prefer-dark-theme", prefer_dark_theme,
                  NULL);

    return theme_name;
  }

  /* Theme variants are specified with the syntax
   * "<theme>:<variant>" e.g. "Adwaita:dark" */
  if (NULL != (p = strrchr (theme_name, ':'))) {
    *p = '\0';
    p++;
    *prefer_dark_theme = g_strcmp0 (p, "dark") == 0;
  }

  return theme_name;
}

static void
hdy_themes_update (GtkCssProvider *css_provider)
{
  g_autofree gchar *theme_name = NULL;
  g_autofree gchar *resource_path = NULL;
  gboolean prefer_dark_theme = FALSE;

  g_assert (GTK_IS_CSS_PROVIDER (css_provider));

  theme_name = hdy_themes_get_theme_name (&prefer_dark_theme);

  /* First check with full path to theme+variant */
  resource_path = g_strdup_printf (HDY_THEMES_PATH"%s%s.css",
                                   theme_name, prefer_dark_theme ? "-dark" : "");

  if (!hdy_resource_exists (resource_path)) {
    /* Now try without the theme variant */
    g_free (resource_path);
    resource_path = g_strdup_printf (HDY_THEMES_PATH"%s.css", theme_name);

    /* Now fallback to shared styling */
    if (!hdy_resource_exists (resource_path)) {
      g_free (resource_path);
      resource_path = g_strdup (HDY_THEMES_PATH"shared.css");

      if (!hdy_resource_exists (resource_path))
        return;
    }
  }

  gtk_css_provider_load_from_resource (css_provider, resource_path);
}

static gboolean
hdy_themes_do_update (GtkCssProvider *css_provider)
{
  g_assert (GTK_IS_CSS_PROVIDER (css_provider));

  queued_update = 0;
  hdy_themes_update (css_provider);

  return G_SOURCE_REMOVE;
}

static void
hdy_themes_queue_update (GtkCssProvider *css_provider)
{
  g_assert (GTK_IS_CSS_PROVIDER (css_provider));

  if (queued_update == 0)
    queued_update = g_idle_add_full (G_PRIORITY_LOW,
                                     (GSourceFunc) hdy_themes_do_update,
                                     css_provider,
                                     NULL);
}

/**
 * hdy_style_init:
 *
 * Initializes the style classes. This must be called once GTK has been
 * initialized.
 *
 * Since: 1.0
 */
static void
hdy_style_init (void)
{
  static volatile gsize guard = 0;
  g_autoptr (GtkCssProvider) css_provider = NULL;
  GtkSettings *settings;

  if (!g_once_init_enter (&guard))
    return;

  css_provider = gtk_css_provider_new ();
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             GTK_STYLE_PROVIDER (css_provider),
                                             HDY_STYLE_PROVIDER_PRIORITY_OVERRIDE);

  settings = gtk_settings_get_default ();
  g_signal_connect_swapped (settings,
                           "notify::gtk-theme-name",
                           G_CALLBACK (hdy_themes_queue_update),
                           css_provider);
  g_signal_connect_swapped (settings,
                           "notify::gtk-application-prefer-dark-theme",
                           G_CALLBACK (hdy_themes_queue_update),
                           css_provider);

  hdy_themes_update (css_provider);

  g_once_init_leave (&guard, 1);
}

static gboolean
init_style_cb (void)
{
  hdy_style_init ();

  return G_SOURCE_REMOVE;
}

/**
 * hdy_constructor:
 *
 * Automatically initializes libhandy.
 */
static void
hdy_constructor (void)
{
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  hdy_init_public_types ();

 /* Initializes the style when the main loop starts, which should be before any
  * window shows up but after GTK is initialized.
  */
  g_idle_add (G_SOURCE_FUNC (init_style_cb), NULL);
}

#else
# error Your platform/compiler is missing constructor support
#endif

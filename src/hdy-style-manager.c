/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Author: Alexander Mikhaylenko <alexander.mikhaylenko@puri.sm>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "hdy-style-manager.h"

#include "hdy-settings-private.h"
#include <gtk/gtk.h>

#define SWITCH_DURATION 250

/**
 * HdyColorScheme:
 * @HDY_COLOR_SCHEME_DEFAULT: Inherit the parent color-scheme. When set on the
 *   #HdyStyleManager returned by hdy_style_manager_get_default(), it's
 *   equivalent to %HDY_COLOR_SCHEME_FORCE_LIGHT.
 * @HDY_COLOR_SCHEME_FORCE_LIGHT: Always use light appearance.
 * @HDY_COLOR_SCHEME_PREFER_LIGHT: Use light appearance unless the system
 *   prefers dark colors.
 * @HDY_COLOR_SCHEME_PREFER_DARK: Use dark appearance unless the system prefers
 *   light colors.
 * @HDY_COLOR_SCHEME_FORCE_DARK: Always use dark appearance.
 *
 * Application color schemes for #HdyStyleManager:color-scheme.
 *
 * Since: 1.6
 */

/**
 * SECTION:hdy-style-manager
 * @short_description: A class for managing application-wide styling
 * @title: HdyStyleManager
 *
 * #HdyStyleManager provides a way to query and influence the application styles
 * such as whether to use dark or high contrast appearance.
 *
 * It allows to set the color scheme via the #HdyStyleManager:color-scheme
 * property, and to query the current appearance, as well as whether a
 * system-wide color scheme preference exists.
 *
 * Important: #GtkSettings:gtk-application-prefer-dark-theme should
 * not be used together with `HdyStyleManager` and will result in a warning.
 * Color schemes should be used instead.
 *
 * Since: 1.6
 */

struct _HdyStyleManager
{
  GObject parent_instance;

  GdkDisplay *display;
  HdySettings *settings;

  HdyColorScheme color_scheme;
  gboolean dark;

  guint animation_timeout_id;
};

G_DEFINE_TYPE (HdyStyleManager, hdy_style_manager, G_TYPE_OBJECT);

enum {
  PROP_0,
  PROP_DISPLAY,
  PROP_COLOR_SCHEME,
  PROP_SYSTEM_SUPPORTS_COLOR_SCHEMES,
  PROP_DARK,
  PROP_HIGH_CONTRAST,
  LAST_PROP,
};

static GParamSpec *props[LAST_PROP];

static GHashTable *display_style_managers = NULL;
static HdyStyleManager *default_instance = NULL;

static void
warn_prefer_dark_theme (HdyStyleManager *self)
{
  g_warning ("Using GtkSettings:gtk-application-prefer-dark-theme together "
             "with HdyStyleManager is unsupported. Please use "
             "HdyStyleManager:color-scheme instead.");
}

static void
unregister_display (GdkDisplay *display)
{
  g_assert (!g_hash_table_contains (display_style_managers, display));

  g_hash_table_remove (display_style_managers, display);
}

static void
register_display (GdkDisplayManager *display_manager,
                  GdkDisplay        *display)
{
  HdyStyleManager *style_manager;

  style_manager = g_object_new (HDY_TYPE_STYLE_MANAGER,
                                "display", display,
                                NULL);

  g_assert (!g_hash_table_contains (display_style_managers, display));

  g_hash_table_insert (display_style_managers, display, style_manager);

  g_signal_connect (display,
                    "closed",
                    G_CALLBACK (unregister_display),
                    NULL);
}

static char *
get_system_theme_name (void)
{
  GdkScreen *screen = gdk_screen_get_default ();
  g_auto (GValue) value = G_VALUE_INIT;

  g_value_init (&value, G_TYPE_STRING);
  if (!gdk_screen_get_setting (screen, "gtk-theme-name", &value))
    return g_strdup ("Adwaita");

  return g_value_dup_string (&value);
}

static gboolean
enable_animations_cb (HdyStyleManager *self)
{
  GdkScreen *screen = gdk_display_get_default_screen (self->display);

  g_object_set (gtk_settings_get_for_screen (screen),
                "gtk-enable-animations", TRUE,
                NULL);

  self->animation_timeout_id = 0;

  return G_SOURCE_REMOVE;
}

static void
update_stylesheet (HdyStyleManager *self)
{
  GdkScreen *screen;
  GtkSettings *gtk_settings;
  const char *theme_name;
  gboolean enable_animations;

  if (!self->display)
    return;

  screen = gdk_display_get_default_screen (self->display);
  gtk_settings = gtk_settings_get_for_screen (screen);

  if (hdy_settings_get_high_contrast (self->settings))
    theme_name = g_strdup (self->dark ? "HighContrastInverse" : "HighContrast");
  else
    theme_name = get_system_theme_name ();

  if (self->animation_timeout_id) {
    g_clear_handle_id (&self->animation_timeout_id, g_source_remove);
    enable_animations = TRUE;
  } else {
    g_object_get (gtk_settings,
                  "gtk-enable-animations", &enable_animations,
                  NULL);
  }

  g_signal_handlers_block_by_func (gtk_settings,
                                   G_CALLBACK (warn_prefer_dark_theme),
                                   self);

  g_object_set (gtk_settings,
                "gtk-enable-animations", FALSE,
                "gtk-theme-name", theme_name,
                "gtk-application-prefer-dark-theme", self->dark,
                NULL);

  g_signal_handlers_unblock_by_func (gtk_settings,
                                     G_CALLBACK (warn_prefer_dark_theme),
                                     self);

  if (enable_animations) {
    self->animation_timeout_id =
      g_timeout_add (SWITCH_DURATION,
                     G_SOURCE_FUNC (enable_animations_cb),
                     self);
  }
}

static inline gboolean
get_is_dark (HdyStyleManager *self)
{
  HdySystemColorScheme system_scheme = hdy_settings_get_color_scheme (self->settings);

  switch (self->color_scheme) {
  case HDY_COLOR_SCHEME_DEFAULT:
    if (self->display)
      return get_is_dark (default_instance);
    return FALSE;
  case HDY_COLOR_SCHEME_FORCE_LIGHT:
    return FALSE;
  case HDY_COLOR_SCHEME_PREFER_LIGHT:
    return system_scheme == HDY_SYSTEM_COLOR_SCHEME_PREFER_DARK;
  case HDY_COLOR_SCHEME_PREFER_DARK:
    return system_scheme != HDY_SYSTEM_COLOR_SCHEME_PREFER_LIGHT;
  case HDY_COLOR_SCHEME_FORCE_DARK:
    return TRUE;
  default:
    g_assert_not_reached ();
  }
}

static void
update_dark (HdyStyleManager *self)
{
  gboolean dark = get_is_dark (self);

  if (dark == self->dark)
    return;

  self->dark = dark;

  update_stylesheet (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_DARK]);
}

static void
notify_high_contrast_cb (HdyStyleManager *self)
{
  update_stylesheet (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_HIGH_CONTRAST]);
}

static void
hdy_style_manager_constructed (GObject *object)
{
  HdyStyleManager *self = HDY_STYLE_MANAGER (object);

  G_OBJECT_CLASS (hdy_style_manager_parent_class)->constructed (object);

  if (self->display) {
    GdkScreen *screen = gdk_display_get_default_screen (self->display);
    GtkSettings *settings = gtk_settings_get_for_screen (screen);
    gboolean prefer_dark_theme;

    g_object_get (settings,
                  "gtk-application-prefer-dark-theme", &prefer_dark_theme,
                  NULL);

    if (prefer_dark_theme)
      warn_prefer_dark_theme (self);

    g_signal_connect_object (settings,
                             "notify::gtk-application-prefer-dark-theme",
                             G_CALLBACK (warn_prefer_dark_theme),
                             self,
                             G_CONNECT_SWAPPED);
  }

  self->settings = hdy_settings_get_default ();

  g_signal_connect_object (self->settings,
                           "notify::color-scheme",
                           G_CALLBACK (update_dark),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->settings,
                           "notify::high-contrast",
                           G_CALLBACK (notify_high_contrast_cb),
                           self,
                           G_CONNECT_SWAPPED);

  update_dark (self);
  update_stylesheet (self);
}

static void
hdy_style_manager_dispose (GObject *object)
{
  HdyStyleManager *self = HDY_STYLE_MANAGER (object);

  g_clear_handle_id (&self->animation_timeout_id, g_source_remove);

  G_OBJECT_CLASS (hdy_style_manager_parent_class)->dispose (object);
}

static void
hdy_style_manager_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  HdyStyleManager *self = HDY_STYLE_MANAGER (object);

  switch (prop_id) {
  case PROP_DISPLAY:
    g_value_set_object (value, hdy_style_manager_get_display (self));
    break;

  case PROP_COLOR_SCHEME:
    g_value_set_enum (value, hdy_style_manager_get_color_scheme (self));
    break;

  case PROP_SYSTEM_SUPPORTS_COLOR_SCHEMES:
    g_value_set_boolean (value, hdy_style_manager_get_system_supports_color_schemes (self));
    break;

  case PROP_DARK:
    g_value_set_boolean (value, hdy_style_manager_get_dark (self));
    break;

    case PROP_HIGH_CONTRAST:
    g_value_set_boolean (value, hdy_style_manager_get_high_contrast (self));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
hdy_style_manager_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  HdyStyleManager *self = HDY_STYLE_MANAGER (object);

  switch (prop_id) {
  case PROP_DISPLAY:
    self->display = g_value_get_object (value);
    break;

  case PROP_COLOR_SCHEME:
    hdy_style_manager_set_color_scheme (self, g_value_get_enum (value));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
hdy_style_manager_class_init (HdyStyleManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = hdy_style_manager_constructed;
  object_class->dispose = hdy_style_manager_dispose;
  object_class->get_property = hdy_style_manager_get_property;
  object_class->set_property = hdy_style_manager_set_property;

  /**
   * HdyStyleManager:display:
   *
   * The display the style manager is associated with.
   *
   * The display will be %NULL for the style manager returned by
   * hdy_style_manager_get_default().
   *
   * Since: 1.6
   */
  props[PROP_DISPLAY] =
    g_param_spec_object ("display",
                         "Display",
                         "Display",
                         GDK_TYPE_DISPLAY,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  /**
   * HdyStyleManager:color-scheme:
   *
   * The requested application color scheme.
   *
   * The effective appearance will be decided based on the application color
   * scheme and the system preferred color scheme. The #HdyStyleManager:dark
   * property can be used to query the current effective appearance.
   *
   * The %HDY_COLOR_SCHEME_PREFER_LIGHT color scheme results in the application
   * using light appearance unless the system prefers dark colors. This is the
   * default value.
   *
   * The %HDY_COLOR_SCHEME_PREFER_DARK color scheme results in the application
   * using dark appearance, but can still switch to the light appearance if the
   * system can prefers it, for example, when the high contrast preference is
   * enabled.
   *
   * The %HDY_COLOR_SCHEME_FORCE_LIGHT and %HDY_COLOR_SCHEME_FORCE_DARK values
   * ignore the system preference entirely, they are useful if the application
   * wants to match its UI to its content or to provide a separate color scheme
   * switcher.
   *
   * If a per-#GdkDisplay style manager has its color scheme set to
   * %HDY_COLOR_SCHEME_DEFAULT, it will inherit the color scheme from the
   * default style manager.
   *
   * For the default style manager, %HDY_COLOR_SCHEME_DEFAULT is equivalent to
   * %HDY_COLOR_SCHEME_FORCE_LIGHT.
   *
   * The #HdyStyleManager:system-supports-color-schemes property can be used to
   * check if the current environment provides a color scheme dddpreference.
   *
   * Since: 1.6
   */
  props[PROP_COLOR_SCHEME] =
    g_param_spec_enum ("color-scheme",
                       _("Color Scheme"),
                       _("The current color scheme"),
                       HDY_TYPE_COLOR_SCHEME,
                       HDY_COLOR_SCHEME_DEFAULT,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * HdyStyleManager:system-supports-color-schemes:
   *
   * Whether the system supports color schemes.
   *
   * This property can be used to check if the current environment provides a
   * color scheme preference. For example, applications might want to show a
   * separate appearance switcher if it's set to %FALSE.
   *
   * It's only set at startup and cannot change its value later.
   *
   * See #HdyStyleManager:color-scheme.
   *
   * Since: 1.6
   */
  props[PROP_SYSTEM_SUPPORTS_COLOR_SCHEMES] =
    g_param_spec_boolean ("system-supports-color-schemes",
                          _("System supports color schemes"),
                          _("Whether the system supports color schemes"),
                          FALSE,
                          G_PARAM_READABLE);

  /**
   * HdyStyleManager:dark:
   *
   * Whether the application is using dark appearance.
   *
   * This property can be used to query the current appearance, as requested via
   * #HdyStyleManager:color-scheme.
   *
   * Since: 1.6
   */
  props[PROP_DARK] =
    g_param_spec_boolean ("dark",
                          _("Dark"),
                          _("Whether the application is using dark appearance"),
                          FALSE,
                          G_PARAM_READABLE);

  /**
   * HdyStyleManager:high-contrast:
   *
   * Whether the application is using high contrast appearance.
   *
   * This cannot be overridden by applications.
   *
   * Since: 1.6
   */
  props[PROP_HIGH_CONTRAST] =
    g_param_spec_boolean ("high-contrast",
                          _("High Contrast"),
                          _("Whether the application is using high contrast appearance"),
                          FALSE,
                          G_PARAM_READABLE);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}

static void
hdy_style_manager_init (HdyStyleManager *self)
{
  self->color_scheme = HDY_COLOR_SCHEME_DEFAULT;
}

static void
hdy_style_manager_ensure (void)
{
  GdkDisplayManager *display_manager = gdk_display_manager_get ();
  g_autoptr (GSList) displays = NULL;
  GSList *l;

  if (display_style_managers)
    return;

  default_instance = g_object_new (HDY_TYPE_STYLE_MANAGER, NULL);
  display_style_managers = g_hash_table_new_full (g_direct_hash,
                                                  g_direct_equal,
                                                  NULL,
                                                  g_object_unref);

  displays = gdk_display_manager_list_displays (display_manager);

  for (l = displays; l; l = l->next)
    register_display (display_manager, l->data);

  g_signal_connect (display_manager,
                    "display-opened",
                    G_CALLBACK (register_display),
                    NULL);
}

/**
 * hdy_style_manager_get_default:
 *
 * Gets the default #HdyStyleManager instance.
 *
 * It manages all #GdkDisplay instances unless the style manager for that
 * display has an override.
 *
 * See hdy_style_manager_get_for_display().
 *
 * Returns: (transfer none): the default style manager
 *
 * Since: 1.6
 */
HdyStyleManager *
hdy_style_manager_get_default (void)
{
  if (!default_instance)
    hdy_style_manager_ensure ();

  return default_instance;
}

/**
 * hdy_style_manager_get_for_display:
 * @display: a #GdkDisplay
 *
 * Gets the #HdyStyleManager instance managing @display.
 *
 * It can be used to override styles for that specific display instead of the
 * whole application.
 *
 * Most applications should use hdy_style_manager_get_default() instead.
 *
 * Returns: (transfer none): the style manager for @display
 *
 * Since: 1.6
 */
HdyStyleManager *
hdy_style_manager_get_for_display (GdkDisplay *display)
{
  g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);

  if (!display_style_managers)
    hdy_style_manager_ensure ();

  g_return_val_if_fail (g_hash_table_contains (display_style_managers, display), NULL);

  return g_hash_table_lookup (display_style_managers, display);
}

/**
 * hdy_style_manager_get_display:
 * @self: a #HdyStyleManager
 *
 * Gets the display the style manager is associated with.
 *
 * The display will be %NULL for the style manager returned by
 * hdy_style_manager_get_default().
 *
 * Returns: (transfer none): (nullable): the display
 *
 * Since: 1.6
 */
GdkDisplay *
hdy_style_manager_get_display (HdyStyleManager *self)
{
  g_return_val_if_fail (HDY_IS_STYLE_MANAGER (self), NULL);

  return self->display;
}

/**
 * hdy_style_manager_get_color_scheme:
 * @self: a #HdyStyleManager
 *
 * Gets the requested application color scheme.
 *
 * Returns: the color scheme
 *
 * Since: 1.6
 */
HdyColorScheme
hdy_style_manager_get_color_scheme (HdyStyleManager *self)
{
  g_return_val_if_fail (HDY_IS_STYLE_MANAGER (self), HDY_COLOR_SCHEME_DEFAULT);

  return self->color_scheme;
}

/**
 * hdy_style_manager_set_color_scheme:
 * @self: a #HdyStyleManager
 * @color_scheme: the color scheme
 *
 * Sets the requested application color scheme.
 *
 * The effective appearance will be decided based on the application color
 * scheme and the system preferred color scheme. The #HdyStyleManager:dark
 * property can be used to query the current effective appearance.
 *
 * Since: 1.6
 */
void
hdy_style_manager_set_color_scheme (HdyStyleManager *self,
                                    HdyColorScheme   color_scheme)
{
  g_return_if_fail (HDY_IS_STYLE_MANAGER (self));

  if (color_scheme == self->color_scheme)
    return;

  self->color_scheme = color_scheme;

  g_object_freeze_notify (G_OBJECT (self));

  update_dark (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_COLOR_SCHEME]);

  g_object_thaw_notify (G_OBJECT (self));

  if (!self->display) {
    GHashTableIter iter;
    HdyStyleManager *manager;

    g_hash_table_iter_init (&iter, display_style_managers);

    while (g_hash_table_iter_next (&iter, NULL, (gpointer) &manager))
      if (manager->color_scheme == HDY_COLOR_SCHEME_DEFAULT)
        update_dark (manager);
  }
}

/**
 * hdy_style_manager_get_system_supports_color_schemes:
 * @self: a #HdyStyleManager
 *
 * Gets whether the system supports color schemes.
 *
 * Returns: whether the system supports color schemes
 *
 * Since: 1.6
 */
gboolean
hdy_style_manager_get_system_supports_color_schemes (HdyStyleManager *self)
{
  g_return_val_if_fail (HDY_IS_STYLE_MANAGER (self), FALSE);

  return hdy_settings_has_color_scheme (self->settings);
}

/**
 * hdy_style_manager_get_dark:
 * @self: a #HdyStyleManager
 *
 * Gets whether the application is using dark appearance.
 *
 * Returns: whether the application is using dark appearance
 *
 * Since: 1.6
 */
gboolean
hdy_style_manager_get_high_contrast (HdyStyleManager *self)
{
  g_return_val_if_fail (HDY_IS_STYLE_MANAGER (self), FALSE);

  return hdy_settings_get_high_contrast (self->settings);
}

/**
 * hdy_style_manager_get_high_contrast:
 * @self: a #HdyStyleManager
 *
 * Gets whether the application is using high contrast appearance.
 *
 * Returns: whether the application is using high contrast appearance
 *
 * Since: 1.6
 */
gboolean
hdy_style_manager_get_dark (HdyStyleManager *self)
{
  g_return_val_if_fail (HDY_IS_STYLE_MANAGER (self), FALSE);

  return self->dark;
}

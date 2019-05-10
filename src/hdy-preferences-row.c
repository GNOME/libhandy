/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "hdy-preferences-row.h"

/**
 * SECTION:hdy-preferences-row
 * @short_description: A #GtkListBox row used to present preferencess
 * @Title: HdyPreferencesRow
 *
 * The #HdyPreferencesRow widget can have a title, a subtitle and an icon. The row
 * can receive preferences widgets at its end, prefix widgets at its start or widgets
 * below it.
 *
 * Note that preferences widgets are packed starting from the end.
 *
 * It is convenient to present a list of preferences and their related preferencess.
 *
 * # HdyPreferencesRow as GtkBuildable
 *
 * The GtkWindow implementation of the GtkBuildable interface supports setting a
 * child as an preferences widget by specifying “preferences” as the “type” attribute of a
 * &lt;child&gt; element.
 *
 * It also supports setting a child as a prefix widget by specifying “prefix” as
 * the “type” attribute of a &lt;child&gt; element.
 *
 * Since: 0.1.0
 */

typedef struct
{
  gchar *title;

  gboolean use_underline;
} HdyPreferencesRowPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (HdyPreferencesRow, hdy_preferences_row, GTK_TYPE_LIST_BOX_ROW)

enum {
  PROP_0,
  PROP_TITLE,
  PROP_USE_UNDERLINE,
  LAST_PROP,
};

static GParamSpec *props[LAST_PROP];

static void
hdy_preferences_row_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  HdyPreferencesRow *self = HDY_PREFERENCES_ROW (object);

  switch (prop_id) {
  case PROP_TITLE:
    g_value_set_string (value, hdy_preferences_row_get_title (self));
    break;
  case PROP_USE_UNDERLINE:
    g_value_set_boolean (value, hdy_preferences_row_get_use_underline (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
hdy_preferences_row_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  HdyPreferencesRow *self = HDY_PREFERENCES_ROW (object);

  switch (prop_id) {
  case PROP_TITLE:
    hdy_preferences_row_set_title (self, g_value_get_string (value));
    break;
  case PROP_USE_UNDERLINE:
    hdy_preferences_row_set_use_underline (self, g_value_get_boolean (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
hdy_preferences_row_finalize (GObject *object)
{
  HdyPreferencesRow *self = HDY_PREFERENCES_ROW (object);
  HdyPreferencesRowPrivate *priv = hdy_preferences_row_get_instance_private (self);

  g_free (priv->title);

  G_OBJECT_CLASS (hdy_preferences_row_parent_class)->finalize (object);
}

static void
hdy_preferences_row_class_init (HdyPreferencesRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = hdy_preferences_row_get_property;
  object_class->set_property = hdy_preferences_row_set_property;
  object_class->finalize = hdy_preferences_row_finalize;

  /**
   * HdyPreferencesRow:title:
   *
   * The title for this row.
   *
   * Since: 0.1.0
   */
  props[PROP_TITLE] =
    g_param_spec_string ("title",
                         _("Title"),
                         _("Title"),
                         "",
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * HdyPreferencesRow:use-underline:
   *
   * Whether an embedded underline in the text of the title indicates a
   * mnemonic.
   *
   * Since: 0.1.0
   */
  props[PROP_USE_UNDERLINE] =
    g_param_spec_boolean ("use-underline",
                          _("Use underline"),
                          _("If set, an underline in the text indicates the next character should be used for the mnemonic accelerator key"),
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}

static void
hdy_preferences_row_init (HdyPreferencesRow *self)
{
}

/**
 * hdy_preferences_row_new:
 *
 * Creates a new #HdyPreferencesRow.
 *
 * Returns: a new #HdyPreferencesRow
 *
 * Since: 0.1.0
 */
HdyPreferencesRow *
hdy_preferences_row_new (void)
{
  return g_object_new (HDY_TYPE_PREFERENCES_ROW, NULL);
}

/**
 * hdy_preferences_row_get_title:
 * @self: a #HdyPreferencesRow
 *
 * Gets the title for @self.
 *
 * Returns: the title for @self.
 *
 * Since: 0.1.0
 */
const gchar *
hdy_preferences_row_get_title (HdyPreferencesRow *self)
{
  HdyPreferencesRowPrivate *priv;

  g_return_val_if_fail (HDY_IS_PREFERENCES_ROW (self), NULL);

  priv = hdy_preferences_row_get_instance_private (self);

  return priv->title;
}

/**
 * hdy_preferences_row_set_title:
 * @self: a #HdyPreferencesRow
 * @title: the title
 *
 * Sets the title for @self.
 *
 * Since: 0.1.0
 */
void
hdy_preferences_row_set_title (HdyPreferencesRow *self,
                               const gchar       *title)
{
  HdyPreferencesRowPrivate *priv;

  g_return_if_fail (HDY_IS_PREFERENCES_ROW (self));

  priv = hdy_preferences_row_get_instance_private (self);

  if (g_strcmp0 (priv->title, title) == 0)
    return;

  g_free (priv->title);
  priv->title = g_strdup (title);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TITLE]);
}

/**
 * hdy_preferences_row_get_use_underline:
 * @self: a #HdyPreferencesRow
 *
 * Gets whether an embedded underline in the text of the title indicates a
 * mnemonic. See hdy_preferences_row_set_use_underline().
 *
 * Returns: %TRUE if an embedded underline in the title indicates the mnemonic
 *          accelerator keys.
 *
 * Since: 0.1.0
 */
gboolean
hdy_preferences_row_get_use_underline (HdyPreferencesRow *self)
{
  HdyPreferencesRowPrivate *priv;

  g_return_val_if_fail (HDY_IS_PREFERENCES_ROW (self), FALSE);

  priv = hdy_preferences_row_get_instance_private (self);

  return priv->use_underline;
}

/**
 * hdy_preferences_row_set_use_underline:
 * @self: a #HdyPreferencesRow
 * @use_underline: %TRUE if underlines in the text indicate mnemonics
 *
 * If true, an underline in the text of the title indicates the next character
 * should be used for the mnemonic accelerator key.
 *
 * Since: 0.1.0
 */
void
hdy_preferences_row_set_use_underline (HdyPreferencesRow *self,
                                       gboolean           use_underline)
{
  HdyPreferencesRowPrivate *priv;

  g_return_if_fail (HDY_IS_PREFERENCES_ROW (self));

  priv = hdy_preferences_row_get_instance_private (self);

  if (priv->use_underline == !!use_underline)
    return;

  priv->use_underline = !!use_underline;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_USE_UNDERLINE]);
}

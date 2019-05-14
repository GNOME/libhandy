/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#if !defined(_HANDY_INSIDE) && !defined(HANDY_COMPILATION)
#error "Only <handy.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define HDY_TYPE_PREFERENCES_GROUP (hdy_preferences_group_get_type())

G_DECLARE_DERIVABLE_TYPE (HdyPreferencesGroup, hdy_preferences_group, HDY, PREFERENCES_GROUP, GtkBox)

/**
 * HdyPreferencesGroupClass
 * @parent_class: The parent class
 */
struct _HdyPreferencesGroupClass
{
  GtkBoxClass parent_class;
};

HdyPreferencesGroup *hdy_preferences_group_new (void);

const gchar *hdy_preferences_group_get_title (HdyPreferencesGroup *self);
void         hdy_preferences_group_set_title (HdyPreferencesGroup *self,
                                              const gchar         *title);

const gchar *hdy_preferences_group_get_description (HdyPreferencesGroup *self);
void         hdy_preferences_group_set_description (HdyPreferencesGroup *self,
                                                    const gchar         *description);

G_END_DECLS

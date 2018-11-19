/*
 * Copyright © 2018 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include <glib/gi18n.h>

#include "hdy-dialog.h"

/**
 * SECTION:hdy-dialog
 * @short_description: An adaptive dialog
 * @Title: HdyDialog
 */

/* Point at which we switch to mobile view */
#define SNAP_POINT 500

typedef struct {
  GtkWindow *parent;
  gulong     size_handler;
  gint       old_width, old_height;
} HdyDialogPrivate;

static void hdy_dialog_buildable_init (GtkBuildableIface  *iface);

G_DEFINE_TYPE_WITH_CODE (HdyDialog, hdy_dialog, GTK_TYPE_DIALOG,
                         G_ADD_PRIVATE (HdyDialog)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, hdy_dialog_buildable_init))

static gboolean
hdy_dialog_delete_event (GtkWidget   *widget,
                         GdkEventAny *event)
{
  HdyDialog        *self = HDY_DIALOG (widget);
  HdyDialogPrivate *priv = hdy_dialog_get_instance_private (self);

  /* If we had a parent disconnect from it */
  if (priv->parent) {
    g_signal_handler_disconnect (G_OBJECT (priv->parent), priv->size_handler);
  }

  if (GTK_WIDGET_CLASS (hdy_dialog_parent_class)->delete_event) {
    return GTK_WIDGET_CLASS (hdy_dialog_parent_class)->delete_event (widget,
                                                                     event);
  }
  return FALSE;
}

/* <= 3.24.1 never actually emits notify::transient-for so 
 * we have this hacky workaround */
#if !GTK_CHECK_VERSION(3, 24, 2)

enum {
  PROP_0,
  /* Wrap the property on GtkWindow */
  PROP_TRANSIENT_FOR,
  LAST_PROP = PROP_TRANSIENT_FOR,
};

static void
hdy_dialog_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  HdyDialog *self = HDY_DIALOG (object);

  switch (prop_id) {
  case PROP_TRANSIENT_FOR:
    g_value_set_object (value, gtk_window_get_transient_for (GTK_WINDOW (self)));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
hdy_dialog_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  HdyDialog *self = HDY_DIALOG (object);

  switch (prop_id) {
  case PROP_TRANSIENT_FOR:
    gtk_window_set_transient_for (GTK_WINDOW (self), g_value_get_object (value));
    g_object_notify (G_OBJECT (self), "transient-for");
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
hdy_dialog_class_init (HdyDialogClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = hdy_dialog_get_property;
  object_class->set_property = hdy_dialog_set_property;

  widget_class->delete_event = hdy_dialog_delete_event;

  g_object_class_override_property (object_class,
                                    PROP_TRANSIENT_FOR,
                                    "transient-for");
}

#else

static void
hdy_dialog_class_init (HdyDialogClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->delete_event = hdy_dialog_delete_event;
}

#endif

static void
size_cb (GtkWidget    *widget,
         GdkRectangle *allocation,
         gpointer      user_data)
{
  HdyDialog        *self = HDY_DIALOG (user_data);
  HdyDialogPrivate *priv = hdy_dialog_get_instance_private (self);
  gint              width, height;

  /* We expect to have two windows */
  g_return_if_fail (GTK_IS_WINDOW (widget));
  g_return_if_fail (GTK_IS_WINDOW (self));

  /* Get the size of the parent */
  gtk_window_get_size (GTK_WINDOW (widget), &width, &height);

  /* When we are below the snap point */
  if (width < SNAP_POINT) {
    /* When no size is cached, cache the current size */
    if (!priv->old_width || !priv->old_height) {
      gtk_window_get_size (GTK_WINDOW (self), &priv->old_width, &priv->old_height);
    }
    /* Resize the dialog to match the parent */
    gtk_window_resize (GTK_WINDOW (self), width, height);
  } else if (priv->old_width || priv->old_height) {
    g_message ("Size!");
    /* Restore the cached size */
    gtk_window_resize (GTK_WINDOW (self), priv->old_width, priv->old_height);
    /* Clear cached size */
    priv->old_width = 0;
    priv->old_height = 0;
  }
}

static void
transient_cb (GObject    *object,
              GParamSpec *pspec,
              gpointer    user_data)
{
  HdyDialog        *self = HDY_DIALOG (object);
  HdyDialogPrivate *priv = hdy_dialog_get_instance_private (self);

  /* If we are being reparented disconnect from the old one */
  if (priv->parent) {
    g_signal_handler_disconnect (G_OBJECT (priv->parent), priv->size_handler);
  }

  /* Get the dialogs new parent */
  priv->parent = gtk_window_get_transient_for (GTK_WINDOW (self));

  /* Check we actually have a parent */
  if (priv->parent) {
    /* Listen for the parent resizing */
    priv->size_handler = g_signal_connect (G_OBJECT (priv->parent),
                                           "size-allocate",
                                           G_CALLBACK (size_cb),
                                           self);
  }
}

static void
hdy_dialog_init (HdyDialog *self)
{
  HdyDialogPrivate *priv = hdy_dialog_get_instance_private (self);

  priv->parent = NULL;
  priv->size_handler = 0;

  priv->old_width = 0;
  priv->old_height = 0;

  g_signal_connect (G_OBJECT (self), "notify::transient-for",
                    G_CALLBACK (transient_cb), NULL);

  g_object_set (G_OBJECT (self),
                "destroy-with-parent", TRUE,
                NULL);
}

static void
hdy_dialog_buildable_init (GtkBuildableIface *iface)
{
}

GtkWidget *
hdy_dialog_new (GtkWindow *parent)
{
  return g_object_new (HDY_TYPE_DIALOG,
                       "transient-for", parent,
                       "modal", TRUE,
                       NULL);
}

/*
 * Copyright (C) 2021 Purism SPC
 *
 * Authors:
 * Julian Sparber <julian@sparber.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "config.h"

#include "hdy-avatar-icon-private.h"

G_DEFINE_QUARK (hdy-avatar-icon-error-quark, hdy_avatar_icon_error)

struct _HdyAvatarIcon
{
  GObject parent_instance;

  HdyAvatarImageLoadFunc load_image_func;
  gpointer load_image_func_target;
  GDestroyNotify load_image_func_target_destroy_notify;
};

static void hdy_avatar_icon_loadable_icon_iface_init (GLoadableIconIface *iface);

G_DEFINE_TYPE_WITH_CODE (HdyAvatarIcon, hdy_avatar_icon, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_ICON,
                                                NULL)
                         G_IMPLEMENT_INTERFACE (G_TYPE_LOADABLE_ICON,
                                                hdy_avatar_icon_loadable_icon_iface_init));

static void
hdy_avatar_icon_finalize (GObject *object)
{
  HdyAvatarIcon *self = HDY_AVATAR_ICON (object);

  if (self->load_image_func_target_destroy_notify != NULL)
    self->load_image_func_target_destroy_notify (self->load_image_func_target);

  G_OBJECT_CLASS (hdy_avatar_icon_parent_class)->finalize (object);
}

static void
hdy_avatar_icon_class_init (HdyAvatarIconClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = hdy_avatar_icon_finalize;
}

static void
hdy_avatar_icon_init (HdyAvatarIcon *file)
{
}

HdyAvatarIcon *
hdy_avatar_icon_new (HdyAvatarImageLoadFunc load_image,
                     gpointer               user_data,
                     GDestroyNotify         destroy)
{
  HdyAvatarIcon *self;

  g_return_val_if_fail (user_data != NULL || (user_data == NULL && destroy == NULL), NULL);

  self = g_object_new (HDY_TYPE_AVATAR_ICON, NULL);

  self->load_image_func = load_image;
  self->load_image_func_target = user_data;
  self->load_image_func_target_destroy_notify = destroy;

  return self;
}

static void
load_pixbuf_cb (GObject      *source_object,
                GAsyncResult *res,
                gpointer      data)
{
  GTask *task = G_TASK (data);
  GError *error = NULL;
  GInputStream *stream;

  if (!g_task_return_error_if_cancelled (task)) {
    stream = g_loadable_icon_load_finish (G_LOADABLE_ICON (source_object), res, NULL, &error);

    if (stream != NULL)
      g_task_return_pointer (task, stream, g_object_unref);
    else
      g_task_return_error (task, error);
  }

  g_object_unref (task);
}

static void
hdy_avatar_icon_load_async (GLoadableIcon       *icon,
                            int                  size,
                            GCancellable        *cancellable,
                            GAsyncReadyCallback  callback,
                            gpointer             user_data)
{
  HdyAvatarIcon *self;
  GTask *task;
  GdkPixbuf *pixbuf = NULL;
  g_return_if_fail (HDY_IS_AVATAR_ICON (icon));

  self = HDY_AVATAR_ICON (icon);

  task = g_task_new (self,
                     cancellable,
                     callback,
                     user_data);

  if (self->load_image_func)
    pixbuf = self->load_image_func (size, self->load_image_func_target);

  if (pixbuf) {
    g_loadable_icon_load_async (G_LOADABLE_ICON (pixbuf),
                                size,
                                cancellable,
                                load_pixbuf_cb,
                                task);

    g_object_unref (pixbuf);
  } else {
    g_task_return_new_error (task,
                             HDY_AVATAR_ICON_ERROR,
                             HDY_AVATAR_ICON_ERROR_EMPTY,
                             "No pixbuf set");

    g_object_unref (task);
  }
}

static GInputStream *
hdy_avatar_icon_load_finish (GLoadableIcon  *icon,
                             GAsyncResult   *res,
                             char          **type,
                             GError        **error)
{
  g_return_val_if_fail (g_task_is_valid (res, icon), NULL);

  return g_task_propagate_pointer (G_TASK (res), error);
}

static void
hdy_avatar_icon_loadable_icon_iface_init (GLoadableIconIface *iface)
{
  iface->load_async = hdy_avatar_icon_load_async;
  iface->load_finish = hdy_avatar_icon_load_finish;
}

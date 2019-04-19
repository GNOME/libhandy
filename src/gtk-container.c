/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

/* Bits taken from GTK 3.24 and tweaked to be used by libhandy. */

#include "gtk-container-private.h"

static void
union_with_clip (GtkWidget *widget,
                 gpointer   data)
{
  GdkRectangle *clip = data;
  GtkAllocation widget_clip;

  if (!gtk_widget_is_visible (widget) ||
      !gtk_widget_get_child_visible (widget))
    return;

  gtk_widget_get_clip (widget, &widget_clip);

  if (clip->width == 0 || clip->height == 0)
    *clip = widget_clip;
  else
    gdk_rectangle_union (&widget_clip, clip, clip);
}

void
gtk_container_get_children_clip (GtkContainer  *container,
                                 GtkAllocation *out_clip)
{
  memset (out_clip, 0, sizeof (GtkAllocation));

  gtk_container_forall (container, union_with_clip, out_clip);
}

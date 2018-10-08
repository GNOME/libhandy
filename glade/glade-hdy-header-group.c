/*
 * glade-gtk-size-group.c - GladeWidgetAdaptor for GtkSizeGroup
 *
 * Copyright (C) 2013 Tristan Van Berkom
 * Copyright (C) 2018 Purism SPC
 *
 * Authors:
 *     Tristan Van Berkom <tristan.van.berkom@gmail.com>
 *     Adrien Plazas <adrien.plazas@puri.sm>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/*
 * This has been copied and modified from glade-gtk-size-group.c from Glade's
 * GTK+ plugin.
 */

#include <config.h>
#include <glib/gi18n-lib.h>
#include <gladeui/glade.h>
#define HANDY_USE_UNSTABLE_API
#include <handy.h>

#define GLADE_TAG_HEADERGROUP_HEADERBARS "headerbars"
#define GLADE_TAG_HEADERGROUP_HEADERBAR  "headerbar"

void glade_hdy_header_group_read_widget (GladeWidgetAdaptor *adaptor,
                                         GladeWidget        *widget,
                                         GladeXmlNode       *node);
void glade_hdy_header_group_write_widget (GladeWidgetAdaptor *adaptor,
                                          GladeWidget        *widget,
                                          GladeXmlContext    *context,
                                          GladeXmlNode       *node);
void glade_hdy_header_group_set_property (GladeWidgetAdaptor *adaptor,
                                          GObject            *object,
                                          const gchar        *property_name,
                                          const GValue       *value);

static void
glade_hdy_header_group_read_widgets (GladeWidget  *widget,
                                     GladeXmlNode *node)
{
  GladeXmlNode *widgets_node;
  GladeProperty *property;
  gchar *string = NULL;

  if ((widgets_node =
       glade_xml_search_child (node, GLADE_TAG_HEADERGROUP_HEADERBARS)) != NULL) {
    GladeXmlNode *node;

    for (node = glade_xml_node_get_children (widgets_node);
         node; node = glade_xml_node_next (node)) {
      gchar *widget_name, *tmp;

      if (!glade_xml_node_verify (node, GLADE_TAG_HEADERGROUP_HEADERBAR))
        continue;

      widget_name = glade_xml_get_property_string_required
          (node, GLADE_TAG_NAME, NULL);

      if (string == NULL)
        string = widget_name;
      else if (widget_name != NULL) {
        tmp = g_strdup_printf ("%s%s%s", string, GPC_OBJECT_DELIMITER,
                               widget_name);
        string = (g_free (string), tmp);
        g_free (widget_name);
      }
    }
  }

  if (string) {
    property = glade_widget_get_property (widget, "headerbars");
    g_assert (property);

    /* we must synchronize this directly after loading this project
     * (i.e. lookup the actual objects after they've been parsed and
     * are present).
     */
    g_object_set_data_full (G_OBJECT (property),
                            "glade-loaded-object", string, g_free);
  }
}

void
glade_hdy_header_group_read_widget (GladeWidgetAdaptor *adaptor,
                                    GladeWidget        *widget,
                                    GladeXmlNode       *node)
{
  if (!(glade_xml_node_verify_silent (node, GLADE_XML_TAG_WIDGET) ||
        glade_xml_node_verify_silent (node, GLADE_XML_TAG_TEMPLATE)))
    return;

  /* First chain up and read in all the normal properties.. */
  GWA_GET_CLASS (G_TYPE_OBJECT)->read_widget (adaptor, widget, node);

  glade_hdy_header_group_read_widgets (widget, node);
}


static void
glade_hdy_header_group_write_widgets (GladeWidget     *widget,
                                      GladeXmlContext *context,
                                      GladeXmlNode    *node)
{
  GladeXmlNode *widgets_node, *widget_node;
  GList *widgets = NULL, *list;
  GladeWidget *awidget;

  widgets_node = glade_xml_node_new (context, GLADE_TAG_HEADERGROUP_HEADERBARS);

  if (glade_widget_property_get (widget, "headerbars", &widgets))
    for (list = widgets; list; list = list->next) {
      awidget = glade_widget_get_from_gobject (list->data);
      widget_node =
          glade_xml_node_new (context, GLADE_TAG_HEADERGROUP_HEADERBAR);
      glade_xml_node_append_child (widgets_node, widget_node);
      glade_xml_node_set_property_string (widget_node, GLADE_TAG_NAME,
                                          glade_widget_get_name (awidget));
    }

  if (!glade_xml_node_get_children (widgets_node))
    glade_xml_node_delete (widgets_node);
  else
    glade_xml_node_append_child (node, widgets_node);
}


void
glade_hdy_header_group_write_widget (GladeWidgetAdaptor *adaptor,
                                     GladeWidget        *widget,
                                     GladeXmlContext    *context,
                                     GladeXmlNode       *node)
{
  if (!(glade_xml_node_verify_silent (node, GLADE_XML_TAG_WIDGET) ||
        glade_xml_node_verify_silent (node, GLADE_XML_TAG_TEMPLATE)))
    return;

  /* First chain up and read in all the normal properties.. */
  GWA_GET_CLASS (G_TYPE_OBJECT)->write_widget (adaptor, widget, context, node);

  glade_hdy_header_group_write_widgets (widget, context, node);
}


void
glade_hdy_header_group_set_property (GladeWidgetAdaptor *adaptor,
                                     GObject            *object,
                                     const gchar        *property_name,
                                     const GValue       *value)
{
  if (!strcmp (property_name, "headerbars")) {
    GSList *sg_widgets, *slist;
    GList *widgets, *list;

    /* remove old widgets */
    if ((sg_widgets =
         hdy_header_group_get_header_bars (HDY_HEADER_GROUP (object))) != NULL) {
      /* copy since we are modifying an internal list */
      sg_widgets = g_slist_copy (sg_widgets);
      for (slist = sg_widgets; slist; slist = slist->next)
        hdy_header_group_remove_header_bar (HDY_HEADER_GROUP (object),
                                            GTK_HEADER_BAR (slist->data));
      g_slist_free (sg_widgets);
    }

    /* add new widgets */
    if ((widgets = g_value_get_boxed (value)) != NULL) {
      for (list = widgets; list; list = list->next)
        hdy_header_group_add_header_bar (HDY_HEADER_GROUP (object),
                                         GTK_HEADER_BAR (list->data));
    }
  } else {
    GWA_GET_CLASS (G_TYPE_OBJECT)->set_property (adaptor, object,
                                                 property_name, value);
  }
}

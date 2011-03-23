
/*
 * gedit-file-browser-message-add-context-item.c
 * This file is part of gedit
 *
 * Copyright (C) 2011 - Jesse van den Kieboom
 *
 * gedit is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * gedit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gedit; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "gedit-file-browser-message-add-context-item.h"
#include <gtk/gtk.h>

#define GEDIT_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM, GeditFileBrowserMessageAddContextItemPrivate))

enum
{
	PROP_0,

	PROP_ACTION,
	PROP_PATH,
	PROP_ID,
};

struct _GeditFileBrowserMessageAddContextItemPrivate
{
	GtkAction *action;
	gchar *path;
	guint id;
};

G_DEFINE_TYPE (GeditFileBrowserMessageAddContextItem, gedit_file_browser_message_add_context_item, GEDIT_TYPE_MESSAGE)

static void
gedit_file_browser_message_add_context_item_finalize (GObject *obj)
{
	GeditFileBrowserMessageAddContextItem *msg = GEDIT_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM (obj);

	if (msg->priv->action)
	{
		g_object_unref (msg->priv->action);
	}
	g_free (msg->priv->path);

	G_OBJECT_CLASS (gedit_file_browser_message_add_context_item_parent_class)->finalize (obj);
}

static void
gedit_file_browser_message_add_context_item_get_property (GObject    *obj,
                                                          guint       prop_id,
                                                          GValue     *value,
                                                          GParamSpec *pspec)
{
	GeditFileBrowserMessageAddContextItem *msg;

	msg = GEDIT_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM (obj);

	switch (prop_id)
	{
		case PROP_ACTION:
			g_value_set_object (value, msg->priv->action);
			break;
		case PROP_PATH:
			g_value_set_string (value, msg->priv->path);
			break;
		case PROP_ID:
			g_value_set_uint (value, msg->priv->id);
			break;
	}
}

static void
gedit_file_browser_message_add_context_item_set_property (GObject      *obj,
                                                          guint         prop_id,
                                                          GValue const *value,
                                                          GParamSpec   *pspec)
{
	GeditFileBrowserMessageAddContextItem *msg;

	msg = GEDIT_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM (obj);

	switch (prop_id)
	{
		case PROP_ACTION:
		{
			if (msg->priv->action)
			{
				g_object_unref (msg->priv->action);
			}
			msg->priv->action = g_value_dup_object (value);
			break;
		}
		case PROP_PATH:
		{
			g_free (msg->priv->path);
			msg->priv->path = g_value_dup_string (value);
			break;
		}
		case PROP_ID:
			msg->priv->id = g_value_get_uint (value);
			break;
	}
}

static void
gedit_file_browser_message_add_context_item_class_init (GeditFileBrowserMessageAddContextItemClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = gedit_file_browser_message_add_context_item_finalize;

	object_class->get_property = gedit_file_browser_message_add_context_item_get_property;
	object_class->set_property = gedit_file_browser_message_add_context_item_set_property;

	g_object_class_install_property (object_class,
	                                 PROP_ACTION,
	                                 g_param_spec_object ("action",
	                                                      "Action",
	                                                      "Action",
	                                                      GTK_TYPE_ACTION,
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_CONSTRUCT |
	                                                      G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
	                                 PROP_PATH,
	                                 g_param_spec_string ("path",
	                                                      "Path",
	                                                      "Path",
	                                                      NULL,
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_CONSTRUCT |
	                                                      G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
	                                 PROP_ID,
	                                 g_param_spec_uint ("id",
	                                                    "Id",
	                                                    "Id",
	                                                    0,
	                                                    G_MAXUINT,
	                                                    0,
	                                                    G_PARAM_READWRITE |
	                                                    G_PARAM_CONSTRUCT |
	                                                    G_PARAM_STATIC_STRINGS));

	g_type_class_add_private (object_class, sizeof (GeditFileBrowserMessageAddContextItemPrivate));
}

static void
gedit_file_browser_message_add_context_item_init (GeditFileBrowserMessageAddContextItem *message)
{
	message->priv = GEDIT_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM_GET_PRIVATE (message);
}

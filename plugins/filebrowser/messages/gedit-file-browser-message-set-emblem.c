
/*
 * gedit-file-browser-message-set-emblem.c
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

#include "gedit-file-browser-message-set-emblem.h"

#define GEDIT_FILE_BROWSER_MESSAGE_SET_EMBLEM_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_FILE_BROWSER_MESSAGE_SET_EMBLEM, GeditFileBrowserMessageSetEmblemPrivate))

enum
{
	PROP_0,

	PROP_ID,
	PROP_EMBLEM,
};

struct _GeditFileBrowserMessageSetEmblemPrivate
{
	gchar *id;
	gchar *emblem;
};

G_DEFINE_TYPE (GeditFileBrowserMessageSetEmblem, gedit_file_browser_message_set_emblem, GEDIT_TYPE_MESSAGE)

static void
gedit_file_browser_message_set_emblem_finalize (GObject *obj)
{
	GeditFileBrowserMessageSetEmblem *msg = GEDIT_FILE_BROWSER_MESSAGE_SET_EMBLEM (obj);

	g_free (msg->priv->id);
	g_free (msg->priv->emblem);

	G_OBJECT_CLASS (gedit_file_browser_message_set_emblem_parent_class)->finalize (obj);
}

static void
gedit_file_browser_message_set_emblem_get_property (GObject    *obj,
                                                    guint       prop_id,
                                                    GValue     *value,
                                                    GParamSpec *pspec)
{
	GeditFileBrowserMessageSetEmblem *msg;

	msg = GEDIT_FILE_BROWSER_MESSAGE_SET_EMBLEM (obj);

	switch (prop_id)
	{
		case PROP_ID:
			g_value_set_string (value, msg->priv->id);
			break;
		case PROP_EMBLEM:
			g_value_set_string (value, msg->priv->emblem);
			break;
	}
}

static void
gedit_file_browser_message_set_emblem_set_property (GObject      *obj,
                                                    guint         prop_id,
                                                    GValue const *value,
                                                    GParamSpec   *pspec)
{
	GeditFileBrowserMessageSetEmblem *msg;

	msg = GEDIT_FILE_BROWSER_MESSAGE_SET_EMBLEM (obj);

	switch (prop_id)
	{
		case PROP_ID:
		{
			g_free (msg->priv->id);
			msg->priv->id = g_value_dup_string (value);
			break;
		}
		case PROP_EMBLEM:
		{
			g_free (msg->priv->emblem);
			msg->priv->emblem = g_value_dup_string (value);
			break;
		}
	}
}

static void
gedit_file_browser_message_set_emblem_class_init (GeditFileBrowserMessageSetEmblemClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = gedit_file_browser_message_set_emblem_finalize;

	object_class->get_property = gedit_file_browser_message_set_emblem_get_property;
	object_class->set_property = gedit_file_browser_message_set_emblem_set_property;

	g_object_class_install_property (object_class,
	                                 PROP_ID,
	                                 g_param_spec_string ("id",
	                                                      "Id",
	                                                      "Id",
	                                                      NULL,
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_CONSTRUCT |
	                                                      G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
	                                 PROP_EMBLEM,
	                                 g_param_spec_string ("emblem",
	                                                      "Emblem",
	                                                      "Emblem",
	                                                      NULL,
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_CONSTRUCT |
	                                                      G_PARAM_STATIC_STRINGS));

	g_type_class_add_private (object_class, sizeof (GeditFileBrowserMessageSetEmblemPrivate));
}

static void
gedit_file_browser_message_set_emblem_init (GeditFileBrowserMessageSetEmblem *message)
{
	message->priv = GEDIT_FILE_BROWSER_MESSAGE_SET_EMBLEM_GET_PRIVATE (message);
}

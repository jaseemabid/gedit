
/*
 * gedit-file-browser-message-get-view.c
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

#include "gedit-file-browser-message-get-view.h"
#include "plugins/filebrowser/gedit-file-browser-view.h"

#define GEDIT_FILE_BROWSER_MESSAGE_GET_VIEW_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_FILE_BROWSER_MESSAGE_GET_VIEW, GeditFileBrowserMessageGetViewPrivate))

enum
{
	PROP_0,

	PROP_VIEW,
};

struct _GeditFileBrowserMessageGetViewPrivate
{
	GeditFileBrowserView *view;
};

G_DEFINE_TYPE (GeditFileBrowserMessageGetView, gedit_file_browser_message_get_view, GEDIT_TYPE_MESSAGE)

static void
gedit_file_browser_message_get_view_finalize (GObject *obj)
{
	GeditFileBrowserMessageGetView *msg = GEDIT_FILE_BROWSER_MESSAGE_GET_VIEW (obj);

	if (msg->priv->view)
	{
		g_object_unref (msg->priv->view);
	}

	G_OBJECT_CLASS (gedit_file_browser_message_get_view_parent_class)->finalize (obj);
}

static void
gedit_file_browser_message_get_view_get_property (GObject    *obj,
                                                  guint       prop_id,
                                                  GValue     *value,
                                                  GParamSpec *pspec)
{
	GeditFileBrowserMessageGetView *msg;

	msg = GEDIT_FILE_BROWSER_MESSAGE_GET_VIEW (obj);

	switch (prop_id)
	{
		case PROP_VIEW:
			g_value_set_object (value, msg->priv->view);
			break;
	}
}

static void
gedit_file_browser_message_get_view_set_property (GObject      *obj,
                                                  guint         prop_id,
                                                  GValue const *value,
                                                  GParamSpec   *pspec)
{
	GeditFileBrowserMessageGetView *msg;

	msg = GEDIT_FILE_BROWSER_MESSAGE_GET_VIEW (obj);

	switch (prop_id)
	{
		case PROP_VIEW:
		{
			if (msg->priv->view)
			{
				g_object_unref (msg->priv->view);
			}
			msg->priv->view = g_value_dup_object (value);
			break;
		}
	}
}

static void
gedit_file_browser_message_get_view_class_init (GeditFileBrowserMessageGetViewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = gedit_file_browser_message_get_view_finalize;

	object_class->get_property = gedit_file_browser_message_get_view_get_property;
	object_class->set_property = gedit_file_browser_message_get_view_set_property;

	g_object_class_install_property (object_class,
	                                 PROP_VIEW,
	                                 g_param_spec_object ("view",
	                                                      "View",
	                                                      "View",
	                                                      GEDIT_TYPE_FILE_BROWSER_VIEW,
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_CONSTRUCT |
	                                                      G_PARAM_STATIC_STRINGS));

	g_type_class_add_private (object_class, sizeof (GeditFileBrowserMessageGetViewPrivate));
}

static void
gedit_file_browser_message_get_view_init (GeditFileBrowserMessageGetView *message)
{
	message->priv = GEDIT_FILE_BROWSER_MESSAGE_GET_VIEW_GET_PRIVATE (message);
}

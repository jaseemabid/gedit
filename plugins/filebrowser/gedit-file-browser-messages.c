/*
 * gedit-file-browser-messages.c
 *
 * Copyright (C) 2006 - Jesse van den Kieboom <jesse@icecrew.nl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "gedit-file-browser-messages.h"
#include "gedit-file-browser-store.h"
#include "messages/messages.h"

#include <gedit/gedit-message.h>

#define MESSAGE_OBJECT_PATH 	"/plugins/filebrowser"
#define WINDOW_DATA_KEY	       	"GeditFileBrowserMessagesWindowData"

#define BUS_CONNECT(bus, name, data) gedit_message_bus_connect(bus, MESSAGE_OBJECT_PATH, #name, (GeditMessageCallback)  message_##name##_cb, data, NULL)

typedef struct
{
	GeditWindow  *window;
	GeditMessage *message;
} MessageCacheData;

typedef struct
{
	guint row_inserted_id;
	guint row_deleted_id;
	guint root_changed_id;
	guint begin_loading_id;
	guint end_loading_id;

	GList *merge_ids;
	GtkActionGroup *merged_actions;

	GeditMessageBus *bus;
	GeditFileBrowserWidget *widget;
	GHashTable *row_tracking;

	GHashTable *filters;
} WindowData;

typedef struct
{
	gulong id;

	GeditWindow  *window;
	GeditMessage *message;
} FilterData;

static WindowData *
window_data_new (GeditWindow            *window,
		 GeditFileBrowserWidget *widget)
{
	WindowData *data = g_slice_new (WindowData);
	GtkUIManager *manager;
	GList *groups;

	data->bus = gedit_window_get_message_bus (window);
	data->widget = widget;
	data->row_tracking = g_hash_table_new_full (g_str_hash,
						    g_str_equal,
						    (GDestroyNotify)g_free,
						    (GDestroyNotify)gtk_tree_row_reference_free);

	data->filters = g_hash_table_new_full (g_str_hash,
					       g_str_equal,
					       (GDestroyNotify)g_free,
					       NULL);

	manager = gedit_file_browser_widget_get_ui_manager (widget);

	data->merge_ids = NULL;
	data->merged_actions = gtk_action_group_new ("MessageMergedActions");

	groups = gtk_ui_manager_get_action_groups (manager);
	gtk_ui_manager_insert_action_group (manager, data->merged_actions, g_list_length (groups));

	g_object_set_data (G_OBJECT (window), WINDOW_DATA_KEY, data);

	return data;
}

static WindowData *
get_window_data (GeditWindow *window)
{
	return (WindowData *) (g_object_get_data (G_OBJECT (window), WINDOW_DATA_KEY));
}

static void
window_data_free (GeditWindow *window)
{
	WindowData *data = get_window_data (window);
	GtkUIManager *manager;
	GList *item;

	g_hash_table_destroy (data->row_tracking);
	g_hash_table_destroy (data->filters);

	manager = gedit_file_browser_widget_get_ui_manager (data->widget);
	gtk_ui_manager_remove_action_group (manager, data->merged_actions);

	for (item = data->merge_ids; item; item = item->next)
		gtk_ui_manager_remove_ui (manager, GPOINTER_TO_INT (item->data));

	g_list_free (data->merge_ids);
	g_object_unref (data->merged_actions);

	g_slice_free (WindowData, data);

	g_object_set_data (G_OBJECT (window), WINDOW_DATA_KEY, NULL);
}

static FilterData *
filter_data_new (GeditWindow  *window,
		 GeditMessage *message)
{
	FilterData *data = g_slice_new (FilterData);
	WindowData *wdata;

	data->window = window;
	data->id = 0;
	data->message = message;

	wdata = get_window_data (window);

	g_hash_table_insert (wdata->filters,
			     gedit_message_type_identifier (gedit_message_get_object_path (message),
			                                    gedit_message_get_method (message)),
			     data);

	return data;
}

static void
filter_data_free (FilterData *data)
{
	WindowData *wdata = get_window_data (data->window);
	gchar *identifier;

	identifier = gedit_message_type_identifier (gedit_message_get_object_path (data->message),
			                            gedit_message_get_method (data->message));

	g_hash_table_remove (wdata->filters, identifier);
	g_free (identifier);

	g_object_unref (data->message);
	g_slice_free (FilterData, data);
}

static GtkTreePath *
track_row_lookup (WindowData  *data,
		  const gchar *id)
{
	GtkTreeRowReference *ref;

	ref = (GtkTreeRowReference *)g_hash_table_lookup (data->row_tracking, id);

	if (!ref)
		return NULL;

	return gtk_tree_row_reference_get_path (ref);
}

static void
message_cache_data_free (MessageCacheData *data)
{
	g_object_unref (data->message);
	g_slice_free (MessageCacheData, data);
}

static MessageCacheData *
message_cache_data_new (GeditWindow  *window,
			GeditMessage *message)
{
	MessageCacheData *data = g_slice_new (MessageCacheData);

	data->window = window;
	data->message = message;

	return data;
}

static void
message_get_root_cb (GeditMessageBus *bus,
		     GeditMessage    *message,
		     WindowData      *data)
{
	GeditFileBrowserStore *store;
	GFile *location;

	store = gedit_file_browser_widget_get_browser_store (data->widget);
	location = gedit_file_browser_store_get_virtual_root (store);

	if (location)
	{
		g_object_set (message, "location", location, NULL);
		g_object_unref (location);
	}
}

static void
message_set_root_cb (GeditMessageBus *bus,
		     GeditMessage    *message,
		     WindowData      *data)
{
	GFile *root;
	GFile *virtual = NULL;

	g_object_get (message, "location", &root, NULL);

	if (!root)
	{
		return;
	}

	g_object_get (message, "virtual", &virtual, NULL);

	if (virtual)
	{
		gedit_file_browser_widget_set_root_and_virtual_root (data->widget, root, virtual);
	}
	else
	{
		gedit_file_browser_widget_set_root (data->widget, root, TRUE);
	}
}

static void
message_set_emblem_cb (GeditMessageBus *bus,
		       GeditMessage    *message,
		       WindowData      *data)
{
	gchar *id = NULL;
	gchar *emblem = NULL;
	GtkTreePath *path;
	GeditFileBrowserStore *store;

	g_object_get (message, "id", &id, "emblem", &emblem, NULL);

	if (!id || !emblem)
	{
		g_free (id);
		g_free (emblem);

		return;
	}

	path = track_row_lookup (data, id);

	if (path != NULL)
	{
		GError *error = NULL;
		GdkPixbuf *pixbuf;

		pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
						   emblem,
						   10,
						   0,
						   &error);

		if (pixbuf)
		{
			GValue value = { 0, };
			GtkTreeIter iter;

			store = gedit_file_browser_widget_get_browser_store (data->widget);

			if (gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path))
			{
				g_value_init (&value, GDK_TYPE_PIXBUF);
				g_value_set_object (&value, pixbuf);

				gedit_file_browser_store_set_value (store,
								    &iter,
								    GEDIT_FILE_BROWSER_STORE_COLUMN_EMBLEM,
								    &value);

				g_value_unset (&value);
			}

			g_object_unref (pixbuf);
		}

		if (error)
			g_error_free (error);
	}

	g_free (id);
	g_free (emblem);
}

static gchar *
item_id (const gchar *path,
	 GFile *location)
{
	gchar *uri;
	gchar *id;

	uri = g_file_get_uri (location);
	id = g_strconcat (path, "::", uri, NULL);
	g_free (uri);

	return id;
}

static gchar *
track_row (WindowData            *data,
	   GeditFileBrowserStore *store,
	   GtkTreePath           *path,
	   GFile		 *location)
{
	GtkTreeRowReference *ref;
	gchar *id;
	gchar *pathstr;

	pathstr = gtk_tree_path_to_string (path);
	id = item_id (pathstr, location);

	ref = gtk_tree_row_reference_new (GTK_TREE_MODEL (store), path);
	g_hash_table_insert (data->row_tracking, g_strdup (id), ref);

	g_free (pathstr);

	return id;
}

static void
set_item_message (WindowData   *data,
		  GtkTreeIter  *iter,
		  GtkTreePath  *path,
		  GeditMessage *message)
{
	GeditFileBrowserStore *store;
	GFile *location;
	guint flags = 0;

	store = gedit_file_browser_widget_get_browser_store (data->widget);
	gtk_tree_model_get (GTK_TREE_MODEL (store), iter,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_LOCATION, &location,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_FLAGS, &flags,
			    -1);

	if (location)
	{
		gchar *track_id;

		if (path && gtk_tree_path_get_depth (path) != 0)
		{
			track_id = track_row (data, store, path, location);
		}
		else
		{
			track_id = NULL;
		}

		g_object_set (message,
		              "id", track_id,
		              "location", location,
		              NULL);

		if (gedit_message_has (message, "is_directory"))
		{
			g_object_set (message,
			              "is_directory",
			              FILE_IS_DIR (flags),
			              NULL);
		}

		g_free (track_id);
		g_object_unref (location);
	}
}

static gboolean
custom_message_filter_func (GeditFileBrowserWidget *widget,
			    GeditFileBrowserStore  *store,
			    GtkTreeIter            *iter,
			    FilterData             *data)
{
	WindowData *wdata = get_window_data (data->window);
	GFile *location;
	guint flags = 0;
	gboolean filter = FALSE;
	GtkTreePath *path;

	gtk_tree_model_get (GTK_TREE_MODEL (store), iter,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_LOCATION, &location,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_FLAGS, &flags,
			    -1);

	if (!location || FILE_IS_DUMMY (flags))
		return FALSE;

	path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), iter);
	set_item_message (wdata, iter, path, data->message);
	gtk_tree_path_free (path);

	g_object_set (data->message, "filter", filter, NULL);

	gedit_message_bus_send_message_sync (wdata->bus, data->message);
	g_object_get (data->message, "filter", &filter, NULL);

	g_object_unref (location);

	return !filter;
}

static void
message_add_filter_cb (GeditMessageBus *bus,
                       GeditMessage    *message,
                       GeditWindow     *window)
{
	const gchar *object_path = NULL;
	const gchar *method = NULL;
	gulong id;
	GeditMessage *cbmessage;
	FilterData *filter_data;
	WindowData *data;
	GType message_type;

	data = get_window_data (window);

	object_path = gedit_message_get_object_path (message);
	method = gedit_message_get_method (message);

	message_type = gedit_message_bus_lookup (bus, object_path, method);

	if (message_type == G_TYPE_INVALID)
	{
		return;
	}

	/* Check if the message type has the correct arguments */
	if (!gedit_message_type_check (message_type, "id", G_TYPE_STRING) ||
	    !gedit_message_type_check (message_type, "location", G_TYPE_FILE) ||
	    !gedit_message_type_check (message_type, "is-directory", G_TYPE_BOOLEAN) ||
	    !gedit_message_type_check (message_type, "filter", G_TYPE_BOOLEAN))
	{
		return;
	}

	cbmessage = g_object_new (message_type,
	                          "object-path", object_path,
	                          "method", method,
	                          "id", NULL,
	                          "location", NULL,
	                          "is-directory", FALSE,
	                          "filter", FALSE,
	                          NULL);

	/* Register the custom filter on the widget */
	filter_data = filter_data_new (window, cbmessage);

	id = gedit_file_browser_widget_add_filter (data->widget,
	                                           (GeditFileBrowserWidgetFilterFunc)custom_message_filter_func,
	                                           filter_data,
	                                           (GDestroyNotify)filter_data_free);

	filter_data->id = id;
}

static void
message_remove_filter_cb (GeditMessageBus *bus,
		          GeditMessage    *message,
		          WindowData      *data)
{
	gulong id = 0;

	g_object_get (message, "id", &id, NULL);

	if (!id)
		return;

	gedit_file_browser_widget_remove_filter (data->widget, id);
}

static void
message_up_cb (GeditMessageBus *bus,
	       GeditMessage    *message,
	       WindowData      *data)
{
	GeditFileBrowserStore *store = gedit_file_browser_widget_get_browser_store (data->widget);

	gedit_file_browser_store_set_virtual_root_up (store);
}

static void
message_history_back_cb (GeditMessageBus *bus,
		         GeditMessage    *message,
		         WindowData      *data)
{
	gedit_file_browser_widget_history_back (data->widget);
}

static void
message_history_forward_cb (GeditMessageBus *bus,
		            GeditMessage    *message,
		            WindowData      *data)
{
	gedit_file_browser_widget_history_forward (data->widget);
}

static void
message_refresh_cb (GeditMessageBus *bus,
		    GeditMessage    *message,
		    WindowData      *data)
{
	gedit_file_browser_widget_refresh (data->widget);
}

static void
message_set_show_hidden_cb (GeditMessageBus *bus,
		            GeditMessage    *message,
		            WindowData      *data)
{
	gboolean active = FALSE;
	GeditFileBrowserStore *store;
	GeditFileBrowserStoreFilterMode mode;

	g_object_get (message, "active", &active, NULL);

	store = gedit_file_browser_widget_get_browser_store (data->widget);
	mode = gedit_file_browser_store_get_filter_mode (store);

	if (active)
		mode &= ~GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_HIDDEN;
	else
		mode |= GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_HIDDEN;

	gedit_file_browser_store_set_filter_mode (store, mode);
}

static void
message_set_show_binary_cb (GeditMessageBus *bus,
		            GeditMessage    *message,
		            WindowData      *data)
{
	gboolean active = FALSE;
	GeditFileBrowserStore *store;
	GeditFileBrowserStoreFilterMode mode;

	g_object_get (message, "active", &active, NULL);

	store = gedit_file_browser_widget_get_browser_store (data->widget);
	mode = gedit_file_browser_store_get_filter_mode (store);

	if (active)
		mode &= ~GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_BINARY;
	else
		mode |= GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_BINARY;

	gedit_file_browser_store_set_filter_mode (store, mode);
}

static void
message_show_bookmarks_cb (GeditMessageBus *bus,
		           GeditMessage    *message,
		           WindowData      *data)
{
	gedit_file_browser_widget_show_bookmarks (data->widget);
}

static void
message_show_files_cb (GeditMessageBus *bus,
		       GeditMessage    *message,
		       WindowData      *data)
{
	gedit_file_browser_widget_show_files (data->widget);
}

static void
message_add_context_item_cb (GeditMessageBus *bus,
			     GeditMessage    *message,
			     WindowData      *data)
{
	GtkAction *action = NULL;
	gchar *path = NULL;
	gchar *name;
	GtkUIManager *manager;
	guint merge_id;

	g_object_get (message,
			   "action", &action,
			   "path", &path,
			   NULL);

	if (!action || !path)
	{
		if (action)
			g_object_unref (action);

		g_free (path);
		return;
	}

	gtk_action_group_add_action (data->merged_actions, action);
	manager = gedit_file_browser_widget_get_ui_manager (data->widget);
	name = g_strconcat (gtk_action_get_name (action), "MenuItem", NULL);
	merge_id = gtk_ui_manager_new_merge_id (manager);

	gtk_ui_manager_add_ui (manager,
			       merge_id,
			       path,
			       name,
			       gtk_action_get_name (action),
			       GTK_UI_MANAGER_AUTO,
			       FALSE);

	if (gtk_ui_manager_get_widget (manager, path))
	{
		data->merge_ids = g_list_prepend (data->merge_ids, GINT_TO_POINTER (merge_id));
		g_object_set (message, "id", merge_id, NULL);
	}
	else
	{
		g_object_set (message, "id", 0, NULL);
	}

	g_object_unref (action);
	g_free (path);
	g_free (name);
}

static void
message_remove_context_item_cb (GeditMessageBus *bus,
				GeditMessage    *message,
				WindowData      *data)
{
	guint merge_id = 0;
	GtkUIManager *manager;

	g_object_get (message, "id", &merge_id, NULL);

	if (merge_id == 0)
		return;

	manager = gedit_file_browser_widget_get_ui_manager (data->widget);

	data->merge_ids = g_list_remove (data->merge_ids, GINT_TO_POINTER (merge_id));
	gtk_ui_manager_remove_ui (manager, merge_id);
}

static void
message_get_view_cb (GeditMessageBus *bus,
		     GeditMessage    *message,
		     WindowData      *data)
{
	GeditFileBrowserView *view;
	view = gedit_file_browser_widget_get_browser_view (data->widget);

	g_object_set (message, "view", view, NULL);
}

static void
register_methods (GeditWindow            *window,
		  GeditFileBrowserWidget *widget)
{
	GeditMessageBus *bus = gedit_window_get_message_bus (window);
	WindowData *data = get_window_data (window);

	/* Register method calls */
	gedit_message_bus_register (bus,
	                            GEDIT_TYPE_FILE_BROWSER_MESSAGE_GET_ROOT,
	                            MESSAGE_OBJECT_PATH,
	                            "get_root");

	gedit_message_bus_register (bus,
	                            GEDIT_TYPE_FILE_BROWSER_MESSAGE_SET_ROOT,
	                            MESSAGE_OBJECT_PATH,
	                            "set_root");

	gedit_message_bus_register (bus,
	                            GEDIT_TYPE_FILE_BROWSER_MESSAGE_SET_EMBLEM,
	                            MESSAGE_OBJECT_PATH,
	                            "set_emblem");

	gedit_message_bus_register (bus,
	                            GEDIT_TYPE_FILE_BROWSER_MESSAGE_ADD_FILTER,
	                            MESSAGE_OBJECT_PATH,
	                            "add_filter");

	gedit_message_bus_register (bus,
	                            GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID,
	                            MESSAGE_OBJECT_PATH,
	                            "remove_filter");

	gedit_message_bus_register (bus,
	                            GEDIT_TYPE_FILE_BROWSER_MESSAGE_ADD_CONTEXT_ITEM,
	                            MESSAGE_OBJECT_PATH,
	                            "add_context_item");

	gedit_message_bus_register (bus,
	                            GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID,
	                            MESSAGE_OBJECT_PATH,
	                            "remove_context_item");

	gedit_message_bus_register (bus,
	                            GEDIT_TYPE_MESSAGE,
	                            MESSAGE_OBJECT_PATH,
	                            "up");

	gedit_message_bus_register (bus,
	                            GEDIT_TYPE_MESSAGE,
	                            MESSAGE_OBJECT_PATH,
	                            "history_back");

	gedit_message_bus_register (bus,
	                            GEDIT_TYPE_MESSAGE,
	                            MESSAGE_OBJECT_PATH,
	                            "history_forward");

	gedit_message_bus_register (bus,
	                            GEDIT_TYPE_MESSAGE,
	                            MESSAGE_OBJECT_PATH,
	                            "refresh");

	gedit_message_bus_register (bus,
	                            GEDIT_TYPE_FILE_BROWSER_MESSAGE_ACTIVATION,
	                            MESSAGE_OBJECT_PATH,
	                            "set_show_hidden");

	gedit_message_bus_register (bus,
	                            GEDIT_TYPE_FILE_BROWSER_MESSAGE_ACTIVATION,
	                            MESSAGE_OBJECT_PATH,
	                            "set_show_binary");

	gedit_message_bus_register (bus,
	                            GEDIT_TYPE_MESSAGE,
	                            MESSAGE_OBJECT_PATH,
	                            "show_bookmarks");

	gedit_message_bus_register (bus,
	                            GEDIT_TYPE_MESSAGE,
	                            MESSAGE_OBJECT_PATH,
	                            "show_files");

	gedit_message_bus_register (bus,
	                            GEDIT_TYPE_FILE_BROWSER_MESSAGE_GET_VIEW,
	                            MESSAGE_OBJECT_PATH,
	                            "get_view");

	BUS_CONNECT (bus, get_root, data);
	BUS_CONNECT (bus, set_root, data);
	BUS_CONNECT (bus, set_emblem, data);
	BUS_CONNECT (bus, add_filter, window);
	BUS_CONNECT (bus, remove_filter, data);

	BUS_CONNECT (bus, add_context_item, data);
	BUS_CONNECT (bus, remove_context_item, data);

	BUS_CONNECT (bus, up, data);
	BUS_CONNECT (bus, history_back, data);
	BUS_CONNECT (bus, history_forward, data);

	BUS_CONNECT (bus, refresh, data);

	BUS_CONNECT (bus, set_show_hidden, data);
	BUS_CONNECT (bus, set_show_binary, data);

	BUS_CONNECT (bus, show_bookmarks, data);
	BUS_CONNECT (bus, show_files, data);

	BUS_CONNECT (bus, get_view, data);
}

static void
store_row_inserted (GeditFileBrowserStore *store,
		    GtkTreePath		  *path,
		    GtkTreeIter           *iter,
		    MessageCacheData      *data)
{
	guint flags = 0;

	gtk_tree_model_get (GTK_TREE_MODEL (store), iter,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_FLAGS, &flags,
			    -1);

	if (!FILE_IS_DUMMY (flags) && !FILE_IS_FILTERED (flags))
	{
		WindowData *wdata = get_window_data (data->window);

		set_item_message (wdata, iter, path, data->message);
		gedit_message_bus_send_message_sync (wdata->bus, data->message);
	}
}

static void
store_row_deleted (GeditFileBrowserStore *store,
		   GtkTreePath		 *path,
		   MessageCacheData      *data)
{
	GtkTreeIter iter;
	guint flags = 0;

	if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path))
		return;

	gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_FLAGS, &flags,
			    -1);

	if (!FILE_IS_DUMMY (flags) && !FILE_IS_FILTERED (flags))
	{
		WindowData *wdata = get_window_data (data->window);

		set_item_message (wdata, &iter, path, data->message);
		gedit_message_bus_send_message_sync (wdata->bus, data->message);
	}
}

static void
store_virtual_root_changed (GeditFileBrowserStore *store,
			    GParamSpec            *spec,
			    MessageCacheData      *data)
{
	WindowData *wdata = get_window_data (data->window);
	GFile *vroot;

	vroot = gedit_file_browser_store_get_virtual_root (store);

	if (!vroot)
	{
		return;
	}

	g_object_set (data->message,
	              "location", vroot,
	              NULL);

	gedit_message_bus_send_message_sync (wdata->bus, data->message);

	g_object_unref (vroot);
}

static void
store_begin_loading (GeditFileBrowserStore *store,
		     GtkTreeIter           *iter,
		     MessageCacheData      *data)
{
	GtkTreePath *path;
	WindowData *wdata = get_window_data (data->window);

	path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), iter);

	set_item_message (wdata, iter, path, data->message);

	gedit_message_bus_send_message_sync (wdata->bus, data->message);
	gtk_tree_path_free (path);
}

static void
store_end_loading (GeditFileBrowserStore *store,
		   GtkTreeIter           *iter,
		   MessageCacheData      *data)
{
	GtkTreePath *path;
	WindowData *wdata = get_window_data (data->window);

	path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), iter);

	set_item_message (wdata, iter, path, data->message);

	gedit_message_bus_send_message_sync (wdata->bus, data->message);
	gtk_tree_path_free (path);
}

static void
register_signals (GeditWindow            *window,
		  GeditFileBrowserWidget *widget)
{
	GeditMessageBus *bus = gedit_window_get_message_bus (window);
	GeditFileBrowserStore *store;

	GeditMessage *message;
	WindowData *data;

	/* Register signals */
	gedit_message_bus_register (bus,
	                            GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID_LOCATION,
	                            MESSAGE_OBJECT_PATH,
	                            "root_changed");

	gedit_message_bus_register (bus,
	                            GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID_LOCATION,
	                            MESSAGE_OBJECT_PATH,
	                            "begin_loading");

	gedit_message_bus_register (bus,
	                            GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID_LOCATION,
	                            MESSAGE_OBJECT_PATH,
	                            "end_loading");

	gedit_message_bus_register (bus,
	                            GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID_LOCATION,
	                            MESSAGE_OBJECT_PATH,
	                            "inserted");

	gedit_message_bus_register (bus,
	                            GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID_LOCATION,
	                            MESSAGE_OBJECT_PATH,
	                            "deleted");

	store = gedit_file_browser_widget_get_browser_store (widget);

	message = g_object_new (GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID_LOCATION,
	                        "object-path", MESSAGE_OBJECT_PATH,
	                        "method", "inserted",
	                        NULL);

	data = get_window_data (window);

	data->row_inserted_id =
		g_signal_connect_data (store,
		                       "row-inserted",
		                       G_CALLBACK (store_row_inserted),
		                       message_cache_data_new (window, message),
		                       (GClosureNotify)message_cache_data_free,
		                       0);

	message = g_object_new (GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID_LOCATION,
	                        "object-path", MESSAGE_OBJECT_PATH,
	                        "method", "deleted",
	                        NULL);

	data->row_deleted_id =
		g_signal_connect_data (store,
		                       "row-deleted",
		                       G_CALLBACK (store_row_deleted),
		                       message_cache_data_new (window, message),
		                       (GClosureNotify)message_cache_data_free,
		                       0);

	message = g_object_new (GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID_LOCATION,
	                        "object-path", MESSAGE_OBJECT_PATH,
	                        "method", "root_changed",
	                        NULL);

	data->root_changed_id =
		g_signal_connect_data (store,
		                       "notify::virtual-root",
		                       G_CALLBACK (store_virtual_root_changed),
		                       message_cache_data_new (window, message),
		                       (GClosureNotify)message_cache_data_free,
		                       0);

	message = g_object_new (GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID_LOCATION,
	                        "object-path", MESSAGE_OBJECT_PATH,
	                        "method", "begin_loading",
	                        NULL);

	data->begin_loading_id =
		g_signal_connect_data (store,
		                       "begin_loading",
		                       G_CALLBACK (store_begin_loading),
		                       message_cache_data_new (window, message),
		                       (GClosureNotify)message_cache_data_free,
		                       0);

	message = g_object_new (GEDIT_TYPE_FILE_BROWSER_MESSAGE_ID_LOCATION,
	                        "object-path", MESSAGE_OBJECT_PATH,
	                        "method", "end_loading",
	                        NULL);

	data->end_loading_id =
		g_signal_connect_data (store,
		                       "end_loading",
		                       G_CALLBACK (store_end_loading),
		                       message_cache_data_new (window, message),
		                       (GClosureNotify)message_cache_data_free,
		                       0);
}

static void
message_unregistered (GeditMessageBus *bus,
		      const gchar     *object_path,
		      const gchar     *method,
		      GeditWindow     *window)
{
	gchar *identifier;
	FilterData *data;
	WindowData *wdata;

	wdata = get_window_data (window);

	identifier = gedit_message_type_identifier (object_path, method);

	data = g_hash_table_lookup (wdata->filters, identifier);

	if (data)
	{
		gedit_file_browser_widget_remove_filter (wdata->widget,
		                                         data->id);
	}

	g_free (identifier);
}

void
gedit_file_browser_messages_register (GeditWindow            *window,
                                      GeditFileBrowserWidget *widget)
{
	window_data_new (window, widget);

	register_methods (window, widget);
	register_signals (window, widget);

	g_signal_connect (gedit_window_get_message_bus (window),
	                  "unregistered",
	                  G_CALLBACK (message_unregistered),
	                  window);
}

static void
cleanup_signals (GeditWindow *window)
{
	WindowData *data = get_window_data (window);
	GeditFileBrowserStore *store;

	store = gedit_file_browser_widget_get_browser_store (data->widget);

	g_signal_handler_disconnect (store, data->row_inserted_id);
	g_signal_handler_disconnect (store, data->row_deleted_id);
	g_signal_handler_disconnect (store, data->root_changed_id);
	g_signal_handler_disconnect (store, data->begin_loading_id);
	g_signal_handler_disconnect (store, data->end_loading_id);

	g_signal_handlers_disconnect_by_func (data->bus, message_unregistered, window);
}

void
gedit_file_browser_messages_unregister (GeditWindow *window)
{
	GeditMessageBus *bus = gedit_window_get_message_bus (window);

	cleanup_signals (window);
	gedit_message_bus_unregister_all (bus, MESSAGE_OBJECT_PATH);

	window_data_free (window);
}
/* ex:ts=8:noet: */

/*
 * gedit-app-osx.c
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Jesse van den Kieboom
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

#include "gedit-app-osx.h"
#include <gedit/gedit-dirs.h>
#include <gedit/gedit-debug.h>
#include <gdk/gdkquartz.h>
#include <gtkosxapplication.h>
#include <string.h>
#include <glib/gi18n.h>

#include "gedit-commands.h"
#include <AvailabilityMacros.h>

G_DEFINE_TYPE (GeditAppOSX, gedit_app_osx, GEDIT_TYPE_APP)

static void
gedit_app_osx_finalize (GObject *object)
{
	G_OBJECT_CLASS (gedit_app_osx_parent_class)->finalize (object);
}

static gboolean
gedit_app_osx_last_window_destroyed_impl (GeditApp *app,
                                          GeditWindow *window)
{
	if (!GPOINTER_TO_INT (g_object_get_data (G_OBJECT (window), "gedit-is-quitting-all")))
	{
		/* Create hidden proxy window on OS X to handle the menu */
		gedit_app_create_window (app, NULL);
		return FALSE;
	}

	return GEDIT_APP_CLASS (gedit_app_osx_parent_class)->last_window_destroyed (app, window);
}

gboolean
gedit_app_osx_show_url (GeditAppOSX *app,
                        const gchar *url)
{
	return [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:[NSString stringWithUTF8String:url]]];
}

static gboolean
gedit_app_osx_show_help_impl (GeditApp    *app,
                              GtkWindow   *parent,
                              const gchar *name,
                              const gchar *link_id)
{
	gboolean ret = FALSE;

	if (name == NULL || g_strcmp0 (name, "gedit.xml") == 0 || g_strcmp0 (name, "gedit") == 0)
	{
		gchar *link;

		if (link_id)
		{
			link = g_strdup_printf ("http://library.gnome.org/users/gedit/stable/%s",
						link_id);
		}
		else
		{
			link = g_strdup ("http://library.gnome.org/users/gedit/stable/");
		}

		ret = gedit_app_osx_show_url (GEDIT_APP_OSX (app), link);
		g_free (link);
	}

	return ret;
}

static void
gedit_app_osx_set_window_title_impl (GeditApp    *app,
                                     GeditWindow *window,
                                     const gchar *title)
{
	NSWindow *native;
	GeditDocument *document;
	GdkWindow *wnd;

	g_return_if_fail (GEDIT_IS_WINDOW (window));

	wnd = gtk_widget_get_window (GTK_WIDGET (window));

	if (wnd == NULL)
	{
		return;
	}

	native = gdk_quartz_window_get_nswindow (wnd);
	document = gedit_window_get_active_document (window);

	if (document)
	{
		bool ismodified;

		if (gedit_document_is_untitled (document))
		{
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5
			[native setRepresentedURL:nil];
#else
			[native setRepresentedFilename:@""];
#endif
		}
		else
		{
			GFile *location;
			gchar *uri;

			location = gedit_document_get_location (document);

#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5
			uri = g_file_get_uri (location);
			g_object_unref (location);

			NSURL *nsurl = [NSURL URLWithString:[NSString stringWithUTF8String:uri]];
			
			[native setRepresentedURL:nsurl];
			g_free (uri);
#else
			if (g_file_has_uri_scheme (location, "file"))
			{
				uri = g_file_get_path (location);
				[native setRepresentedFilename:[NSString stringWithUTF8String:uri]];

				g_free (uri);
			}
#endif
		}

		ismodified = !gedit_document_is_untouched (document); 
		[native setDocumentEdited:ismodified];
	}
	else
	{
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5
		[native setRepresentedURL:nil];
#else
		[native setRepresentedFilename:@""];
#endif
		[native setDocumentEdited:false];
	}

	GEDIT_APP_CLASS (gedit_app_osx_parent_class)->set_window_title (app, window, title);
}

static void
gedit_app_osx_quit_impl (GeditApp *app)
{
	GtkOSXApplication *osxapp;

	osxapp = g_object_new (GTK_TYPE_OSX_APPLICATION, NULL);
	gtk_osxapplication_cleanup (osxapp);

	GEDIT_APP_CLASS (gedit_app_osx_parent_class)->quit (app);
}

static void
load_accels (void)
{
	gchar *filename;

	filename = g_build_filename (gedit_dirs_get_gedit_data_dir (),
	                             "osx.accels",
	                             NULL);

	if (filename != NULL)
	{
		gedit_debug_message (DEBUG_APP, "Loading accels from %s\n", filename);

		gtk_accel_map_load (filename);
		g_free (filename);
	}
}

static void
load_keybindings (void)
{
	gchar *filename;

	filename = g_build_filename (gedit_dirs_get_gedit_data_dir (),
	                             "osx.css",
	                             NULL);

	if (filename != NULL)
	{
		GtkCssProvider *provider;
		GError *error = NULL;

		gedit_debug_message (DEBUG_APP, "Loading keybindings from %s\n", filename);

		provider = gtk_css_provider_new ();

		if (!gtk_css_provider_load_from_path (provider,
		                                      filename,
		                                      &error))
		{
			g_warning ("Failed to load osx keybindings from `%s':\n%s",
			           filename,
			           error->message);

			g_error_free (error);
		}
		else
		{
			gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
			                                           GTK_STYLE_PROVIDER (provider),
			                                           GTK_STYLE_PROVIDER_PRIORITY_SETTINGS);
		}

		g_object_unref (provider);
		g_free (filename);
	}
}

static void
gedit_app_osx_constructed (GObject *object)
{
	/* First load the osx specific accel overrides */
	load_accels ();
	load_keybindings ();

	if (G_OBJECT_CLASS (gedit_app_osx_parent_class)->constructed)
	{
		/* Then chain up to load the user specific accels */
		G_OBJECT_CLASS (gedit_app_osx_parent_class)->constructed (object);
	}
}

static GtkMenuItem *
ui_manager_menu_item (GtkUIManager *uimanager,
                      const gchar  *path)
{
	return GTK_MENU_ITEM (gtk_ui_manager_get_widget (uimanager, path));
}

static void
setup_mac_menu (GeditWindow *window)
{
	GtkAction *action;
	GtkOSXApplication *osxapp;
	GtkUIManager *manager;
	GtkWidget *menu;

	manager = gedit_window_get_ui_manager (window);
	
	/* Hide the menu bar */
	menu = gtk_ui_manager_get_widget (manager, "/MenuBar");
	gtk_widget_hide (menu);

	osxapp = g_object_new (GTK_TYPE_OSX_APPLICATION, NULL);

	action = gtk_ui_manager_get_action (manager, "/ui/MenuBar/HelpMenu/HelpAboutMenu");
	gtk_action_set_label (action, _("About gedit"));

	gtk_widget_hide (GTK_WIDGET (ui_manager_menu_item (manager,
	                             "/ui/MenuBar/FileMenu/FileQuitMenu")));

	gtk_osxapplication_set_menu_bar (osxapp,
	                                 GTK_MENU_SHELL (menu));

	gtk_osxapplication_set_help_menu (osxapp,
	                                  ui_manager_menu_item (manager,
	                                                        "/ui/MenuBar/HelpMenu"));

	gtk_osxapplication_set_window_menu (osxapp, NULL);

	gtk_osxapplication_insert_app_menu_item (osxapp,
	                                         GTK_WIDGET (ui_manager_menu_item (manager,
	                                                                           "/ui/MenuBar/HelpMenu/HelpAboutMenu")),
	                                         0);

	gtk_osxapplication_insert_app_menu_item (osxapp,
	                                         g_object_ref (gtk_separator_menu_item_new ()),
	                                         1);

	gtk_osxapplication_insert_app_menu_item (osxapp,
	                                         GTK_WIDGET (ui_manager_menu_item (manager,
	                                                                           "/ui/MenuBar/EditMenu/EditPreferencesMenu")),
	                                         2);

	/* We remove the accel group of the uimanager from the window */
	gtk_window_remove_accel_group (GTK_WINDOW (window),
	                               gtk_ui_manager_get_accel_group (manager));
}

static GeditWindow *
gedit_app_osx_create_window_impl (GeditApp *app)
{
	GeditWindow *window;

	window = GEDIT_APP_CLASS (gedit_app_osx_parent_class)->create_window (app);

	setup_mac_menu (window);

	return window;
}

static gboolean
gedit_app_osx_process_window_event_impl (GeditApp    *app,
                                         GeditWindow *window,
                                         GdkEvent    *event)
{
	NSEvent *nsevent;

	/* For OS X we will propagate the event to NSApp, which handles some OS X
	 * specific keybindings and the accelerators for the menu
	 */
	nsevent = gdk_quartz_event_get_nsevent (event);
	[NSApp sendEvent:nsevent];

	/* It does not really matter what we return here since it's the last thing
	 * in the chain. Also we can't get from sendEvent whether the event was
	 * actually handled by NSApp anyway
	 */
	return TRUE;
}

static void
gedit_app_osx_ready_impl (GeditApp *app)
{
	GtkOSXApplication *osxapp;

	osxapp = g_object_new (GTK_TYPE_OSX_APPLICATION, NULL);
	gtk_osxapplication_ready (osxapp);

	GEDIT_APP_CLASS (gedit_app_osx_parent_class)->ready (app);
}

static void
gedit_app_osx_class_init (GeditAppOSXClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditAppClass *app_class = GEDIT_APP_CLASS (klass);

	object_class->finalize = gedit_app_osx_finalize;
	object_class->constructed = gedit_app_osx_constructed;

	app_class->last_window_destroyed = gedit_app_osx_last_window_destroyed_impl;
	app_class->show_help = gedit_app_osx_show_help_impl;
	app_class->set_window_title = gedit_app_osx_set_window_title_impl;
	app_class->quit = gedit_app_osx_quit_impl;
	app_class->create_window = gedit_app_osx_create_window_impl;
	app_class->process_window_event = gedit_app_osx_process_window_event_impl;
	app_class->ready = gedit_app_osx_ready_impl;
}

static void
on_osx_will_terminate (GtkOSXApplication *osxapp,
                       GeditAppOSX       *app)
{
	_gedit_app_quit (GEDIT_APP (app));
}

static gboolean
on_osx_block_termination (GtkOSXApplication *osxapp,
                          GeditAppOSX       *app)
{
	GtkUIManager *manager;
	GtkAction *action;
	GeditWindow *window;

	window = gedit_app_get_active_window (GEDIT_APP (app));

	// Synthesize quit-all
	manager = gedit_window_get_ui_manager (window);

	action = gtk_ui_manager_get_action (manager,
	                                    "/ui/MenuBar/FileMenu/FileQuitMenu");

	_gedit_cmd_file_quit (action, window);
	return TRUE;
}

static gboolean
on_osx_open_files (GtkOSXApplication  *osxapp,
                   gchar const       **paths,
                   GeditAppOSX        *app)
{
	GSList *locations = NULL;

	while (paths && *paths)
	{
		locations = g_slist_prepend (locations,
		                             g_file_new_for_path (*paths));
		++paths;
	}
	
	locations = g_slist_reverse (locations);
	
	if (locations != NULL)
	{
		GSList *files;
		GeditWindow *window;
		
		window = gedit_app_get_active_window (GEDIT_APP (app));

		files = gedit_commands_load_locations (window,
		                                       locations,
		                                       NULL,
		                                       0,
		                                       0);

		g_slist_free_full (locations, g_object_unref);
	}
	
	return TRUE;
}

static void
gedit_app_osx_init (GeditAppOSX *app)
{
	GtkOSXApplication *osxapp;

	/* This is a singleton */
	osxapp = g_object_new (GTK_TYPE_OSX_APPLICATION, NULL);

	/* manually set name and icon */
	g_set_application_name ("gedit");
	gtk_window_set_default_icon_name ("accessories-text-editor");

	g_signal_connect (osxapp,
	                  "NSApplicationWillTerminate",
	                  G_CALLBACK (on_osx_will_terminate),
	                  app);

	g_signal_connect (osxapp,
	                  "NSApplicationBlockTermination",
	                  G_CALLBACK (on_osx_block_termination),
	                  app);

	g_signal_connect (osxapp,
	                  "NSApplicationOpenFiles",
	                  G_CALLBACK (on_osx_open_files),
	                  app);

	gtk_osxapplication_set_use_quartz_accelerators (osxapp, FALSE);
}

/* ex:set ts=8 noet: */

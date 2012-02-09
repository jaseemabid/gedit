/*
 * gedit-debug.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002 - 2005 Paolo Maggi
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the gedit Team, 1998-2005. See the AUTHORS file for a
 * list of people on the gedit Team.
 * See the ChangeLog files for a list of changes.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include "gedit-debug.h"

#define ENABLE_PROFILING

#ifdef ENABLE_PROFILING
static GTimer *timer = NULL;
static gdouble last = 0.0;
#endif

static GeditDebugSection debug = GEDIT_NO_DEBUG;

#define DEBUG_IS_ENABLED(section_rval) (debug & (section_rval))

/**
 * gedit_debug_init:
 *
 * Initializes the debugging subsystem of Gedit.
 *
 * The function checks for the existence of certain environment variables to
 * determine whether to enable output for a debug section. To enable output
 * for a specific debug section, set an environment variable of the same name;
 * e.g. to enable output for the %GEDIT_DEBUG_PLUGINS section, set a
 * <code>GEDIT_DEBUG_PLUGINS</code> environment variable. To enable output
 * for all debug sections, set the <code>GEDIT_DEBUG</code> environment
 * variable.
 *
 * This function must be called before any of the other debug functions are
 * called. It must only be called once.
 */
void
gedit_debug_init (void)
{
	if (g_getenv ("GEDIT_DEBUG") != NULL)
	{
		/* enable all debugging */
		debug = ~GEDIT_NO_DEBUG;
		goto out;
	}

	if (g_getenv ("GEDIT_DEBUG_VIEW") != NULL)
		debug = debug | GEDIT_DEBUG_VIEW;
	if (g_getenv ("GEDIT_DEBUG_SEARCH") != NULL)
		debug = debug | GEDIT_DEBUG_SEARCH;
	if (g_getenv ("GEDIT_DEBUG_PREFS") != NULL)
		debug = debug | GEDIT_DEBUG_PREFS;
	if (g_getenv ("GEDIT_DEBUG_PRINT") != NULL)
		debug = debug | GEDIT_DEBUG_PRINT;
	if (g_getenv ("GEDIT_DEBUG_PLUGINS") != NULL)
		debug = debug | GEDIT_DEBUG_PLUGINS;
	if (g_getenv ("GEDIT_DEBUG_TAB") != NULL)
		debug = debug | GEDIT_DEBUG_TAB;
	if (g_getenv ("GEDIT_DEBUG_DOCUMENT") != NULL)
		debug = debug | GEDIT_DEBUG_DOCUMENT;
	if (g_getenv ("GEDIT_DEBUG_COMMANDS") != NULL)
		debug = debug | GEDIT_DEBUG_COMMANDS;
	if (g_getenv ("GEDIT_DEBUG_APP") != NULL)
		debug = debug | GEDIT_DEBUG_APP;
	if (g_getenv ("GEDIT_DEBUG_SESSION") != NULL)
		debug = debug | GEDIT_DEBUG_SESSION;
	if (g_getenv ("GEDIT_DEBUG_UTILS") != NULL)
		debug = debug | GEDIT_DEBUG_UTILS;
	if (g_getenv ("GEDIT_DEBUG_METADATA") != NULL)
		debug = debug | GEDIT_DEBUG_METADATA;
	if (g_getenv ("GEDIT_DEBUG_WINDOW") != NULL)
		debug = debug | GEDIT_DEBUG_WINDOW;
	if (g_getenv ("GEDIT_DEBUG_LOADER") != NULL)
		debug = debug | GEDIT_DEBUG_LOADER;
	if (g_getenv ("GEDIT_DEBUG_SAVER") != NULL)
		debug = debug | GEDIT_DEBUG_SAVER;
	if (g_getenv ("GEDIT_DEBUG_PANEL") != NULL)
		debug = debug | GEDIT_DEBUG_PANEL;
	if (g_getenv ("GEDIT_DEBUG_DBUS") != NULL)
		debug = debug | GEDIT_DEBUG_DBUS;
out:

#ifdef ENABLE_PROFILING
	if (debug != GEDIT_NO_DEBUG)
		timer = g_timer_new ();
#endif
	return;
}

/**
 * gedit_debug:
 * @section: Debug section.
 * @file: Name of the source file containing the call to gedit_debug().
 * @line: Line number within the file named by @file of the call to gedit_debug().
 * @function: Name of the function that is calling gedit_debug().
 *
 * If output for debug section @section is enabled, then logs the trace
 * information @file, @line, and @function.
 */
void gedit_debug (GeditDebugSection  section,
		  const gchar       *file,
		  gint               line,
		  const gchar       *function)
{
	if (G_UNLIKELY (DEBUG_IS_ENABLED (section)))
	{
#ifdef ENABLE_PROFILING
		gdouble seconds;

		g_return_if_fail (timer != NULL);

		seconds = g_timer_elapsed (timer, NULL);
		g_print ("[%f (%f)] %s:%d (%s)\n",
			 seconds, seconds - last, file, line, function);
		last = seconds;
#else
		g_print ("%s:%d (%s)\n", file, line, function);
#endif

		fflush (stdout);
	}
}

/**
 * gedit_debug_message:
 * @section: Debug section.
 * @file: Name of the source file containing the call to gedit_debug_message().
 * @line: Line number within the file named by @file of the call to gedit_debug_message().
 * @function: Name of the function that is calling gedit_debug_message().
 * @format: A g_vprintf() format string.
 * @...: The format string arguments.
 *
 * If output for debug section @section is enabled, then logs the trace
 * information @file, @line, and @function along with the message obtained by
 * formatting @format with the given format string arguments.
 */
void
gedit_debug_message (GeditDebugSection  section,
		     const gchar       *file,
		     gint               line,
		     const gchar       *function,
		     const gchar       *format, ...)
{
	if (G_UNLIKELY (DEBUG_IS_ENABLED (section)))
	{
		va_list args;
		gchar *msg;

#ifdef ENABLE_PROFILING
		gdouble seconds;

		g_return_if_fail (timer != NULL);

		seconds = g_timer_elapsed (timer, NULL);
#endif

		g_return_if_fail (format != NULL);

		va_start (args, format);
		msg = g_strdup_vprintf (format, args);
		va_end (args);

#ifdef ENABLE_PROFILING
		g_print ("[%f (%f)] %s:%d (%s) %s\n",
			 seconds, seconds - last, file, line, function, msg);
		last = seconds;
#else
		g_print ("%s:%d (%s) %s\n", file, line, function, msg);
#endif

		fflush (stdout);

		g_free (msg);
	}
}

/**
 * gedit_debug_plugin_message:
 * @file: Name of the source file containing the call to gedit_debug_plugin_message().
 * @line: Line number within the file named by @file of the call to gedit_debug_plugin_message().
 * @function: Name of the function that is calling gedit_debug_plugin_message().
 * @message: An informational message.
 *
 * If output for debug section %GEDIT_DEBUG_PLUGINS is enabled, then logs the trace
 * information @file, @line, and @function along with the informational message
 * @message.
 *
 * This function may be overridden by GObject Introspection language bindings
 * to be more language-specific.
 *
 * <emphasis>Python</emphasis>
 *
 * A PyGObject override is provided that has the following signature:
 * <informalexample>
 *   <programlisting>
 *     def debug_plugin_message(format_str, *format_args):
 *         #...
 *   </programlisting>
 * </informalexample>
 *
 * It automatically supplies parameters @file, @line, and @function, and it
 * formats <code>format_str</code> with the given format arguments. The syntax
 * of the format string is the usual Python string formatting syntax described
 * by <ulink url="http://docs.python.org/library/stdtypes.html#string-formatting">5.6.2. String Formatting Operations</ulink>.
 *
 * Since: 3.4
 */
void
gedit_debug_plugin_message (const gchar       *file,
			    gint               line,
			    const gchar       *function,
			    const gchar       *message)
{
	gedit_debug_message (GEDIT_DEBUG_PLUGINS, file, line, function, "%s",
			     message);
}

/* ex:set ts=8 noet: */

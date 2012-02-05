/*
 * gedit-debug.h
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

#ifndef __GEDIT_DEBUG_H__
#define __GEDIT_DEBUG_H__

#include <glib.h>

/**
 * GeditDebugSection:
 *
 * Enumeration of debug sections.
 *
 * Debugging output for a section is enabled by setting an environment variable
 * of the same name. For example, setting the <code>GEDIT_DEBUG_PLUGINS</code>
 * environment variable enables all debugging output for the #GEDIT_DEBUG_PLUGINS
 * section. Setting the special environment variable <code>GEDIT_DEBUG</code>
 * enables output for all sections.
 */
typedef enum {
	GEDIT_NO_DEBUG       = 0,
	GEDIT_DEBUG_VIEW     = 1 << 0,
	GEDIT_DEBUG_SEARCH   = 1 << 1,
	GEDIT_DEBUG_PRINT    = 1 << 2,
	GEDIT_DEBUG_PREFS    = 1 << 3,
	GEDIT_DEBUG_PLUGINS  = 1 << 4,
	GEDIT_DEBUG_TAB      = 1 << 5,
	GEDIT_DEBUG_DOCUMENT = 1 << 6,
	GEDIT_DEBUG_COMMANDS = 1 << 7,
	GEDIT_DEBUG_APP      = 1 << 8,
	GEDIT_DEBUG_SESSION  = 1 << 9,
	GEDIT_DEBUG_UTILS    = 1 << 10,
	GEDIT_DEBUG_METADATA = 1 << 11,
	GEDIT_DEBUG_WINDOW   = 1 << 12,
	GEDIT_DEBUG_LOADER   = 1 << 13,
	GEDIT_DEBUG_SAVER    = 1 << 14,
	GEDIT_DEBUG_PANEL    = 1 << 15,
	GEDIT_DEBUG_DBUS     = 1 << 16
} GeditDebugSection;


/* FIXME this is an issue for introspection */
#define	DEBUG_VIEW	GEDIT_DEBUG_VIEW,    __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_SEARCH	GEDIT_DEBUG_SEARCH,  __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_PRINT	GEDIT_DEBUG_PRINT,   __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_PREFS	GEDIT_DEBUG_PREFS,   __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_PLUGINS	GEDIT_DEBUG_PLUGINS, __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_TAB	GEDIT_DEBUG_TAB,     __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_DOCUMENT	GEDIT_DEBUG_DOCUMENT,__FILE__, __LINE__, G_STRFUNC
#define	DEBUG_COMMANDS	GEDIT_DEBUG_COMMANDS,__FILE__, __LINE__, G_STRFUNC
#define	DEBUG_APP	GEDIT_DEBUG_APP,     __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_SESSION	GEDIT_DEBUG_SESSION, __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_UTILS	GEDIT_DEBUG_UTILS,   __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_METADATA	GEDIT_DEBUG_METADATA,__FILE__, __LINE__, G_STRFUNC
#define	DEBUG_WINDOW	GEDIT_DEBUG_WINDOW,  __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_LOADER	GEDIT_DEBUG_LOADER,  __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_SAVER	GEDIT_DEBUG_SAVER,   __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_PANEL	GEDIT_DEBUG_PANEL,   __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_DBUS	GEDIT_DEBUG_DBUS,    __FILE__, __LINE__, G_STRFUNC

/**
 * gedit_debug_init:
 *
 * Initializes the debugging subsystem of Gedit.
 *
 * The function checks for the existence of certain environment variables to
 * determine whether to enable output for a debug section. To enable output
 * for a specific debug section, set an environment variable of the same name;
 * e.g. to enable output for the #GEDIT_DEBUG_PLUGINS section, set a
 * <code>GEDIT_DEBUG_PLUGINS</code> environment variable. To enable output
 * for all debug sections, set the <code>GEDIT_DEBUG</code> environment
 * variable.
 *
 * This function must be called before any of the other debug functions are
 * called. It must only be called once.
 */
void gedit_debug_init (void);

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
		  const gchar       *function);

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
void gedit_debug_message (GeditDebugSection  section,
			  const gchar       *file,
			  gint               line,
			  const gchar       *function,
			  const gchar       *format, ...) G_GNUC_PRINTF(5, 6);


#endif /* __GEDIT_DEBUG_H__ */
/* ex:set ts=8 noet: */

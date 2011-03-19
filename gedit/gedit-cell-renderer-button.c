/*
 * gedit-cell-renderer-button.c
 * This file is part of gedit
 *
 * Copyright (C) 2011 - Garrett Regier
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-cell-renderer-button.h"

G_DEFINE_TYPE (GeditCellRendererButton, gedit_cell_renderer_button, GTK_TYPE_CELL_RENDERER_PIXBUF)

enum
{
	CLICKED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void
gedit_cell_renderer_button_render (GtkCellRenderer      *cell,
                                   cairo_t              *cr,
                                   GtkWidget            *widget,
                                   const GdkRectangle   *background_area,
                                   const GdkRectangle   *cell_area,
                                   GtkCellRendererState  flags)
{
  	if ((flags & GTK_CELL_RENDERER_PRELIT) == 0)
  	{
		return;
	}

	GTK_CELL_RENDERER_CLASS (gedit_cell_renderer_button_parent_class)->render (cell, cr, widget,
	                                                                           background_area,
	                                                                           cell_area,
	                                                                           flags);
}

static gboolean
gedit_cell_renderer_button_activate (GtkCellRenderer      *cell,
                                     GdkEvent             *event,
                                     GtkWidget            *widget,
                                     const gchar          *path,
                                     const GdkRectangle   *background_area,
                                     const GdkRectangle   *cell_area,
                                     GtkCellRendererState  flags)
{
	if (event != NULL && event->type == GDK_BUTTON_PRESS)
	{
		g_signal_emit (cell, signals[CLICKED], 0, path);
		return TRUE;
	}

	return FALSE;
}

static void
gedit_cell_renderer_button_class_init (GeditCellRendererButtonClass *klass)
{
	GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (klass);

	cell_class->render = gedit_cell_renderer_button_render;
	cell_class->activate = gedit_cell_renderer_button_activate;

	signals[CLICKED] = g_signal_new ("clicked",
		                         G_OBJECT_CLASS_TYPE (klass),
		                         G_SIGNAL_RUN_LAST,
		                         G_STRUCT_OFFSET (GeditCellRendererButtonClass, clicked),
		                         NULL, NULL,
		                         g_cclosure_marshal_VOID__STRING,
		                         G_TYPE_NONE, 1,
		                         G_TYPE_STRING);
}

static void
gedit_cell_renderer_button_init (GeditCellRendererButton *button)
{
	g_object_set (button, "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE, NULL);
}

GtkCellRenderer *
gedit_cell_renderer_button_new (void)
{
	return g_object_new (GEDIT_TYPE_CELL_RENDERER_BUTTON, NULL);
}

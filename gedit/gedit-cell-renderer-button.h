/*
 * gedit-cell-renderer-button.h
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

#ifndef __GEDIT_CELL_RENDERER_BUTTON_H__
#define __GEDIT_CELL_RENDERER_BUTTON_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_CELL_RENDERER_BUTTON            (gedit_cell_renderer_button_get_type ())
#define GEDIT_CELL_RENDERER_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_CELL_RENDERER_BUTTON, GeditCellRendererButton))
#define GEDIT_CELL_RENDERER_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_CELL_RENDERER_BUTTON, GeditCellRendererButtonClass))
#define GEDIT_IS_CELL_RENDERER_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_CELL_RENDERER_BUTTON))
#define GEDIT_IS_CELL_RENDERER_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_CELL_RENDERER_BUTTON))
#define GEDIT_CELL_RENDERER_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_CELL_RENDERER_BUTTON, GeditCellRendererButtonClass))

typedef struct _GeditCellRendererButton      GeditCellRendererButton;
typedef struct _GeditCellRendererButtonClass GeditCellRendererButtonClass;

struct _GeditCellRendererButton
{
	GtkCellRendererPixbuf  parent_instance;
};

struct _GeditCellRendererButtonClass
{
	GtkCellRendererPixbufClass  parent_class;

	void (* clicked) (GeditCellRendererButton *cell,
	                  const gchar             *path);
};


GType            gedit_cell_renderer_button_get_type (void) G_GNUC_CONST;

GtkCellRenderer *gedit_cell_renderer_button_new      (void);


G_END_DECLS

#endif /* __GEDIT_CELL_RENDERER_BUTTON_H__ */

/*
 * gedit-notebook-popup-menu.h
 * This file is part of gedit
 *
 * Copyright (C) 2011 - Ignacio Casal Quinteiro
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
 * along with gedit. If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __GEDIT_NOTEBOOK_POPUP_MENU_H__
#define __GEDIT_NOTEBOOK_POPUP_MENU_H__

#include <gtk/gtk.h>
#include "gedit-window.h"
#include "gedit-tab.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_NOTEBOOK_POPUP_MENU			(gedit_notebook_popup_menu_get_type ())
#define GEDIT_NOTEBOOK_POPUP_MENU(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_NOTEBOOK_POPUP_MENU, GeditNotebookPopupMenu))
#define GEDIT_NOTEBOOK_POPUP_MENU_CONST(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_NOTEBOOK_POPUP_MENU, GeditNotebookPopupMenu const))
#define GEDIT_NOTEBOOK_POPUP_MENU_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_NOTEBOOK_POPUP_MENU, GeditNotebookPopupMenuClass))
#define GEDIT_IS_NOTEBOOK_POPUP_MENU(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_NOTEBOOK_POPUP_MENU))
#define GEDIT_IS_NOTEBOOK_POPUP_MENU_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_NOTEBOOK_POPUP_MENU))
#define GEDIT_NOTEBOOK_POPUP_MENU_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_NOTEBOOK_POPUP_MENU, GeditNotebookPopupMenuClass))

typedef struct _GeditNotebookPopupMenu		GeditNotebookPopupMenu;
typedef struct _GeditNotebookPopupMenuClass	GeditNotebookPopupMenuClass;
typedef struct _GeditNotebookPopupMenuPrivate	GeditNotebookPopupMenuPrivate;

struct _GeditNotebookPopupMenu
{
	GtkMenu parent;

	GeditNotebookPopupMenuPrivate *priv;
};

struct _GeditNotebookPopupMenuClass
{
	GtkMenuClass parent_class;
};

GType                gedit_notebook_popup_menu_get_type     (void) G_GNUC_CONST;

GtkWidget           *gedit_notebook_popup_menu_new          (GeditWindow *window,
                                                             GeditTab    *tab);

G_END_DECLS

#endif /* __GEDIT_NOTEBOOK_POPUP_MENU_H__ */

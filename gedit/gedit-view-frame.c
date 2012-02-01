/*
 * gedit-view-frame.c
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Ignacio Casal Quinteiro
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-view-frame.h"
#include "gedit-marshal.h"
#include "gedit-debug.h"
#include "gedit-utils.h"
#include "gedit-animated-overlay.h"
#include "gedit-floating-slider.h"

#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include <stdlib.h>

#define GEDIT_VIEW_FRAME_SEARCH_DIALOG_TIMEOUT (30*1000) /* 30 seconds */

#define SEARCH_POPUP_MARGIN 12

#define GEDIT_VIEW_FRAME_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_VIEW_FRAME, GeditViewFramePrivate))

typedef enum
{
	GOTO_LINE,
	SEARCH
} SearchMode;

struct _GeditViewFramePrivate
{
	GtkWidget   *view;
	GtkWidget   *overlay;

	SearchMode   search_mode;
	SearchMode   request_search_mode;

	GtkTextMark *start_mark;

	/* used to restore the search state if an
	 * incremental search is cancelled
	 */
	gchar       *old_search_text;
	guint        old_search_flags;

	/* used to remeber the state of the last
	 * incremental search (the document search
	 * state may be changed by the search dialog)
	 */
	guint        search_flags;

	GtkWidget   *slider;
	GtkWidget   *search_entry;
	GtkWidget   *go_up_button;
	GtkWidget   *go_down_button;

	guint        typeselect_flush_timeout;
	glong        view_scroll_event_id;
	glong        search_entry_focus_out_id;
	glong        search_entry_changed_id;

	guint        disable_popdown : 1;
	guint        wrap_around : 1;
};

enum
{
	PROP_0,
	PROP_DOCUMENT,
	PROP_VIEW
};

typedef enum
{
	GEDIT_SEARCH_ENTRY_NORMAL,
	GEDIT_SEARCH_ENTRY_NOT_FOUND
} GeditSearchEntryBgColor;

G_DEFINE_TYPE (GeditViewFrame, gedit_view_frame, GTK_TYPE_BOX)

static void
gedit_view_frame_finalize (GObject *object)
{
	GeditViewFrame *frame = GEDIT_VIEW_FRAME (object);

	g_free (frame->priv->old_search_text);

	G_OBJECT_CLASS (gedit_view_frame_parent_class)->finalize (object);
}

static void
gedit_view_frame_dispose (GObject *object)
{
	GeditViewFrame *frame = GEDIT_VIEW_FRAME (object);

	if (frame->priv->typeselect_flush_timeout != 0)
	{
		g_source_remove (frame->priv->typeselect_flush_timeout);
		frame->priv->typeselect_flush_timeout = 0;
	}

	G_OBJECT_CLASS (gedit_view_frame_parent_class)->dispose (object);
}

static void
gedit_view_frame_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
	GeditViewFrame *frame = GEDIT_VIEW_FRAME (object);

	switch (prop_id)
	{
		case PROP_DOCUMENT:
			g_value_set_object (value,
			                    gedit_view_frame_get_document (frame));
			break;
		case PROP_VIEW:
			g_value_set_object (value,
			                    gedit_view_frame_get_view (frame));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
hide_search_widget (GeditViewFrame *frame,
                    gboolean        cancel)
{
	GtkTextBuffer *buffer;

	if (frame->priv->disable_popdown)
	{
		return;
	}

	g_signal_handler_block (frame->priv->search_entry,
	                        frame->priv->search_entry_focus_out_id);

	if (frame->priv->view_scroll_event_id != 0)
	{
		g_signal_handler_disconnect (frame->priv->view,
		                             frame->priv->view_scroll_event_id);
		frame->priv->view_scroll_event_id = 0;
	}

	if (frame->priv->typeselect_flush_timeout != 0)
	{
		g_source_remove (frame->priv->typeselect_flush_timeout);
		frame->priv->typeselect_flush_timeout = 0;
	}

	/* To hide the slider we just set the animation-state property */
	g_object_set (G_OBJECT (frame->priv->slider),
	              "animation-state", GEDIT_THEATRICS_ANIMATION_STATE_INTENDING_TO_GO,
	              NULL);

	if (cancel)
	{
		GtkTextBuffer *buffer;
		GtkTextIter iter;

		buffer = GTK_TEXT_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (frame->priv->view)));
		gtk_text_buffer_get_iter_at_mark (buffer, &iter,
		                                  frame->priv->start_mark);
		gtk_text_buffer_place_cursor (buffer, &iter);

		gedit_view_scroll_to_cursor (GEDIT_VIEW (frame->priv->view));
	}

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (frame->priv->view));
	gtk_text_buffer_delete_mark (buffer, frame->priv->start_mark);

	/* Make sure the view is the one who has the focus when we destroy
	   the search widget */
	gtk_widget_grab_focus (frame->priv->view);

	g_signal_handler_unblock (frame->priv->search_entry,
	                          frame->priv->search_entry_focus_out_id);
}

static gboolean
search_entry_flush_timeout (GeditViewFrame *frame)
{
	GDK_THREADS_ENTER ();

	frame->priv->typeselect_flush_timeout = 0;
	hide_search_widget (frame, FALSE);

	GDK_THREADS_LEAVE ();

	return FALSE;
}

static void
set_entry_background (GeditViewFrame          *frame,
                      GtkWidget               *entry,
                      GeditSearchEntryBgColor  col)
{
	GtkStyleContext *context;

	context = gtk_widget_get_style_context (GTK_WIDGET (entry));

	if (col == GEDIT_SEARCH_ENTRY_NOT_FOUND)
	{
		gtk_style_context_add_class (context, "not-found");
	}
	else
	{
		gtk_style_context_remove_class (context, "not-found");
	}
}

static gboolean
run_search (GeditViewFrame   *frame,
            const gchar      *entry_text,
            gboolean          search_backward,
            gboolean          wrap_around,
            gboolean          typing)
{
	GtkTextIter    start_iter;
	GtkTextIter    match_start;
	GtkTextIter    match_end;
	gboolean       found = FALSE;
	GeditDocument *doc;

	g_return_val_if_fail (frame->priv->search_mode == SEARCH, FALSE);

	doc = gedit_view_frame_get_document (frame);

	gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (doc),
	                                  &start_iter,
	                                  frame->priv->start_mark);

	if (*entry_text != '\0')
	{
		if (!search_backward)
		{
			if (!typing)
			{
				/* forward and _NOT_ typing */
				gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (doc),
				                                      &start_iter,
				                                      &match_end);

				gtk_text_iter_order (&match_end, &start_iter);
			}

			/* run search */
			found = gedit_document_search_forward (doc,
			                                       &start_iter,
			                                       NULL,
			                                       &match_start,
			                                       &match_end);
		}
		else if (!typing)
		{
			/* backward and not typing */
			gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (doc),
			                                      &start_iter,
			                                      &match_end);

			/* run search */
			found = gedit_document_search_backward (doc,
			                                        NULL,
			                                        &start_iter,
			                                        &match_start,
			                                        &match_end);
		}
		else
		{
			/* backward (while typing) */
			g_return_val_if_reached (FALSE);
		}

		if (!found && wrap_around)
		{
			if (!search_backward)
			{
				found = gedit_document_search_forward (doc,
				                                       NULL,
				                                       NULL, /* FIXME: set the end_inter */
				                                       &match_start,
				                                       &match_end);
			}
			else
			{
				found = gedit_document_search_backward (doc,
				                                        NULL, /* FIXME: set the start_inter */
				                                        NULL,
				                                        &match_start,
				                                        &match_end);
			}
		}
	}
	else
	{
		gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (doc),
		                                      &start_iter,
		                                      NULL);
	}
	
	if (found)
	{
		gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (doc),
		                              &match_start);

		gtk_text_buffer_move_mark_by_name (GTK_TEXT_BUFFER (doc),
		                                   "selection_bound", &match_end);
	}
	else if (typing)
	{
		gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (doc),
		                                  &start_iter,
		                                  frame->priv->start_mark);
		gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (doc),
		                              &start_iter);
	}

	if (found || (*entry_text == '\0'))
	{
		gedit_view_scroll_to_cursor (GEDIT_VIEW (frame->priv->view));

		set_entry_background (frame, frame->priv->search_entry,
		                      GEDIT_SEARCH_ENTRY_NORMAL);
	}
	else
	{
		set_entry_background (frame, frame->priv->search_entry,
		                      GEDIT_SEARCH_ENTRY_NOT_FOUND);
	}

	return found;
}

static void
search_again (GeditViewFrame *frame,
              gboolean        search_backward)
{
	const gchar *entry_text;

	g_return_if_fail (frame->priv->search_mode == SEARCH);

	/* SEARCH mode */
	/* renew the flush timeout */
	if (frame->priv->typeselect_flush_timeout != 0)
	{
		g_source_remove (frame->priv->typeselect_flush_timeout);
		frame->priv->typeselect_flush_timeout =
			g_timeout_add (GEDIT_VIEW_FRAME_SEARCH_DIALOG_TIMEOUT,
			               (GSourceFunc)search_entry_flush_timeout,
			               frame);
	}

	entry_text = gtk_entry_get_text (GTK_ENTRY (frame->priv->search_entry));

	run_search (frame,
	            entry_text,
	            search_backward,
	            frame->priv->wrap_around,
	            FALSE);
}

static gboolean
search_widget_scroll_event (GtkWidget      *widget,
                            GdkEventScroll *event,
                            GeditViewFrame *frame)
{
	gboolean retval = FALSE;

	if (frame->priv->search_mode == GOTO_LINE)
		return retval;

	/* SEARCH mode */
	if ((event->state & GDK_CONTROL_MASK) &&
	    event->direction == GDK_SCROLL_UP)
	{
		search_again (frame, TRUE);
		retval = TRUE;
	}
	else if ((event->state & GDK_CONTROL_MASK) &&
	         event->direction == GDK_SCROLL_DOWN)
	{
		search_again (frame, FALSE);
		retval = TRUE;
	}

	return retval;
}

static gboolean
search_widget_key_press_event (GtkWidget      *widget,
                               GdkEventKey    *event,
                               GeditViewFrame *frame)
{
	gboolean retval = FALSE;
	guint modifiers;

	modifiers = gtk_accelerator_get_default_mod_mask ();

	/* Close window */
	if (event->keyval == GDK_KEY_Tab)
	{
		hide_search_widget (frame, FALSE);
		retval = TRUE;
	}

	/* Close window and cancel the search */
	if (event->keyval == GDK_KEY_Escape)
	{
		if (frame->priv->search_mode == SEARCH)
		{
			GeditDocument *doc;

			/* restore document search so that Find Next does the right thing */
			doc = gedit_view_frame_get_document (frame);
			gedit_document_set_search_text (doc,
			                                frame->priv->old_search_text,
			                                frame->priv->old_search_flags);
		}

		hide_search_widget (frame, TRUE);
		retval = TRUE;
	}

	if (frame->priv->search_mode == GOTO_LINE)
		return retval;

	/* SEARCH mode */

	/* select previous matching iter */
	if (event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_KP_Up)
	{
		search_again (frame, TRUE);
		retval = TRUE;
	}

	if (((event->state & modifiers) == (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) && 
	    (event->keyval == GDK_KEY_g || event->keyval == GDK_KEY_G))
	{
		search_again (frame, TRUE);
		retval = TRUE;
	}

	/* select next matching iter */
	if (event->keyval == GDK_KEY_Down || event->keyval == GDK_KEY_KP_Down)
	{
		search_again (frame, FALSE);
		retval = TRUE;
	}

	if (((event->state & modifiers) == GDK_CONTROL_MASK) && 
	    (event->keyval == GDK_KEY_g || event->keyval == GDK_KEY_G))
	{
		search_again (frame, FALSE);
		retval = TRUE;
	}

	return retval;
}

static void
update_search (GeditViewFrame *frame)
{
	if (frame->priv->search_mode == SEARCH)
	{
		GeditDocument *doc;
		const gchar *entry_text;
		gchar *search_text;
		guint  search_flags;

		doc = gedit_view_frame_get_document (frame);

		entry_text = gtk_entry_get_text (GTK_ENTRY (frame->priv->search_entry));

		search_text = gedit_document_get_search_text (doc, &search_flags);

		if ((search_text == NULL) ||
		    (strcmp (search_text, entry_text) != 0) ||
		     search_flags != frame->priv->search_flags)
		{
			gedit_document_set_search_text (doc,
			                                entry_text,
			                                frame->priv->search_flags);
		}

		g_free (search_text);
	}
}

static void
wrap_around_menu_item_toggled (GtkCheckMenuItem *checkmenuitem,
                               GeditViewFrame   *frame)
{
	frame->priv->wrap_around = gtk_check_menu_item_get_active (checkmenuitem);
}

static void
match_entire_word_menu_item_toggled (GtkCheckMenuItem *checkmenuitem,
                                     GeditViewFrame   *frame)
{
	GEDIT_SEARCH_SET_ENTIRE_WORD (frame->priv->search_flags,
	                              gtk_check_menu_item_get_active (checkmenuitem));
	update_search (frame);
}

static void
match_case_menu_item_toggled (GtkCheckMenuItem *checkmenuitem,
                              GeditViewFrame   *frame)
{
	GEDIT_SEARCH_SET_CASE_SENSITIVE (frame->priv->search_flags,
	                                 gtk_check_menu_item_get_active (checkmenuitem));
	update_search (frame);
}

static void
add_popup_menu_items (GtkWidget      *menu,
                      GeditViewFrame *frame)
{
	GtkWidget *menu_item;

	/* create "Wrap Around" menu item. */
	menu_item = gtk_check_menu_item_new_with_mnemonic (_("_Wrap Around"));
	g_signal_connect (G_OBJECT (menu_item), "toggled",
			  G_CALLBACK (wrap_around_menu_item_toggled),
			  frame);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item),
					frame->priv->wrap_around);
	gtk_widget_show (menu_item);

	/* create "Match Entire Word Only" menu item. */
	menu_item = gtk_check_menu_item_new_with_mnemonic (_("Match _Entire Word Only"));
	g_signal_connect (G_OBJECT (menu_item), "toggled",
			  G_CALLBACK (match_entire_word_menu_item_toggled),
			  frame);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item),
					GEDIT_SEARCH_IS_ENTIRE_WORD (frame->priv->search_flags));
	gtk_widget_show (menu_item);

	/* create "Match Case" menu item. */
	menu_item = gtk_check_menu_item_new_with_mnemonic (_("_Match Case"));
	g_signal_connect (G_OBJECT (menu_item), "toggled",
			  G_CALLBACK (match_case_menu_item_toggled),
			  frame);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item),
					GEDIT_SEARCH_IS_CASE_SENSITIVE (frame->priv->search_flags));
	gtk_widget_show (menu_item);
}

static gboolean
real_search_enable_popdown (gpointer data)
{
	GeditViewFrame *frame = GEDIT_VIEW_FRAME (data);

	GDK_THREADS_ENTER ();

	frame->priv->disable_popdown = FALSE;

	GDK_THREADS_LEAVE ();

	return FALSE;
}

static void
search_enable_popdown (GtkWidget      *widget,
                       GeditViewFrame *frame)
{
	g_timeout_add (200, real_search_enable_popdown, frame);

	/* renew the flush timeout */
	if (frame->priv->typeselect_flush_timeout != 0)
		g_source_remove (frame->priv->typeselect_flush_timeout);

	frame->priv->typeselect_flush_timeout =
		g_timeout_add (GEDIT_VIEW_FRAME_SEARCH_DIALOG_TIMEOUT,
		               (GSourceFunc)search_entry_flush_timeout,
		               frame);
}

static void
search_entry_populate_popup (GtkEntry       *entry,
                             GtkMenu        *menu,
                             GeditViewFrame *frame)
{
	GtkWidget *menu_item;

	frame->priv->disable_popdown = TRUE;
	g_signal_connect (menu, "hide",
			  G_CALLBACK (search_enable_popdown), frame);

	if (frame->priv->search_mode == GOTO_LINE)
		return;

	/* separator */
	menu_item = gtk_menu_item_new ();
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	add_popup_menu_items (GTK_WIDGET (menu), frame);
}

static void
search_entry_icon_release (GtkEntry            *entry,
                           GtkEntryIconPosition icon_pos,
                           GdkEventButton      *event,
                           GeditViewFrame      *frame)
{
	GtkWidget *menu;

	if (frame->priv->search_mode == GOTO_LINE ||
	    icon_pos != GTK_ENTRY_ICON_PRIMARY)
		return;

	menu = gtk_menu_new ();
	gtk_widget_show (menu);

	frame->priv->disable_popdown = TRUE;
	g_signal_connect (menu, "hide",
	                  G_CALLBACK (search_enable_popdown), frame);

	add_popup_menu_items (menu, frame);

	gtk_menu_popup (GTK_MENU (menu),
	                NULL, NULL,
	                NULL, NULL,
	                event->button, event->time);
}

static void
search_entry_activate (GtkEntry       *entry,
                       GeditViewFrame *frame)
{
	hide_search_widget (frame, FALSE);
}

static void
search_entry_insert_text (GtkEditable    *editable,
                          const gchar    *text,
                          gint            length,
                          gint           *position,
                          GeditViewFrame *frame)
{
	if (frame->priv->search_mode == GOTO_LINE)
	{
		gunichar c;
		const gchar *p;
		const gchar *end;
		const gchar *next;

		p = text;
		end = text + length;

		if (p == end)
			return;

		c = g_utf8_get_char (p);
		
		if (((c == '-' || c == '+') && *position == 0) ||
		    (c == ':' && *position != 0))
		{
			gchar *s = NULL;
		
			if (c == ':')
			{
				s = gtk_editable_get_chars (editable, 0, -1);
				s = g_utf8_strchr (s, -1, ':');
			}
			
			if (s == NULL || s == p)
			{
				next = g_utf8_next_char (p);
				p = next;
			}
			
			g_free (s);
		}

		while (p != end)
		{
			next = g_utf8_next_char (p);

			c = g_utf8_get_char (p);

			if (!g_unichar_isdigit (c))
			{
				g_signal_stop_emission_by_name (editable, "insert_text");
				gtk_widget_error_bell (frame->priv->search_entry);
				break;
			}

			p = next;
		}
	}
	else
	{
		/* SEARCH mode */
		static gboolean  insert_text = FALSE;
		gchar           *escaped_text;
		gint             new_len;

		gedit_debug_message (DEBUG_SEARCH, "Text: %s", text);

		/* To avoid recursive behavior */
		if (insert_text)
			return;

		escaped_text = gedit_utils_escape_search_text (text);

		gedit_debug_message (DEBUG_SEARCH, "Escaped Text: %s", escaped_text);

		new_len = strlen (escaped_text);

		if (new_len == length)
		{
			g_free (escaped_text);
			return;
		}

		insert_text = TRUE;

		g_signal_stop_emission_by_name (editable, "insert_text");
		
		gtk_editable_insert_text (editable, escaped_text, new_len, position);

		insert_text = FALSE;

		g_free (escaped_text);
	}
}

static void
customize_for_search_mode (GeditViewFrame *frame)
{
	GIcon *icon;

	if (frame->priv->search_mode == SEARCH)
	{
		icon = g_themed_icon_new_with_default_fallbacks ("edit-find-symbolic");

		gtk_widget_set_tooltip_text (frame->priv->search_entry,
		                             _("String you want to search for"));

		gtk_widget_show (frame->priv->go_up_button);
		gtk_widget_show (frame->priv->go_down_button);
	}
	else
	{
		icon = g_themed_icon_new_with_default_fallbacks ("go-jump-symbolic");

		gtk_widget_set_tooltip_text (frame->priv->search_entry,
		                             _("Line you want to move the cursor to"));

		gtk_widget_hide (frame->priv->go_up_button);
		gtk_widget_hide (frame->priv->go_down_button);
	}

	gtk_entry_set_icon_from_gicon (GTK_ENTRY (frame->priv->search_entry),
	                               GTK_ENTRY_ICON_PRIMARY,
	                               icon);

	g_object_unref (icon);
}

static void
search_init (GtkWidget      *entry,
             GeditViewFrame *frame)
{
	const gchar *entry_text;

	/* renew the flush timeout */
	if (frame->priv->typeselect_flush_timeout != 0)
	{
		g_source_remove (frame->priv->typeselect_flush_timeout);
		frame->priv->typeselect_flush_timeout =
			g_timeout_add (GEDIT_VIEW_FRAME_SEARCH_DIALOG_TIMEOUT,
			               (GSourceFunc)search_entry_flush_timeout,
			               frame);
	}

	entry_text = gtk_entry_get_text (GTK_ENTRY (entry));

	if (frame->priv->search_mode == SEARCH)
	{
		update_search (frame);

		run_search (frame,
		            entry_text,
		            FALSE,
		            frame->priv->wrap_around,
		            TRUE);
	}
	else
	{
		if (*entry_text != '\0')
		{
			gboolean moved, moved_offset;
			gint line;
			gint offset_line = 0;
			gint line_offset = 0;
			gchar **split_text = NULL;
			const gchar *text;
			GtkTextIter iter;
			GeditDocument *doc;

			doc = gedit_view_frame_get_document (frame);

			gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (doc),
			                                  &iter,
			                                  frame->priv->start_mark);

			split_text = g_strsplit (entry_text, ":", -1);

			if (g_strv_length (split_text) > 1)
			{
				text = split_text[0];
			}
			else
			{
				text = entry_text;
			}

			if (*text == '-')
			{
				gint cur_line = gtk_text_iter_get_line (&iter);

				if (*(text + 1) != '\0')
					offset_line = MAX (atoi (text + 1), 0);

				line = MAX (cur_line - offset_line, 0);
			}
			else if (*entry_text == '+')
			{
				gint cur_line = gtk_text_iter_get_line (&iter);

				if (*(text + 1) != '\0')
					offset_line = MAX (atoi (text + 1), 0);

				line = cur_line + offset_line;
			}
			else
			{
				line = MAX (atoi (text) - 1, 0);
			}

			if (split_text[1] != NULL)
			{
				line_offset = atoi (split_text[1]);
			}

			g_strfreev (split_text);

			moved = gedit_document_goto_line (doc, line);
			moved_offset = gedit_document_goto_line_offset (doc, line,
			                                                line_offset);

			gedit_view_scroll_to_cursor (GEDIT_VIEW (frame->priv->view));

			if (!moved || !moved_offset)
			{
				set_entry_background (frame, frame->priv->search_entry,
				                      GEDIT_SEARCH_ENTRY_NOT_FOUND);
			}
			else
			{
				set_entry_background (frame, frame->priv->search_entry,
				                      GEDIT_SEARCH_ENTRY_NORMAL);
			}
		}
	}
}

static gboolean
search_entry_focus_out_event (GtkWidget      *widget,
                              GdkEventFocus  *event,
                              GeditViewFrame *frame)
{
	/* hide interactive search dialog */
	hide_search_widget (frame, FALSE);

	return FALSE;
}

static GtkWidget *
create_button_from_symbolic (const gchar *icon_name)
{
	GtkWidget *button;

	button = gtk_button_new ();
	gtk_widget_set_can_focus (button, FALSE);
	gtk_button_set_image (GTK_BUTTON (button),
	                      gtk_image_new_from_icon_name (icon_name,
	                                                    GTK_ICON_SIZE_MENU));

	return button;
}

static void
on_go_up_button_clicked (GtkWidget      *button,
                         GeditViewFrame *frame)
{
	search_again (frame, TRUE);
}

static void
on_go_down_button_clicked (GtkWidget      *button,
                           GeditViewFrame *frame)
{
	search_again (frame, FALSE);
}

static GtkWidget *
create_search_widget (GeditViewFrame *frame)
{
	GtkWidget *search_widget;
	GtkStyleContext *context;

	search_widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

	context = gtk_widget_get_style_context (search_widget);
	gtk_style_context_add_class (context, GTK_STYLE_CLASS_LINKED);

	gtk_widget_show (search_widget);

	g_signal_connect (search_widget, "key-press-event",
	                  G_CALLBACK (search_widget_key_press_event),
	                  frame);
	g_signal_connect (search_widget, "scroll-event",
	                  G_CALLBACK (search_widget_scroll_event),
	                  frame);

	/* add entry */
	frame->priv->search_entry = gtk_entry_new ();
	gtk_widget_show (frame->priv->search_entry);

	gtk_entry_set_width_chars (GTK_ENTRY (frame->priv->search_entry), 25);

	g_signal_connect (frame->priv->search_entry, "populate-popup",
	                  G_CALLBACK (search_entry_populate_popup),
	                  frame);
	g_signal_connect (frame->priv->search_entry, "icon-release",
	                  G_CALLBACK (search_entry_icon_release),
	                  frame);
	g_signal_connect (frame->priv->search_entry, "activate",
	                  G_CALLBACK (search_entry_activate),
	                  frame);
	g_signal_connect (frame->priv->search_entry, "insert_text",
	                  G_CALLBACK (search_entry_insert_text),
	                  frame);
	frame->priv->search_entry_changed_id =
		g_signal_connect (frame->priv->search_entry, "changed",
		                  G_CALLBACK (search_init),
		                  frame);
	frame->priv->search_entry_focus_out_id =
		g_signal_connect (frame->priv->search_entry, "focus-out-event",
		                  G_CALLBACK (search_entry_focus_out_event),
		                  frame);

	gtk_container_add (GTK_CONTAINER (search_widget),
	                   frame->priv->search_entry);

	/* Up/Down buttons */
	frame->priv->go_up_button = create_button_from_symbolic ("go-up-symbolic");
	gtk_box_pack_start (GTK_BOX (search_widget), frame->priv->go_up_button,
	                    FALSE, FALSE, 0);
	g_signal_connect (frame->priv->go_up_button,
	                  "clicked",
	                  G_CALLBACK (on_go_up_button_clicked),
	                  frame);

	frame->priv->go_down_button = create_button_from_symbolic ("go-down-symbolic");
	gtk_box_pack_start (GTK_BOX (search_widget), frame->priv->go_down_button,
	                    FALSE, FALSE, 0);
	g_signal_connect (frame->priv->go_down_button,
	                  "clicked",
	                  G_CALLBACK (on_go_down_button_clicked),
	                  frame);

	return search_widget;
}

static gboolean
get_selected_text (GtkTextBuffer *doc, gchar **selected_text, gint *len)
{
	GtkTextIter start, end;

	g_return_val_if_fail (selected_text != NULL, FALSE);
	g_return_val_if_fail (*selected_text == NULL, FALSE);

	if (!gtk_text_buffer_get_selection_bounds (doc, &start, &end))
	{
		if (len != NULL)
			len = 0;

		return FALSE;
	}

	*selected_text = gtk_text_buffer_get_slice (doc, &start, &end, TRUE);

	if (len != NULL)
		*len = g_utf8_strlen (*selected_text, -1);

	return TRUE;
}

static void
init_search_entry (GeditViewFrame *frame)
{
	GtkTextBuffer *buffer;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (frame->priv->view));

	customize_for_search_mode (frame);

	if (frame->priv->search_mode == GOTO_LINE)
	{
		gint   line;
		gchar *line_str;
		GtkTextIter iter;

		gtk_text_buffer_get_iter_at_mark (buffer,
		                                  &iter,
		                                  frame->priv->start_mark);

		line = gtk_text_iter_get_line (&iter);

		line_str = g_strdup_printf ("%d", line + 1);

		gtk_entry_set_text (GTK_ENTRY (frame->priv->search_entry),
		                    line_str);
		gtk_editable_select_region (GTK_EDITABLE (frame->priv->search_entry),
		                            0, -1);

		g_free (line_str);

		return;
	}
	else
	{
		/* SEARCH mode */
		gboolean  selection_exists;
		gchar    *find_text = NULL;
		gchar    *old_find_text;
		guint     old_find_flags = 0;
		gint      sel_len = 0;

		old_find_text = gedit_document_get_search_text (GEDIT_DOCUMENT (buffer),
		                                                &old_find_flags);

		if (old_find_flags != 0)
		{
			frame->priv->old_search_flags = old_find_flags;
		}

		selection_exists = get_selected_text (buffer,
		                                      &find_text,
		                                      &sel_len);

		if (selection_exists  && (find_text != NULL) && (sel_len <= 160))
		{
			gtk_entry_set_text (GTK_ENTRY (frame->priv->search_entry),
			                    find_text);
			gtk_editable_set_position (GTK_EDITABLE (frame->priv->search_entry),
			                           -1);
		}
		else if (old_find_text != NULL)
		{
			g_free (frame->priv->old_search_text);
			frame->priv->old_search_text = old_find_text;
			g_signal_handler_block (frame->priv->search_entry,
			                        frame->priv->search_entry_changed_id);

			gtk_entry_set_text (GTK_ENTRY (frame->priv->search_entry),
			                    old_find_text);
			gtk_editable_select_region (GTK_EDITABLE (frame->priv->search_entry),
			                            0, -1);

			g_signal_handler_unblock (frame->priv->search_entry,
			                          frame->priv->search_entry_changed_id);
		}

		g_free (find_text);
	}
}

static void
start_interactive_search_real (GeditViewFrame *frame)
{
	GtkTextBuffer *buffer;
	GtkTextIter iter;
	GtkTextMark *mark;

	if (gtk_bin_get_child (GTK_BIN (frame->priv->slider)) == NULL)
	{
		gtk_container_add (GTK_CONTAINER (frame->priv->slider),
		                   create_search_widget (frame));
	}

	if (gtk_widget_get_visible (frame->priv->slider))
	{
		if (frame->priv->search_mode != frame->priv->request_search_mode)
		{
			hide_search_widget (frame, TRUE);
		}
		else
		{
			gtk_editable_select_region (GTK_EDITABLE (frame->priv->search_entry),
			                            0, -1);
			return;
		}
	}

	frame->priv->search_mode = frame->priv->request_search_mode;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (frame->priv->view));

	if (frame->priv->search_mode == SEARCH)
	{
		GtkTextIter start, end;

		if (gtk_text_buffer_get_selection_bounds (buffer, &start, &end))
		{
			if (gtk_text_iter_compare (&start, &end) == -1)
			{
				iter = start;
			}
			else
			{
				iter = end;
			}
		}
		else
		{
			mark = gtk_text_buffer_get_selection_bound (buffer);
			gtk_text_buffer_get_iter_at_mark (buffer, &iter, mark);
		}
	}
	else
	{
		mark = gtk_text_buffer_get_insert (buffer);
		gtk_text_buffer_get_iter_at_mark (buffer, &iter, mark);
	}

	frame->priv->start_mark = gtk_text_buffer_create_mark (buffer, NULL,
	                                                       &iter, FALSE);

	/* To slide in we set the right animation state in the animatable */
	g_object_set (G_OBJECT (frame->priv->slider),
	              "animation-state", GEDIT_THEATRICS_ANIMATION_STATE_COMING,
	              NULL);

	/* NOTE: we must be very careful here to not have any text before
	   focusing the entry because when the entry is focused the text is
	   selected, and gtk+ doesn't allow us to have more than one selection
	   active */
	g_signal_handler_block (frame->priv->search_entry,
	                        frame->priv->search_entry_changed_id);
	gtk_entry_set_text (GTK_ENTRY (frame->priv->search_entry),
	                    "");
	g_signal_handler_unblock (frame->priv->search_entry,
	                          frame->priv->search_entry_changed_id);

	/* We need to grab the focus after the widget has been added */
	gtk_widget_grab_focus (frame->priv->search_entry);

	init_search_entry (frame);

	/* Manage the scroll also for the view */
	frame->priv->view_scroll_event_id =
		g_signal_connect (frame->priv->view, "scroll-event",
			          G_CALLBACK (search_widget_scroll_event),
			          frame);

	frame->priv->typeselect_flush_timeout =
		g_timeout_add (GEDIT_VIEW_FRAME_SEARCH_DIALOG_TIMEOUT,
		               (GSourceFunc) search_entry_flush_timeout,
		               frame);
}

static void
gedit_view_frame_class_init (GeditViewFrameClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_view_frame_finalize;
	object_class->dispose = gedit_view_frame_dispose;
	object_class->get_property = gedit_view_frame_get_property;

	g_object_class_install_property (object_class, PROP_DOCUMENT,
	                                 g_param_spec_object ("document",
	                                                      "Document",
	                                                      "The Document",
	                                                      GEDIT_TYPE_DOCUMENT,
	                                                      G_PARAM_READABLE |
	                                                      G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class, PROP_VIEW,
	                                 g_param_spec_object ("view",
	                                                      "View",
	                                                      "The View",
	                                                      GEDIT_TYPE_VIEW,
	                                                      G_PARAM_READABLE |
	                                                      G_PARAM_STATIC_STRINGS));

	g_type_class_add_private (object_class, sizeof (GeditViewFramePrivate));
}

static GMountOperation *
view_frame_mount_operation_factory (GeditDocument   *doc,
				    gpointer         user_data)
{
	GeditViewFrame *frame = GEDIT_VIEW_FRAME (user_data);
	GtkWidget *window;

	window = gtk_widget_get_toplevel (GTK_WIDGET (frame));

	return gtk_mount_operation_new (GTK_WINDOW (window));
}

static void
gedit_view_frame_init (GeditViewFrame *frame)
{
	GeditDocument *doc;
	GtkWidget *sw;

	frame->priv = GEDIT_VIEW_FRAME_GET_PRIVATE (frame);

	frame->priv->typeselect_flush_timeout = 0;
	frame->priv->wrap_around = TRUE;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (frame),
	                                GTK_ORIENTATION_VERTICAL);

	doc = gedit_document_new ();

	_gedit_document_set_mount_operation_factory (doc,
						     view_frame_mount_operation_factory,
						     frame);

	frame->priv->view = gedit_view_new (doc);
	gtk_widget_set_vexpand (frame->priv->view, TRUE);
	gtk_widget_show (frame->priv->view);

	g_object_unref (doc);

	/* Create the scrolled window */
	sw = gtk_scrolled_window_new (NULL, NULL);

	gtk_container_add (GTK_CONTAINER (sw), frame->priv->view);

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	gtk_widget_show (sw);

	frame->priv->overlay = gedit_animated_overlay_new ();
	gtk_container_add (GTK_CONTAINER (frame->priv->overlay), sw);
	gtk_widget_show (frame->priv->overlay);

	gtk_box_pack_start (GTK_BOX (frame), frame->priv->overlay, TRUE, TRUE, 0);

	/* Add slider */
	frame->priv->slider = gedit_floating_slider_new ();
	gtk_widget_set_name (frame->priv->slider, "search-slider");
	gtk_widget_set_halign (frame->priv->slider, GTK_ALIGN_END);
	gtk_widget_set_valign (frame->priv->slider, GTK_ALIGN_START);

	if (gtk_widget_get_direction (frame->priv->slider) == GTK_TEXT_DIR_LTR)
	{
		gtk_widget_set_margin_right (frame->priv->slider, SEARCH_POPUP_MARGIN);
	}
	else
	{
		gtk_widget_set_margin_left (frame->priv->slider, SEARCH_POPUP_MARGIN);
	}

	g_object_set (G_OBJECT (frame->priv->slider),
	              "easing", GEDIT_THEATRICS_CHOREOGRAPHER_EASING_EXPONENTIAL_IN_OUT,
	              "blocking", GEDIT_THEATRICS_CHOREOGRAPHER_BLOCKING_DOWNSTAGE,
	              "orientation", GTK_ORIENTATION_VERTICAL,
	              NULL);

	gedit_animated_overlay_add_animated_overlay (GEDIT_ANIMATED_OVERLAY (frame->priv->overlay),
	                                             GEDIT_ANIMATABLE (frame->priv->slider));
}

GeditViewFrame *
gedit_view_frame_new ()
{
	return g_object_new (GEDIT_TYPE_VIEW_FRAME, NULL);
}

GeditDocument *
gedit_view_frame_get_document (GeditViewFrame *frame)
{
	g_return_val_if_fail (GEDIT_IS_VIEW_FRAME (frame), NULL);

	return GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (frame->priv->view)));
}

GeditView *
gedit_view_frame_get_view (GeditViewFrame *frame)
{
	g_return_val_if_fail (GEDIT_IS_VIEW_FRAME (frame), NULL);

	return GEDIT_VIEW (frame->priv->view);
}

void
gedit_view_frame_popup_search (GeditViewFrame *frame)
{
	g_return_if_fail (GEDIT_IS_VIEW_FRAME (frame));

	frame->priv->request_search_mode = SEARCH;

	start_interactive_search_real (frame);
}

void
gedit_view_frame_popup_goto_line (GeditViewFrame *frame)
{
	g_return_if_fail (GEDIT_IS_VIEW_FRAME (frame));

	frame->priv->request_search_mode = GOTO_LINE;

	start_interactive_search_real (frame);
}

void
gedit_view_frame_clear_search (GeditViewFrame *frame)
{
	GeditDocument *doc;

	g_return_if_fail (GEDIT_IS_VIEW_FRAME (frame));

	doc = gedit_view_frame_get_document (frame);

	gedit_document_set_search_text (doc, "", GEDIT_SEARCH_DONT_SET_FLAGS);
	g_signal_handler_block (frame->priv->search_entry,
	                        frame->priv->search_entry_changed_id);
	gtk_entry_set_text (GTK_ENTRY (frame->priv->search_entry), "");
	g_signal_handler_unblock (frame->priv->search_entry,
	                          frame->priv->search_entry_changed_id);

	gtk_widget_grab_focus (frame->priv->view);
}

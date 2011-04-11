/*
 * gedit-rounded-frame.c
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Ignacio Casal Quinteiro
 *
 * Work based on Aaron Bockover <abockover@novell.com>
 *               Gabriel Burt <gburt@novell.com>
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

#include "gedit-rounded-frame.h"
#include "theatrics/gedit-theatrics-utils.h"


#define GEDIT_ROUNDED_FRAME_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_ROUNDED_FRAME, GeditRoundedFramePrivate))

struct _GeditRoundedFramePrivate
{
	GtkAllocation child_allocation;
};

G_DEFINE_TYPE (GeditRoundedFrame, gedit_rounded_frame, GTK_TYPE_BIN)

static void
gedit_rounded_frame_finalize (GObject *object)
{
	G_OBJECT_CLASS (gedit_rounded_frame_parent_class)->finalize (object);
}

static void
gedit_rounded_frame_get_preferred_size (GtkWidget      *widget,
                                        GtkOrientation  orientation,
                                        gint           *minimum_size,
                                        gint           *natural_size)
{
	GtkStyleContext *context;
	GtkStateFlags state;
	GtkBorder padding;
	GtkWidget *child;
	gint child_min, child_nat;
	gint minimum, natural;
	guint border_width;

	context = gtk_widget_get_style_context (widget);
	state = gtk_widget_get_state_flags (widget);

	gtk_style_context_save (context);
	gtk_style_context_add_class (context, GTK_STYLE_CLASS_FRAME);
	gtk_style_context_get_padding (context, state, &padding);

	minimum = 0;
	natural = 0;

	child = gtk_bin_get_child (GTK_BIN (widget));
	if (child && gtk_widget_get_visible (child))
	{
		if (orientation == GTK_ORIENTATION_HORIZONTAL)
		{
			gtk_widget_get_preferred_width (child,
			                                &child_min, &child_nat);
		}
		else
		{
			gtk_widget_get_preferred_height (child,
			                                 &child_min, &child_nat);
		}

		minimum += child_min;
		natural += child_nat;
	}

	border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));

	if (orientation == GTK_ORIENTATION_HORIZONTAL)
	{
		minimum += (border_width * 2) + padding.left + padding.right;
		natural += (border_width * 2) + padding.left + padding.right;
	}
	else
	{
		minimum += (border_width * 2) + padding.top + padding.bottom;
		natural += (border_width * 2) + padding.top + padding.bottom;
	}

	if (minimum_size)
		*minimum_size = minimum;

	if (natural_size)
		*natural_size = natural;

	gtk_style_context_restore (context);
}

static void
gedit_rounded_frame_get_preferred_width (GtkWidget *widget,
                                         gint      *minimum,
                                         gint      *natural)
{
	gedit_rounded_frame_get_preferred_size (widget, GTK_ORIENTATION_HORIZONTAL, minimum, natural);
}

static void
gedit_rounded_frame_get_preferred_height (GtkWidget *widget,
                                          gint      *minimum,
                                          gint      *natural)
{
	gedit_rounded_frame_get_preferred_size (widget, GTK_ORIENTATION_VERTICAL, minimum, natural);
}

static void
gedit_rounded_frame_size_allocate (GtkWidget     *widget,
                                   GtkAllocation *allocation)
{
	GeditRoundedFramePrivate *priv = GEDIT_ROUNDED_FRAME (widget)->priv;
	GtkAllocation child_allocation;
	GtkStyleContext *context;
	GtkStateFlags state;
	GtkBorder padding;
	gint border_width;
	GtkWidget *child;

	GTK_WIDGET_CLASS (gedit_rounded_frame_parent_class)->size_allocate (widget, allocation);

	/* If the child allocation changed, that means that the frame is drawn
	 * in a new place, so we must redraw the entire widget.
	 */
	if (gtk_widget_get_mapped (widget))
	{
		gdk_window_invalidate_rect (gtk_widget_get_window (widget),
		                            allocation, FALSE);
	}

	context = gtk_widget_get_style_context (widget);
	state = gtk_widget_get_state_flags (widget);

	gtk_style_context_save (context);
	gtk_style_context_add_class (context, GTK_STYLE_CLASS_FRAME);

	gtk_style_context_get_padding (context, state, &padding);

	border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));

	child_allocation.x = border_width + padding.left;
	child_allocation.y = border_width + padding.top;
	child_allocation.width = MAX (1, allocation->width - (border_width * 2) -
	                              padding.left - padding.right);
	child_allocation.height = MAX (1, (allocation->height - child_allocation.y -
	                                   border_width - padding.bottom));

	child_allocation.x += allocation->x;
	child_allocation.y += allocation->y;

	gtk_style_context_restore (context);

	child = gtk_bin_get_child (GTK_BIN (widget));

	if (child && gtk_widget_get_visible (child))
	{
		gtk_widget_size_allocate (child, &child_allocation);
	}

	priv->child_allocation = child_allocation;
}

static void
draw_frame (GtkWidget    *frame,
            cairo_t      *cr,
            GdkRectangle *area)
{
	GtkStyleContext *context;
	GdkRGBA bg_color;
	GdkRGBA border_color;

	context = gtk_widget_get_style_context (frame);
	gtk_style_context_save (context);
	gtk_style_context_add_class (context, GTK_STYLE_CLASS_FRAME);

	gedit_theatrics_utils_draw_round_rectangle (cr,
	                                            FALSE,
	                                            FALSE,
	                                            TRUE,
	                                            TRUE,
	                                            area->x,
	                                            area->y,
	                                            MIN (0.30 * area->width, 0.30 * area->height),
	                                            area->width,
	                                            area->height);

	gtk_style_context_get_background_color (context, GTK_STATE_FLAG_NORMAL,
	                                        &bg_color);
	gtk_style_context_get_border_color (context, GTK_STATE_FLAG_ACTIVE,
	                                    &border_color);

	gdk_cairo_set_source_rgba (cr, &bg_color);
	cairo_fill_preserve (cr);

	gdk_cairo_set_source_rgba (cr, &border_color);
	/* TODO: get the size from the theme? */
	cairo_set_line_width (cr, 1.5);
	cairo_stroke (cr);

	gtk_style_context_restore (context);
}

static gboolean
gedit_rounded_frame_draw (GtkWidget      *widget,
                          cairo_t        *cr)
{
	GeditRoundedFramePrivate *priv = GEDIT_ROUNDED_FRAME (widget)->priv;
	GdkRectangle area;
	GtkAllocation allocation;
	GtkStyleContext *context;
	GtkStateFlags state;
	GtkBorder padding;

	if (!gtk_widget_is_drawable (widget))
	{
		return FALSE;
	}

	context = gtk_widget_get_style_context (widget);
	state = gtk_widget_get_state_flags (widget);
	gtk_widget_get_allocation (widget, &allocation);

	gtk_style_context_save (context);
	gtk_style_context_add_class (context, GTK_STYLE_CLASS_FRAME);

	gtk_style_context_get_padding (context, state, &padding);

	area.x = priv->child_allocation.x - allocation.x - padding.left;
	area.y = priv->child_allocation.y - allocation.y - padding.top;
	area.width = priv->child_allocation.width + padding.left + padding.right;
	area.height =  priv->child_allocation.height + padding.top + padding.bottom;

	draw_frame (widget, cr, &area);

	gtk_style_context_restore (context);

	return GTK_WIDGET_CLASS (gedit_rounded_frame_parent_class)->draw (widget, cr);
}

static void
gedit_rounded_frame_class_init (GeditRoundedFrameClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	object_class->finalize = gedit_rounded_frame_finalize;

	widget_class->get_preferred_width = gedit_rounded_frame_get_preferred_width;
	widget_class->get_preferred_height = gedit_rounded_frame_get_preferred_height;
	widget_class->size_allocate = gedit_rounded_frame_size_allocate;
	widget_class->draw = gedit_rounded_frame_draw;

	g_type_class_add_private (object_class, sizeof (GeditRoundedFramePrivate));
}

static void
gedit_rounded_frame_init (GeditRoundedFrame *frame)
{
	frame->priv = GEDIT_ROUNDED_FRAME_GET_PRIVATE (frame);
}

GtkWidget *
gedit_rounded_frame_new ()
{
	return GTK_WIDGET (g_object_new (GEDIT_TYPE_ROUNDED_FRAME, NULL));
}

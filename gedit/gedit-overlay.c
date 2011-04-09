/*
 * gedit-overlay.c
 * This file is part of gedit
 *
 * Copyright (C) 2011 - Ignacio Casal Quinteiro
 *
 * Based on Mike Krüger <mkrueger@novell.com> work.
 *
 * gedit is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * gedit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "gedit-overlay.h"
#include "gedit-overlay-child.h"

#define GEDIT_OVERLAY_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_OVERLAY, GeditOverlayPrivate))

struct _GeditOverlayPrivate
{
	GtkWidget *main_widget;
	GtkWidget *relative_widget;
	GSList    *children;

	guint composited : 1;
};

enum
{
	PROP_0,
	PROP_MAIN_WIDGET,
	PROP_RELATIVE_WIDGET,
	PROP_COMPOSITED
};

G_DEFINE_TYPE (GeditOverlay, gedit_overlay, GTK_TYPE_CONTAINER)

static void
enable_compositing (GeditOverlay *overlay,
                    GtkWidget    *child,
                    gboolean      enable)
{
	GdkWindow *window;
	GdkWindow *mywindow;

	mywindow = gtk_widget_get_window (GTK_WIDGET (overlay));
	window = gtk_widget_get_window (child);

	if (window != NULL && window != mywindow)
	{
		gdk_window_set_composited (window, enable);
	}
}

static void
on_child_realized (GtkWidget    *child,
                   GeditOverlay *overlay)
{
	enable_compositing (overlay, child, overlay->priv->composited);
}

static void
add_toplevel_widget (GeditOverlay *overlay,
                     GtkWidget    *child)
{
	gtk_widget_set_parent (child, GTK_WIDGET (overlay));

	overlay->priv->children = g_slist_append (overlay->priv->children,
	                                          child);

	enable_compositing (overlay, child, overlay->priv->composited);

	g_signal_connect_after (child,
	                        "realize",
	                        G_CALLBACK (on_child_realized),
	                        overlay);
}

static void
gedit_overlay_dispose (GObject *object)
{
	G_OBJECT_CLASS (gedit_overlay_parent_class)->dispose (object);
}

static void
set_enable_compositing (GeditOverlay *overlay,
                        gboolean      enabled)
{
	GSList *item;

	if (overlay->priv->composited == enabled)
	{
		return;
	}

	overlay->priv->composited = enabled;

	/* Enable/disable compositing on all the children */
	for (item = overlay->priv->children; item; item = g_slist_next (item))
	{
		GtkWidget *child = item->data;

		enable_compositing (overlay, child, enabled);
	}

	g_object_notify (G_OBJECT (overlay), "composited");
}

static void
gedit_overlay_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
	GeditOverlay *overlay = GEDIT_OVERLAY (object);
	GeditOverlayPrivate *priv = overlay->priv;

	switch (prop_id)
	{
		case PROP_MAIN_WIDGET:
			g_value_set_object (value, priv->main_widget);
			break;

		case PROP_RELATIVE_WIDGET:
			g_value_set_object (value, priv->relative_widget);
			break;

		case PROP_COMPOSITED:
			g_value_set_boolean (value, priv->composited);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
set_transparent_background_color (GtkWidget *widget)
{
	GtkStyleContext *context;
	GdkRGBA bg_color;

	context = gtk_widget_get_style_context (widget);
	gtk_style_context_get_background_color (context,
	                                        GTK_STATE_NORMAL,
	                                        &bg_color);

	bg_color.alpha = 0;

	gtk_widget_override_background_color (widget,
	                                      GTK_STATE_FLAG_NORMAL,
	                                      &bg_color);
}

static GtkWidget *
wrap_child_if_needed (GtkWidget *widget,
                      gboolean   make_transparent)
{
	GtkWidget *child;

	if (GEDIT_IS_OVERLAY_CHILD (widget))
	{
		return widget;
	}

	child = GTK_WIDGET (gedit_overlay_child_new (widget));
	gtk_widget_show (child);

	if (make_transparent)
	{
		set_transparent_background_color (child);
	}

	g_signal_connect_swapped (widget,
	                          "destroy",
	                          G_CALLBACK (gtk_widget_destroy),
	                          child);

	return child;
}

static void
gedit_overlay_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
	GeditOverlay *overlay = GEDIT_OVERLAY (object);
	GeditOverlayPrivate *priv = overlay->priv;

	switch (prop_id)
	{
		case PROP_MAIN_WIDGET:
		{
			priv->main_widget = wrap_child_if_needed (g_value_get_object (value),
			                                          FALSE);

			add_toplevel_widget (overlay,
			                     priv->main_widget);
			break;
		}
		case PROP_RELATIVE_WIDGET:
			priv->relative_widget = g_value_get_object (value);
			break;

		case PROP_COMPOSITED:
			set_enable_compositing (overlay,
			                        g_value_get_boolean (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_overlay_realize (GtkWidget *widget)
{
	GtkAllocation allocation;
	GdkWindow *window;
	GdkWindowAttr attributes;
	gint attributes_mask;
	GtkStyleContext *context;

	gtk_widget_set_realized (widget, TRUE);

	gtk_widget_get_allocation (widget, &allocation);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = allocation.x;
	attributes.y = allocation.y;
	attributes.width = allocation.width;
	attributes.height = allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gtk_widget_get_visual (widget);
	attributes.event_mask = gtk_widget_get_events (widget);
	attributes.event_mask |= GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK;

	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

	window = gdk_window_new (gtk_widget_get_parent_window (widget),
	                         &attributes, attributes_mask);
	gtk_widget_set_window (widget, window);
	gdk_window_set_user_data (window, widget);

	context = gtk_widget_get_style_context (widget);
	gtk_style_context_set_state (context, GTK_STATE_FLAG_NORMAL);
	gtk_style_context_set_background (context, window);
}

static void
gedit_overlay_get_preferred_width (GtkWidget *widget,
                                   gint      *minimum,
                                   gint      *natural)
{
	GeditOverlayPrivate *priv = GEDIT_OVERLAY (widget)->priv;

	*minimum = 0;
	*natural = 0;

	if (priv->main_widget)
	{
		gtk_widget_get_preferred_width (priv->main_widget, minimum, natural);
	}
}

static void
gedit_overlay_get_preferred_height (GtkWidget *widget,
                                    gint      *minimum,
                                    gint      *natural)
{
	GeditOverlayPrivate *priv = GEDIT_OVERLAY (widget)->priv;

	*minimum = 0;
	*natural = 0;

	if (priv->main_widget)
	{
		gtk_widget_get_preferred_height (priv->main_widget, minimum, natural);
	}
}

static void
gedit_overlay_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation)
{
	GeditOverlay *overlay = GEDIT_OVERLAY (widget);
	GeditOverlayPrivate *priv = overlay->priv;
	GtkAllocation main_alloc;
	GSList *l;

	GTK_WIDGET_CLASS (gedit_overlay_parent_class)->size_allocate (widget, allocation);

	/* main widget allocation */
	main_alloc.x = 0;
	main_alloc.y = 0;
	main_alloc.width = allocation->width;
	main_alloc.height = allocation->height;

	gtk_widget_size_allocate (overlay->priv->main_widget, &main_alloc);

	/* if a relative widget exists place the floating widgets in relation to it */
	if (priv->relative_widget)
	{
		gtk_widget_get_allocation (priv->relative_widget, &main_alloc);
	}

	for (l = priv->children; l != NULL; l = g_slist_next (l))
	{
		GtkWidget *child = GTK_WIDGET (l->data);
		GtkRequisition req;
		GtkAllocation alloc;
		guint offset;

		if (child == priv->main_widget)
			continue;

		gtk_widget_get_preferred_size (child, NULL, &req);
		offset = gedit_overlay_child_get_offset (GEDIT_OVERLAY_CHILD (child));

		/* FIXME: Add all the positions here */
		switch (gedit_overlay_child_get_position (GEDIT_OVERLAY_CHILD (child)))
		{
			/* The gravity is treated as position and not as a gravity */
			case GEDIT_OVERLAY_CHILD_POSITION_NORTH_EAST:
				alloc.x = MAX (main_alloc.x, main_alloc.width - req.width - (gint) offset);
				alloc.y = 0;
				break;
			case GEDIT_OVERLAY_CHILD_POSITION_NORTH_WEST:
				alloc.x = offset;
				alloc.y = 0;
				break;
			case GEDIT_OVERLAY_CHILD_POSITION_SOUTH_WEST:
				alloc.x = offset;
				alloc.y = MAX (main_alloc.y, main_alloc.height - req.height);
				break;
			case GEDIT_OVERLAY_CHILD_POSITION_SOUTH_EAST:
				alloc.x = MAX (main_alloc.x, main_alloc.width - req.width - (gint) offset);
				alloc.y = MAX (main_alloc.y, main_alloc.height - req.height);
				break;
			default:
				alloc.x = 0;
				alloc.y = 0;
		}

		alloc.width = MIN (main_alloc.width, req.width);
		alloc.height = MIN (main_alloc.height, req.height);

		gtk_widget_size_allocate (child, &alloc);
	}
}

static GeditOverlayChild *
get_overlay_child (GeditOverlay *overlay,
                   GtkWidget    *widget)
{
	GeditOverlayChild *overlay_child = NULL;
	GSList *l;

	for (l = overlay->priv->children; l != NULL; l = g_slist_next (l))
	{
		GtkWidget *child = GTK_WIDGET (l->data);

		/* skip the main widget as it is not a OverlayChild */
		if (child == overlay->priv->main_widget)
			continue;

		if (child == widget)
		{
			overlay_child = GEDIT_OVERLAY_CHILD (child);
			break;
		}
		else
		{
			GtkWidget *in_widget;

			/* let's try also with the internal widget */
			g_object_get (child, "widget", &in_widget, NULL);
			g_assert (in_widget != NULL);

			if (in_widget == widget)
			{
				overlay_child = GEDIT_OVERLAY_CHILD (child);
				g_object_unref (in_widget);

				break;
			}

			g_object_unref (in_widget);
		}
	}

	return overlay_child;
}

static void
overlay_add (GtkContainer *overlay,
             GtkWidget    *widget)
{
	GeditOverlayChild *child;

	/* check that the widget is not added yet */
	child = get_overlay_child (GEDIT_OVERLAY (overlay), widget);

	if (child == NULL)
	{
		add_toplevel_widget (GEDIT_OVERLAY (overlay),
		                     wrap_child_if_needed (widget, TRUE));
	}
}

static void
gedit_overlay_remove (GtkContainer *overlay,
                      GtkWidget    *widget)
{
	GeditOverlayPrivate *priv = GEDIT_OVERLAY (overlay)->priv;
	GSList *l;

	for (l = priv->children; l != NULL; l = g_slist_next (l))
	{
		GtkWidget *child = l->data;

		if (child == widget)
		{
			g_signal_handlers_disconnect_by_func (child,
			                                      on_child_realized,
			                                      overlay);

			gtk_widget_unparent (widget);
			priv->children = g_slist_remove_link (priv->children,
			                                      l);

			g_slist_free (l);
			break;
		}
	}
}

static void
gedit_overlay_forall (GtkContainer *overlay,
                      gboolean      include_internals,
                      GtkCallback   callback,
                      gpointer      callback_data)
{
	GeditOverlayPrivate *priv = GEDIT_OVERLAY (overlay)->priv;
	GSList *children;

	children = priv->children;
	while (children)
	{
		GtkWidget *child = GTK_WIDGET (children->data);
		children = children->next;

		(* callback) (child, callback_data);
	}
}

static GType
gedit_overlay_child_type (GtkContainer *overlay)
{
	return GTK_TYPE_WIDGET;
}

static gboolean
gedit_overlay_draw (GtkWidget *widget,
                    cairo_t   *cr)
{
	GeditOverlay *overlay;
	GSList *item;
	GdkWindow *mywindow;
	GdkRectangle cliprect;
	gboolean isclipped;

	GTK_WIDGET_CLASS (gedit_overlay_parent_class)->draw (widget, cr);

	overlay = GEDIT_OVERLAY (widget);

	/* Draw composited children if necessary */
	if (!overlay->priv->composited ||
	    !gdk_display_supports_composite (gtk_widget_get_display (widget)))
	{
		return FALSE;
	}

	mywindow = gtk_widget_get_window (widget);
	isclipped = gdk_cairo_get_clip_rectangle (cr, &cliprect);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	for (item = overlay->priv->children; item; item = g_slist_next (item))
	{
		GtkWidget *child = item->data;
		GdkWindow *window;
		GtkAllocation allocation;
		GdkRectangle childrect;
		GdkRectangle cliparea;

		window = gtk_widget_get_window (child);

		if (window == NULL || window == mywindow)
		{
			continue;
		}

		gtk_widget_get_allocation (child, &allocation);

		childrect.x = allocation.x;
		childrect.y = allocation.y;
		childrect.width = allocation.width;
		childrect.height = allocation.height;

		if (isclipped)
		{
			gdk_rectangle_intersect (&cliprect,
			                         &childrect,
			                         &cliparea);
		}
		else
		{
			cliparea = childrect;
		}

		cairo_save (cr);

		gdk_cairo_rectangle (cr, &cliparea);
		cairo_clip (cr);

		gdk_cairo_set_source_window (cr,
		                             window,
		                             allocation.x,
		                             allocation.y);

		cairo_paint (cr);
		cairo_restore (cr);
	}

	return FALSE;
}

static void
gedit_overlay_class_init (GeditOverlayClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

	object_class->dispose = gedit_overlay_dispose;
	object_class->get_property = gedit_overlay_get_property;
	object_class->set_property = gedit_overlay_set_property;

	widget_class->realize = gedit_overlay_realize;
	widget_class->get_preferred_width = gedit_overlay_get_preferred_width;
	widget_class->get_preferred_height = gedit_overlay_get_preferred_height;
	widget_class->size_allocate = gedit_overlay_size_allocate;
	widget_class->draw = gedit_overlay_draw;

	container_class->add = overlay_add;
	container_class->remove = gedit_overlay_remove;
	container_class->forall = gedit_overlay_forall;
	container_class->child_type = gedit_overlay_child_type;

	g_object_class_install_property (object_class, PROP_MAIN_WIDGET,
	                                 g_param_spec_object ("main-widget",
	                                                      "Main Widget",
	                                                      "The Main Widget",
	                                                      GTK_TYPE_WIDGET,
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_CONSTRUCT_ONLY |
	                                                      G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class, PROP_RELATIVE_WIDGET,
	                                 g_param_spec_object ("relative-widget",
	                                                      "Relative Widget",
	                                                      "Widget on which the floating widgets are placed",
	                                                      GTK_TYPE_WIDGET,
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
	                                 PROP_COMPOSITED,
	                                 g_param_spec_boolean ("composited",
	                                                       "Composited",
	                                                       "Whether the overlay composites its children",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE |
	                                                       G_PARAM_STATIC_STRINGS));

	g_type_class_add_private (object_class, sizeof (GeditOverlayPrivate));
}

static void
gedit_overlay_init (GeditOverlay *overlay)
{
	overlay->priv = GEDIT_OVERLAY_GET_PRIVATE (overlay);

	gtk_widget_set_app_paintable (GTK_WIDGET (overlay), TRUE);
}

/**
 * gedit_overlay_new:
 * @main_widget: a #GtkWidget
 * @relative_widget: (allow-none): a #Gtkwidget
 *
 * Creates a new #GeditOverlay. If @relative_widget is not %NULL the floating
 * widgets will be placed in relation to it, if not @main_widget will be use
 * for this purpose.
 *
 * Returns: a new #GeditOverlay object.
 */
GtkWidget *
gedit_overlay_new (GtkWidget *main_widget,
                   GtkWidget *relative_widget)
{
	g_return_val_if_fail (GTK_IS_WIDGET (main_widget), NULL);

	return GTK_WIDGET (g_object_new (GEDIT_TYPE_OVERLAY,
	                                 "main-widget", main_widget,
	                                 "relative-widget", relative_widget,
	                                 NULL));
}

/**
 * gedit_overlay_add:
 * @overlay: a #GeditOverlay
 * @widget: a #GtkWidget to be added to the container
 * @position: a #GeditOverlayChildPosition
 * @offset: offset for @widget
 *
 * Adds @widget to @overlay in a specific position.
 */
void
gedit_overlay_add (GeditOverlay             *overlay,
                   GtkWidget                *widget,
                   GeditOverlayChildPosition position,
                   guint                     offset)
{
	GeditOverlayChild *child;

	g_return_if_fail (GEDIT_IS_OVERLAY (overlay));
	g_return_if_fail (GTK_IS_WIDGET (widget));

	gtk_container_add (GTK_CONTAINER (overlay), widget);

	/* NOTE: can we improve this without exposing overlay child? */
	child = get_overlay_child (overlay, widget);
	g_assert (child != NULL);

	gedit_overlay_child_set_position (child, position);
	gedit_overlay_child_set_offset (child, offset);
}

void
gedit_overlay_set_composited (GeditOverlay *overlay,
                              gboolean      enabled)
{
	g_return_if_fail (GEDIT_IS_OVERLAY (overlay));

	set_enable_compositing (overlay, enabled);
}

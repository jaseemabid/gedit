/*
 * gedit-floating-slider.c
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Ignacio Casal Quinteiro
 *
 * Based on Scott Peterson <lunchtimemama@gmail.com> work.
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

#include "gedit-floating-slider.h"
#include "gedit-animatable.h"

struct _GeditFloatingSliderPrivate
{
	GtkAllocation widget_alloc;
	GtkAllocation child_alloc;
	GeditTheatricsChoreographerEasing easing;
	GeditTheatricsChoreographerBlocking blocking;
	GeditTheatricsAnimationState animation_state;
	GtkOrientation orientation;
	guint duration;
	gdouble bias;
	gdouble percent;
};

struct _GeditFloatingSliderClassPrivate
{
	GtkCssProvider *css;
};

enum
{
	PROP_0,
	PROP_EASING,
	PROP_BLOCKING,
	PROP_ANIMATION_STATE,
	PROP_DURATION,
	PROP_PERCENT,
	PROP_BIAS,
	PROP_ORIENTATION
};

G_DEFINE_TYPE_WITH_CODE (GeditFloatingSlider, gedit_floating_slider, GTK_TYPE_BIN,
                         g_type_add_class_private (g_define_type_id, sizeof (GeditFloatingSliderClassPrivate));
                         G_IMPLEMENT_INTERFACE (GEDIT_TYPE_ANIMATABLE,
                                                NULL)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE,
                                                NULL))

static void
gedit_floating_slider_finalize (GObject *object)
{
	G_OBJECT_CLASS (gedit_floating_slider_parent_class)->finalize (object);
}

static void
gedit_floating_slider_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
	GeditFloatingSliderPrivate *priv = GEDIT_FLOATING_SLIDER (object)->priv;

	switch (prop_id)
	{
		case PROP_EASING:
			g_value_set_enum (value, priv->easing);
			break;
		case PROP_BLOCKING:
			g_value_set_enum (value, priv->blocking);
			break;
		case PROP_ANIMATION_STATE:
			g_value_set_enum (value, priv->animation_state);
			break;
		case PROP_DURATION:
			g_value_set_uint (value, priv->duration);
			break;
		case PROP_PERCENT:
			g_value_set_double (value, priv->percent);
			break;
		case PROP_BIAS:
			g_value_set_double (value, priv->bias);
			break;
		case PROP_ORIENTATION:
			g_value_set_enum (value, priv->orientation);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_floating_slider_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
	GeditFloatingSliderPrivate *priv = GEDIT_FLOATING_SLIDER (object)->priv;

	switch (prop_id)
	{
		case PROP_EASING:
			priv->easing = g_value_get_enum (value);
			break;
		case PROP_BLOCKING:
			priv->blocking = g_value_get_enum (value);
			break;
		case PROP_ANIMATION_STATE:
			priv->animation_state = g_value_get_enum (value);
			break;
		case PROP_DURATION:
			priv->duration = g_value_get_uint (value);
			break;
		case PROP_PERCENT:
			priv->percent = g_value_get_double (value);
			gtk_widget_queue_resize_no_redraw (GTK_WIDGET (object));

			/* Make the widget visibility automagic */
			if (priv->percent > 0 && !gtk_widget_get_visible (GTK_WIDGET (object)))
			{
				gtk_widget_show (GTK_WIDGET (object));
			}
			else if (priv->percent == 0 && gtk_widget_get_visible (GTK_WIDGET (object)))
			{
				gtk_widget_hide (GTK_WIDGET (object));
			}
			break;
		case PROP_BIAS:
			priv->bias = g_value_get_double (value);
			break;
		case PROP_ORIENTATION:
			priv->orientation = g_value_get_enum (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_floating_slider_get_preferred_width (GtkWidget *widget,
                                           gint      *minimum,
                                           gint      *natural)
{
	GeditFloatingSliderPrivate *priv = GEDIT_FLOATING_SLIDER (widget)->priv;
	GtkWidget *child;
	gint width;

	child = gtk_bin_get_child (GTK_BIN (widget));

	if (child != NULL)
	{
		GtkStyleContext *context;
		GtkBorder padding;
		gint child_min, child_nat;

		gtk_widget_get_preferred_width (child, &child_min, &child_nat);

		priv->child_alloc.width = child_nat;

		context = gtk_widget_get_style_context (widget);
		gtk_style_context_get_padding (context, GTK_STATE_FLAG_NORMAL,
		                               &padding);

		priv->widget_alloc.width = child_nat + padding.left + padding.right;
	}


	if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
	{
		width = gedit_theatrics_choreographer_pixel_compose (priv->percent,
		                                                     priv->widget_alloc.width,
		                                                     priv->easing);
	}
	else
	{
		width = priv->widget_alloc.width;
	}

	*minimum = *natural = width;
}

static void
gedit_floating_slider_get_preferred_height (GtkWidget *widget,
                                            gint      *minimum,
                                            gint      *natural)
{
	GeditFloatingSliderPrivate *priv = GEDIT_FLOATING_SLIDER (widget)->priv;
	GtkWidget *child;
	gint height;

	child = gtk_bin_get_child (GTK_BIN (widget));

	if (child != NULL)
	{
		GtkStyleContext *context;
		GtkBorder padding;
		gint child_min, child_nat;

		gtk_widget_get_preferred_height (child, &child_min, &child_nat);

		priv->child_alloc.height = child_nat;

		context = gtk_widget_get_style_context (widget);
		gtk_style_context_get_padding (context, GTK_STATE_FLAG_NORMAL,
		                               &padding);

		priv->widget_alloc.height = child_nat + padding.top + padding.bottom;
	}

	if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
	{
		height = priv->widget_alloc.height;
	}
	else
	{
		height = gedit_theatrics_choreographer_pixel_compose (priv->percent,
		                                                      priv->widget_alloc.height,
		                                                      priv->easing);
	}

	*minimum = *natural = height;
}

static void
gedit_floating_slider_size_allocate (GtkWidget     *widget,
                                     GtkAllocation *allocation)
{
	GeditFloatingSliderPrivate *priv = GEDIT_FLOATING_SLIDER (widget)->priv;
	GtkWidget *child;

	GTK_WIDGET_CLASS (gedit_floating_slider_parent_class)->size_allocate (widget, allocation);

	child = gtk_bin_get_child (GTK_BIN (widget));

	if (child != NULL)
	{
		GtkStyleContext *context;
		GtkBorder padding;
		GtkAllocation child_alloc;

		context = gtk_widget_get_style_context (widget);
		gtk_style_context_get_padding (context, GTK_STATE_FLAG_NORMAL,
		                               &padding);

		child_alloc = priv->child_alloc;

		if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
		{
			child_alloc.y = padding.top;
			child_alloc.x = padding.left;

			if (priv->blocking == GEDIT_THEATRICS_CHOREOGRAPHER_BLOCKING_DOWNSTAGE)
			{
				child_alloc.x = allocation->width - priv->child_alloc.width - padding.right;
			}
		}
		else
		{
			child_alloc.y = padding.top;
			child_alloc.x = padding.left;

			if (priv->blocking == GEDIT_THEATRICS_CHOREOGRAPHER_BLOCKING_DOWNSTAGE)
			{
				child_alloc.y = allocation->height - priv->child_alloc.height - padding.bottom;
			}
		}

		if (child_alloc.height > 0 && child_alloc.width > 0)
		{
			gtk_widget_size_allocate (child, &child_alloc);
		}
	}
}

static gboolean
gedit_floating_slider_draw (GtkWidget *widget,
                            cairo_t   *cr)
{
	GtkStyleContext *context;

	context = gtk_widget_get_style_context (widget);

	gtk_style_context_save (context);
	gtk_style_context_set_state (context,
	                             gtk_widget_get_state_flags (widget));

	gtk_render_background (context, cr, 0, 0,
	                       gtk_widget_get_allocated_width (widget),
	                       gtk_widget_get_allocated_height (widget));

	gtk_render_frame (context, cr, 0, 0,
	                  gtk_widget_get_allocated_width (widget),
	                  gtk_widget_get_allocated_height (widget));

	gtk_style_context_restore (context);

	return GTK_WIDGET_CLASS (gedit_floating_slider_parent_class)->draw (widget, cr);
}

static void
gedit_floating_slider_class_init (GeditFloatingSliderClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	static const gchar style[] =
	"* {"
	  "background-image: -gtk-gradient (linear,\n"
	                                    "left top, left bottom,\n"
	                                    "from (shade (@notebook_tab_gradient_a, 0.97)),\n"
	                                    "to (shade (@notebook_tab_gradient_b, 0.90)));\n"

	  "padding: 6px;\n"
	  "border-color: shade (@notebook_tab_gradient_b, 0.80);\n"

	  "border-radius: 0 0 3px 3px;\n"
	  "border-width: 0 1px 1px 1px;\n"
	  "border-style: solid;\n"
	"}\n"

	".button {"
	  "background-color: alpha (@theme_base_color, 0.0);"
	  "background-image: none;"

	  "padding: 0;\n"
	  "border-style: none;"
	  "border-image: none;"

	  "-GtkButton-image-spacing: 0;"
	  "-GtkButton-inner-border: 0;"
	"}";

	object_class->finalize = gedit_floating_slider_finalize;
	object_class->get_property = gedit_floating_slider_get_property;
	object_class->set_property = gedit_floating_slider_set_property;

	widget_class->get_preferred_width = gedit_floating_slider_get_preferred_width;
	widget_class->get_preferred_height = gedit_floating_slider_get_preferred_height;
	widget_class->size_allocate = gedit_floating_slider_size_allocate;
	widget_class->draw = gedit_floating_slider_draw;

	g_object_class_override_property (object_class, PROP_EASING,
	                                  "easing");

	g_object_class_override_property (object_class, PROP_BLOCKING,
	                                  "blocking");

	g_object_class_override_property (object_class, PROP_ANIMATION_STATE,
	                                  "animation-state");

	g_object_class_override_property (object_class, PROP_DURATION,
	                                  "duration");

	g_object_class_override_property (object_class, PROP_PERCENT,
	                                  "percent");

	g_object_class_override_property (object_class, PROP_BIAS,
	                                  "bias");

	g_object_class_override_property (object_class, PROP_ORIENTATION,
	                                  "orientation");

	g_type_class_add_private (object_class, sizeof (GeditFloatingSliderPrivate));

	klass->priv = G_TYPE_CLASS_GET_PRIVATE (klass, GEDIT_TYPE_FLOATING_SLIDER,
	                                        GeditFloatingSliderClassPrivate);

	klass->priv->css = gtk_css_provider_new ();
	gtk_css_provider_load_from_data (klass->priv->css, style, -1, NULL);
}

static void
gedit_floating_slider_init (GeditFloatingSlider *slider)
{
	GtkStyleContext *context;

	slider->priv = G_TYPE_INSTANCE_GET_PRIVATE (slider,
	                                            GEDIT_TYPE_FLOATING_SLIDER,
	                                            GeditFloatingSliderPrivate);

	slider->priv->orientation = GTK_ORIENTATION_VERTICAL;

	context = gtk_widget_get_style_context (GTK_WIDGET (slider));
	gtk_style_context_add_provider (context,
	                                GTK_STYLE_PROVIDER (GEDIT_FLOATING_SLIDER_GET_CLASS (slider)->priv->css),
	                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

GtkWidget *
gedit_floating_slider_new (void)
{
	return g_object_new (GEDIT_TYPE_FLOATING_SLIDER, NULL);
}

/* ex:set ts=8 noet: */

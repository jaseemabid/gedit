/*
 * gedit-smart-charset-converter.c
 * This file is part of gedit
 *
 * Copyright (C) 2009 - Ignacio Casal Quinteiro
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

#include "gedit-smart-charset-converter.h"
#include "gedit-debug.h"

#include <gio/gio.h>
#include <glib/gi18n.h>

#define GEDIT_SMART_CHARSET_CONVERTER_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_SMART_CHARSET_CONVERTER, GeditSmartCharsetConverterPrivate))

struct _GeditSmartCharsetConverterPrivate
{
	GCharsetConverter *charset_conv;

	GSList *encodings;
	GSList *current_encoding;

	guint is_utf8 : 1;
	guint use_first : 1;
};

static void gedit_smart_charset_converter_iface_init    (GConverterIface *iface);

G_DEFINE_TYPE_WITH_CODE (GeditSmartCharsetConverter, gedit_smart_charset_converter,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (G_TYPE_CONVERTER,
						gedit_smart_charset_converter_iface_init))

static void
gedit_smart_charset_converter_finalize (GObject *object)
{
	GeditSmartCharsetConverter *smart = GEDIT_SMART_CHARSET_CONVERTER (object);

	g_slist_free (smart->priv->encodings);

	gedit_debug_message (DEBUG_UTILS, "finalizing smart charset converter");

	G_OBJECT_CLASS (gedit_smart_charset_converter_parent_class)->finalize (object);
}

static void
gedit_smart_charset_converter_dispose (GObject *object)
{
	GeditSmartCharsetConverter *smart = GEDIT_SMART_CHARSET_CONVERTER (object);

	if (smart->priv->charset_conv != NULL)
	{
		g_object_unref (smart->priv->charset_conv);
		smart->priv->charset_conv = NULL;
	}

	gedit_debug_message (DEBUG_UTILS, "disposing smart charset converter");

	G_OBJECT_CLASS (gedit_smart_charset_converter_parent_class)->dispose (object);
}

static void
gedit_smart_charset_converter_class_init (GeditSmartCharsetConverterClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_smart_charset_converter_finalize;
	object_class->dispose = gedit_smart_charset_converter_dispose;

	g_type_class_add_private (object_class, sizeof (GeditSmartCharsetConverterPrivate));
}

static void
gedit_smart_charset_converter_init (GeditSmartCharsetConverter *self)
{
	self->priv = GEDIT_SMART_CHARSET_CONVERTER_GET_PRIVATE (self);

	self->priv->charset_conv = NULL;
	self->priv->encodings = NULL;
	self->priv->current_encoding = NULL;
	self->priv->is_utf8 = FALSE;

	gedit_debug_message (DEBUG_UTILS, "initializing smart charset converter");
}

static const GeditEncoding *
get_encoding (GeditSmartCharsetConverter *smart)
{
	if (smart->priv->current_encoding == NULL)
	{
		smart->priv->current_encoding = smart->priv->encodings;
	}
	else
	{
		smart->priv->current_encoding = g_slist_next (smart->priv->current_encoding);
	}

	if (smart->priv->current_encoding != NULL)
		return (const GeditEncoding *)smart->priv->current_encoding->data;

	/* If we tried all encodings, we return the first encoding */
	smart->priv->use_first = TRUE;
	smart->priv->current_encoding = smart->priv->encodings;

	return (const GeditEncoding *)smart->priv->current_encoding->data;
}

static GCharsetConverter *
guess_encoding (GeditSmartCharsetConverter *smart,
		const void                 *inbuf,
		gsize                       inbuf_size)
{
	GCharsetConverter *conv = NULL;

	/* We just check the first block */
	while (TRUE)
	{
		const GeditEncoding *enc;
		gchar outbuf[inbuf_size];
		GConverterResult ret;
		gsize read, written;
		GError *err = NULL;

		if (conv != NULL)
		{
			g_object_unref (conv);
			conv = NULL;
		}

		/* We get the first encoding we have in the list */
		enc = get_encoding (smart);

		/* if it is NULL we didn't guess anything */
		if (enc == NULL)
		{
			break;
		}

		if (enc == gedit_encoding_get_utf8 ())
		{
			gsize remainder;
			const gchar *end;
		
			if (g_utf8_validate (inbuf, inbuf_size, &end))
			{
				smart->priv->is_utf8 = TRUE;
				break;
			}

			/* Check if the end is just less than one char */
			remainder = inbuf_size - (end - (gchar *)inbuf);
			if (remainder < 6)
			{
				smart->priv->is_utf8 = TRUE;
				break;
			}

			continue;
		}

		conv = g_charset_converter_new ("UTF-8",
						gedit_encoding_get_charset (enc),
						NULL);

		ret = g_converter_convert (G_CONVERTER (conv),
					   inbuf,
					   inbuf_size,
					   outbuf,
					   inbuf_size,
					   0,
					   &read,
					   &written,
					   &err);

		if (err != NULL)
		{
			/* FIXME: Is this ok or we should just skip it? */
			if (err->code == G_CONVERT_ERROR_PARTIAL_INPUT)
			{
				g_error_free (err);
				break;
			}

			g_error_free (err);
		}
		else
		{
			break;
		}
	}

	return conv;
}

static GConverterResult
gedit_smart_charset_converter_convert (GConverter *converter,
				       const void *inbuf,
				       gsize       inbuf_size,
				       void       *outbuf,
				       gsize       outbuf_size,
				       GConverterFlags flags,
				       gsize      *bytes_read,
				       gsize      *bytes_written,
				       GError    **error)
{
	GeditSmartCharsetConverter *smart = GEDIT_SMART_CHARSET_CONVERTER (converter);

	if (inbuf_size == 0)
	{
		if (flags & G_CONVERTER_INPUT_AT_END)
			return G_CONVERTER_FINISHED;

		if (flags & G_CONVERTER_FLUSH)
			return G_CONVERTER_FLUSHED;

		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PARTIAL_INPUT,
				     _("Incomplete multibyte sequence in input"));
		return G_CONVERTER_ERROR;
	}

	/* Guess the encoding if we didn't make it yet */
	if (smart->priv->charset_conv == NULL &&
	    !smart->priv->is_utf8)
	{
		smart->priv->charset_conv = guess_encoding (smart, inbuf, inbuf_size);

		/* If we still have the previous case is that we didn't guess
		   anything */
		if (smart->priv->charset_conv == NULL &&
		    !smart->priv->is_utf8)
		{
			g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED,
					     _("It is not possible to detect the encoding automatically"));
			return G_CONVERTER_ERROR;
		}
	}

	/* Now if the encoding is utf8 just redirect the input to the output */
	if (smart->priv->is_utf8)
	{
		gsize size;

		size = MIN (inbuf_size, outbuf_size);

		memcpy (outbuf, inbuf, size);
		*bytes_read = size;
		*bytes_written = size;

		return G_CONVERTER_CONVERTED;
	}

	/* If we reached here is because we need to convert the text so, we
	   convert it with the charset converter */
	return g_converter_convert (G_CONVERTER (smart->priv->charset_conv),
				    inbuf,
				    inbuf_size,
				    outbuf,
				    outbuf_size,
				    flags,
				    bytes_read,
				    bytes_written,
				    error);
}

static void
gedit_smart_charset_converter_reset (GConverter *converter)
{
	GeditSmartCharsetConverter *smart = GEDIT_SMART_CHARSET_CONVERTER (converter);

	smart->priv->current_encoding = NULL;
	smart->priv->is_utf8 = FALSE;

	if (smart->priv->charset_conv != NULL)
	{
		g_object_unref (smart->priv->charset_conv);
		smart->priv->charset_conv = NULL;
	}
}

static void
gedit_smart_charset_converter_iface_init (GConverterIface *iface)
{
	iface->convert = gedit_smart_charset_converter_convert;
	iface->reset = gedit_smart_charset_converter_reset;
}

GeditSmartCharsetConverter *
gedit_smart_charset_converter_new (GSList *candidate_encodings)
{
	GeditSmartCharsetConverter *smart;

	g_return_val_if_fail (candidate_encodings != NULL, NULL);

	smart = g_object_new (GEDIT_TYPE_SMART_CHARSET_CONVERTER, NULL);

	smart->priv->encodings = g_slist_copy (candidate_encodings);

	return smart;
}

const GeditEncoding *
gedit_smart_charset_converter_get_guessed (GeditSmartCharsetConverter *smart)
{
	g_return_val_if_fail (GEDIT_IS_SMART_CHARSET_CONVERTER (smart), NULL);

	if (smart->priv->current_encoding != NULL)
	{
		return (const GeditEncoding *)smart->priv->current_encoding->data;
	}

	return NULL;
}
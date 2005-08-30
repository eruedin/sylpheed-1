/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2005 Hiroyuki Yamamoto
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "prefs.h"
#include "prefs_ui.h"
#include "codeconv.h"
#include "utils.h"
#include "gtkutils.h"

typedef enum
{
	DUMMY_PARAM
} DummyEnum;

void prefs_dialog_create(PrefsDialog *dialog)
{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *notebook;

	GtkWidget *confirm_area;
	GtkWidget *ok_btn;
	GtkWidget *cancel_btn;
	GtkWidget *apply_btn;

	g_return_if_fail(dialog != NULL);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width (GTK_CONTAINER (window), 8);
	gtk_window_set_position (GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_modal (GTK_WINDOW (window), TRUE);
	gtk_window_set_policy (GTK_WINDOW(window), FALSE, TRUE, FALSE);

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show(vbox);
	gtk_container_add (GTK_CONTAINER (window), vbox);

	notebook = gtk_notebook_new ();
	gtk_widget_show(notebook);
	gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (notebook), 2);
	/* GTK_WIDGET_UNSET_FLAGS (notebook, GTK_CAN_FOCUS); */
	gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);

	gtkut_stock_button_set_create(&confirm_area,
				      &ok_btn, GTK_STOCK_OK,
				      &cancel_btn, GTK_STOCK_CANCEL,
				      &apply_btn, GTK_STOCK_APPLY);
	gtk_widget_show(confirm_area);
	gtk_box_pack_end (GTK_BOX(vbox), confirm_area, FALSE, FALSE, 0);
	gtk_widget_grab_default(ok_btn);

	dialog->window       = window;
	dialog->notebook     = notebook;
	dialog->confirm_area = confirm_area;
	dialog->ok_btn       = ok_btn;
	dialog->cancel_btn   = cancel_btn;
	dialog->apply_btn    = apply_btn;
}

void prefs_dialog_destroy(PrefsDialog *dialog)
{
	gtk_widget_destroy(dialog->window);
	dialog->window     = NULL;
	dialog->notebook   = NULL;
	dialog->ok_btn     = NULL;
	dialog->cancel_btn = NULL;
	dialog->apply_btn  = NULL;
}

void prefs_button_toggled(GtkToggleButton *toggle_btn, GtkWidget *widget)
{
	gboolean is_active;

	is_active = gtk_toggle_button_get_active(toggle_btn);
	gtk_widget_set_sensitive(widget, is_active);
}

void prefs_set_dialog(PrefParam *param)
{
	gint i;

	for (i = 0; param[i].name != NULL; i++) {
		if (param[i].widget_set_func)
			param[i].widget_set_func(&param[i]);
	}
}

void prefs_set_data_from_dialog(PrefParam *param)
{
	gint i;

	for (i = 0; param[i].name != NULL; i++) {
		if (param[i].data_set_func)
			param[i].data_set_func(&param[i]);
	}
}

void prefs_set_dialog_to_default(PrefParam *param)
{
	gint	   i;
	PrefParam  tmpparam;
	gchar	  *str_data = NULL;
	gint	   int_data;
	gushort    ushort_data;
	gboolean   bool_data;
	DummyEnum  enum_data;

	for (i = 0; param[i].name != NULL; i++) {
		if (!param[i].widget_set_func) continue;

		tmpparam = param[i];

		switch (tmpparam.type) {
		case P_STRING:
			if (tmpparam.defval) {
				if (!g_ascii_strncasecmp
					(tmpparam.defval, "ENV_", 4)) {
					str_data = g_strdup
						(g_getenv(param[i].defval + 4));
					tmpparam.data = &str_data;
					break;
				} else if (tmpparam.defval[0] == '~') {
					str_data =
						g_strconcat(get_home_dir(),
							    param[i].defval + 1,
							    NULL);
					tmpparam.data = &str_data;
					break;
				}
			}
			tmpparam.data = &tmpparam.defval;
			break;
		case P_INT:
			if (tmpparam.defval)
				int_data = atoi(tmpparam.defval);
			else
				int_data = 0;
			tmpparam.data = &int_data;
			break;
		case P_USHORT:
			if (tmpparam.defval)
				ushort_data = atoi(tmpparam.defval);
			else
				ushort_data = 0;
			tmpparam.data = &ushort_data;
			break;
		case P_BOOL:
			if (tmpparam.defval) {
				if (!g_ascii_strcasecmp(tmpparam.defval, "TRUE"))
					bool_data = TRUE;
				else
					bool_data = atoi(tmpparam.defval)
						? TRUE : FALSE;
			} else
				bool_data = FALSE;
			tmpparam.data = &bool_data;
			break;
		case P_ENUM:
			if (tmpparam.defval)
				enum_data = (DummyEnum)atoi(tmpparam.defval);
			else
				enum_data = 0;
			tmpparam.data = &enum_data;
			break;
		case P_OTHER:
			break;
		}
		tmpparam.widget_set_func(&tmpparam);
		g_free(str_data);
		str_data = NULL;
	}
}

void prefs_set_data_from_entry(PrefParam *pparam)
{
	gchar **str;
	const gchar *entry_str;

	g_return_if_fail(*pparam->widget != NULL);

	entry_str = gtk_entry_get_text(GTK_ENTRY(*pparam->widget));

	switch (pparam->type) {
	case P_STRING:
		str = (gchar **)pparam->data;
		g_free(*str);
		*str = entry_str[0] ? g_strdup(entry_str) : NULL;
		break;
	case P_USHORT:
		*((gushort *)pparam->data) = atoi(entry_str);
		break;
	case P_INT:
		*((gint *)pparam->data) = atoi(entry_str);
		break;
	default:
		g_warning("Invalid PrefType for GtkEntry widget: %d\n",
			  pparam->type);
	}
}

void prefs_set_entry(PrefParam *pparam)
{
	gchar **str;

	g_return_if_fail(*pparam->widget != NULL);

	switch (pparam->type) {
	case P_STRING:
		str = (gchar **)pparam->data;
		gtk_entry_set_text(GTK_ENTRY(*pparam->widget),
				   *str ? *str : "");
		break;
	case P_INT:
		gtk_entry_set_text(GTK_ENTRY(*pparam->widget),
				   itos(*((gint *)pparam->data)));
		break;
	case P_USHORT:
		gtk_entry_set_text(GTK_ENTRY(*pparam->widget),
				   itos(*((gushort *)pparam->data)));
		break;
	default:
		g_warning("Invalid PrefType for GtkEntry widget: %d\n",
			  pparam->type);
	}
}

void prefs_set_data_from_text(PrefParam *pparam)
{
	gchar **str;
	gchar *text = NULL, *tp = NULL;
	gchar *tmp, *tmpp;

	g_return_if_fail(*pparam->widget != NULL);
	g_return_if_fail(GTK_IS_EDITABLE(*pparam->widget) ||
			 GTK_IS_TEXT_VIEW(*pparam->widget));

	switch (pparam->type) {
	case P_STRING:
		str = (gchar **)pparam->data;
		g_free(*str);
		if (GTK_IS_EDITABLE(*pparam->widget)) {
			tp = text = gtk_editable_get_chars
				(GTK_EDITABLE(*pparam->widget), 0, -1);
		} else if (GTK_IS_TEXT_VIEW(*pparam->widget)) {
			GtkTextView *textview = GTK_TEXT_VIEW(*pparam->widget);
			GtkTextBuffer *buffer;
			GtkTextIter start, end;

			buffer = gtk_text_view_get_buffer(textview);
			gtk_text_buffer_get_start_iter(buffer, &start);
			gtk_text_buffer_get_iter_at_offset(buffer, &end, -1);
			tp = text = gtk_text_buffer_get_text
				(buffer, &start, &end, FALSE);
		}

		g_return_if_fail(tp != NULL && text != NULL);

		if (text[0] == '\0') {
			*str = NULL;
			g_free(text);
			break;
		}

		Xalloca(tmpp = tmp, strlen(text) * 2 + 1,
			{ *str = NULL; break; });
		while (*tp) {
			if (*tp == '\n') {
				*tmpp++ = '\\';
				*tmpp++ = 'n';
				tp++;
			} else
				*tmpp++ = *tp++;
		}
		*tmpp = '\0';
		*str = g_strdup(tmp);
		g_free(text);
		break;
	default:
		g_warning("Invalid PrefType for GtkTextView widget: %d\n",
			  pparam->type);
	}
}

void prefs_set_text(PrefParam *pparam)
{
	gchar *buf, *sp, *bufp;
	gchar **str;
	GtkTextView *text;
	GtkTextBuffer *buffer;
	GtkTextIter iter;

	g_return_if_fail(*pparam->widget != NULL);

	switch (pparam->type) {
	case P_STRING:
		str = (gchar **)pparam->data;
		if (*str) {
			bufp = buf = alloca(strlen(*str) + 1);
			if (!buf) buf = "";
			else {
				sp = *str;
				while (*sp) {
					if (*sp == '\\' && *(sp + 1) == 'n') {
						*bufp++ = '\n';
						sp += 2;
					} else
						*bufp++ = *sp++;
				}
				*bufp = '\0';
			}
		} else
			buf = "";

		text = GTK_TEXT_VIEW(*pparam->widget);
		buffer = gtk_text_view_get_buffer(text);
		gtk_text_buffer_set_text(buffer, "", 0);
		gtk_text_buffer_get_start_iter(buffer, &iter);
		gtk_text_buffer_insert(buffer, &iter, buf, -1);
		break;
	default:
		g_warning("Invalid PrefType for GtkTextView widget: %d\n",
			  pparam->type);
	}
}

void prefs_set_data_from_toggle(PrefParam *pparam)
{
	g_return_if_fail(pparam->type == P_BOOL);
	g_return_if_fail(*pparam->widget != NULL);
	
	*((gboolean *)pparam->data) =
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(*pparam->widget));
}

void prefs_set_toggle(PrefParam *pparam)
{
	g_return_if_fail(pparam->type == P_BOOL);
	g_return_if_fail(*pparam->widget != NULL);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(*pparam->widget),
				     *((gboolean *)pparam->data));
}

void prefs_set_data_from_spinbtn(PrefParam *pparam)
{
	g_return_if_fail(*pparam->widget != NULL);

	switch (pparam->type) {
	case P_INT:
		*((gint *)pparam->data) =
			gtk_spin_button_get_value_as_int
			(GTK_SPIN_BUTTON(*pparam->widget));
		break;
	case P_USHORT:
		*((gushort *)pparam->data) =
			(gushort)gtk_spin_button_get_value_as_int
			(GTK_SPIN_BUTTON(*pparam->widget));
		break;
	default:
		g_warning("Invalid PrefType for GtkSpinButton widget: %d\n",
			  pparam->type);
	}
}

void prefs_set_spinbtn(PrefParam *pparam)
{
	g_return_if_fail(*pparam->widget != NULL);

	switch (pparam->type) {
	case P_INT:
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(*pparam->widget),
					  (gfloat)*((gint *)pparam->data));
		break;
	case P_USHORT:
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(*pparam->widget),
					  (gfloat)*((gushort *)pparam->data));
		break;
	default:
		g_warning("Invalid PrefType for GtkSpinButton widget: %d\n",
			  pparam->type);
	}
}

void prefs_set_data_from_fontbtn(PrefParam *pparam)
{
	gchar **str;
	const gchar *font_str;

	g_return_if_fail(*pparam->widget != NULL);

	font_str = gtk_font_button_get_font_name
		(GTK_FONT_BUTTON(*pparam->widget));

	switch (pparam->type) {
	case P_STRING:
		str = (gchar **)pparam->data;
		g_free(*str);
		*str = font_str[0] ? g_strdup(font_str) : NULL;
		break;
	default:
		g_warning("Invalid PrefType for GtkFontButton widget: %d\n",
			  pparam->type);
	}
}

void prefs_set_fontbtn(PrefParam *pparam)
{
	gchar **str;

	g_return_if_fail(*pparam->widget != NULL);

	switch (pparam->type) {
	case P_STRING:
		str = (gchar **)pparam->data;
		gtk_font_button_set_font_name(GTK_FONT_BUTTON(*pparam->widget),
					      *str ? *str : "");
		break;
	default:
		g_warning("Invalid PrefType for GtkFontButton widget: %d\n",
			  pparam->type);
	}
}

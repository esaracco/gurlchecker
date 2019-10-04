/*
 * Copyright (C) 2002-2005 
 * Emmanuel Saracco <esaracco@users.labs.libre-entreprise.org>
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

#ifndef H_UC_CALLBACK
#define H_UC_CALLBACK

#include "general.h"

void on_urls_list_cursor_changed (GtkTreeView * selection,
				  gpointer user_data);
gboolean on_url_list_mouse_clicked (GtkWidget * widget,
				    GdkEventButton * event, gpointer data);
gboolean on_main_treeview_motion_notify_event (GtkWidget * widget,
					       GdkEventButton * event,
					       gpointer data);
gboolean on_main_treeview_leave_notify_event (GtkWidget * widget,
					      GdkEventCrossing * event,
					      gpointer user_data);
gboolean on_main_treeview_enter_notify_event (GtkWidget * widget,
					      GdkEventCrossing * event,
					      gpointer user_data);
void on_search_list_cursor_changed (GtkTreeView * selection,
				    gpointer user_data);

#endif

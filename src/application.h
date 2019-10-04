/*
 * Copyright (C) 2002-2010
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

#ifndef H_UC_APPLICATION
#define H_UC_APPLICATION

#include "general.h"
#include "project.h"


void uc_application_progress_dialog_set_modal (const gboolean modal);
void uc_application_buffer_show (const gchar * title, const gchar * message);
void uc_application_w3c_validate (void);
gchar *uc_application_input_file_dialog (const gchar * title,
					 const gchar * text);
gchar *uc_application_input_dialog (const gchar * title, const gchar * text);
gboolean uc_application_cookie_warning_dialog_show (const gchar * server,
						    const gchar * page,
						    const gchar * label,
						    gchar ** name,
						    gchar ** value,
						    gchar ** path,
						    gchar ** expires,
						    const UCCookiesAction
						    action);
gboolean uc_application_auth_dialog_show (const gchar * title,
                                          const gchar *host);
void uc_application_set_status_bar (const gfloat progress, const gchar * msg);
void uc_application_open_project_dialog_show (void);
gboolean uc_application_project_information_dialog_show (void);
gboolean uc_application_treeview_get_selected_iter (const GtkTreeView
						    * tv, GtkTreeIter * iter);
void uc_application_build_url_treeview (void);
void uc_application_build_projects_treeview (void);
void uc_application_reset_menu_protocols (void);
void uc_application_reset_menu_status (void);
void uc_application_reset_menu_types (void);
void uc_application_main_tree_apply_filters (const gchar *wname);
void uc_application_main_tree_display_all (void);
void uc_application_main_tree_collapse_all (void);
void uc_application_main_tree_expand_all (void);
void uc_application_main_tree_display_branch (const UCLinkProperties * prop);
void uc_application_display_search_message (const guint label_pos,
					    const gchar * message);
gint uc_application_dialog_yes_no_show (const gchar * message,
					const GtkMessageType msg_type);
void uc_application_search_dialog_show (void);
void uc_application_new_search_dialog_show (void);
void uc_application_add_filter_directory_add (void);
void uc_application_add_filter_domain_add (void);
void uc_application_add_filter_domain_remove (void);
void uc_application_add_filter_directory_remove (void);
void uc_application_add_filter_directory_dialog_show (void);
void uc_application_page_information_dialog_show (void);
void
uc_application_display_state_message (const guint label_pos,
				      const gchar * message);
void uc_application_display_informations (GtkTreeView * treeview);
void uc_application_treeview_activate_popup (GdkEventButton * event);
void uc_application_view_similar_links_dialog_show (void);
void uc_application_view_bad_extensions_dialog_show (void);
void uc_application_new_instance_launch (void);
void uc_application_launch_web_browser (const gchar * url);
void uc_application_menu_set_sensitive_all (const gchar * name,
					    const gboolean val);
void uc_application_proxy_frame_control_sensitive (void);
void uc_application_make_paths (void);
void uc_application_remove_paths (void);
void uc_application_view_source_dialog_show (void);
void uc_application_view_image_dialog_show (void);
void uc_application_dialog_show (const gchar * message,
				 const GtkMessageType msg_type);
void uc_application_globals_init (void);
void uc_application_init (gchar * url, gchar * auth_user,
			  gchar * auth_password, gboolean no_urls_args);
gboolean uc_application_quit (void);
void uc_application_draw_main_frames (void);
void uc_application_search_get_data (void);
gboolean uc_application_project_get_data (const UCProjectType type);
void uc_application_get_bookmarks_project_data (void);
void uc_application_new_web_dialog_show (void);
void uc_application_new_local_file_dialog_show (void);
void uc_application_new_bookmarks_dialog_show (void);
void uc_application_open_dialog_show (void);
void uc_application_about_dialog_show (void);
void uc_application_progress_dialog_show ();
void uc_application_settings_dialog_show (void);
gboolean uc_application_settings_get_data (void);
void uc_application_status_code_properties_init (void);
void uc_application_urls_user_actions_init (void);
UCURLsUserActions *uc_application_get_urls_user_action (gchar * key);
UCURLsUserActions *uc_application_get_urls_user_action_by_value (gchar *
								 value);
UCStatusCode *uc_application_get_status_code_properties (const gchar *
							 status_code);
#endif

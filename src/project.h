/*
 * Copyright (C) 2002-2009
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

#ifndef H_UC_PROJECT
#define H_UC_PROJECT

#include "general.h"

/* enum of project types */
typedef enum
{
  UC_PROJECT_TYPE_WEB_SITE,
  UC_PROJECT_TYPE_LOCAL_FILE,
  UC_PROJECT_TYPE_BOOKMARKS,
  UC_PROJECT_TYPE_ALL,
  UC_PROJECT_TYPES_LEN
}
UCProjectType;

typedef struct
{
  guint32 id;
  gchar *location;
  gchar *title;
  gchar *description;
  time_t create;
  time_t update;
  UCProjectType type;
}
UCProjectProjects;

GHashTable *uc_project_get_cookies (void);
void uc_project_set_cookies (GHashTable * value);
void uc_project_projects_list_load (void);
gboolean uc_project_save_all (void);
void uc_project_report_save (void);
void uc_project_xml_load_settings (const UCProjectProjects * p);
void uc_project_general_settings_save (void);
void uc_project_save_index (void);
void uc_project_delete (const gint32 id);
gint32 uc_project_treeview_get_selected_row_id (void);
GdkPixbuf *uc_project_get_type_icon (const UCProjectType project_type);
gboolean uc_project_open (const gint32 id);
void uc_project_display_informations (GtkTreeView * treeview);
GList *uc_project_get_projects_list (void);
gboolean uc_project_save_properties (void);
gint uc_project_save (void);
void uc_project_free (void);
void uc_project_new (void);
void uc_project_get_reject_directories (gchar *** items, guint * item_size);
void uc_project_get_reject_domains (gchar *** items, guint * item_size);
gchar *uc_project_get_reject_documents (void);
gchar *uc_project_get_reject_images (void);
gchar * uc_project_get_security_bad_extensions (void);
gchar *uc_project_get_security_virii_extensions (void);
gchar *uc_project_get_w3c_html_extensions (void);
gchar *uc_project_get_w3c_css_extensions (void);
void uc_project_set_reject_directories (const gchar * items);
void uc_project_set_reject_documents (const gchar * items);
void uc_project_set_reject_images (const gchar * items);
void uc_project_set_security_bad_extensions (const gchar * items);
void uc_project_set_security_virii_extensions (const gchar * items);
void uc_project_set_w3c_html_extensions (const gchar * items);
void uc_project_set_w3c_css_extensions (const gchar * items);
void uc_project_set_reject_domains (const gchar * items);
UCProjectType uc_project_get_type (void);
gchar *uc_project_get_title (void);
gchar *uc_project_get_description (void);
gboolean uc_project_get_save_bookmarks (void);
gboolean uc_project_get_save (void);
G_CONST_RETURN gchar *uc_project_get_local_charset (void);
gboolean uc_project_get_check_is_current (void);
gboolean uc_project_get_check_is_bookmarks (void);
gboolean uc_project_get_check_is_main (void);
gchar *uc_project_get_url (void);
guint uc_project_get_id (void);
gboolean uc_project_get_speed_check (void);
gchar *uc_project_get_bookmarks_file (void);
UCBookmarksType uc_project_get_bookmarks_type (void);
gchar *uc_project_get_report_export_path (void);
guint uc_project_get_timeouts_blocked (void);
gchar *uc_project_get_current_host (void);
gboolean uc_project_speed_check_get_download_content (void);
gchar *uc_project_get_current_port (void);
gchar *uc_project_get_proxy_host (void);
gchar *uc_project_get_working_path (void);
gchar *uc_project_get_chroot_path (void);
gchar *uc_project_get_cache_name (void);
gboolean uc_project_get_stylesheet_check (void);
gboolean uc_project_get_stylesheet_validate (void);
gboolean uc_project_get_proto_file_is_error (void);
gboolean uc_project_get_proto_file_check (void);
gboolean uc_project_get_cookies_accept (void);
const gchar *uc_project_get_w3c_html_level (void);
gboolean uc_project_get_proto_mailto (void);
gboolean uc_project_get_proto_https (void);
gboolean uc_project_get_proto_ftp (void);
gboolean uc_project_get_passive_ftp (void);
gboolean uc_project_get_proto_mailto_check_mx (void);
gboolean uc_project_get_cookies_warn_added (void);
gboolean uc_project_get_cookies_warn_updated (void);
gboolean uc_project_get_cookies_warn_deleted (void);
guint uc_project_get_check_wait (void);
guint uc_project_get_depth_level (void);
gboolean uc_project_get_check_chroot (void);
gboolean uc_project_get_limit_local (void);
gboolean uc_project_get_use_proxy (void);
guint uc_project_get_proxy_port (void);
gchar *uc_project_get_auth_line (void);
gboolean uc_project_get_download_images_content (void);
gboolean uc_project_get_no_urls_args (void);
gboolean uc_project_get_download_archives_content (void);
guint uc_project_get_check_timeout (void);
gboolean uc_project_get_prompt_auth (void);
gboolean uc_project_get_security_checks (const gchar * type);
gboolean uc_project_get_w3c_checks (const gchar * type);
gboolean uc_project_get_export_labels (void);
gboolean uc_project_get_export_numbering (void);
gboolean uc_project_get_export_external (void);
gboolean uc_project_get_export_ip (void);
gboolean uc_project_get_debug_mode (void);
gboolean uc_project_get_dump_properties (void);
guint uc_project_get_tooltips_delay (void);
gboolean uc_project_get_save (void);
gboolean uc_project_get_onsave (void);
gchar *uc_project_get_browser_path (void);
void uc_project_speed_check_set_download_content (const gboolean value);
void uc_project_set_speed_check (const gboolean value);
void uc_project_set_onsave (const gboolean value);
void uc_project_set_type (const UCProjectType type);
void uc_project_set_id (const guint id);
void uc_project_set_title (const gchar * value);
void uc_project_set_description (const gchar * value);
void uc_project_set_save_bookmarks (const gboolean value);
void uc_project_set_timeouts_blocked (const guint value);
void uc_project_set_save (const gboolean value);
void uc_project_set_local_charset (G_CONST_RETURN gchar * value);
void uc_project_set_check_is_current (const gboolean value);
void uc_project_set_check_is_main (const gboolean value);
void uc_project_set_url (const gchar * value);
void uc_project_set_bookmarks_file (const gchar * value);
void uc_project_set_bookmarks_type (const UCBookmarksType value);
void uc_project_set_report_export_path (const gchar * value);
void uc_project_set_current_host (const gchar * value);
void uc_project_set_current_port (const gchar * value);
void uc_project_set_check_wait (const guint value);
void uc_project_set_depth_level (const guint value);
void uc_project_set_working_path (const gchar * value);
void uc_project_set_cache_name (const gchar * value);
void uc_project_set_proto_file_is_error (const gboolean value);
void uc_project_set_stylesheet_check (const gboolean value);
void uc_project_set_stylesheet_validate (const gboolean value);
void uc_project_set_proto_file_check (const gboolean value);
void uc_project_set_cookies_accept (const gboolean value);
void uc_project_set_w3c_html_level (const gchar * value);
void uc_project_set_proto_mailto (const gboolean value);
void uc_project_set_proto_https (const gboolean value);
void uc_project_set_proto_ftp (const gboolean value);
void uc_project_set_passive_ftp (const gboolean value);
void uc_project_set_proto_mailto_check_mx (const gboolean value);
void uc_project_set_cookies_warn_added (const gboolean value);
void uc_project_set_cookies_warn_updated (const gboolean value);
void uc_project_set_cookies_warn_deleted (const gboolean value);
void uc_project_set_check_chroot (const gboolean value);
void uc_project_set_limit_local (const gboolean value);
void uc_project_set_use_proxy (const gboolean value);
void uc_project_set_proxy_host (const gchar * value);
void uc_project_set_proxy_port (const guint value);
void uc_project_set_auth_line (const gchar * value);
void uc_project_set_chroot_path (const gchar * value);
void uc_project_set_no_urls_args (const gboolean value);
void uc_project_set_download_images_content (const gboolean value);
void uc_project_set_download_archives_content (const gboolean value);
void uc_project_set_check_timeout (const guint value);
void uc_project_set_prompt_auth (const gboolean value);
void uc_project_set_security_checks (const gchar * type,
				     const gboolean value);
void uc_project_set_w3c_checks (const gchar * type, const gboolean value);
void uc_project_set_export_labels (const gboolean value);
void uc_project_set_export_numbering (const gboolean value);
void uc_project_set_export_external(const gboolean value);
void uc_project_set_export_ip (const gboolean value);
void uc_project_set_debug_mode (const gboolean value);
void uc_project_set_dump_properties (const gboolean value);
void uc_project_set_tooltips_delay (const guint value);
void uc_project_set_browser_path (const gchar * value);

#endif

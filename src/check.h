/* 
 * Copyright (C) 2002-2013
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

#ifndef H_UC_CHECK
#define H_UC_CHECK

#include "general.h"

#define UC_CHECK_PARSER_WHILE_CONDITION \
	( \
		(item) && \
		(!uc_check_cancel_get_value ()) \
	)


#define UC_CHECK_ABORT_IF_CONDITION \
	uc_check_cancel_get_value () || \
	(!prop->url) || \
	strcasecmp (uc_project_get_current_host (), host) != 0 || \
	depth == uc_project_get_depth_level ()


/* definition of the treeview columns */
enum
{
  UID_COLUMN,
  LINK_STATUS_ICON_COLUMN,
  SECURITY_ALERT_ICON_COLUMN,
  W3C_ALERT_ICON_COLUMN,
  LINK_ICON_COLUMN,
  ACTION_COLUMN,
  LABEL_COLUMN,
  URL_COLUMN,
  LM_COLUMN,
  N_COLUMNS
};


/*
 * enum for return code of the href properties
 * function
 */
typedef enum
{
  UC_HTTP_CHECK_RETURN_SAME,
  UC_HTTP_CHECK_RETURN_OK,
  UC_HTTP_CHECK_RETURN_BAD,
  UC_HTTP_CHECK_RETURN_REDIR,
  UC_HTTP_CHECK_RETURN_IGNORE,
  UC_HTTP_CHECK_RETURN_FAKED_URL,
  UC_HTTP_CHECK_RETURN_TIMEOUT,
  UC_HTTP_CHECK_RETURN_MALFORMED
}
UCHTTPCheckReturn;


GHashTable *uc_check_url_get_header (UCLinkProperties * prop);
gboolean uc_check_content_type_w3c_accepted (const gchar * type,
					     gchar * path,
					     gchar * content_type);
gboolean uc_check_is_w3c_alert (const UCLinkProperties * prop);
gboolean uc_check_html_is_valid (const gchar * buffer);
gchar *uc_check_url_get_content (UCLinkProperties * prop,
				 const gchar * current_path,
				 const gchar * current_port,
				 const gchar * current_args,
				 gchar * current_host);
void uc_check_run_security_checks (UCLinkProperties * prop);
void uc_check_run_w3c_checks (UCLinkProperties * prop);
guint32 uc_check_get_main_page_id (void);
void uc_check_refresh_report (void);
void uc_check_set_depth (GList * list, const guint depth);
UCLinkProperties *uc_check_copy_node (UCLinkProperties * src,
				      UCLinkProperties * dst);
gboolean uc_check_is_main_page (const UCLinkProperties * prop);
gchar *uc_check_get_link_type_icon_path (const UCLinkType link_type,
					 const gchar * proto);
gchar *uc_check_get_link_type_label (const UCLinkType type);
void uc_check_refresh_link (const guint32 id);
void uc_check_refresh_link_real (UCLinkProperties * prop);
gboolean uc_check_refresh_link_get_value (void);
void uc_check_refresh_link_set_value (const gboolean value);
gchar *uc_check_get_link_type_for_icon (const gchar * proto);
GdkPixbuf *uc_check_get_link_type_icon (const UCLinkType link_type,
					const gchar * proto);
void uc_check_currentitem_init (UCLinkProperties * parent,
                                gchar * current_host, UCHTMLTag * tag,
                                gchar * url, gboolean is_first);
void uc_check_wait (void);
gboolean uc_check_link_already_checked_with_insert (UCLinkProperties * prop,
						    gchar * url);
gpointer uc_check_register_link (const gchar * normalized_url,
				 UCLinkProperties * lp);
UCLinkProperties *uc_check_link_get_properties (const guint depth,
						gchar * current_host,
						gchar * current_path,
						UCHTMLTag * tag,
						UCLinkProperties * old_prop,
						gboolean * accept,
						guint retry);
UCLinkProperties *uc_check_link_properties_node_new (void);
void uc_check_mx_is_valid (void);
gboolean uc_check_status_is_ignored (const gchar * status);
gboolean uc_check_status_is_good (const gchar * status);
gboolean uc_check_status_is_bad (const gchar * status);
gboolean uc_check_status_is_malformed (const gchar * status);
gboolean uc_check_status_is_timeout (const gchar * status);
gboolean uc_check_status_is_email (const gchar * status);
void uc_check_suspend_continue (void);
gboolean uc_check_cancel_get_value (void);
gboolean uc_check_suspend_get_value (void);
gboolean uc_check_ignore_item_get_value (void);
gint32 uc_check_treeview_get_selected_row_id (void);
void uc_check_web_begin (void);
void uc_check_bookmarks_begin (void);
void uc_check_cancel_set_value (const gboolean value);
void uc_check_ignore_item_set_value (const gboolean value);
void uc_check_display_items_active_all (void);
void uc_check_display_list (GList * list, gchar * path, GtkTreeIter iter);
void uc_check_reset (void);
void uc_check_alarm_callback (const int dum);
void uc_check_link_view_source (void);

#endif

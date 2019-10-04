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

#ifndef H_UC_UTILS
#define H_UC_UTILS

#include "general.h"

gboolean uc_utils_str_is_not_alphanum (const gchar *str);
void uc_utils_clear_container (GtkContainer * container);
void uc_utils_swap_file_proto_option (const gboolean begin);
time_t uc_utils_http_atotm (const gchar * time_string);
gchar *uc_utils_url_reencode_escapes (const gchar * s);
gboolean uc_utils_get_auth_fields (GladeXML * g,
				   GtkWidget * w_auth_user,
				   GtkWidget * w_auth_password,
				   gchar ** auth_user, gchar ** auth_password,
				   gchar ** auth_line);
gchar *uc_utils_build_auth_line (const gchar * auth_user,
				 const gchar * auth_password);
gboolean uc_utils_ftp_code_search (const gchar * buffer, const gchar * code,
				   const gint len);
gchar *uc_utils_get_string_from_size (const gsize size);
gchar *uc_utils_get_server_from_header_field (gchar * field);
void uc_utils_copy (const gchar * src, const gchar * dest);
gboolean uc_utils_get_meta_refresh_location (UCLinkProperties * prop,
					     UCHTMLTag * tag);
gchar *uc_utils_to_utf8 (const gchar * data);
void uc_utils_copy_files (const gchar * src, const gchar * dest);
void uc_utils_debug (const gchar * format, ...);
void uc_utils_debug_dump_link_properties (const UCLinkProperties * prop);
void uc_utils_set_userfriendly_treeview_column (const GtkTreeView * tv,
						const gint position);
gchar *uc_utils_get_file_content (const gchar * path, gsize * length);
void uc_utils_split_email (const gchar * email, gchar ** user,
			   gchar ** domain);
gchar *uc_utils_get_mx (const gchar * domain);
gboolean uc_utils_get_yesno (const gchar * yesno);
gboolean uc_utils_mx_is_valid (const gchar * mx);
gchar *uc_utils_get_ip (const gchar * host);
UCEmailStatus uc_utils_email_is_valid (const gchar * email,
				       const gboolean check_mx);
gchar *uc_utils_string_cut (const gchar * label, const gsize size);
gchar *uc_utils_string_format4display (const gchar * label, const gsize size);
gchar *uc_utils_replace (const gchar * str, const gchar * old,
			 const gchar * new);
gchar *uc_utils_replace1 (gchar * string, const gchar c1, const char c2);
gchar *uc_utils_replacelr (gchar * string, const gchar c);
gchar *uc_utils_convert_uid2file (const guint32 uid);
void uc_utils_get_gnome_proxy_conf (gchar ** host, guint * port);
gchar *uc_utils_clean_tag_link_value (gchar * value);
gpointer uc_utils_search_string_next (gpointer buf, const gchar * str,
				      const gchar limit_char);
gboolean uc_utils_memcasecmp (const gchar * str1, const gchar * str2);
void uc_utils_rmfiles (const gchar * path);
gboolean uc_utils_mkdirs (const gchar * path, const gboolean create_all);
void uc_utils_rmdirs (const gchar * path, const gboolean delete_all);
guint32 uc_utils_vector_length (gpointer data);
gchar *uc_utils_strpbrk_or_eos (const gchar * str, const gchar * accept);
gchar *uc_utils_strdup_delim (const gchar * begin, const gchar * end);
gboolean uc_utils_test_socket_open (const guint sock);
gchar *uc_utils_get_gnome_browser_conf (void);

#endif

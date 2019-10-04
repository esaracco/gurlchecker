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


#ifndef H_UC_URL
#define H_UC_URL

#include "general.h"

gchar *uc_url_extract_url_from_local_path (const gchar * path);
gboolean uc_url_is_faked (UCLinkProperties * prop, UCHTMLTag * tag);
void uc_url_correction (UCLinkProperties * prop);
gchar *uc_url_normalize (const gchar * current_host,
			 const gchar * current_path, gchar * url);
gchar *uc_url_get_hostname (const gchar * current_host, const gchar * url);
gchar *uc_url_get_port (const gchar * url);
gchar *uc_url_add_protocol (const gchar * proto, const gchar * host);
gchar *uc_url_add_slash (const gchar * url);
gboolean uc_url_is_valid (const gchar * url);
gchar *uc_url_get_protocol (const gchar * url);
gboolean uc_url_parse (const gchar * current_host, const gchar * current_path,
		       gchar * rurl, gchar ** host, gchar ** port,
		       gchar ** path, gchar ** args);
gchar *uc_url_get_ip (gchar * host);

#endif

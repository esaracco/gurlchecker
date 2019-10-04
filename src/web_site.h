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

#ifndef H_UC_WEB_SITE
#define H_UC_WEB_SITE


#include "general.h"

GList *uc_web_site_get_links_real (UCLinkProperties * url,
				   const gchar * host, const guint depth);
void uc_web_site_refresh_parent (const guint id);

void uc_web_site_refresh_all (void);
void uc_web_site_refresh_branch (const gint32 id);
void uc_web_site_refresh_link (const gint32 id);
void uc_web_site_begin_check (void);

#endif

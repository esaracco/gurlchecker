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

#ifndef H_UC_BOOKMARKS
#define H_UC_BOOKMARKS

#include "general.h"

void uc_bookmarks_free (void);
UCBookmarksType uc_bookmarks_guess_type (const gchar *file);
gint uc_bookmarks_save_changes (void);
void uc_bookmarks_delete_bad_links (GList *list);
void uc_bookmarks_delete_link (UCLinkProperties *prop);
gboolean uc_bookmarks_format_is_googlechrome (const gchar * file);
gboolean uc_bookmarks_format_is_firefox (const gchar *file);
gboolean uc_bookmarks_format_is_xbel (const gchar * file);
gboolean uc_bookmarks_format_is_opera (const gchar * file);
void uc_bookmarks_begin_check (void);

#endif

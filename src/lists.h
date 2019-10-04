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

#ifndef H_UC_LISTS
#define H_UC_LISTS


#include "general.h"


void uc_lists_html_tags_node_free (GList **list, UCHTMLTag **tag);
void uc_lists_checked_links_dump (GList *list, FILE **fd,
                                  const UCExportFormat ftype);
void uc_lists_refresh_preserved_links_reset (void);
void uc_lists_refresh_preserved_links_append (UCLinkProperties * prop);
GList *uc_lists_refresh_preserved_links_restore (GList * src);
GList *uc_lists_links_first_minus_second (GList * first, GList * second);
void uc_lists_similar_links_remove_item (GList * list, const gchar * url);
void uc_lists_similar_links_remove_items (UCLinkProperties * prop);
void uc_lists_already_checked_links_remove_branch_items (UCLinkProperties *
							 prop);
void uc_lists_already_checked_links_remove (const gchar * url);
void uc_lists_already_checked_links_remove_items (UCLinkProperties * prop);
void uc_lists_checked_links_node_replace (UCLinkProperties * old,
					  UCLinkProperties * new);
gpointer uc_lists_already_checked_links_lookup (const gchar * url,
						const gboolean return_node);
gboolean uc_lists_already_checked_links_append (const gchar * url,
						gpointer ac_p);
void uc_lists_already_checked_free (void);
guint32 uc_lists_checked_links_is_empty (void);
void uc_lists_checked_links_set (GList * list);
GList *uc_lists_checked_links_get (void);
void uc_lists_checked_links_node_free (GList **list, UCLinkProperties **prop);
gpointer uc_lists_checked_links_lookup (const gchar * url);
gboolean uc_lists_checked_links_exist (const gchar * url);
UCLinkProperties *uc_lists_checked_links_lookup_by_uid (const guint32 uid);
void uc_lists_checked_links_free (void);
gboolean uc_lists_similar_links_exist (GList * list, const gchar * url);
void uc_lists_similar_links_append (gpointer prop_p, const gchar *url);

#endif

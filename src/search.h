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

#ifndef H_UC_SEARCH
#define H_UC_SEARCH


#include "general.h"


gboolean uc_search_get_exit (void);
void uc_search_set_exit (const gboolean val);
void uc_search_main_select (GtkTreeView * selection);
void uc_search_check_verification (void);
void uc_search_free (void);
void uc_search_begin (void);

#endif

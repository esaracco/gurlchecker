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

#ifndef H_UC_HTML_PARSER
#define H_UC_HTML_PARSER

#include "general.h"


void uc_html_parser_node_free (UCHTMLTag **tag);
GList *uc_html_parser_get_tags (gchar * buffer_orig);
UCHTMLTag *uc_html_parser_node_new (void);
GList *uc_css_parser_get_tags (gchar * buffer_orig);

#endif

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

#ifdef ENABLE_TIDY
#ifndef H_UC_TIDY
#define H_UC_TIDY

#include "general.h"

typedef struct _UCTidy UCTidy;
struct _UCTidy
{
  TidyBuffer errbuf;
  TidyDoc tdoc;
};

UCTidy *uc_uctidy_new (void);
gint uc_uctidy_validate (UCTidy * tidy, const gchar * buffer);
void uc_uctidy_free (UCTidy ** tidy);

#endif
#endif

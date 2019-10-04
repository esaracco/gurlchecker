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

#ifdef ENABLE_CROCO
#ifndef H_UC_CROCO
#define H_UC_CROCO

#include "general.h"

typedef struct _UCCroco UCCroco;
struct _UCCroco
{
  CRDocHandler *css_handler;
  gboolean is_valid;
  gboolean log_error;
  CRParser *parser;
  GString *def;
  GString *error_buffer;
  guint error_count;
};

UCCroco *uc_uccroco_new (void);
gboolean uc_uccroco_validate (UCCroco * croco, const gchar * path);
void uc_uccroco_free (UCCroco ** croco);

#endif
#endif

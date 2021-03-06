/*
 * Copyright (C) 2002-2009
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

#ifdef ENABLE_CLAMAV
#ifndef H_UC_CLAM
#define H_UC_CLAM

#include "general.h"

typedef struct _UCClam UCClam;
struct _UCClam
{
  gsize size;
  unsigned int sigs;
  const char *virname;
  struct cl_engine *engine;
};

UCClam *uc_ucclam_new (gchar **error);
gboolean uc_ucclam_scan (UCClam * clam, const gchar * path, gchar ** virname);
void uc_ucclam_free (UCClam ** clam);

#endif
#endif

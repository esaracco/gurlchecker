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

#include "utils.h"

#include "cache.h"


/**
 * uc_cache_change_id:
 * @old: Old #UCLinkProperties node id.
 * @new: New #UCLinkProperties node id.
 *
 * Replace "old" with "new" (swap cached files). Mainly used after rescanning
 * a link.
 */
void
uc_cache_change_id (const guint32 old, const guint32 new)
{
  FILE *fd;
  gchar *buffer = NULL;
  gchar *path = NULL;
  gsize length = 0;


  if ((buffer = uc_cache_get_source (new, &length)) == NULL)
    return;

  path = uc_utils_convert_uid2file (old);
  fd = fopen (path, "w");
  g_free (path), path = NULL;
  g_assert (fd != NULL);

  fwrite (buffer, 1, length, fd);
  fclose (fd);

  path = uc_utils_convert_uid2file (new);
  unlink (path);
  g_free (path), path = NULL;

  g_free (buffer), buffer = NULL;
}


/**
 * uc_cache_get_source:
 * @id: #UCLinkProperties node id of the wanted link content.
 * @length: Set this argument with the size of the returned buffer.
 * 
 * Retreive the source page in cache for a link.
 * 
 * Returns: The source code of the given #UCLinkProperties node id.
 */
gchar *
uc_cache_get_source (const guint32 id, gsize * length)
{
  gchar *buffer = NULL;
  gchar *path = NULL;


  path = uc_utils_convert_uid2file (id);
  g_assert (path != NULL);
  buffer = uc_utils_get_file_content (path, &(*length));
  g_free (path), path = NULL;

  return buffer;
}


/**
 * uc_cache_append_source:
 * @id: #UCLinkProperties node id of the element to put in cache.
 * @src: Content to work with.
 * @length: Lenght of the given buffer.
 * 
 * Put a source page in the cache.
 */
void
uc_cache_append_source (const guint32 id, gchar * src, const gsize length)
{
  FILE *fd = NULL;
  gchar *path = NULL;
  gchar *src_tmp = NULL;


  if (src == NULL || strlen (src) == 0 || 
      (path = uc_utils_convert_uid2file (id)) == NULL)
  {
    g_warning ("No data to cache!");
    return;
  }

  // Pass HTTP header
  src_tmp = uc_utils_search_string_next (src, "\r\n\r\n", '<');

  if (src_tmp != NULL)
  {
    fd = fopen (path, "wb");
    g_assert (fd != NULL);
    fwrite (src_tmp, 1, length, fd);
    fclose (fd);
  }

  g_free (path), path = NULL;
}

/*
 * Copyright (C) 2002-2011
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
#include "ucclam.h"
#include "utils.h"

UCClam *
uc_ucclam_new (gchar **error)
{
  gint ret;
  UCClam *clam = NULL;

  clam = g_new0 (UCClam, 1);
  clam->virname = NULL;
  clam->size = 0;

  // Do not call cl_init() twice
  if (!uc_init_clamav_done && (ret = cl_init (CL_INIT_DEFAULT)) != CL_SUCCESS)
    {
      if (error != NULL)
        *error = g_strdup_printf ("cl_init() error: %s",
                                  (gchar *) cl_strerror (ret));

      uc_ucclam_free (&clam);
      return NULL;
    }

  uc_init_clamav_done = TRUE;

  if (!(clam->engine = cl_engine_new ()))
    {
      if (error != NULL)
        *error = g_strdup ("cl_engine_new() error");
      uc_ucclam_free (&clam);
      return NULL;
    }

  if ((ret = cl_load (cl_retdbdir (),
		      clam->engine, &clam->sigs, CL_DB_STDOPT)) != CL_SUCCESS)
    {
      if (error != NULL)
        *error = g_strdup_printf ("cl_load(%s) error: %s",
                                  cl_retdbdir (), (gchar *) cl_strerror (ret));
      uc_ucclam_free (&clam);
      return NULL;
    }

  if ((ret = cl_engine_compile (clam->engine)) != CL_SUCCESS)
    {
      if (error != NULL)
        *error = g_strdup_printf ("cl_compile() error: %s",
                                  (gchar *) cl_strerror (ret));
      uc_ucclam_free (&clam);
      return NULL;
    }

  return clam;
}

gboolean
uc_ucclam_scan (UCClam * clam, const gchar * path, gchar ** virname)
{
  gint fd = 0;
  gboolean ret = FALSE;

  if ((fd = open (path, O_RDONLY)) == -1)
    {
      g_warning ("Can't open file \"%s\" to scan it for virii!", path);
      return FALSE;
    }

  clam->virname = path;

  uc_utils_debug ("[SECURITY] Scanning %s for virii...\n", (path)?path:"");
  if ((ret = cl_scandesc (fd, &clam->virname, &clam->size, clam->engine,
			  CL_SCAN_STDOPT) == CL_VIRUS))
    {
      *virname = g_strdup (clam->virname);
      uc_utils_debug ("[SECURITY]\tVirus found here!\n");
    }

  close (fd);

  return (ret == CL_VIRUS);
}

void
uc_ucclam_free (UCClam ** clam)
{
  if (!clam)
    return;

  if ((*clam)->engine)
    cl_engine_free ((*clam)->engine), (*clam)->engine = NULL;

  g_free (*clam), *clam = NULL;
}
#endif

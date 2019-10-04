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
#include "project.h"
#include "uctidy.h"
#include "utils.h"

UCTidy *
uc_uctidy_new (void)
{
  UCTidy *tidy = NULL;

  tidy = g_new0 (UCTidy, 1);
  memset (&tidy->errbuf, 0, sizeof (tidy->errbuf));
  if (!(tidy->tdoc = tidyCreate ()))
    {
      uc_uctidy_free (&tidy);
      return NULL;
    }

  tidyOptSetBool (tidy->tdoc, TidyShowWarnings, yes);
  tidySetCharEncoding (tidy->tdoc, "utf8");

  return tidy;
}

gint
uc_uctidy_validate (UCTidy * tidy, const gchar * buffer)
{
  gint ret = -1;
  gchar *tmp = NULL;

  tmp = uc_utils_to_utf8 (buffer);

  ret = tidySetErrorBuffer (tidy->tdoc, &tidy->errbuf);
  if (ret >= 0)
    ret = tidyParseString (tidy->tdoc, tmp);
  if (ret >= 0)
    ret = tidyRunDiagnostics (tidy->tdoc);

  /* If user want to consider that even with warnings, page is valid */
  if (ret == 1 && !strcmp (uc_project_get_w3c_html_level (), "errors"))
    ret = 0;

  g_free (tmp), tmp = NULL;

  return ret;
}

void
uc_uctidy_free (UCTidy ** tidy)
{
  if (!tidy)
    return;

  tidyBufFree (&(*tidy)->errbuf);
  if ((*tidy)->tdoc)
    tidyRelease ((*tidy)->tdoc);

  g_free (*tidy), *tidy = NULL;
}
#endif

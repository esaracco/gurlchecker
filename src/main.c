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

#include "utils.h"
#include "url.h"
#include "application.h"

static struct option long_options[] = {
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'V'},
  {"http-user", required_argument, NULL, 'u'},
  {"http-passwd", required_argument, NULL, 'p'},
  {"no-urls-args", required_argument, NULL, 'A'},
  {NULL, 0, NULL, 0}
};

static void uc_display_version (void);
static void uc_display_help (void);


int
main (int argc, char *argv[])
{
  guint i = 0;
  guint uc_argc = 1;
  gchar *uc_argv[] = { PACKAGE, NULL };
  gchar *url = NULL;
  gchar *auth_user = NULL;
  gchar *auth_password = NULL;
  gboolean no_urls_args = FALSE;

#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  LIBXML_TEST_VERSION;
  xmlIndentTreeOutput = 1;
  xmlKeepBlanksDefault (0);

  g_thread_init (NULL);
  gnet_init ();
  gnome_program_init (PACKAGE, UC_VERSION, LIBGNOMEUI_MODULE,
		      uc_argc, uc_argv,
		      GNOME_PARAM_APP_DATADIR, PACKAGE_DATA_DIR, NULL);

  while ((i =
	  getopt_long (argc, argv, "hu:p:VA", long_options, (gint32 *) 0))
	 != EOF)
    {
      switch (i)
	{
	case 'V':
	  uc_display_version ();
	  exit (0);
	case 'A':
	  no_urls_args = TRUE;;
	  break;
	case 'u':
	  auth_user = optarg;
	  break;
	case 'p':
	  auth_password = optarg;
	  break;
	default:
	  uc_display_help ();
	  exit (0);
	}
    }

  glade_gnome_init ();
  gconf_init (uc_argc, uc_argv, NULL);

  uc_application_globals_init ();

  /* check if there is a command line url
   * argument */
  if (argc > 1)
    {
      gchar *tmp_url = NULL;

      tmp_url = uc_url_add_protocol (UC_PROTOCOL_HTTP, argv[argc - 1]);

      if (uc_url_is_valid (tmp_url))
	url = tmp_url;
      else
	g_free (tmp_url), tmp_url = NULL;
    }

  uc_application_init (url, auth_user, auth_password, no_urls_args);
  g_free (url), url = NULL;

  GDK_THREADS_ENTER ();
  gtk_main ();
  GDK_THREADS_LEAVE ();

  return 0;
}


static void
uc_display_version (void)
{
  g_print (PACKAGE " " UC_VERSION
	   "\n%s\n\nCopyright (C) 2002-2005 Emmanuel Saracco.\n"
	   "%s\n",
	   _
	   ("Wrote by Emmanuel Saracco <esaracco@users.labs.libre-entreprise.org>."),
	   _("This is free software; see the source for copying conditions.  "
	     "There is NO warranty; not even for MERCHANTABILITY or FITNESS "
	     "FOR A PARTICULAR PURPOSE."));
}


static void
uc_display_help (void)
{
  g_print (_("Usage: %s [OPTION] [URL]\n"
	     "A graphic web link checker.\n\n"
	     "  -h, --help          display this help\n"
	     "  -V, --version       display %s version\n"
	     "  -u, --http-user     user for basic authentication\n"
	     "  -p, --http-passwd   password for basic authentication\n"
	     "  -A, --no-urls-args   ignore URLs arguments\n\n"
	     "Report bugs to <esaracco@users.labs.libre-entreprise.org>\n"),
	   PACKAGE, PACKAGE);
}

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
#include "project.h"


#include "timeout.h"


/**
 * UCTODomains:
 * @count: Number timeouts for a host.
 * @domain: The host.
 * @blocked: %TRUE if this host must not be checked anymore.
 *
 * Struct to manage timedouted domains filtering.
 */
typedef struct _UCTODomains UCTODomains;
struct _UCTODomains
{
  guint count;
  gchar *domain;
  gboolean blocked;
};


// Global variable to register timeouted domains to
// prevent rescaning them */
static GList *uc_timeout_domains = NULL;

static UCTODomains *uc_timeout_domains_new (const gchar * domain);
static UCTODomains *uc_timeout_domains_lookup (const gchar * domain);


/**
 * uc_timeout_domains_init:
 *
 * Initialisation of timeout_domains stuff.
 */
void
uc_timeout_domains_init (void)
{
  if (uc_timeout_domains != NULL)
    uc_timeout_domains_free ();
}


/**
 * uc_timeout_domains_free:
 *
 * Clean all timeout domains stuff.
 */
void
uc_timeout_domains_free (void)
{
  GList *item = NULL;


  item = g_list_first (uc_timeout_domains);
  while (item != NULL)
  {
    UCTODomains *d = (UCTODomains *) item->data;
    item = g_list_next (item);
    g_free (d->domain), d->domain = NULL;
    g_free (d), d = NULL;
  }

  g_list_free (uc_timeout_domains), uc_timeout_domains = NULL;
}


/**
 * uc_timeout_domains_new:
 * @domain: The host.
 * 
 * Create a new record for the given host.
 * 
 * Returns: A new #UCTODomains node.
 */
UCTODomains *
uc_timeout_domains_new (const gchar * domain)
{
  UCTODomains *d = NULL;


  g_return_val_if_fail (domain != NULL, NULL);

  d = g_new0 (UCTODomains, 1);
  d->count = 0;
  d->domain = g_strdup (domain);
  d->blocked = FALSE;

  return d;
}


/**
 * uc_timeout_domains_lookup:
 * @domain: The host.
 * 
 * Returns: A #UCTODomains node.
 */
static UCTODomains *
uc_timeout_domains_lookup (const gchar * domain)
{
  GList *item = NULL;


  g_return_val_if_fail (domain != NULL, NULL);

  item = g_list_first (uc_timeout_domains);
  while (item != NULL)
    {
      UCTODomains *d = (UCTODomains *) item->data;


      item = g_list_next (item);

      if (!g_ascii_strcasecmp (domain, d->domain))
	return (gpointer) d;
    }

  return NULL;
}


/**
 * uc_timeout_domains_register:
 * @domain: The host.
 *
 * Register a new host. Just increment timeout count if the host
 * already exist. If not, create a new entry for this host.
 */
void
uc_timeout_domains_register (const gchar * domain)
{
  UCTODomains *item = NULL;


  g_return_if_fail (domain != NULL);

  item = uc_timeout_domains_lookup (domain);
  if (item == NULL)
  {
    item = uc_timeout_domains_new (domain);
    uc_timeout_domains = g_list_append (uc_timeout_domains, (gpointer) item);
  }
  else
    item->count++;

  if (item->count >= uc_project_get_timeouts_blocked ())
    item->blocked = TRUE;
}


/**
 * uc_timeout_domains_is_blocked:
 * @domain: The host.
 *
 * Check if a given host must  not be checked anymore
 * because of too many timeouts.
 * 
 * Returns: %TRUE if the given host must not be checked
 *          anymore.
 */
gboolean
uc_timeout_domains_is_blocked (const gchar * domain)
{
  UCTODomains *item = NULL;
  gboolean ret = FALSE;


  g_return_val_if_fail (domain != NULL, FALSE);

  if (g_ascii_strcasecmp (domain, uc_project_get_current_host ()))
  {
    item = uc_timeout_domains_lookup (domain);
    ret = (item != NULL && item->blocked);
  }

  if (ret)
    uc_utils_debug ("       -> %s has been blocked (to many timeouts)!\n",
		    (domain)?domain:"");

  return ret;
}

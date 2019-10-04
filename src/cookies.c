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
#include "application.h"


typedef struct _UCCookie UCCookie;
struct _UCCookie
{
  gchar *name;
  gchar *value;
  gchar *expires_a;
  time_t expires_tm;
};

static void
uc_cookies_free_callback (gpointer key, gpointer value, gpointer user_data);
static GList *uc_cookies_value_get (const gchar * key);
static void uc_cookies_get_value (const gchar * cookie,
				  const gchar * name, gchar ** key,
				  gchar ** value);
static gboolean
uc_cookies_value_get_cb (gpointer key, gpointer value, gpointer data);


static void
uc_cookies_free_callback (gpointer key, gpointer value, gpointer user_data)
{
  GList *items = NULL;
  GList *list = (GList *) value;


  if (list == NULL)
    return;

  items = g_list_first (list);
  while (items != NULL)
  {
    UCCookie *c = (UCCookie *) items->data;


    items = g_list_next (items);

    g_free (c->name), c->name = NULL;
    g_free (c->value), c->value = NULL;
    g_free (c->expires_a), c->expires_a = NULL;
    g_free (c), c = NULL;
  }

  g_list_free (list), list = NULL;
}


void
uc_cookies_free (void)
{
  GHashTable *cookies = uc_project_get_cookies ();


  g_hash_table_foreach (cookies, uc_cookies_free_callback, NULL);
  g_hash_table_destroy (cookies), cookies = NULL;

  uc_project_set_cookies (cookies);
}


static gboolean
uc_cookies_value_get_cb (gpointer key, gpointer value, gpointer data)
{
  return (uc_utils_memcasecmp ((gchar *) data, (gchar *) key));
}


static GList *
uc_cookies_value_get (const gchar * key)
{
  return g_hash_table_find (uc_project_get_cookies (),
                            uc_cookies_value_get_cb, (gpointer) key);
}


gchar *
uc_cookies_get_header_field (gchar * path)
{
  GList *list = NULL;
  gchar *ret = NULL;
  time_t now = time (NULL);


  if ((list = uc_cookies_value_get (path)))
  {
    GList *item = NULL;
    GString *str = g_string_new ("Cookie:");


    item = g_list_first (list);
    while (item != NULL)
    {
      UCCookie *c = (UCCookie *) item->data;


      if (c->expires_tm > 0 && c->expires_tm <= now)
        ;
      else
        g_string_append_printf (str, " %s=%s;", c->name, c->value);

       item = g_list_next (item);
    }

    if (strchr (str->str, '=') != NULL)
    {
      g_string_erase (str, str->len - 1, 1);
      g_string_append (str, "\r\n");
      ret = str->str;
      g_string_free (str, FALSE);
    }
    else
      g_string_free (str, TRUE);
  }

  return (ret != NULL) ? ret : g_strdup ("");
}


/**
 * uc_cookies_get_value:
 *
 *
 * If @name is %NULL, we fill @key and @value. If @name is not %NULL we look
 * for @name and fill @value (@key is not filled)
 */
static void
uc_cookies_get_value (const gchar * cookie, const gchar * name,
                      gchar ** key, gchar ** value)
{
  gchar **fields = NULL;
  gchar **name_value = NULL;
  guint i = 0;


  fields = g_strsplit (cookie, ";", 0);
  if (name == NULL)
  {
    g_strstrip (fields[0]);
    name_value = g_strsplit (fields[0], "=", 2);
    *key = g_strdup (name_value[0]);
    *value = g_strdup (name_value[1]);
    g_strfreev (name_value), name_value = NULL;
  }
  else
  {
    for (i = 0; fields[i] != NULL && *value == NULL; i++)
    {
      g_strstrip (fields[i]);
      name_value = g_strsplit (fields[i], "=", 2);
      if (name_value != NULL && name_value[0] != NULL &&
          name_value[1] != NULL && strcasecmp (name_value[0], name) == 0)
        *value = g_strdup (name_value[1]);
      g_strfreev (name_value), name_value = NULL;
    }
  }

  g_strfreev (fields), fields = NULL;
}


void
uc_cookies_add (const UCLinkProperties * prop, const gchar * cook)
{
  gchar *key = NULL;
  gchar *value = NULL;
  gchar *expires = NULL;
  time_t expires_tm;
  time_t now = time (NULL);
  gchar *path = NULL;
  gchar *cookie = NULL;
  GList *list = NULL;
  GList *item = NULL;
  gboolean found = FALSE;
  GHashTable *cookies = uc_project_get_cookies ();


  // Add a default path "/" if there is no path specified 
  if (strstr (cook, "ath=/") == NULL && strstr (cook, "ATH=/") == NULL)
    cookie = g_strconcat (cook, "; path=/", NULL);
  else
    cookie = g_strdup (cook);

  // Get path
  uc_cookies_get_value (cookie, "path", NULL, &path);

  // Get expiration
  uc_cookies_get_value (cookie, "expires", NULL, &expires);

  // Get cookie
  uc_cookies_get_value (cookie, NULL, &key, &value);

  if ((list = (GList *) g_hash_table_lookup (cookies, path)))
  {
    item = g_list_first (list);
    while (item != NULL && !found)
    {
      UCCookie *c = (UCCookie *) item->data;


      item = g_list_next (item);

      if (strcmp (c->name, key) == 0)
      {
        found = TRUE;

        // If value is empty, or cookie has expires
        if (strlen (value) == 0 ||
            (expires && (uc_utils_http_atotm (expires) <= now)))
        {
          if (uc_project_get_cookies_warn_deleted () &&
              !uc_application_cookie_warning_dialog_show (
                prop->h_name, prop->path,
                _("The following cookie will be <b>deleted</b>"), &key, &value,
                &path, &expires, UC_COOKIES_ACTION_DELETE))
            goto exit_refused;
  
          g_free (c->name), c->name = NULL;
          g_free (c->value), c->value = NULL;
          g_free (c->expires_a), c->expires_a = NULL;
          list = g_list_remove (list, (gpointer) c);
          g_free (c), c = NULL;
  
          g_hash_table_replace (cookies, g_strdup (path), list);
  
          goto exit_refused;
        }
        // Update cookie content
        else
        {
          if (uc_project_get_cookies_warn_updated () &&
              !uc_application_cookie_warning_dialog_show (
                prop->h_name, prop->path,
                _("The following cookie will be <b>updated</b>"), &key, &value,
                &path, &expires, UC_COOKIES_ACTION_UPDATE))
            goto exit_refused;
  
          g_free (c->value), c->value = NULL;
          c->value = value;
          g_free (c->expires_a), c->expires_a = NULL;
          c->expires_tm = 0;

          if ((expires_tm = uc_utils_http_atotm (expires)) > 0)
          {
            c->expires_a = strdup (expires);
            c->expires_tm = expires_tm;
          }
          else if (expires)
            ;// bad cookie
        }
      }
    }
  }

  expires_tm = uc_utils_http_atotm (expires);

  // Add cookie
  if (!found && (!expires || (uc_utils_http_atotm (expires) > now)))
  {
     UCCookie *c = NULL;


    if (uc_project_get_cookies_warn_added () &&
        !uc_application_cookie_warning_dialog_show (prop->h_name, prop->path,
          _("The following cookie will be <b>added</b>"), &key, &value, &path,
          &expires, UC_COOKIES_ACTION_ADD))
      goto exit_refused;

    expires_tm = uc_utils_http_atotm (expires);

    // If user entered a expired date
    if (expires && expires_tm <= now)
    {
      if (expires_tm < 0)
        ;//bad cookie;
      goto exit_refused;
    }

    c = g_new0 (UCCookie, 1);
    c->name = key;
    c->value = value;
    c->expires_a = NULL;
    c->expires_tm = 0;
    if (expires_tm > 0)
    {
      c->expires_a = g_strdup (expires);
      c->expires_tm = expires_tm;
    }

    list = g_list_prepend (list, c);
  }
  // The cookie to add has already expired
  else if (!found && (expires && expires_tm <= now))
  {
    if (expires_tm < 0)
      ;//bad cookie
    goto exit_refused;
  }
  // The cookie was updated, so key (cookie name) remain the same
  else
    g_free (key), key = NULL;

  g_hash_table_replace (cookies, path, list);

  goto exit_ok;

exit_refused:

  g_free (key), key = NULL;
  g_free (value), value = NULL;
  g_free (path), path = NULL;

exit_ok:

  g_free (expires), expires = NULL;
  g_free (cookie), cookie = NULL;

  uc_project_set_cookies (cookies);
}

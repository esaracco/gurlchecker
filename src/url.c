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

#include "url.h"


static gchar *uc_url_get_absolute (gchar * url);
static gboolean uc_url_is_relative (const gchar * url);


/**
 * uc_url_add_slash:
 * @url: a URL.
 *
 * Check if the given url @url end by a slash. if not, add it.
 * 
 * Returns: a new allocated string ended by a slash.
 */
gchar *
uc_url_add_slash (const gchar * url)
{
  g_return_val_if_fail (url != NULL, g_strdup (""));

  return (url[strlen (url) - 1] != '/') ?
    g_strconcat (url, "/", NULL) : g_strdup (url);
}


/**
 * uc_url_is_faked:
 * @prop: A #UCLinkProperties object.
 * @tag: A #UCHTMLTag.
 * 
 * Check if the link is in fact a redirection or if it
 * has another location.
 * 
 *  Returns: %TRUE if URL has been modified. Tag struct is modified.
 */
gboolean
uc_url_is_faked (UCLinkProperties * prop, UCHTMLTag * tag)
{
  gchar *new_location = NULL;
  gboolean ret = FALSE;
  gboolean concat = FALSE;
  gchar *tmp = NULL;
  gchar *port = NULL;


  if (prop == NULL || prop->header == NULL)
    return ret;

  new_location = g_hash_table_lookup (prop->header, UC_LOCATION);
  if (new_location == NULL)
    new_location = g_hash_table_lookup (prop->header, UC_CONTENT_LOCATION);

  // If new_location is ok, go one, else continue to default
  if (new_location != NULL)
    {
      ret = TRUE;

      if (strcmp (prop->port, UC_URL_DEFAULT_SSL_PORT) == 0 ||
          strcmp (prop->port, UC_URL_DEFAULT_PORT) == 0)
        port = g_strdup ("");
      else
        port = g_strconcat (":", prop->port, NULL);

      concat =
	(!(uc_utils_memcasecmp (new_location, UC_PROTOCOL_HTTP) ||
	   uc_utils_memcasecmp (new_location, UC_PROTOCOL_HTTPS)
	   || *tag->value == '/'));

      g_free (tag->value), tag->value = NULL;

      if (*new_location == '/')
	tag->value =
	  g_strconcat (prop->protocol, "://",
                       prop->h_name, port, new_location, NULL);
      else
	{
	  tmp = strrchr (prop->path, '/');
	  if (tmp != NULL)
	    *(tmp + 1) = '\0';

	  if (!concat &&
	      *new_location != '/' &&
	      !uc_utils_memcasecmp (new_location, UC_PROTOCOL_HTTP) &&
	      !uc_utils_memcasecmp (new_location, UC_PROTOCOL_HTTPS))
	    concat = TRUE;

	  if (concat)
	    {
	      tag->value =
		g_strconcat (prop->protocol, "://", prop->h_name, port,
			     prop->path,
			     ((*new_location != '/')
			      && (prop->path[strlen (prop->path) - 1] !=
				  '/')) ? "/" : "", new_location, NULL);
	    }
	  else if (uc_utils_memcasecmp (new_location, UC_PROTOCOL_HTTP)
		   || uc_utils_memcasecmp (new_location, UC_PROTOCOL_HTTPS))
	    tag->value = g_strdup (new_location);
	  else
	    tag->value = g_strconcat ("/", new_location, NULL);
	}

      g_free (port), port = NULL;

      uc_utils_debug (
        "              Real location: \"%s\" for the following link:\n",
        (tag->value)?tag->value:"");
    }

  return ret;
}


static gboolean
uc_url_is_relative (const gchar * url)
{
  const gchar *tmp = NULL;


  return (url != NULL &&
	  (tmp = strstr (url, "./")) != NULL &&
	  !g_ascii_isalpha (*(tmp - 1)));
}


/**
 * uc_url_get_absolute:
 * @url: A complete URI or just a path.
 *
 * If the given URI contain a relative path, rewrite it in
 * absolute.
 *
 * For example:
 * http://www.perl.com/first/second/third/../../.././index.html will become
 * http://www.perl.com/index.html
 * 
 * Returns: The absolute path. A new allocated string.
 */
static gchar *
uc_url_get_absolute (gchar * url)
{
  gchar *p1 = NULL;
  gchar *p2 = NULL;
  gchar *p3 = NULL;
  gchar *begin = NULL;
  gchar *end = NULL;
  gchar *ret = NULL;
  gsize inc = 0;


  g_return_val_if_fail (url != NULL, g_strdup (""));

  if (strstr (url, "./") == NULL)
    return g_strdup (url);

  // If it is a path, it must not begin with more than on slash
  while (url[1] && url[1] == '/')
    url++;

  if ((p2 = strstr (url, "../")) != NULL)
    {
      while (*p2 == '.' || *p2 == '/')
	--p2;
      ++p2;

      p1 = url;
      begin = uc_utils_strdup_delim (p1, p2);

      p1 = p2 + 1;
      p2 = &url[strlen (url)];
      end = uc_utils_strdup_delim (p1, p2);

      p2 = begin;
      p1 = end;
      while (p2 && (p1 = strstr (p1, "./")) != NULL)
	{
	  p1 += 2;
	  if ((p2 = strrchr (begin, '/')) != NULL && *(p2 - 1) != '/')
	    {

	      if ((p2 - 2) && *(p2 - 2) == ':')
		p2 = NULL;
	      else
		*p2 = '\0';
	    }
	  else
	    p2 = NULL;
	  p3 = p1;
	}

      if (begin[strlen (begin)] == '/')
	begin[strlen (begin)] = '\0';
      if (*p3 == '/')
	*p3 = '\0';

      ret = g_strconcat (begin, "/", p3, NULL);

      g_free (begin), begin = NULL;
      g_free (end), end = NULL;
    }
  else
    ret = g_strdup (url);

  p1 = ret;
  while ((p1 = strstr ((gchar *) p1, "./")) != NULL && uc_url_is_relative (p1))
    {
      if (*(p1 - 1) == '.')
	--p1, inc = 3;
      else
	inc = 2;

      g_memmove (p1, p1 + inc, &ret[strlen (ret)] - (p1 + inc));
      ret[strlen (ret) - inc] = '\0';
    }

  return ret;
}


/**
 * uc_url_normalize:
 * @current_host: The current host.
 * @current_path: The current path.
 * @url: The URI.
 * 
 * Build a full URI from the given parameters.
 * 
 * Returns: A full and normalized URI. A new allocated string.
 */
gchar *
uc_url_normalize (const gchar * current_host,
		  const gchar * current_path, gchar * url)
{
  gchar *protocol = NULL;
  gchar *host = NULL;
  gchar *port = NULL;
  gchar *path = NULL;
  gchar *args = NULL;
  gchar *normalized_url = NULL;
  gchar *tmp = NULL;


  protocol = uc_url_get_protocol (url);

  if (strcmp (protocol, UC_PROTOCOL_FILE) == 0 ||
      strcmp (protocol, UC_PROTOCOL_MAILTO) == 0)
  {
    g_free (protocol), protocol = NULL;
    return g_strdup (url);
  }

  if (uc_url_parse (current_host, current_path, url,
                    &host, &port, &path, &args))
  {
    if (path[strlen (path) - 1] == '/')
      path[strlen (path) - 1] = '\0';

    if (strcmp (port, UC_URL_DEFAULT_SSL_PORT) == 0 ||
        strcmp (port, UC_URL_DEFAULT_PORT) == 0)
      tmp = g_strdup ("");
    else
      tmp = g_strconcat (":", port, NULL);

    normalized_url = 
      g_strconcat (protocol, "://", host, tmp, path, args, NULL);

    g_free (tmp), tmp = NULL;
  }
  else
    normalized_url = g_strdup (url);

  if (host || port || path || args)
  {
    g_free (host), host = NULL;
    g_free (port), port = NULL;
    g_free (path), path = NULL;
    g_free (args), args = NULL;
  }

  g_free (protocol), protocol = NULL;

  return normalized_url;
}


/**
 * uc_url_add_protocol:
 * @proto: The protocol to add.
 * @host: The host.
 * 
 * Add the given protocol to the given url.
 *
 * Returns: The host plus the given protocole. A new allocated string.
 */
gchar *
uc_url_add_protocol (const gchar * proto, const gchar * host)
{
  return (strstr (host, "://") != NULL) ?
    g_strdup (host) : g_strconcat (proto, "://", host, NULL);
}


/**
 * uc_url_get_hostname:
 * @current_host: The current host.
 * @url: The URI.
 * 
 * Get the hostname of a given url. If the url doesn't have host name, then
 * current_host is send.
 *
 * Returns: The hostname of a given URI. A new allocated string.
 */
gchar *
uc_url_get_hostname (const gchar * current_host, const gchar * url)
{
  const gchar *b = NULL;
  const gchar *e = NULL;
  gchar *ret = NULL;


  if (url == NULL || (b = strstr (url, "://")) == NULL)
  {
    if (current_host != NULL)
      ret = g_strdup (current_host);
    else if (uc_project_get_type () == UC_PROJECT_TYPE_LOCAL_FILE)
      ret = g_strdup ("");
    else
      ret = NULL;
  }
  else
  {
    b += 3;
    if ((e = strpbrk (b, ":/")) == NULL)
      ret =  g_strdup (b);
    else
      ret = uc_utils_strdup_delim (b, e);
  }

  return ret;
}


/**
 * uc_url_parse:
 * @current_host: The current host.
 * @current_path: The current_path.
 * @rurl: The URI to parse.
 * @host: String to put the host.
 * @port: String to put the port.
 * @path: String to put the path.
 * @args: String to put the arguments.
 *
 * Parse a given url.
 *
 * Returns: %TRUE if it is ok. parameters %host, %port, %path and %args are
 *          modified.
 */
gboolean
uc_url_parse (const gchar * current_host,
	      const gchar * current_path, gchar * rurl, gchar ** host,
	      gchar ** port, gchar ** path, gchar ** args)
{
  GURI *uri = NULL;
  const gchar *path_b = NULL;
  const gchar *path_e = NULL;
  const gchar *p = NULL;
  gchar *real_cpath = NULL;
  gchar *project_host = NULL;
  gchar *url = NULL;
  gsize url_len = 0;
  gboolean ret = TRUE;


  url = uc_utils_replace (rurl, "&amp;", "&");
  url_len = strlen (url);
  project_host = uc_project_get_current_host ();

  if (strcmp (url, "..") == 0 ||
      (url_len > 3 && strcmp (&url[url_len - 3], "/..") == 0))
    {
      gchar *tmp = NULL;

      tmp = g_strconcat (url, "/", NULL);
      g_free (url), url = NULL;
      url = tmp;
    }

  /* If this is jus a anchor (URL like "#section")  */
  if (*url == '#')
    {
      gchar *tmp = NULL;

      tmp = g_strconcat (current_path, url, NULL);
      g_free (url), url = NULL;
      url = tmp;
    }

  path_b = current_path;
  path_e = strrchr (current_path, '/');

  /* If this is a URL like "http://hostname", without path */
  if (((p = strstr (rurl, "://")) || (p = strstr (rurl, "://"))) &&
      !strchr ((p + 3), '/'))
    real_cpath = g_strdup ("/");
  /* If this is a URL with a path */
  else if (path_e != NULL && path_b != path_e)
    real_cpath = uc_utils_strdup_delim (path_b, path_e + 1);
  else
    real_cpath = g_strdup ("/");

  /* Get absolute path */
  {
    gchar *tmp = NULL;

    tmp = uc_url_get_absolute (real_cpath);
    g_free (real_cpath), real_cpath = NULL;
    real_cpath = tmp;
  }

  /* Because it seems that there is a bug in gnet with latin1 characters, 
   * so we can not use the "gnet_uri_escape" method to escape URLs) */
  {
    gchar *tmp = NULL;

    if ((tmp = uc_utils_url_reencode_escapes (url)) != url)
      {
	g_free (url), url = NULL;
	url = tmp;
      }
    uri = gnet_uri_new (url);
  }

  if (uri != NULL)
    {
      gnet_uri_set_fragment (uri, NULL);

      if (uri->scheme == NULL)
	gnet_uri_set_scheme (uri, (!strcmp (uc_project_get_current_port (),
					    UC_URL_DEFAULT_SSL_PORT)) ?
			     UC_PROTOCOL_HTTPS : UC_PROTOCOL_HTTP);

      if (uri->hostname == NULL)
	gnet_uri_set_hostname (uri, current_host);

      if (uri->port == 0)
	gnet_uri_set_port (uri,
			   (!g_ascii_strcasecmp
			    (uri->scheme,
			     UC_PROTOCOL_HTTP)) ? atoi (UC_URL_DEFAULT_PORT) :
			   atoi (UC_URL_DEFAULT_SSL_PORT));

      if (uri->path == NULL
	  && !g_ascii_strcasecmp (uri->hostname, project_host))
	gnet_uri_set_path (uri, real_cpath);
      else if (uri->path == NULL)
	gnet_uri_set_path (uri, "/");

      //gnet_uri_unescape (uri);
      //gnet_uri_escape (uri);

      *host = g_strdup (uri->hostname);
      *port = g_strdup_printf ("%u", uri->port);
      *path = g_strdup (uri->path);
      if (uc_project_get_no_urls_args ())
	*args = g_strdup ("");
      else
	*args =
	  (uri->query) ? g_strconcat ("?", uri->query, NULL) : g_strdup ("");

      if (g_ascii_strcasecmp (uri->scheme, UC_PROTOCOL_MAILTO) != 0)
	{
	  //gchar *tmp = NULL;

	  if ((!strcmp (*port, UC_URL_DEFAULT_PORT) ||
	       !strcmp (*port, UC_URL_DEFAULT_SSL_PORT))
	      && !g_ascii_strcasecmp (*host, project_host))
	    {
	      g_free (*port), *port = NULL;
	      *port = g_strdup (uc_project_get_current_port ());
	    }

	  if (**path != '/')
	    {
	      gchar *tmp = NULL;

	      tmp = g_strconcat (real_cpath, uri->path, NULL);
	      gnet_uri_set_path (uri, tmp);

	      g_free (*path), *path = NULL;
	      *path = tmp;
	    }

	  /* if there is relative path indications in the url
	   * we should apply a special treatment*/
	  /* FIXME
	     if ((tmp = strstr (*path, "./")) &&
	     !((gpointer) tmp != (gpointer) path
	     && (*(tmp - 1) != '.' || *(tmp - 1) != '/'))) */
	  //if ((tmp = strstr (*path, "./")) && !g_ascii_isalpha (*(tmp - 1)))
	  if (uc_url_is_relative (*path))
	    {
	      gchar *relative_url = NULL;
	      gchar *absolute_url = NULL;

	      gnet_uri_unescape (uri);

	      relative_url = gnet_uri_get_string (uri);
	      absolute_url = uc_url_get_absolute (relative_url);
	      g_free (relative_url), relative_url = NULL;

	      uc_url_parse (current_host, real_cpath, absolute_url,
			    &(*host), &(*port), &(*path), &(*args));
	      g_free (absolute_url), absolute_url = NULL;
	    }
	}
    }
  else
    {
      ret = FALSE;

      *host = g_strdup ("");
      *port = g_strdup ("");
      *args = g_strdup ("");

      /* gnet_uri_* do yet not support file protocol, so we must handle
       * it here */
      if (uc_utils_memcasecmp (url, "file://"))
	{
	  *path = g_strdup (strstr (url, "://") + 3);

	  ret = TRUE;
	}
    }

  if (*path == NULL)
    *path = g_strdup ("");

  g_free (url), url = NULL;
  g_free (real_cpath), real_cpath = NULL;
  gnet_uri_delete (uri), uri = NULL;

  return ret;
}


/**
 * uc_url_get_port:
 * @url: The URI.
 * 
 * Get the port of a given url.
 *
 * Returns: The port of the given URI. A new allocated string.
 */
gchar *
uc_url_get_port (const gchar * url)
{
  gchar *p1 = NULL;
  gchar *p2 = NULL;
  gchar *ret = NULL;


  if ((p1 = strstr (url, "://")) == NULL)
    ret = g_strdup (UC_URL_DEFAULT_PORT);
  else
  {
    p1 += 3;
    if ((p2 = strchr (p1, ':')) == NULL)
    {
      if (g_ascii_strncasecmp (url, UC_PROTOCOL_HTTPS,
                               strlen (UC_PROTOCOL_HTTPS)) == 0)
        ret = g_strdup (UC_URL_DEFAULT_SSL_PORT);
      else
        ret = g_strdup (UC_URL_DEFAULT_PORT);
    }
    else
    {
      ++p2;
  
      p1 = p2;
      while (isdigit ((gint) *p2))
        ++p2;
  
      ret = uc_utils_strdup_delim (p1, p2);
    }
  }
  
  return ret;
}


/**
 * uc_url_get_protocol:
 * @url: The URI.
 * 
 * Get the protocol of a given url.
 *
 * Returns: The protocole of the given URI. If not, a return a default
 *          protocol: #UC_PROTOCOL_HTTP or #UC_PROTOCOL_HTTPS. 
 *          A new allocated string.
 */
gchar *
uc_url_get_protocol (const gchar * url)
{
  gchar **split = NULL;
  gchar *ret = NULL;


  // Search only for ":", and not "://" because we want also pseudo protocols
  // like "mailto:"
  if (strchr (url, ':') == NULL)
  {
    if (strcmp (uc_project_get_current_port (), UC_URL_DEFAULT_SSL_PORT) == 0)
      ret = g_strdup (UC_PROTOCOL_HTTPS);
    else
      ret = g_strdup (UC_PROTOCOL_HTTP);
  }
  else
  {
    split = g_strsplit (url, ":", 0);
    ret = g_utf8_strdown (split[0], -1);
    g_strfreev (split), split = NULL;
  }

  return ret;
}


/**
 * uc_url_extract_url_from_local_path:
 * @path: The path from which to extract URL.
 * 
 * A little tricky: extract a URL from a path. The URL must be at the end of
 * the given path.
 *
 * Returns: The URL in a newly allocated string or %NULL.
 */
gchar *
uc_url_extract_url_from_local_path (const gchar * path)
{
  const gchar *begin = NULL;


  if ((begin = strstr (path, UC_PROTOCOL_HTTPS "://")) == NULL &&
      (begin = strstr (path, UC_PROTOCOL_HTTP "://")) == NULL &&
      (begin = strstr (path, UC_PROTOCOL_FTP "://")) == NULL)
    return NULL;

  return uc_utils_strdup_delim (begin, &path[strlen (path)]);
}


/**
 * uc_url_is_valid:
 * @url: The URI.
 * 
 * Quikly (loosly) check the validity of a given url.
 *
 * Returns: %TRUE if the given URI is valid.
 */
gboolean
uc_url_is_valid (const gchar * url)
{
  GURI *uri = NULL;
  gchar *ip = NULL;
  gboolean ret = FALSE;


  uri = gnet_uri_new (url);

  if (uri->scheme != NULL &&
      g_ascii_strcasecmp (uri->scheme, UC_PROTOCOL_FILE) == 0)
    ret = TRUE;
  else if (uri->hostname == NULL)
    ret = FALSE;
  else if (g_ascii_strcasecmp (uri->scheme, UC_PROTOCOL_HTTP) != 0 &&
           g_ascii_strcasecmp (uri->scheme, UC_PROTOCOL_HTTPS) != 0 &&
           g_ascii_strcasecmp (uri->scheme, UC_PROTOCOL_FTP) != 0)
    ret = FALSE;
  else if ((ret = ((ip = uc_utils_get_ip (uri->hostname)) != NULL)))
    g_free (ip), ip = NULL;

  gnet_uri_delete (uri);

  return ret;
}

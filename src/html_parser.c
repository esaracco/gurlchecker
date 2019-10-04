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
#include "application.h"
#include "check.h"
#include "project.h"
#include "html_parser.h"

#define UC_HTML_PARSER_WHILE_CONDITION \
        ( \
                  !uc_check_cancel_get_value () && \
                  !uc_check_ignore_item_get_value () \
        )

static gchar *href_base = NULL;

static gboolean uc_html_parser_authorized_tag_name (const gchar * name);
static gchar *uc_html_parser_add_base (const gchar * base, const gchar * url);
static GList *uc_html_parser_parse (GList * tags, htmlNodePtr cur);
static UCLinkType uc_html_parser_get_tag_type (htmlNodePtr cur);
static gchar *uc_html_parser_get_tag_label (htmlNodePtr cur);
static gchar *uc_html_parser_get_tag_value (htmlNodePtr cur, UCLinkType type);
static GList *uc_html_parser_parse (GList * tags, htmlNodePtr cur);
static gchar *uc_html_parser_clean_string (gchar * str);

/**
 * uc_html_parser_authorized_tag_name:
 * @name: Tag to check.
 * 
 * Check if a given tag must be proceed or not.
 * 
 * Returns: TRUE if the given name is usefull for us.
 */
static gboolean
uc_html_parser_authorized_tag_name (const gchar * name)
{
  return (name != NULL &&
	  (!g_ascii_strcasecmp (name, "IMG") ||
	   !g_ascii_strcasecmp (name, "A") ||
	   !g_ascii_strcasecmp (name, "AREA") ||
	   !g_ascii_strcasecmp (name, "BASE") ||
	   !g_ascii_strcasecmp (name, "META") ||
	   !g_ascii_strcasecmp (name, "LINK") ||
	   !g_ascii_strcasecmp (name, "FRAME") ||
	   !g_ascii_strcasecmp (name, "IFRAME")));
}

/**
 * uc_html_parser_get_tags:
 * @buffer_orig: Buffer containing HTML code to proceed.
 * 
 * Parse the given buffer to extract HTML tags.
 *
 * See: uc_css_parser_get_tags ()
 * 
 * Returns: A #GList of #UCHTMLTag.
 */
GList *
uc_html_parser_get_tags (gchar * buffer_orig)
{
  GList *tags = NULL;
  gchar *buffer = NULL;
  htmlParserCtxtPtr ctxt = NULL;

  uc_application_display_state_message (UC_CHECK_MESSAGE_LABEL_THIRD,
					_("Parsing HTML code..."));
  UC_UPDATE_UI;

  if (!(buffer = uc_utils_replace (buffer_orig, "&", "&amp;")))
    return NULL;

  if ((ctxt = htmlCreateMemoryParserCtxt (buffer, strlen (buffer))))
    {
      htmlCtxtUseOptions (ctxt, HTML_PARSE_NOWARNING | HTML_PARSE_NOERROR);
      htmlParseDocument (ctxt);

      tags = uc_html_parser_parse (tags, ctxt->myDoc->children);

      g_free (href_base), href_base = NULL;
      xmlFreeDoc (ctxt->myDoc), ctxt->myDoc = NULL;
      htmlFreeParserCtxt (ctxt), ctxt = NULL;
    }
  else
    g_warning ("Problem with htmlCreateMemoryParserCtxt ()");

  g_free (buffer), buffer = NULL;

  return tags;
}

/**
 * uc_css_parser_get_tags:
 * @buffer_orig: a buffer with CSS file content.
 * 
 * Parse the buffer for some CSS instructions. For the moment, only
 * "import" and "url" instructions are interesting for us.
 * 
 * See: uc_html_parser_get_tags ()
 * 
 * Returns: A #GList of CSS properties (#UCHTMLTag).
 */
GList *
uc_css_parser_get_tags (gchar * buffer_orig)
{
  GList *tags = NULL;
  gchar *ext = NULL;
  gchar *tmp = NULL;
  gchar *buffer = buffer_orig;

  uc_application_display_state_message (UC_CHECK_MESSAGE_LABEL_THIRD,
					_("Parsing CSS code..."));
  UC_UPDATE_UI;

  while (*buffer != '\0' && (buffer = strpbrk (buffer, "@uU")) != NULL)
    {
      gchar *url = NULL;
      const gchar *b = NULL;
      const gchar *e = NULL;
      UCLinkType link_type = LINK_TYPE_NONE;

      /* "import" tag */
      if (uc_utils_memcasecmp (buffer, "@import"))
	{
	  b = (buffer += 8);

	  if ((b = uc_utils_strpbrk_or_eos (b, "('\"")) != NULL);
	  {
	    do
	      b++;
	    while (*b == ' ' || *b == '\'' || *b == '"');

	    if ((e = uc_utils_strpbrk_or_eos (b, "'\";) ")) != NULL)
	      {
		tmp = uc_utils_strdup_delim (b, e);
		url = uc_utils_clean_tag_link_value (tmp);
		g_free (tmp), tmp = NULL;

		/* file must have a extension */
		if ((ext = strrchr (url, '.')) != NULL)
		  link_type = LINK_TYPE_CSS;
	      }
	  }
	}
      /* "url" tag */
      else if (uc_utils_memcasecmp (buffer, "url"))
	{
	  b = (buffer += 4);

	  /* we do not want "url*s*" tag */
	  if ((!g_ascii_isalpha (*(b - 5))) &&
	      (!g_ascii_isalpha (*(b - 1))) && (e = strchr (b, ')')) != NULL)
	    {
	      tmp = uc_utils_strdup_delim (b, e);
	      url = uc_utils_clean_tag_link_value (tmp);
	      g_free (tmp), tmp = NULL;

	      /* file must have a extension */
	      if ((ext = strrchr (url, '.')) != NULL)
		link_type = (!strncasecmp (".css", ext, 3)) ?
		  LINK_TYPE_CSS :
		  (!strncasecmp (url, UC_PROTOCOL_FILE,
				 strlen (UC_PROTOCOL_FILE) - 1)) ?
		  LINK_TYPE_FILE_IMAGE : LINK_TYPE_IMAGE;
	    }
	}

      if (link_type != LINK_TYPE_NONE)
	{
	  UCHTMLTag *tag = NULL;

	  g_strstrip (url);

	  tag = uc_html_parser_node_new ();
	  tag->type = link_type;
	  tag->label = g_strdup ("");
	  tag->value = g_strdup (url);

	  tags = g_list_append (tags, tag);
	}

      g_free (url), url = NULL;

      if (*buffer != '\0')
	buffer++;
    }

  return tags;
}

/**
 * uc_html_parser_get_tag_type:
 * @cur: XML cursor pointer.
 *
 * Proceed a HTML tag to see its type.
 * 
 * Returns: #UCLinkType of the HTML tag.
 */
static UCLinkType
uc_html_parser_get_tag_type (htmlNodePtr cur)
{
  xmlAttrPtr attr = NULL;
  UCLinkType ret = LINK_TYPE_NONE;

  /* BASE */
  if (!g_ascii_strcasecmp ((gchar *) cur->name, "BASE"))
    ret = LINK_TYPE_BASE_HREF;
  /* META */
  else if (!g_ascii_strcasecmp ((gchar *) cur->name, "META"))
    ret = LINK_TYPE_META;
  /* (IMG) -> "SRC" */
  else if (!g_ascii_strcasecmp ((gchar *) cur->name, "IMG"))
    {
      attr = cur->properties;
      while (attr != NULL && ret == LINK_TYPE_NONE)
	{
	  if (attr->name != NULL &&
	      !g_ascii_strcasecmp ((gchar *) attr->name, "SRC") &&
	      attr->children != NULL && attr->children->content != NULL)
	    {
	      ret = (uc_utils_memcasecmp ((gchar *) attr->children->content,
					  "FILE:")) ? LINK_TYPE_FILE_IMAGE :
		LINK_TYPE_IMAGE;
	    }
	  attr = attr->next;
	}
    }
  /* (LINK) -> (REL) -> "stylesheet" */
  else if (!g_ascii_strcasecmp ((gchar *) cur->name, "LINK"))
    {
      attr = cur->properties;
      while (attr != NULL && ret == LINK_TYPE_NONE)
	{
	  if (attr->name != NULL &&
	      !g_ascii_strcasecmp ((gchar *) attr->name, "REL") &&
	      attr->children != NULL && attr->children->content != NULL &&
	      uc_utils_memcasecmp ((gchar *) attr->children->content,
				   "stylesheet"))
	    ret = LINK_TYPE_CSS;

	  attr = attr->next;
	}
    }
  /* (A or AREA) -> (HREF) -> "MAILTO:" "FILE:" */
  else if (!g_ascii_strcasecmp ((gchar *) cur->name, "A") ||
	   !g_ascii_strcasecmp ((gchar *) cur->name, "AREA"))
    {
      attr = cur->properties;
      while (attr != NULL && ret == LINK_TYPE_NONE)
	{
	  if (attr->name != NULL &&
	      !g_ascii_strcasecmp ((gchar *) attr->name, "HREF") &&
	      attr->children != NULL && attr->children->content != NULL)
	    {
	      if (uc_utils_memcasecmp ((gchar *) attr->children->content,
				       "MAILTO:"))
		ret = LINK_TYPE_EMAIL;
	      else if (uc_utils_memcasecmp ((gchar *) attr->children->content,
					    "FILE:"))
		ret = LINK_TYPE_FILE_HREF;
	      else
		ret = LINK_TYPE_HREF;
	    }

	  attr = attr->next;
	}
    }
  /* FRAME or IFRAME */
  else if (!g_ascii_strcasecmp ((gchar *) cur->name, "FRAME") ||
	   !g_ascii_strcasecmp ((gchar *) cur->name, "IFRAME"))
    ret = LINK_TYPE_FRAME;

  return ret;
}

/**
 * uc_html_parser_get_tag_imbricated_label:
 * @cur: XML cursor pointer.
 * @ret: A pointer that will point on the tag label. 
 *
 * This function is called when tag label contain HTML tags.
 * It fill the @ret argument with the label of the given tag, without HTML
 * tags. @ret will point on a new allocated string.
 */
static void
uc_html_parser_get_tag_imbricated_label (htmlNodePtr cur, gchar ** ret)
{
  while (cur != NULL)
    {
      if (cur->name != NULL &&
	  !g_ascii_strcasecmp ((gchar *) cur->name, "text")
	  && cur->content != NULL)
	{
	  if (*ret == NULL)
	    *ret = g_strdup ((gchar *) cur->content);
	  else
	    {
	      gchar *sav = NULL;

	      sav = g_strdup_printf ("%s %s", *ret, cur->content);
	      g_free (*ret), *ret = NULL;
	      *ret = sav;
	    }
	}

      if (cur->children != NULL)
	uc_html_parser_get_tag_imbricated_label (cur->children, &(*ret));

      cur = cur->next;
    }
}

/**
 * uc_html_parser_get_tag_label:
 * @cur: XML cursor pointer.
 *
 * Proceed a given HTML tag to extract its label. If label contain
 * other HTML tags, those are deleted.
 * 
 * See: uc_html_parser_get_tag_imbricated_label(), 
 *      uc_html_parser_get_tag_value()
 *      
 * Returns: Label of the tag.
 */
static gchar *
uc_html_parser_get_tag_label (htmlNodePtr cur)
{
  gchar *ret = NULL;
  xmlAttrPtr attr = NULL;

  if (cur->children != NULL)
    {
      gchar *tmp = NULL;

      uc_html_parser_get_tag_imbricated_label (cur->children, &tmp);

      if (tmp != NULL)
	{
	  ret = uc_utils_to_utf8 (tmp);
	  g_free (tmp), tmp = NULL;
	}
    }
  else
    {
      attr = cur->properties;
      while (attr != NULL && ret == NULL)
	{
	  if (attr->name != NULL && attr->children != NULL
	      && (!g_ascii_strcasecmp ((gchar *) attr->name, "TITLE")
		  || !g_ascii_strcasecmp ((gchar *) attr->name, "ALT")
		  || !g_ascii_strcasecmp ((gchar *) attr->name, "NAME")
		  || !g_ascii_strcasecmp ((gchar *) attr->name,
					  "HTTP-EQUIV")))
	    ret = uc_utils_to_utf8 ((gchar *) attr->children->content);

	  attr = attr->next;
	}
    }

  if (ret == NULL)
    ret = g_strdup ("");
  else
    {
      gchar *tmp = NULL;

      tmp = uc_html_parser_clean_string (ret);
      g_free (ret), ret = NULL;
      ret = tmp;
    }

  return ret;
}

/**
 * uc_html_parser_get_tag_value:
 * @cur: XML cursor pointer.
 * @type: Tag type.
 *
 * Proceed a given tag to extract its value.
 * 
 * See: uc_html_parser_get_tag_label()
 * 
 * Returns: The value of the tag.
 */
static gchar *
uc_html_parser_get_tag_value (htmlNodePtr cur, UCLinkType type)
{
  xmlAttrPtr attr = NULL;
  gchar *p = NULL;
  gchar *ret = NULL;

  switch (type)
    {
    case LINK_TYPE_IMAGE:
    case LINK_TYPE_FILE_IMAGE:
    case LINK_TYPE_FRAME:
    case LINK_TYPE_EMAIL:
    case LINK_TYPE_BASE_HREF:
    case LINK_TYPE_FILE_HREF:
    case LINK_TYPE_HREF:
    case LINK_TYPE_CSS:
      attr = cur->properties;
      while (attr != NULL && ret == NULL)
	{
	  if (attr->name != NULL &&
	      !g_ascii_strcasecmp ((gchar *) attr->name, "RECTANGLE"))
	    {
	      if ((p = strrchr ((char *) attr->children->content, ')')))
		ret = uc_utils_to_utf8 (++p), g_strstrip (ret);
	      else
		ret = uc_utils_to_utf8 ((gchar *) attr->children->content);
	    }
	  else if (attr->name != NULL &&
		   (!g_ascii_strcasecmp ((gchar *) attr->name, "HREF") ||
		    !g_ascii_strcasecmp ((gchar *) attr->name, "SRC"))
		   && attr->children != NULL
		   && attr->children->content != NULL)
	    {
	      ret = uc_utils_to_utf8 ((gchar *) attr->children->content);
	    }

	  attr = attr->next;
	}
      break;

    case LINK_TYPE_META:
      attr = cur->properties;
      while (attr != NULL && ret == NULL)
	{
	  if (attr->name != NULL && !g_ascii_strcasecmp ((gchar *) attr->name,
							 "CONTENT")
	      && attr->children != NULL && attr->children->content != NULL)
	    ret = uc_utils_to_utf8 ((gchar *) attr->children->content);

	  attr = attr->next;
	}
      break;

    default:
      ;
    }

  if (ret == NULL)
    ret = g_strdup ("");
  else
    {
      gchar *tmp = NULL;

      tmp = uc_html_parser_clean_string (ret);
      g_free (ret), ret = NULL;
      ret = tmp;
    }

  return ret;
}

static gchar *
uc_html_parser_clean_string (gchar * str)
{
  GString *clean = NULL;
  gchar *ret = NULL;

  clean = g_string_new ("");

  do
  {
    if (!strchr ("\n\r\t", *str))
      g_string_append_c (clean, *str);
  }
  while (*(++str));

  ret = clean->str;
  g_strstrip (ret);

  g_string_free (clean, FALSE), clean = NULL;

  return ret;
}

/**
 * uc_html_parser_parse:
 * @tags: #GList of #UCHTMLTag items.
 * @cur: XML cursor pointer.
 * 
 * Parse a given XML structure to extract HTML tags.
 * 
 * Returns: A #GList of #UCHTMLTag.
 */
static GList *
uc_html_parser_parse (GList * tags, htmlNodePtr cur)
{
  gchar *tmp = NULL;

  while (cur != NULL && UC_HTML_PARSER_WHILE_CONDITION)
    {
      if (cur->type == XML_ELEMENT_NODE && cur->name != NULL &&
	  uc_html_parser_authorized_tag_name ((gchar *) cur->name))
	{
	  UCHTMLTag *tag = NULL;

	  tag = uc_html_parser_node_new ();
	  tag->type = uc_html_parser_get_tag_type (cur);

	  /* label */
	  tag->label = uc_html_parser_get_tag_label (cur);

	  /* value */
	  tag->value = uc_html_parser_get_tag_value (cur, tag->type);

	  /* type correction: if it is a mailto: on a img rectangle attribut */
	  if (uc_utils_memcasecmp (tmp, "MAILTO:"))
	    tag->type = LINK_TYPE_EMAIL;

	  switch (tag->type)
	    {
	    case LINK_TYPE_META:
	    case LINK_TYPE_EMAIL:
	    case LINK_TYPE_FILE_HREF:
	      break;

	    case LINK_TYPE_BASE_HREF:
	      if (href_base == NULL)
		{
		  href_base = g_strdup (tag->value);
		  if (strlen (href_base) >= 2
		      && (href_base)[strlen (href_base) - 1] == '/'
		      && (href_base)[strlen (href_base) - 2] == '/')
		    (href_base)[strlen (href_base) - 1] = '\0';
		}
	      break;

	    default:
	      tmp = uc_html_parser_add_base (href_base, tag->value);
	      g_free (tag->value), tag->value = NULL;
	      tag->value = uc_utils_to_utf8 (tmp);
	      g_free (tmp), tmp = NULL;
	    }

	  if (tag->type != LINK_TYPE_BASE_HREF)
	    tags = g_list_append (tags, tag);
	  else
	    {
	      g_free (tag->label), tag->label = NULL;
	      g_free (tag->value), tag->value = NULL;

	      g_free (tag), tag = NULL;
	    }
	}

      if (cur->children)
	tags = uc_html_parser_parse (tags, cur->children);

      cur = cur->next;
    }

  return tags;
}

/**
 * uc_html_parser_add_base:
 * @base: HTML base tag to add.
 * @url: URL for base tag concatenation.
 * 
 * Add the base tag value to the link if necessary.
 *
 * Returns: A new allocated string with new URL.
 */
static gchar *
uc_html_parser_add_base (const gchar * base, const gchar * url)
{
  gchar *path = NULL;
  gchar *file = NULL;
  gchar *new = NULL;

  path = g_strdup (base);

  if (path == NULL || url == NULL || strstr (url, "://") != NULL ||
      (file = strrchr (path, '/')) == NULL || url[0] == '/')
    {
      g_free (path), path = NULL;
      return g_strdup (url);
    }

  if (*(file - 1) == '/')
    file = NULL;
  else
    *(file++) = '\0';

  if (path[strlen (path) - 1] == '/')
    path[strlen (path) - 1] = '\0';

  new = (!strchr ("?#", url[0])) ?
    g_strconcat (path, ((url[0] != '/') ? "/" : ""), url, NULL) :
    g_strconcat (path, "/", file, url, NULL);

  g_free (path), path = NULL;

  return new;
}

/**
 * uc_html_parser_node_new:
 * 
 * Create a new tag list node.
 *
 * Returns: A new allocated #UCHTMLTag node.
 */
UCHTMLTag *
uc_html_parser_node_new (void)
{
  UCHTMLTag *item = NULL;

  item = g_new0 (UCHTMLTag, 1);
  item->depth = 0;
  item->label = NULL;
  item->value = NULL;
  item->type = LINK_TYPE_NONE;

  return item;
}

void
uc_html_parser_node_free (UCHTMLTag **tag)
{
  g_free ((*tag)->label), (*tag)->label = NULL;
  g_free ((*tag)->value), (*tag)->value = NULL;

  g_free ((*tag)), (*tag) = NULL;
}

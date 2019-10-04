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
#include "url.h"
#include "html_parser.h"
#include "project.h"

#include "lists.h"


/*
 * struct used for the uc_lists_already_checked_links_list
 * list
 */
typedef struct
{
  gchar *url;
  UCLinkProperties *parent;
}
UCAlreadyChecked;

/* list of already checked urls */
static GList *uc_lists_already_checked_links_list = NULL;

/* list of already checked urls */
static GList *uc_lists_checked_links_list = NULL;

/* list of links to reinject in the node's childs before
 * a update */
static GList *uc_lists_refresh_preserved_links_list = NULL;

static void uc_lists_checked_links_free_real (GList **list);
static UCAlreadyChecked *uc_lists_already_checked_links_node_new (
  const gchar *url, gpointer ac_p);
static UCLinkProperties *uc_lists_checked_links_lookup_by_uid_real (
  GList *list, const guint uid);
static void uc_lists_html_tags_free (GList ** list);


void
uc_lists_refresh_preserved_links_reset (void)
{
  g_list_free (uc_lists_refresh_preserved_links_list),
    uc_lists_refresh_preserved_links_list = NULL;
}

void
uc_lists_refresh_preserved_links_append (UCLinkProperties * prop)
{
  uc_lists_refresh_preserved_links_list =
    g_list_append (uc_lists_refresh_preserved_links_list, prop);
}

GList *
uc_lists_refresh_preserved_links_restore (GList * src)
{
  return g_list_concat (src,
			g_list_copy (uc_lists_refresh_preserved_links_list));
}

/*
 * free a html tags list node
 */
void
uc_lists_html_tags_node_free (GList **list, UCHTMLTag **tag)
{
  *list = g_list_remove (*list, (gpointer) *tag);
  uc_html_parser_node_free (tag);
}


/*
 * free a html tags list
 */
static void
uc_lists_html_tags_free (GList ** list)
{
  GList *item = NULL;


  g_return_if_fail (*list != NULL);

  item = g_list_first (*list);
  while (item != NULL)
  {
    UCHTMLTag *tag = (UCHTMLTag *) item->data;
    item = g_list_next (item);
    uc_lists_html_tags_node_free (list, &tag);
  }

  g_list_free (*list), *list = NULL;
}


/*
 * append a item to the uc_lists_already_checked_links_list
 * list
 */
gboolean
uc_lists_already_checked_links_append (const gchar * url, gpointer ac_p)
{
  UCAlreadyChecked *node = NULL;

  /* do not insert it twice */
  if (uc_lists_already_checked_links_lookup (url, TRUE) != NULL)
    return FALSE;

  node = uc_lists_already_checked_links_node_new (url, ac_p);

  uc_lists_already_checked_links_list =
    g_list_append (uc_lists_already_checked_links_list, node);

  return TRUE;
}

/*
 * return TRUE if the list of checked
 * links is empty
 */
guint
uc_lists_checked_links_is_empty (void)
{
  return (g_list_length (uc_lists_checked_links_list) == 0);
}

/*
 * public wrapper to freeing the uc_lists_checked_links_list
 * list
 */
void
uc_lists_checked_links_free (void)
{
  uc_lists_checked_links_free_real (&uc_lists_checked_links_list);
}


/*
 * free a UCLinkProperties node
 */
void
uc_lists_checked_links_node_free (GList ** list, UCLinkProperties ** prop)
{
  if (*prop == NULL)
    return;

  g_free ((*prop)->treeview_path), (*prop)->treeview_path = NULL;
  if ((*prop)->link_icon != NULL)
    g_object_unref ((*prop)->link_icon), (*prop)->link_icon = NULL;
  g_free ((*prop)->user_action), (*prop)->user_action = NULL;
  g_free ((*prop)->link_value), (*prop)->link_value = NULL;
  g_free ((*prop)->label), (*prop)->label = NULL;
  g_free ((*prop)->url), (*prop)->url = NULL;
  g_free ((*prop)->normalized_url), (*prop)->normalized_url = NULL;
  g_free ((*prop)->protocol), (*prop)->protocol = NULL;
  g_free ((*prop)->ip_addr), (*prop)->ip_addr = NULL;
  g_free ((*prop)->h_name), (*prop)->h_name = NULL;
  g_free ((*prop)->port), (*prop)->port = NULL;
  g_free ((*prop)->path), (*prop)->path = NULL;
  g_free ((*prop)->domain), (*prop)->domain = NULL;
  g_free ((*prop)->args), (*prop)->args = NULL;
  g_free ((*prop)->current_path), (*prop)->current_path = NULL;
  g_free ((*prop)->virname), (*prop)->virname = NULL;

  if ((*prop)->header != NULL)
    g_hash_table_destroy ((*prop)->header), (*prop)->header = NULL;

  if ((*prop)->similar_links_parents != NULL)
    {
      g_list_foreach ((*prop)->similar_links_parents, (GFunc) g_free, NULL);
      g_list_free ((*prop)->similar_links_parents),
	(*prop)->similar_links_parents = NULL;
    }

  if ((*prop)->bad_extensions != NULL)
    {
      g_list_foreach ((*prop)->bad_extensions, (GFunc) g_free, NULL);
      g_list_free ((*prop)->bad_extensions), (*prop)->bad_extensions = NULL;
    }

  /* (*prop)->all_links (ptr on (*prop)->tags items) */
  g_list_free ((*prop)->all_links), (*prop)->all_links = NULL;
  /* (*prop)->emails (ptr on (*prop)->tags items) */
  g_list_free ((*prop)->emails), (*prop)->emails = NULL;
  /* (*prop)->metas ((ptr on (*prop)->tags items) */
  g_list_free ((*prop)->metas), (*prop)->metas = NULL;

  if ((*prop)->tags != NULL)
    uc_lists_html_tags_free (&(*prop)->tags);

  if ((*prop)->childs != NULL)
    uc_lists_checked_links_free_real (&(*prop)->childs);

  if (list != NULL)
    *list = g_list_remove (*list, (gpointer) *prop);

  g_free (*prop), *prop = NULL;
}


/*
 * free all the allocated objects of the
 * url properties list
 */
static void
uc_lists_checked_links_free_real (GList **list)
{
  GList *item = NULL;


  if (*list == NULL)
    return;

  item = g_list_first (*list);
  while (item != NULL)
    {
      UCLinkProperties *prop = (UCLinkProperties *) item->data;
      item = g_list_next (item);
      uc_lists_checked_links_node_free (list, &prop);
    }

  g_list_free (*list), *list = NULL;
}


/*
 * TRUE if the given url already in
 * the checked urls list
 */
gboolean
uc_lists_checked_links_exist (const gchar * url)
{
  return (uc_lists_checked_links_lookup (url) != NULL);
}

/*
 * return a pointer on the found item
 */
gpointer
uc_lists_checked_links_lookup (const gchar * url)
{
  return uc_lists_already_checked_links_lookup (url, FALSE);
}

/*
 * TRUE if the given url already in
 * the checked urls list
 */
gboolean
uc_lists_already_checked_links_exist (gchar * url)
{
  return (uc_lists_already_checked_links_lookup (url, TRUE) != NULL);
}

/*
 * return a pointer on the found item
 */
gpointer
uc_lists_already_checked_links_lookup (const gchar * url,
				       const gboolean return_node)
{
  GList *item = NULL;
  gchar *new_url = NULL;
  gpointer ret = NULL;

  new_url = uc_url_add_slash (url);

  item = g_list_first (uc_lists_already_checked_links_list);
  while (item != NULL && ret == NULL)
    {
      UCAlreadyChecked *ac = (UCAlreadyChecked *) item->data;

      if (!g_ascii_strcasecmp ((gchar *) ac->url, new_url))
	ret = (return_node) ? (gpointer) ac : (gpointer) ac->parent;

      item = g_list_next (item);
    }

  g_free (new_url), new_url = NULL;

  return ret;
}

GList *
uc_lists_links_first_minus_second (GList * first, GList * second)
{
  GList *item = NULL;
  GList *item1 = NULL;
  GList *res = NULL;

  item = g_list_first (first);
  while (item != NULL)
    {
      UCLinkProperties *prop = (UCLinkProperties *) item->data;
      gboolean found = FALSE;

      item1 = g_list_first (second);
      while (item1 != NULL && !found)
	{
	  UCLinkProperties *prop1 = (UCLinkProperties *) item1->data;

	  item1 = g_list_next (item1);

	  if (!g_ascii_strcasecmp (prop->url, prop1->url))
	    found = TRUE;
	}

      if (!found)
	res = g_list_append (res, prop);

      item = g_list_next (item);
    }

  return res;
}

void
uc_lists_already_checked_links_remove (const gchar * url)
{
  UCAlreadyChecked *node = NULL;

  node =
    (UCAlreadyChecked *) uc_lists_already_checked_links_lookup (url, TRUE);

  if (node != NULL)
    {
      g_free (node->url), node->url = NULL;

      uc_lists_already_checked_links_list =
	g_list_remove (uc_lists_already_checked_links_list, (gpointer) node);

      g_free (node), node = NULL;
    }
}

void
uc_lists_already_checked_links_remove_branch_items (UCLinkProperties * prop)
{
  GList *item = NULL;

  uc_lists_already_checked_links_remove (prop->normalized_url);

  item = g_list_first (prop->childs);
  while (item != NULL)
    {
      UCLinkProperties *lp = (UCLinkProperties *) item->data;

      item = g_list_next (item);

      uc_lists_already_checked_links_remove (lp->normalized_url);

      if (lp->childs != NULL)
	uc_lists_already_checked_links_remove_branch_items (lp);
    }
}

void
uc_lists_already_checked_links_remove_items (UCLinkProperties * prop)
{
  GList *item = NULL;

  uc_lists_already_checked_links_remove (prop->normalized_url);

  item = g_list_first (prop->childs);
  while (item != NULL)
    {
      UCLinkProperties *lp = (UCLinkProperties *) item->data;

      item = g_list_next (item);

      uc_lists_already_checked_links_remove (lp->normalized_url);
    }
}

void
uc_lists_similar_links_remove_item (GList * list, const gchar * url)
{
  GList *item = NULL;

  item = g_list_first (list);
  while (item != NULL)
    {
      UCLinkProperties *prop = (UCLinkProperties *) item->data;
      GList *tmp = NULL;

      item = g_list_next (item);

      tmp = g_list_first (prop->similar_links_parents);
      while (tmp != NULL)
	{
	  gchar *sl = (gchar *) tmp->data;

	  tmp = g_list_next (tmp);

	  if (!g_ascii_strcasecmp (sl, url))
	    {
	      prop->similar_links_parents =
		g_list_remove (prop->similar_links_parents, (gpointer) sl);

	      g_free (sl), sl = NULL;
	    }

	  if (prop->childs)
	    uc_lists_similar_links_remove_item (prop->childs, url);
	}
    }
}

void
uc_lists_similar_links_remove_items (UCLinkProperties * prop)
{
  GList *item = NULL;

  item = g_list_first (prop->childs);
  while (item != NULL)
    {
      UCLinkProperties *lp = (UCLinkProperties *) item->data;
      GList *i = NULL;

      item = g_list_next (item);

      i = g_list_first (lp->similar_links_parents);
      while (i != NULL)
	{
	  gchar *tmp = (gchar *) i->data;

	  i = g_list_next (i);

	  if (!g_ascii_strcasecmp (tmp, lp->url))
	    {
	      lp->similar_links_parents =
		g_list_remove (lp->similar_links_parents, (gpointer) tmp);

	      g_free (tmp), tmp = NULL;
	    }
	}
    }
}

/*
 * Append a url in the similar links list
 */
void
uc_lists_similar_links_append (gpointer prop_p, const gchar *url)
{
  UCLinkProperties *prop = (UCLinkProperties *) prop_p;


  g_return_if_fail (prop != NULL);

  prop->similar_links_parents =
    g_list_append (prop->similar_links_parents, g_strdup (url));
}


/*
 * free all allocated url strings
 */
void
uc_lists_already_checked_free (void)
{
  GList *item = NULL;


  if (uc_lists_already_checked_links_list == NULL)
    return;

  item = g_list_first (uc_lists_already_checked_links_list);
  while (item != NULL)
    {
      UCAlreadyChecked *ac = (UCAlreadyChecked *) item->data;
      item = g_list_next (item);
      g_free (ac->url), ac->url = NULL;
      g_free (ac), ac = NULL;
    }

  g_list_free (uc_lists_already_checked_links_list),
    uc_lists_already_checked_links_list = NULL;
}


/*
 * allocate and fill a uc_lists_already_checked_links_list node
 */
static UCAlreadyChecked *
uc_lists_already_checked_links_node_new (const gchar * url, gpointer ac_p)
{
  UCAlreadyChecked *ac = NULL;

  ac = g_new0 (UCAlreadyChecked, 1);

  ac->url = uc_url_add_slash (url);
  ac->parent = ac_p;

  return ac;
}

/*
 * return properties of the given checked url
 * uid
 */
UCLinkProperties *
uc_lists_checked_links_lookup_by_uid (const guint32 uid)
{
  return uc_lists_checked_links_lookup_by_uid_real
    (uc_lists_checked_links_list, uid);
}

/*
 * set the already checked urls list
 */
void
uc_lists_checked_links_set (GList * list)
{
  uc_lists_checked_links_list = list;
}

void
uc_lists_checked_links_dump (GList *list, FILE **fd,
                             const UCExportFormat ftype)
{
  GList *item = NULL;
  UCLinkProperties *lp = NULL;
  UCHTMLTag *lps = NULL;
  gboolean labels = uc_project_get_export_labels ();
  gboolean numbers = uc_project_get_export_numbering ();
  gboolean external = uc_project_get_export_external ();
  gboolean ip = uc_project_get_export_ip ();


  g_assert (list != NULL);

  item = g_list_first (list);
  while (item != NULL)
  {
    if (ftype != UC_EXPORT_LIST_SIMPLE)
    {
      lp = (UCLinkProperties *) item->data;

      if (external && strcmp (lp->h_name, uc_project_get_current_host ()) == 0)
        ;
      else
      {
        /* Treepath dump */
        if (numbers)
        {
          fwrite (lp->treeview_path, sizeof (gchar),
                  strlen (lp->treeview_path), *fd);
          fwrite ("\t", 1, 1, *fd);
        }
    
        /* IP dump */
        if (ip)
        {
          if (lp->ip_addr)
            fwrite (lp->ip_addr, sizeof (gchar), strlen (lp->ip_addr), *fd);
          fwrite ("\t", 1, 1, *fd);
        }

        /* URL dump */
        fwrite (lp->normalized_url, sizeof (gchar),
                strlen (lp->normalized_url), *fd);
  
        /* Link's label dump */
        if (labels)
        {
          fwrite ("\t", 1, 1, *fd);
          fwrite (lp->label, sizeof (gchar), strlen (lp->label), *fd);
        }
  
        /* EOL */
        fwrite ("\n", 1, 1, *fd);
      } 
 
      if (lp->childs != NULL)
        uc_lists_checked_links_dump (lp->childs, fd, ftype);
    }
    else
    {
      lps = (UCHTMLTag *) item->data;

      /* URL dump */
      fwrite (lps->value, sizeof (gchar), strlen (lps->value), *fd);

      /* Link's label dump */
      if (labels)
      {
        fwrite ("\t", 1, 1, *fd);
        fwrite (lps->label, sizeof (gchar), strlen (lps->label), *fd);
      }

      /* EOL */
      fwrite ("\n", 1, 1, *fd);
    }

    item = g_list_next (item);
  }
}

/*
 * return the already checked urls list
 */
GList *
uc_lists_checked_links_get (void)
{
  return uc_lists_checked_links_list;
}

/*
 * recursively search a item in the
 * url properties list
 */
static UCLinkProperties *
uc_lists_checked_links_lookup_by_uid_real (GList * list, const guint32 uid)
{
  GList *item = NULL;
  UCLinkProperties *dum = NULL;
  UCLinkProperties *lp = NULL;

  g_return_val_if_fail (list != NULL, NULL);

  item = g_list_first (list);
  while (item != NULL)
    {
      lp = (UCLinkProperties *) item->data;

      item = g_list_next (item);

      if (lp->uid == uid)
	return lp;

      if (lp->childs != NULL &&
	  (dum =
	   uc_lists_checked_links_lookup_by_uid_real (lp->childs,
						      uid)) != NULL)
	return dum;
    }

  return NULL;
}

/*
 * return TRUE if the given url already exist in
 * the given similar links list
 */
gboolean
uc_lists_similar_links_exist (GList * list, const gchar * url)
{
  GList *item = NULL;

  if (list == NULL)
    return FALSE;

  item = g_list_first (list);
  while (item != NULL)
    {
      gchar *str = (gchar *) item->data;

      item = g_list_next (item);

      if (!g_ascii_strcasecmp (str, url))
	return TRUE;
    }

  return FALSE;
}

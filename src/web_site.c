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

#include "application.h"
#include "check.h"
#include "project.h"
#include "url.h"
#include "report.h"
#include "lists.h"
#include "html_parser.h"
#include "utils.h"
#include "timeout.h"

#include "web_site.h"


static GList *uc_web_site_old_childs_list = NULL;

static void uc_web_site_refresh_branch_real (GList * list);
static void uc_web_site_refresh_link_real (UCLinkProperties * prop);
static void uc_web_site_refresh_apply_diff (GList * list, GList * sav,
					    UCLinkProperties * prop);
static void uc_web_site_refresh_main_page (UCLinkProperties * prop);
static void uc_web_site_refresh_apply_diff_main (GList * list, GList * sav,
						 GList * main_list);
static UCLinkProperties *uc_web_site_get_old_child (const gchar * url);
static GList *uc_web_site_url_get_links (UCLinkProperties * url,
                                         const guint depth);


/*
 * get all the links of a page
 */
GList *
uc_web_site_get_links_real (UCLinkProperties * prop,
			    const gchar * host, const guint depth)
{
  UCLinkProperties *lp = NULL;
  GList *list = NULL;
  GList *item = NULL;
  gchar *current_host = NULL;
  gchar *current_path = NULL;
  gchar *current_port = NULL;
  gchar *current_args = NULL;
  gboolean accept = FALSE;


  uc_url_parse (prop->h_name, prop->path, prop->url, &current_host,
                &current_port, &current_path, &current_args);

  if (prop->tags != NULL)
  {
    item = g_list_first (prop->tags);
    while (UC_CHECK_PARSER_WHILE_CONDITION)
    {
      gchar *normalized_url = NULL;
      UCHTMLTag *tag = (UCHTMLTag *) item->data;
      UCLinkProperties *old_prop = NULL;


      item = g_list_next (item);

      // META tag (already processed in the 
      // uc_check_link_get_properties () function)
      if (tag->type == LINK_TYPE_META || tag->type == LINK_TYPE_NONE)
        continue;

      uc_report_set_alllinks (-1);
      uc_application_display_state_message (UC_CHECK_MESSAGE_LABEL_SECOND,
                                            tag->value);
      UC_UPDATE_UI;

      normalized_url = uc_url_normalize (current_host,
                                         current_path, tag->value);

      uc_check_currentitem_init (prop, current_host, tag, normalized_url,
                                 FALSE);

      if (uc_check_refresh_link_get_value ())
        old_prop = uc_web_site_get_old_child (normalized_url);

      // Retrieve properties of the link and check if its ok for register
      lp = uc_check_link_get_properties (depth, current_host,
                                         current_path, (UCHTMLTag *) tag,
                                         old_prop, &accept, 0);

      if (lp != NULL && accept)
      {
        lp->normalized_url = normalized_url;
        list = g_list_append (list,
                               uc_check_register_link (normalized_url, lp));

        // Run security tests if requested */
        if (uc_project_get_security_checks ("any"))
          uc_check_run_security_checks (lp);

        // Run w3c tests if requested */
        if (uc_project_get_w3c_checks ("any"))
          uc_check_run_w3c_checks (lp);

        uc_check_wait ();
      }
      // Rejected link
      else
      {
        if (lp != NULL)
          uc_lists_checked_links_node_free (NULL, &lp);

        g_free (normalized_url), normalized_url = NULL;
      }

       uc_report_display_update ();
    }
  }

  if (current_host)
    g_free (current_host), current_host = NULL;
  if (current_port)
    g_free (current_port), current_port = NULL;
  if (current_path)
    g_free (current_path), current_path = NULL;
  if (current_args)
    g_free (current_args), current_args = NULL;

  return list;
}


/*
 * put in a list all the urls of
 * a given url
 */
static GList *
uc_web_site_url_get_links (UCLinkProperties * prop, const guint depth)
{
  gchar *host = NULL;
  GList *urls = NULL;
  GList *item = NULL;
  GList *tmp_list = NULL;


  if (prop == NULL)
    return NULL;

  host = uc_url_get_hostname ("", prop->url);

  if (UC_CHECK_ABORT_IF_CONDITION)
    {
      g_free (host), host = NULL;

      return NULL;
    }

  uc_application_display_state_message (UC_CHECK_MESSAGE_LABEL_FIRST,
					prop->url);
  uc_application_display_state_message (UC_CHECK_MESSAGE_LABEL_SECOND, "");
  uc_application_display_state_message (UC_CHECK_MESSAGE_LABEL_THIRD,
					_("Searching..."));
  UC_UPDATE_UI;

  urls = uc_web_site_get_links_real (prop, host, depth);

  item = g_list_first (urls);
  while (item != NULL)
    {
      UCLinkProperties *lp = (UCLinkProperties *) item->data;

      UC_UPDATE_UI;

      item = g_list_next (item);

      /* Main project page has already been 
       * proceed */
      if (uc_check_is_main_page (lp))
	continue;

      if (lp->is_parsable && !lp->checked)
	{
	  /* Mark this link as already checked */
	  prop->checked = TRUE;

	  uc_utils_debug ("Checking %s...\n", (lp->url)?lp->url:"");
	  lp->childs = uc_web_site_url_get_links (lp, depth + 1);
	}

      tmp_list = g_list_append (tmp_list, (gpointer) lp);
    }

  g_free (host), host = NULL;
  g_list_free (urls), urls = NULL;

  return tmp_list;
}

static void
uc_web_site_refresh_branch_real (GList * list)
{
  GList *item = NULL;


  item = g_list_first (list);
  while (UC_CHECK_PARSER_WHILE_CONDITION)
    {
      UCLinkProperties *prop = (UCLinkProperties *) item->data;


      item = g_list_next (item);

      uc_web_site_refresh_link_real (prop);

      if (prop->childs != NULL && !uc_check_is_main_page (prop))
	uc_web_site_refresh_branch_real (prop->childs);
    }
}

void
uc_web_site_refresh_parent (const guint32 id)
{
  GtkTreePath *treepath = NULL;
  UCLinkProperties *prop = NULL;

  prop = uc_lists_checked_links_lookup_by_uid (id);

  g_return_if_fail (prop != NULL);

  if (prop->parent != NULL)
    {
      uc_web_site_refresh_link (prop->parent->uid);

      treepath = gtk_tree_path_new_from_string (prop->treeview_path);
      gtk_tree_view_expand_row (treeview, treepath, (prop->childs != NULL));
      gtk_tree_view_set_cursor (treeview, treepath, NULL, FALSE);
      gtk_tree_path_free (treepath), treepath = NULL;
    }
}

void
uc_web_site_refresh_link (const gint32 id)
{
  time_t t;
  UCLinkProperties *prop = NULL;
  guint timer_id = 0;

  if (uc_project_get_type () == UC_PROJECT_TYPE_LOCAL_FILE)
    uc_utils_swap_file_proto_option (TRUE);

  uc_project_set_check_is_current (TRUE);
  uc_check_cancel_set_value (FALSE);

  uc_application_set_status_bar (0, _("Updating link..."));
  uc_application_progress_dialog_show ();
  uc_application_display_state_message (UC_CHECK_MESSAGE_LABEL_FIRST,
                                        _("Check in progress..."));
  WSENS ("bt_ignore_progress_dialog", FALSE);
  WSENS ("bt_suspend_progress_dialog", FALSE);
  UC_UPDATE_UI;

  prop = uc_lists_checked_links_lookup_by_uid (id);

  time (&t);
  t -= (uc_report_get_elapsedtime () + 3600);
  timer_id = g_timeout_add (1000, &uc_report_timer_callback, (gpointer) t);

  uc_timeout_domains_free ();

  /* refresh link */
  uc_web_site_refresh_link_real (prop);

  g_source_remove (timer_id), timer_id = 0;

  gtk_widget_hide (WGET ("progress_dialog"));

  /* refresh report */
  uc_check_refresh_report ();

  uc_check_cancel_set_value (FALSE);
  uc_project_set_check_is_current (FALSE);

  if (!uc_project_get_check_is_bookmarks ())
    uc_project_set_save (TRUE);

  uc_application_set_status_bar (0, "");

  if (uc_project_get_type () == UC_PROJECT_TYPE_LOCAL_FILE)
    uc_utils_swap_file_proto_option (FALSE);
}

static void
uc_web_site_refresh_main_page (UCLinkProperties * prop)
{
  GList *item = NULL;
  GList *sav = NULL;
  UCLinkProperties *new_prop = NULL;
  GList *list = NULL;
  gboolean accept = FALSE;
  GtkTreePath *treepath = NULL;
  GList *main_list = NULL;


  uc_check_refresh_link_set_value (TRUE);

  /* get main page links and set it temporarly as childs */
  item = g_list_first (uc_lists_checked_links_get ());
  while (item != NULL)
    {
      UCLinkProperties *lp = (UCLinkProperties *) item->data;

      if (!uc_check_is_main_page (lp))
	sav = g_list_append (sav, lp);

      item = g_list_next (item);
    }
  prop->childs = sav;

  /* we will use this list to find some informations about
   * old childs during the new check */
  uc_web_site_old_childs_list = sav;

  uc_lists_already_checked_links_remove_items (prop);

  uc_check_currentitem_init (NULL, prop->h_name, prop->tag,
                             prop->normalized_url, FALSE);

  uc_lists_refresh_preserved_links_reset ();

  new_prop = uc_check_link_get_properties (
    0, prop->h_name, prop->current_path, (UCHTMLTag *) prop->tag,
    (prop->childs != NULL) ? NULL : prop, &accept, 0);

  if (new_prop != NULL && accept)
    {
      new_prop->normalized_url = g_strdup (prop->url);
// FIXME
      uc_check_register_link (uc_project_get_url (), prop);

      list = uc_web_site_get_links_real (new_prop, new_prop->h_name, 0);
      main_list = uc_lists_checked_links_get ();
      uc_web_site_refresh_apply_diff_main (list, sav, main_list);
      if (list != NULL)
	main_list = uc_lists_refresh_preserved_links_restore (main_list);
      uc_lists_checked_links_set (main_list);
      prop->childs = NULL;
      g_list_free (sav), sav = NULL;

      /* refreshing the depth value of all nodes */
      uc_check_set_depth (uc_lists_checked_links_get (), 0);
    }
  else if (new_prop != NULL)
    uc_lists_checked_links_node_free (NULL, &new_prop);

  treepath = gtk_tree_path_new_from_string (prop->treeview_path);
  uc_application_main_tree_display_all ();
  gtk_tree_view_expand_to_path (treeview, treepath);
  gtk_tree_view_set_cursor (treeview, treepath, NULL, FALSE);
  gtk_tree_path_free (treepath);

  uc_check_refresh_link_set_value (FALSE);

  uc_application_set_status_bar (0, "");
}

static void
uc_web_site_refresh_apply_diff_main (GList * list, GList * sav,
				     GList * main_list)
{
  GList *item = NULL;

  /* parcourir les liens recuperes */
  item = g_list_first (list);
  while (item != NULL)
    {
      UCLinkProperties *lp = (UCLinkProperties *) item->data;
      GList *item1 = NULL;

      lp->parent = NULL;

      /* supprimer les liens qui n'existent plus */
      item1 = g_list_first (sav);
      while (item1 != NULL)
	{
	  UCLinkProperties *lp1 = (UCLinkProperties *) item1->data;
	  GList *item2 = NULL;
	  gboolean found = FALSE;

	  if (lp1 != NULL)
	    {
	      item2 = g_list_first (list);
	      while (item2 != NULL && !found)
		{
		  UCLinkProperties *lp2 = (UCLinkProperties *) item2->data;
		  found = (!g_ascii_strcasecmp (lp1->url, lp2->url));
		  item2 = g_list_next (item2);
		}

	      if (!found)
		{
		  uc_lists_already_checked_links_remove_branch_items (lp1);
		  main_list = g_list_remove (main_list, (gpointer) lp1);
		  lp1 = NULL;
		}
	    }

	  item1 = g_list_next (item1);
	}

      /* si le lien existait deja alors on lui associe
       * ses anciens enfants */
      item1 = g_list_first (list);
      while (item1 != NULL)
	{
	  UCLinkProperties *lp1 = (UCLinkProperties *) item1->data;

	  if (!g_ascii_strcasecmp (lp1->url, lp->url))
	    {
	      GList *item2 = NULL;
	      g_list_free (lp->childs), lp->childs = NULL;
	      lp->childs = lp1->childs;
              // FIXME
	      lp->depth_level = 0;

	      /* on parcours les anciens enfant et on leur donne le
	       * nouveau pere */
	      item2 = g_list_first (lp->childs);
	      while (item2 != NULL)
		{
		  UCLinkProperties *lp2 = (UCLinkProperties *) item2->data;
		  lp2->parent = lp;
                  // FIXME
		  lp2->depth_level = 1;
		  item2 = g_list_next (item2);
		}
	    }

	  item1 = g_list_next (item1);
	}

      item = g_list_next (item);
    }

  /* add links that was not here before the update */
  item = g_list_first (uc_lists_links_first_minus_second (list, sav));
  while (item != NULL)
    {
      UCLinkProperties *lp1 = (UCLinkProperties *) item->data;
      main_list = g_list_append (main_list, lp1);
      item = g_list_next (item);
    }
}

static void
uc_web_site_refresh_apply_diff (GList * list, GList * sav,
				UCLinkProperties * prop)
{
  GList *item = NULL;

  /* parcourir les liens recuperes */
  item = g_list_first (list);
  while (item != NULL)
    {
      UCLinkProperties *lp = (UCLinkProperties *) item->data;
      GList *item1 = NULL;

      lp->parent = prop;

      /* supprimer les liens qui n'existent plus */
      item1 = g_list_first (sav);
      while (item1 != NULL)
	{
	  UCLinkProperties *lp1 = (UCLinkProperties *) item1->data;
	  GList *item2 = NULL;
	  gboolean found = FALSE;

	  if (lp1 != NULL)
	    {
	      item2 = g_list_first (list);
	      while (item2 != NULL && !found)
		{
		  UCLinkProperties *lp2 = (UCLinkProperties *) item2->data;
		  found = (!g_ascii_strcasecmp (lp1->url, lp2->url));
		  item2 = g_list_next (item2);
		}

	      if (!found)
		{
		  uc_lists_already_checked_links_remove_branch_items (lp1);
		  prop->childs = g_list_remove (prop->childs, (gpointer) lp1);
		  lp1 = NULL;
		}
	    }

	  item1 = g_list_next (item1);
	}

      /* si le lien existait deja alors on lui associe
       * ses anciens enfants */
      item1 = g_list_first (prop->childs);
      while (item1 != NULL)
	{
	  UCLinkProperties *lp1 = (UCLinkProperties *) item1->data;
	  if (!g_ascii_strcasecmp (lp1->url, lp->url))
	    {
	      GList *item2 = NULL;
	      g_list_free (lp->childs), lp->childs = NULL;
	      lp->childs = lp1->childs;
	      lp->depth_level = lp1->depth_level;

	      /* on parcours les anciens enfants et on leur donne le
	       * nouveau pere */
	      item2 = g_list_first (lp->childs);
	      while (item2 != NULL)
		{
		  UCLinkProperties *lp2 = (UCLinkProperties *) item2->data;
		  lp2->parent = lp;
		  lp2->depth_level = lp->depth_level + 1;
		  item2 = g_list_next (item2);
		}
	    }

	  item1 = g_list_next (item1);
	}

      item = g_list_next (item);
    }
}


static UCLinkProperties *
uc_web_site_get_old_child (const gchar * url)
{
  GList *item = NULL;


  item = g_list_first (uc_web_site_old_childs_list);
  while (item != NULL)
  {
    UCLinkProperties *prop = (UCLinkProperties *) item->data;


    if (!g_ascii_strcasecmp (url, prop->normalized_url))
      return prop;

    item = g_list_next (item);
  }

  return NULL;
}


static void
uc_web_site_refresh_link_real (UCLinkProperties * prop)
{
  UCLinkProperties *new_prop = NULL;
  GList *list = NULL;
  GtkTreePath *treepath = NULL;
  gboolean accept = FALSE;
  gchar *normalized_url = NULL;
  UCLinkProperties *parent = NULL;
  gchar *old_status = NULL;
  gchar *new_status = NULL;

  /* special update method for main project page */
  if (uc_check_is_main_page (prop))
    {
      uc_web_site_refresh_main_page (prop);

      return;
    }

  old_status = g_hash_table_lookup (prop->header, UC_HEADER_STATUS);

  /* do not check malformed links */
  if (uc_check_status_is_malformed (old_status))
    return;

  uc_check_refresh_link_set_value (TRUE);

  /* we will use this list to find some informations about
   * old childs during the new check */
  uc_web_site_old_childs_list = prop->childs;

  parent = (prop->parent != NULL &&
	    prop->parent->link_type != LINK_TYPE_BOOKMARK_FOLDER) ?
    prop->parent : prop;

  normalized_url =
    uc_url_normalize (uc_project_get_current_host (),
		      parent->path, ((UCHTMLTag *) prop->tag)->value);

  uc_check_currentitem_init (prop, uc_project_get_current_host (), prop->tag,
                             normalized_url, FALSE);

  /* remove links in already checked list to check them
   * again */
  uc_lists_already_checked_links_remove_items (prop);

  uc_lists_refresh_preserved_links_reset ();

  new_prop = uc_check_link_get_properties (prop->depth_level,
					   prop->h_name,
					   prop->current_path,
					   (UCHTMLTag *) prop->tag,
					   (prop->childs !=
					    NULL) ? NULL : prop, &accept, 0);

  if (new_prop != NULL && accept)
    {
      /* get actual status */
      new_status = g_hash_table_lookup (new_prop->header, UC_HEADER_STATUS);

      new_prop->normalized_url = normalized_url;
// FIXME
      uc_check_register_link (normalized_url, prop);

      /* we do not check other site that the main site */
      if (!g_ascii_strcasecmp (uc_project_get_current_host (), prop->h_name))
	if (uc_check_status_is_good (new_status))
	  {
	    /* retrieve page's links */
	    list =
	      uc_web_site_get_links_real (new_prop, new_prop->h_name,
					  new_prop->depth_level);

	    /* fusion the two childs lists (old and new) */
	    uc_web_site_refresh_apply_diff (list, prop->childs, prop);
	  }

      prop = uc_check_copy_node (new_prop, prop);
      g_list_free (prop->childs), prop->childs = NULL;
      prop->childs = list;
      prop->childs = uc_lists_refresh_preserved_links_restore (prop->childs);

      /* refreshing teh depth value of all nodes */
      uc_check_set_depth (uc_lists_checked_links_get (), 0);

      treepath = gtk_tree_path_new_from_string (prop->treeview_path);
      uc_application_main_tree_display_all ();
      gtk_tree_view_expand_to_path (treeview, treepath);
      if (uc_project_get_bookmarks_file () == NULL)
	gtk_tree_view_set_cursor (treeview, treepath, NULL, FALSE);
      gtk_tree_path_free (treepath);
    }
  else
    {
      if (new_prop != NULL)
        uc_lists_checked_links_node_free (NULL, &new_prop);

      g_free (normalized_url), normalized_url = NULL;
    }

  uc_check_refresh_link_set_value (FALSE);
}

void
uc_web_site_refresh_branch (const gint32 id)
{
  time_t t;
  guint timer_id = 0;
  UCLinkProperties *prop = NULL;

  if (uc_project_get_type () == UC_PROJECT_TYPE_LOCAL_FILE)
    uc_utils_swap_file_proto_option (TRUE);

  uc_project_set_check_is_current (TRUE);
  uc_check_cancel_set_value (FALSE);

  uc_application_set_status_bar (0, _("Updating branch..."));
  uc_application_progress_dialog_show ();
  WSENS ("bt_ignore_progress_dialog", FALSE);
  WSENS ("bt_suspend_progress_dialog", FALSE);
  UC_UPDATE_UI;

  prop = uc_lists_checked_links_lookup_by_uid (id);
  g_assert (prop->childs);

  time (&t);
  t -= (uc_report_get_elapsedtime () + 3600);
  timer_id = g_timeout_add (1000, &uc_report_timer_callback, (gpointer) t);

  uc_timeout_domains_free ();

  /* refresh links */
  uc_web_site_refresh_link_real (prop);
  uc_web_site_refresh_branch_real (prop->childs);

  g_source_remove (timer_id), timer_id = 0;

  gtk_widget_hide (WGET ("progress_dialog"));

  /* refresh report */
  uc_check_refresh_report ();

  uc_project_set_check_is_current (FALSE);
  uc_check_cancel_set_value (FALSE);

  if (!uc_project_get_check_is_bookmarks ())
    uc_project_set_save (TRUE);

  uc_application_set_status_bar (0, "");

  if (uc_project_get_type () == UC_PROJECT_TYPE_LOCAL_FILE)
    uc_utils_swap_file_proto_option (FALSE);
}

void
uc_web_site_refresh_all (void)
{
  time_t t;
  guint timer_id = 0;

  if (uc_project_get_type () == UC_PROJECT_TYPE_LOCAL_FILE)
    uc_utils_swap_file_proto_option (TRUE);

  uc_project_set_check_is_current (TRUE);
  uc_check_cancel_set_value (FALSE);

  uc_application_set_status_bar (0, _("Updating all project..."));
  uc_application_progress_dialog_show ();
  WSENS ("bt_ignore_progress_dialog", FALSE);
  WSENS ("bt_suspend_progress_dialog", FALSE);
  UC_UPDATE_UI;

  time (&t);
  t -= (uc_report_get_elapsedtime () + 3600);
  timer_id = g_timeout_add (1000, &uc_report_timer_callback, (gpointer) t);

  uc_timeout_domains_free ();

  /* refresh links */
  uc_web_site_refresh_branch_real (uc_lists_checked_links_get ());

  g_source_remove (timer_id), timer_id = 0;

  gtk_widget_hide (WGET ("progress_dialog"));

  /* refresh report */
  uc_check_refresh_report ();

  uc_project_set_check_is_current (FALSE);
  uc_check_cancel_set_value (FALSE);

  if (!uc_project_get_check_is_bookmarks ())
    uc_project_set_save (TRUE);

  uc_application_set_status_bar (0, "");

  if (uc_project_get_type () == UC_PROJECT_TYPE_LOCAL_FILE)
    uc_utils_swap_file_proto_option (FALSE);

}

/*
 * begin the page urls check
 */
void
uc_web_site_begin_check (void)
{
  UCLinkProperties *prop = NULL;
  GtkWidget *widget = NULL;
  GList *list = NULL;
  gchar *tmp = NULL;
  guint timer_id = 0;
  gboolean accept = FALSE;
  gboolean have_result = FALSE;
  gboolean have_redir = FALSE;
  UCHTMLTag *tag = NULL;
  gchar *error_msg = NULL;
  GtkTreeIter iter;

  uc_application_draw_main_frames ();
  uc_application_progress_dialog_show ();
  WSENS ("pd_dynamic_values_security_checks",
	 uc_project_get_type () == UC_PROJECT_TYPE_WEB_SITE);

  UC_GET_WIDGET ("urls_list", WGET ("main_window"), widget);
  treeview = GTK_TREE_VIEW (widget);

  treestore = gtk_tree_store_new (N_COLUMNS,
				  G_TYPE_INT,
				  GDK_TYPE_PIXBUF,
				  GDK_TYPE_PIXBUF,
				  GDK_TYPE_PIXBUF,
				  GDK_TYPE_PIXBUF,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING, G_TYPE_STRING);

  uc_application_build_url_treeview ();
  gtk_widget_show (widget);

  /* set general project settings */
  tmp = uc_url_get_hostname (NULL, uc_project_get_url ());
  uc_project_set_current_host (tmp);
  g_free (tmp), tmp = NULL;
  tmp = uc_url_get_port (uc_project_get_url ());
  uc_project_set_current_port (tmp);
  g_free (tmp), tmp = NULL;
  tmp = g_strdup_printf (_("Checking %s..."), uc_project_get_url ());
  uc_application_display_state_message (UC_CHECK_MESSAGE_LABEL_FIRST, tmp);
  g_free (tmp), tmp = NULL;

  /* main window title */
  tmp = g_strconcat ("gURLChecker ", UC_VERSION, " - ", uc_project_get_url (),
		     NULL);
  gtk_window_set_title (GTK_WINDOW (WGET ("main_window")), tmp);
  g_free (tmp), tmp = NULL;

  /* check timer */
  timer_id = g_timeout_add (1000, &uc_report_timer_callback, NULL);

  /* create the first node (main page) */
  tag = uc_html_parser_node_new ();
  tag->label = g_strdup (_("MAIN PAGE"));
  tag->value = g_strdup (uc_project_get_url ());
  tag->type = (uc_project_get_type () == UC_PROJECT_TYPE_LOCAL_FILE) ?
    LINK_TYPE_FILE_HREF : LINK_TYPE_HREF;

  UC_UPDATE_UI;

  /* get properties for the main page */
  uc_check_currentitem_init (NULL, uc_project_get_current_host (), tag,
                             uc_project_get_url (), TRUE);
  prop = uc_check_link_get_properties (0, uc_project_get_current_host (), "",
				       (UCHTMLTag *) tag, NULL, &accept, 0);

  if (prop != NULL)
    {
      /* authentication needed but user did not provided login/password */
      if (!strcmp (g_hash_table_lookup (prop->header, UC_HEADER_STATUS),
		   UC_STATUS_CODE_RESTRICTED))
	{
	  error_msg = _("Access to the specified project URL is restricted.\n"
			"Check has been cancelled");
	  goto cancel_check;
	}

      prop->is_parsable = TRUE;

      /* if there was a redirection for the main url */
      have_redir = (g_ascii_strcasecmp (prop->h_name,
					uc_project_get_current_host ()));
      if (have_redir)
	uc_project_set_current_host (prop->h_name);

      uc_utils_debug ("Checking %s...\n", (prop->url)?prop->url:"");

      /* retreive project's pages */
      uc_project_set_check_is_current (TRUE);
      uc_project_set_check_is_main (TRUE);
      prop->normalized_url =
	uc_url_normalize (uc_project_get_current_host (), "", tag->value);
      uc_lists_already_checked_links_append (prop->normalized_url, prop);
      list = uc_web_site_url_get_links (prop, 0);
      uc_project_set_check_is_main (FALSE);
      uc_project_set_check_is_current (FALSE);

      /* if there is some result, display it in the main
       * tree view */
      // FIXME if (g_list_length (list) >= 1)
      {
	have_result = TRUE;

	list = g_list_prepend (list, prop);
	uc_lists_checked_links_set (list);

	/* display main tree view */
	gtk_tree_store_clear (treestore);

        UC_DISPLAY_SET_ALL;
	uc_check_display_list (uc_lists_checked_links_get (), NULL, iter);

	/* update application menu */
        WSENS ("mwm_delete_all_bad_links", FALSE);
	WSENS ("mwm_display", TRUE);
	WSENS ("mwm_refresh_all", TRUE);
	WSENS ("mwm_refresh_main_page", TRUE);
	WSENS ("mwm_find", !uc_project_get_speed_check ());
	WSENS ("mwm_project_properties", TRUE);
#ifndef ENABLE_GNUTLS
        if (strcmp (prop->protocol, UC_PROTOCOL_HTTPS) == 0)
          WSENS ("mwm_refresh_main_page", FALSE);
#endif
#ifdef ENABLE_CLAMAV
        WSENS ("mwm_display_security_alerts", !uc_project_get_speed_check ());
#endif

	uc_project_set_save (TRUE);
      }
    }

  if (!have_result)
    error_msg = _("The specified project URL does not respond.\n"
		  "Check has been cancelled.");

cancel_check:

  if (error_msg)
    uc_application_dialog_show (error_msg, GTK_MESSAGE_ERROR);

  g_source_remove (timer_id), timer_id = 0;

  uc_check_cancel_set_value (FALSE);

  gtk_widget_hide (WGET ("progress_dialog"));
}

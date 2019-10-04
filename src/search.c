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

#include "application.h"
#include "utils.h"
#include "lists.h"
#include "search.h"
#include "callbacks.h"
#include "cache.h"

static guint32 uc_search_result_count = 0;
static gboolean uc_search_exit_value = FALSE;

enum
{
  SEARCH_UID_COLUMN,
  SEARCH_URL_COLUMN,
  SEARCH_N_COLUMNS
};

static void uc_search_display_result (void);
static void uc_search_pages_content (GList * list);
static void uc_search_links_name (GList * list);
static void uc_search_meta_content (GList * list);
static gboolean uc_search_meta_content_lookup (GList * metas);
static void uc_search_emails (GList * list);
static gboolean uc_search_emails_lookup (GList * emails);
static int uc_search_list_get_selected_row_id (GtkTreeView * list);
static void uc_search_treeview_add_row (const UCLinkProperties * prop,
					const guint count);
static gboolean uc_search_strstr (const gchar * buffer);

void
uc_search_set_exit (const gboolean val)
{
  if ((uc_search_exit_value = val))
    gtk_widget_hide (WGET ("search_dialog"));
}

gboolean
uc_search_get_exit (void)
{
  return uc_search_exit_value;
}

/*
 * reset the search properties
 */
void
uc_search_free (void)
{
  g_free (search_state.string), search_state.string = NULL;
  search_state.case_sensitive = FALSE;
  search_state.status_code = FALSE;
  search_state.pages_content = FALSE;
  search_state.links_name = FALSE;
  search_state.meta_content = FALSE;
  search_state.emails = FALSE;

  uc_search_result_count = 0;
}

/*
 * control the check button state
 */
void
uc_search_check_verification (void)
{
  if (gtk_toggle_button_get_active
      (GTK_TOGGLE_BUTTON (WGET ("nsd_search_pages_content"))))
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				    (WGET ("nsd_search_links_name")), FALSE);
      WSENS ("nsd_search_links_name", FALSE);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				    (WGET ("nsd_search_meta_content")),
				    FALSE);
      WSENS ("nsd_search_meta_content", FALSE);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				    (WGET ("nsd_search_emails")), FALSE);
      WSENS ("nsd_search_emails", FALSE);
    }
  else
    {
      WSENS ("nsd_search_status_code", TRUE);
      WSENS ("nsd_search_links_name", TRUE);
      WSENS ("nsd_search_meta_content", TRUE);
      WSENS ("nsd_search_emails", TRUE);
    }
}

/*
 * create and display the list for the search
 * result
 */
static void
uc_search_display_result (void)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column = NULL;
  GtkWidget *scroll = NULL;

  scroll = WGET ("sd_search_scroll");

  treestore_search =
    gtk_list_store_new (SEARCH_N_COLUMNS, G_TYPE_INT, G_TYPE_STRING);
  treeview_search =
    GTK_TREE_VIEW (gtk_tree_view_new_with_model
		   (GTK_TREE_MODEL (treestore_search)));

  renderer = gtk_cell_renderer_text_new ();

  gtk_tree_view_insert_column_with_attributes (treeview_search, -1,
					       _("Id"), renderer,
					       "text", SEARCH_UID_COLUMN,
					       NULL);
  column = gtk_tree_view_get_column (treeview_search, SEARCH_UID_COLUMN);
  gtk_tree_view_column_set_visible (column, FALSE);

  gtk_tree_view_insert_column_with_attributes (treeview_search,
					       -1,
					       _("URL"), renderer,
					       "text", SEARCH_URL_COLUMN,
					       NULL);

  gtk_tree_view_set_rules_hint (treeview_search, TRUE);

  uc_utils_clear_container (GTK_CONTAINER (scroll));
  gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET (treeview_search));
  gtk_widget_show_all (scroll);
}

/*
 * set the main url treeview cursor on the same item
 * than the treeview search
 */
void
uc_search_main_select (GtkTreeView * selection)
{
  UCLinkProperties *prop = NULL;
  GtkTreePath *path = NULL;
  gint32 id = 0;

  id = uc_search_list_get_selected_row_id (selection);
  if (id <= 0)
    return;

  prop = uc_lists_checked_links_lookup_by_uid (id);

  path = gtk_tree_path_new_from_string (prop->treeview_path);
  gtk_tree_view_set_cursor (treeview, path, NULL, FALSE);
  gtk_tree_path_free (path), path = NULL;
}

/*
 * return the id of the current selected row in the result
 * list
 */
static int
uc_search_list_get_selected_row_id (GtkTreeView * list)
{
  GtkTreeIter iter;
  GtkTreeModel *model = NULL;
  GtkTreeSelection *select = NULL;
  gint32 ret = -1;

  if (uc_search_result_count == 0 ||
      !(select = gtk_tree_view_get_selection (list)))
    return ret;

  uc_application_main_tree_display_all ();
  uc_application_main_tree_expand_all ();

  if (gtk_tree_selection_get_selected (select, &model, &iter))
    gtk_tree_model_get (model, &iter, 0, &ret, -1);

  return ret;
}

/* FIXME
 * -> do a case insensitive search when requested...
 *  
 * search the given string case or not case sensitive,
 * according to the user choice
 */
static gboolean
uc_search_strstr (const gchar * buffer)
{
  return (strstr (buffer, search_state.string) != NULL);
}

/*
 * search in pages content
 */
static void
uc_search_pages_content (GList * list)
{
  GList *item = NULL;
  gsize length = 0;

  item = g_list_first (list);
  while (item != NULL && !uc_search_get_exit ())
    {
      UCLinkProperties *prop = (UCLinkProperties *) item->data;
      gchar *buffer = NULL;

      item = g_list_next (item);

      if (prop->link_type == LINK_TYPE_BOOKMARK_FOLDER)
	{
	  if (prop->childs != NULL)
	    uc_search_pages_content (prop->childs);

	  continue;
	}
      else if (!prop->is_parsable || prop->to_delete ||
	       (buffer = uc_cache_get_source (prop->uid, &length)) == NULL)
	continue;

      uc_application_display_search_message (UC_CHECK_MESSAGE_LABEL_SECOND,
					     prop->url);
      UC_UPDATE_UI;

      if (!uc_search_get_exit () && uc_search_strstr (buffer))
	{
	  uc_search_treeview_add_row (prop, uc_search_result_count);
	  uc_search_result_count++;
	}

      g_free (buffer), buffer = NULL;

      if (prop->childs != NULL)
	uc_search_pages_content (prop->childs);
    }
}

/*
 * search in links name
 */
static void
uc_search_links_name (GList * list)
{
  GList *item = NULL;

  item = g_list_first (list);
  while (item != NULL && !uc_search_get_exit ())
    {
      UCLinkProperties *prop = (UCLinkProperties *) item->data;

      item = g_list_next (item);

      if (prop->link_type == LINK_TYPE_BOOKMARK_FOLDER)
	{
	  if (prop->childs != NULL)
	    uc_search_links_name (prop->childs);

	  continue;
	}
      else if (prop->to_delete)
	continue;

      uc_application_display_search_message (UC_CHECK_MESSAGE_LABEL_SECOND,
					     prop->url);
      UC_UPDATE_UI;

      if (!uc_search_get_exit () && uc_search_strstr (prop->url))
	{
	  uc_search_treeview_add_row (prop, uc_search_result_count);
	  uc_search_result_count++;
	}

      if (prop->childs != NULL)
	uc_search_links_name (prop->childs);
    }
}

/*
 * search a string in all metas of a page
 */
static gboolean
uc_search_meta_content_lookup (GList * metas)
{
  GList *item = NULL;

  item = g_list_first (metas);
  while (item != NULL)
    {
      UCHTMLTag *tag = (UCHTMLTag *) item->data;

      if (uc_search_strstr (tag->value))
	return TRUE;

      item = g_list_next (item);
    }

  return FALSE;
}

/*
 * search in meta content
 */
static void
uc_search_meta_content (GList * list)
{
  GList *item = NULL;

  item = g_list_first (list);
  while (item != NULL && !uc_search_get_exit ())
    {
      UCLinkProperties *prop = (UCLinkProperties *) item->data;

      item = g_list_next (item);

      if (prop->link_type == LINK_TYPE_BOOKMARK_FOLDER)
	{
	  if (prop->childs != NULL)
	    uc_search_pages_content (prop->childs);

	  continue;
	}
      else if (prop->to_delete)
	continue;

      uc_application_display_search_message (UC_CHECK_MESSAGE_LABEL_SECOND,
					     prop->url);
      UC_UPDATE_UI;

      if (!uc_search_get_exit ()
	  && uc_search_meta_content_lookup (prop->metas))
	{
	  uc_search_treeview_add_row (prop, uc_search_result_count);
	  uc_search_result_count++;
	}

      if (prop->childs != NULL)
	uc_search_pages_content (prop->childs);
    }
}

/*
 * search a string in all emails of a page
 */
static gboolean
uc_search_emails_lookup (GList * emails)
{
  GList *item = NULL;

  item = g_list_first (emails);
  while (item != NULL)
    {
      UCHTMLTag *email = (UCHTMLTag *) item->data;

      item = g_list_next (item);

      if (uc_search_strstr (email->value))
	return TRUE;
    }

  return FALSE;
}

/*
 * search in emails
 */
static void
uc_search_emails (GList * list)
{
  GList *item = NULL;

  item = g_list_first (list);
  while (item != NULL && !uc_search_get_exit ())
    {
      UCLinkProperties *prop = (UCLinkProperties *) item->data;

      item = g_list_next (item);

      if (prop->link_type == LINK_TYPE_BOOKMARK_FOLDER)
	{
	  if (prop->childs != NULL)
	    uc_search_emails (prop->childs);

	  continue;
	}
      else if (prop->to_delete)
	continue;

      uc_application_display_search_message (UC_CHECK_MESSAGE_LABEL_SECOND,
					     prop->url);
      UC_UPDATE_UI;

      if (!uc_search_get_exit () && uc_search_emails_lookup (prop->emails))
	{
	  uc_search_treeview_add_row (prop, uc_search_result_count);
	  uc_search_result_count++;
	}

      if (prop->childs != NULL)
	uc_search_emails (prop->childs);
    }
}

/*
 * search in status code
 */
static void
uc_search_status_codes (GList * list)
{
  GList *item = NULL;

  item = g_list_first (list);
  while (item != NULL && !uc_search_get_exit ())
    {
      UCLinkProperties *prop = (UCLinkProperties *) item->data;

      item = g_list_next (item);

      if (prop->link_type == LINK_TYPE_BOOKMARK_FOLDER)
	{
	  if (prop->childs != NULL)
	    uc_search_status_codes (prop->childs);

	  continue;
	}
      else if (prop->to_delete)
	continue;

      uc_application_display_search_message (UC_CHECK_MESSAGE_LABEL_SECOND,
					     prop->url);
      UC_UPDATE_UI;

      if (!uc_search_get_exit () &&
	  uc_search_strstr (g_hash_table_lookup
			    (prop->header, UC_HEADER_STATUS)))
	{
	  uc_search_treeview_add_row (prop, uc_search_result_count);
	  uc_search_result_count++;
	}

      if (prop->childs != NULL)
	uc_search_status_codes (prop->childs);
    }
}

static void
uc_search_treeview_add_row (const UCLinkProperties * prop, const guint count)
{
  gchar *strpath = NULL;
  GtkTreeIter iter;

  gtk_list_store_append (treestore_search, &iter);
  gtk_list_store_set (treestore_search, &iter,
		      SEARCH_UID_COLUMN, prop->uid,
		      SEARCH_URL_COLUMN, prop->url, -1);
  gtk_list_store_move_before (treestore_search, &iter, NULL);

  /* to always view the last inserted item in the list */
  strpath = g_strdup_printf ("%u", count);
  if (!uc_search_get_exit ())
    gtk_tree_view_set_cursor (treeview_search,
			      gtk_tree_path_new_from_string
			      (strpath), NULL, TRUE);
  g_free (strpath), strpath = NULL;

  UC_UPDATE_UI;
}

/*
 * launch the search
 */
void
uc_search_begin (void)
{
  gchar *text = NULL;

  uc_search_result_count = 0;
  uc_search_set_exit (FALSE);

  uc_application_search_dialog_show ();

  WSENS ("mwm_find", FALSE);
  WSENS ("mwm_project", FALSE);
  WSENS ("mw_bt_new", FALSE);
  WSENS ("mw_bt_open", FALSE);

  uc_search_display_result ();

  if (search_state.status_code)
    uc_search_status_codes (uc_lists_checked_links_get ());
  else if (search_state.links_name)
    uc_search_links_name (uc_lists_checked_links_get ());
  else if (search_state.meta_content)
    uc_search_meta_content (uc_lists_checked_links_get ());
  else if (search_state.emails)
    uc_search_emails (uc_lists_checked_links_get ());
  /* the default choice is search_state.pages_content */
  else
    uc_search_pages_content (uc_lists_checked_links_get ());

  if (uc_search_get_exit ())
    return;

  text =
    g_strdup_printf (_("<b>%u</b> item(s) found"), uc_search_result_count);
  uc_application_display_search_message (UC_CHECK_MESSAGE_LABEL_FIRST, text);
  uc_application_display_search_message (UC_CHECK_MESSAGE_LABEL_SECOND, "");
  g_free (text), text = NULL;

  gtk_window_set_title (GTK_WINDOW (WGET ("search_dialog")),
			_("Search ended"));

  g_signal_connect (G_OBJECT (treeview_search), "cursor_changed",
		    G_CALLBACK (on_search_list_cursor_changed), NULL);

  g_signal_connect (G_OBJECT (treeview_search), "button_press_event",
		    G_CALLBACK (on_url_list_mouse_clicked), NULL);

  g_signal_connect (G_OBJECT (treeview_search), "button_release_event",
		    G_CALLBACK (on_url_list_mouse_clicked), NULL);

  uc_utils_set_userfriendly_treeview_column (treeview_search,
					     SEARCH_URL_COLUMN);
}

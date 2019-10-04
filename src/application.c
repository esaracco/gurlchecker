/*
 * Copyright (C) 2002-2013
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

#include "url.h"
#include "cache.h"
#include "web_site.h"
#include "bookmarks.h"
#include "lists.h"
#include "search.h"
#include "tooltips.h"
#include "callbacks.h"
#include "utils.h"
#include "check.h"
#include "timeout.h"
#include "connection.h"
#include "html_parser.h"
#include "ucclam.h"
#include "uctidy.h"
#include "uccroco.h"
#include "cookies.h"

#include "application.h"


#define PROGRESS_LABEL_MAX_LEN 70


extern char **environ;

typedef struct _UCURLsUserActionsData UCURLsUserActionsData;
struct _UCURLsUserActionsData
{
  GtkTreeIter iter;
  GtkListStore *model;
};

/* Hash table of the status code
   properties (message, bg etc.) */
static GHashTable *uc_application_status_code_properties_hash;

/* Hash table of the URLs user actions */
static GHashTable *uc_application_urls_user_actions_hash;

static void uc_application_menu_init (void);
static void uc_application_page_information_dialog_display_links (
              const UCLinkProperties *prop);
static void uc_application_page_information_dialog_display_emails (
              const UCLinkProperties *prop);
static void uc_application_page_information_dialog_display_header (
              const UCLinkProperties *prop);
static void uc_application_page_information_dialog_display_meta_tags (
              const UCLinkProperties *prop);
static void uc_application_settings_dialog_display_filters_list (
              const gchar *glade_name, gpointer items, guint items_size,
              GtkTreeView **tv, GtkListStore **model);
static void uc_application_filters_save_settings (GtkTreeView * treeview,
						  const gchar * path);
static void uc_application_status_code_properties_free_cb (gpointer data);
static void uc_application_urls_user_actions_free_cb (gpointer data);
static gboolean uc_application_cookie_warning_dialog_not_display_cb (
                  GtkToggleButton * button, gpointer user_data);
static void uc_application_action_changed_cb (GtkCellRendererText * renderer,
                                              gchar * path,
                                              gchar * new_text,
                                              GtkListStore * master);
static gboolean uc_application_get_urls_user_action_value_cb (
                  gpointer key, gpointer value, gpointer data);
static gint uc_application_fill_urls_user_actions_combo_cb (gconstpointer a,
                                                            gconstpointer b);
static void uc_application_fill_urls_user_actions_combo (GtkListStore ** model);
static void uc_application_clean_data (void);


void
uc_application_build_projects_treeview (void)
{
  enum
  {
    UID_COLUMN,
    ICON_COLUMN,
    TITLE_COLUMN,
    N_COLUMNS
  };
  GtkCellRenderer *trenderer = NULL;
  GtkCellRenderer *prenderer = NULL;
  GtkWidget *treeview = NULL;
  GtkTreeViewColumn *column = NULL;

  treeview = WGET ("opd_treeview");

  trenderer = gtk_cell_renderer_text_new ();
  prenderer = gtk_cell_renderer_pixbuf_new ();

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
					       -1, _("Id"), trenderer,
					       "text", UID_COLUMN, NULL);
  column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), UID_COLUMN);
  gtk_tree_view_column_set_visible (column, FALSE);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
					       -1, _("Type"), prenderer,
					       "pixbuf", ICON_COLUMN, NULL);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW
					       (treeview), -1,
					       _("URL"), trenderer,
					       "text", TITLE_COLUMN, NULL);
  column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), TITLE_COLUMN);
  gtk_tree_view_column_set_sort_column_id (column, TITLE_COLUMN);

  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
}

static void
uc_application_action_editing_started_cb (GtkCellRendererText * renderer,
					  gchar * path,
					  gchar * new_text,
					  GtkListStore * master)
{
  gtk_list_store_clear (master);
  uc_application_fill_urls_user_actions_combo (&master);
}

static void
uc_application_action_changed_cb (GtkCellRendererText * renderer,
				  gchar * path,
				  gchar * new_text, GtkListStore * master)
{
  GtkTreeIter iter;
  GtkTreeModel *model = NULL;
  GtkTreeSelection *select = NULL;
  UCLinkProperties *prop = NULL;
  UCURLsUserActions *action = NULL;
  gint ret;


  select = gtk_tree_view_get_selection (treeview);
  if (select != NULL &&
      gtk_tree_selection_get_selected (select, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, 0, &ret, -1);

      /* Update URL prop list */
      ret = uc_check_treeview_get_selected_row_id ();
      prop = uc_lists_checked_links_lookup_by_uid (ret);
      action = uc_application_get_urls_user_action_by_value (new_text);
      g_free (prop->user_action);
      prop->user_action = g_strdup (action->id);

      /* Update main treeview model */
      gtk_tree_store_set (GTK_TREE_STORE (model), &iter, ACTION_COLUMN,
			  new_text, -1);

      if (!uc_project_get_check_is_bookmarks ())
        uc_project_set_save (TRUE);
    }
  else
    g_warning ("Error during main treeview action update");
}


static gint
uc_application_fill_urls_user_actions_combo_cb (gconstpointer a,
						gconstpointer b)
{
  UCURLsUserActions *aa = (UCURLsUserActions *) a;
  UCURLsUserActions *bb = (UCURLsUserActions *) b;

  return (strcasecmp (aa->label, bb->label));
}


static void
uc_application_fill_urls_user_actions_combo (GtkListStore ** model)
{
  GtkTreeIter iter;
  GList *item = NULL;
  GList *list =
    g_hash_table_get_values (uc_application_urls_user_actions_hash);


  item = g_list_sort (list,
		      (GCompareFunc)
		      uc_application_fill_urls_user_actions_combo_cb);
  while (item != NULL)
    {
      UCURLsUserActions *a = (UCURLsUserActions *) item->data;

      item = g_list_next (item);

      gtk_list_store_append (*model, &iter);
      gtk_list_store_set (*model, &iter, 0, a->label, -1);
    }
}


/**
 * uc_application_build_url_treeview:
 *
 * Build all needed elements for the main URL tree view.
 */
void
uc_application_build_url_treeview (void)
{
  GtkCellRenderer *prenderer = NULL;
  GtkCellRenderer *trenderer = NULL;
  GtkCellRenderer *crenderer = NULL;
  GtkTreeViewColumn *column = NULL;
  GtkListStore *model = NULL;

  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (treestore));
  gtk_tree_view_set_headers_visible (treeview, TRUE);

  /* ID column */
  trenderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (treeview, -1,
					       NULL, trenderer,
					       "text", UID_COLUMN, NULL);
  column = gtk_tree_view_get_column (treeview, UID_COLUMN);
  gtk_tree_view_column_set_visible (column, FALSE);

  /* LINK STATUS column */
  column = gtk_tree_view_column_new ();

  prenderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, prenderer, FALSE);
  gtk_tree_view_column_set_attributes (column, prenderer,
				       "pixbuf", LINK_STATUS_ICON_COLUMN,
				       NULL);

  /* LINK TYPE column */
  prenderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, prenderer, FALSE);
  gtk_tree_view_column_set_attributes (column, prenderer,
				       "pixbuf", LINK_ICON_COLUMN, NULL);

  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (treeview, column);

  /* SECURITY ALERT column */
  prenderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, prenderer, FALSE);
  gtk_tree_view_column_set_attributes (column, prenderer,
				       "pixbuf", SECURITY_ALERT_ICON_COLUMN,
				       NULL);

  /* W3C ALERT column */
  prenderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, prenderer, FALSE);
  gtk_tree_view_column_set_attributes (column, prenderer,
				       "pixbuf", W3C_ALERT_ICON_COLUMN, NULL);

  /* User actions */
  crenderer = gtk_cell_renderer_combo_new ();
  model = gtk_list_store_new (1, G_TYPE_STRING);
  uc_application_fill_urls_user_actions_combo (&model);

  g_object_set (G_OBJECT (crenderer),
		"model", model,
		"has-entry", TRUE, "editable", TRUE, "text-column", 0, NULL);
  g_signal_connect (G_OBJECT (crenderer), "edited",
		    G_CALLBACK (uc_application_action_changed_cb), NULL);
  g_signal_connect (G_OBJECT (crenderer), "editing-started",
		    G_CALLBACK (uc_application_action_editing_started_cb),
		    model);
  g_signal_connect (G_OBJECT (crenderer), "edited",
		    G_CALLBACK (uc_application_action_changed_cb), NULL);


  gtk_tree_view_insert_column_with_attributes (treeview, -1,
					       _("Custom action"), crenderer,
					       "text", ACTION_COLUMN, NULL);
  column = gtk_tree_view_get_column (treeview, ACTION_COLUMN - 3);
  gtk_tree_view_column_set_sort_column_id (column, ACTION_COLUMN);
  gtk_tree_view_column_set_reorderable (column, TRUE);

  // For the moment, we can not manage user actions for bookmarks
  gtk_tree_view_column_set_visible (column,
                                   (!uc_project_get_check_is_bookmarks ()));

  /* LABEL column */
  gtk_tree_view_insert_column_with_attributes (treeview, -1,
					       _("Label"), trenderer,
					       "text", LABEL_COLUMN, NULL);
  column = gtk_tree_view_get_column (treeview, LABEL_COLUMN - 3);
  gtk_tree_view_column_set_sort_column_id (column, LABEL_COLUMN);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_reorderable (column, TRUE);

  /* URL column */
  gtk_tree_view_insert_column_with_attributes (treeview, -1,
					       _("URL"), trenderer,
					       "text", URL_COLUMN, NULL);
  column = gtk_tree_view_get_column (treeview, URL_COLUMN - 3);
  gtk_tree_view_column_set_sort_column_id (column, URL_COLUMN);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_reorderable (column, TRUE);

  /* Last modified column */
  gtk_tree_view_insert_column_with_attributes (treeview, -1,
					       _("Date"), trenderer,
					       "text", LM_COLUMN, NULL);
  column = gtk_tree_view_get_column (treeview, LM_COLUMN - 3);
  gtk_tree_view_column_set_sort_column_id (column, LM_COLUMN);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_reorderable (column, TRUE);

  gtk_tree_view_set_rules_hint (treeview, TRUE);
}


void
uc_application_reset_menu_types (void)
{
  guint i = 0;
  gchar *filters[] = {
    "mwm_document_type",
    "mwm_image_type",
    "mwm_frame_type",
    "mwm_email_type",
    "mwm_stylesheet_type",
    NULL
  };

  // Already done
  if (UC_DISPLAY_TYPE_IS_NONE)
    return;

  do
  {
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (WGET (filters[i])),
                                    FALSE);
  } 
  while (filters[++i]);

  UC_DISPLAY_TYPE_SET_NONE;
}


void
uc_application_reset_menu_status (void)
{
  guint i = 0;
  gchar *filters[] = {
    "mwm_display_security_alerts",
    "mwm_display_w3c_alerts",
    "mwm_display_goodlinks",
    "mwm_display_badlinks",
    "mwm_display_malformedlinks",
    "mwm_display_timeouts",
    NULL
  };

  // Already done
  if (UC_DISPLAY_STATUS_IS_NONE)
    return;

  do
  {
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (WGET (filters[i])),
                                    FALSE);
  } 
  while (filters[++i]);

  UC_DISPLAY_STATUS_SET_NONE;
}


void
uc_application_reset_menu_protocols (void)
{
  guint i = 0;
  gchar *filters[] = {
    "mwm_http_protocol",
    "mwm_https_protocol",
    "mwm_ftp_protocol",
    NULL
  };

  // Already done
  if (UC_DISPLAY_PROTO_IS_NONE)
    return;

  do
  {
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (WGET (filters[i])),
                                    FALSE);
  } 
  while (filters[++i]);

  UC_DISPLAY_PROTO_SET_NONE;
}


/**
 * uc_application_main_tree_display_all:
 * 
 * Display the whole list of checked url in the
 * main url treeview.
 */
void
uc_application_main_tree_display_all (void)
{
  GtkTreeIter iter;

  WSENS ("mwm_display", TRUE);

  uc_application_reset_menu_status ();
  uc_application_reset_menu_types ();
  uc_application_reset_menu_protocols ();

  gtk_tree_store_clear (treestore);

  uc_check_display_list (uc_lists_checked_links_get (), NULL, iter);
}


void
uc_application_main_tree_apply_filters (const gchar *wname)
{
  GtkTreeIter iter;

  WSENS ("mwm_display", TRUE);
  WSENS (wname, TRUE);

  gtk_tree_store_clear (treestore);

  uc_check_display_list (uc_lists_checked_links_get (), NULL, iter);
}


/**
 * uc_application_main_tree_expand_all:
 * 
 * Expand the main url treeview.
 */
void
uc_application_main_tree_expand_all (void)
{
  WSENS ("mwm_display", TRUE);

  gtk_tree_view_expand_all (treeview);
}


/**
 * uc_application_main_tree_collapse_all:
 * 
 * Collapse the main url treeview.
 */
void
uc_application_main_tree_collapse_all (void)
{
  WSENS ("mwm_display", TRUE);

  gtk_tree_view_collapse_all (treeview);
}


/**
 * uc_application_page_information_dialog_display_meta_tags:
 * @UCLinkProperties: URL list node.
 * 
 * Display meta tags.
 */
static void
uc_application_page_information_dialog_display_meta_tags (
  const UCLinkProperties * prop)
{
  enum
  {
    NAME_COLUMN,
    CONTENT_COLUMN,
    N_COLUMNS
  };
  GtkTreeIter iter;
  GtkTreeView *tv = NULL;
  GtkListStore *model = NULL;
  GtkCellRenderer *renderer = NULL;
  GtkWidget *scroll = NULL;
  GList *item = NULL;

  scroll = WGET ("pid_general_scrolled_down");

  model = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);

  item = g_list_first (prop->metas);
  while (item != NULL)
    {
      UCHTMLTag *lp = (UCHTMLTag *) item->data;

      gtk_list_store_append (model, &iter);
      gtk_list_store_set (model, &iter,
			  NAME_COLUMN, (gchar *) lp->label,
			  CONTENT_COLUMN, (gchar *) lp->value, -1);

      item = g_list_next (item);
    }

  tv = GTK_TREE_VIEW (gtk_tree_view_new_with_model (GTK_TREE_MODEL (model)));
  g_object_unref (model), model = NULL;

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (tv,
					       -1,
					       _("Name"), renderer,
					       "text", NAME_COLUMN, NULL);
  uc_utils_set_userfriendly_treeview_column (tv, NAME_COLUMN);

  gtk_tree_view_insert_column_with_attributes (tv,
					       -1,
					       _("Content"), renderer,
					       "text", CONTENT_COLUMN, NULL);
  uc_utils_set_userfriendly_treeview_column (tv, CONTENT_COLUMN);

  gtk_tree_view_set_rules_hint (tv, TRUE);

  uc_utils_clear_container (GTK_CONTAINER (scroll));
  gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET (tv));

  gtk_widget_show_all (scroll);
}

/**
 * uc_application_open_project_dialog_show:
 * 
 * Show the open project dialog.
 */
void
uc_application_open_project_dialog_show (void)
{
  enum
  {
    UID_COLUMN,
    ICON_COLUMN,
    TITLE_COLUMN,
    COMBO_COLUMN,
    N_COLUMNS
  };
  GtkTreeIter iter;
  GtkWidget *treeview = NULL;
  GList *list = NULL;
  GtkWidget *dialog = NULL;
  GtkListStore *treestore = NULL;

  if (!uc_project_save_all ())
    return;

  dialog = WGET ("open_project_dialog");

  treeview = WGET ("opd_treeview");

  if ((treestore =
       GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (treeview)))))
    g_object_unref (treestore), treestore = NULL;

  treestore = gtk_list_store_new (N_COLUMNS, G_TYPE_INT,
				  GDK_TYPE_PIXBUF, G_TYPE_STRING,
				  G_TYPE_STRING);

  list = g_list_first (uc_project_get_projects_list ());
  while (list != NULL)
    {
      UCProjectProjects *item = (UCProjectProjects *) list->data;
      GdkPixbuf *icon = NULL;

      list = g_list_next (list);

      icon = uc_project_get_type_icon (item->type);
      gtk_list_store_append (treestore, &iter);
      gtk_list_store_set (treestore, &iter,
			  UID_COLUMN, item->id,
			  ICON_COLUMN, icon, TITLE_COLUMN, item->title, -1);
      g_object_unref (icon), icon = NULL;
    }

  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview),
			   GTK_TREE_MODEL (treestore));

  gtk_widget_show_all (dialog);
}

/**
 * uc_application_input_file_dialog:
 * @text: The text to display.
 * @title: Title of the popup.
 * 
 * Show a prompt popup to input a filename or directory.
 *
 * Returns: The user string. This is a newly allocated string.
 */
gchar *
uc_application_input_file_dialog (const gchar * title, const gchar * text)
{
  GtkWidget *widget = NULL;
  GtkWidget *dialog = NULL;
  gchar *ret = NULL;
  const gchar *input_text = NULL;
  gint res = 0;

  dialog = WGET ("input_file_dialog");
  gtk_window_set_title (GTK_WINDOW (dialog), title);

  widget = WGET ("ifd_label");
  gtk_label_set_text (GTK_LABEL (widget), text);

  gtk_widget_show_all (dialog);

again:

  res = gtk_dialog_run (GTK_DIALOG (dialog));

  widget = WGET ("ifd_input_text");
  input_text = gtk_entry_get_text (GTK_ENTRY (widget));

  if (res == GTK_RESPONSE_OK && strlen (input_text) == 0)
    goto again;
  else
    {
      ret = g_strdup (input_text);
      gtk_widget_hide (dialog);
    }

  return ret;
}

/**
 * uc_application_input_dialog:
 * @text: The text to display.
 * @title: title of the popup.
 * 
 * Show a prompt popup.
 *
 * Returns: The user string. This is a newly allocated string.
 */
gchar *
uc_application_input_dialog (const gchar * title, const gchar * text)
{
  GtkWidget *widget = NULL;
  gchar *ret = NULL;
  const gchar *input_text = NULL;
  gint res = 0;
  GtkWidget *dialog = NULL;

  dialog = WGET ("input_dialog");
  gtk_window_set_title (GTK_WINDOW (dialog), title);

  widget = WGET ("id_label");
  gtk_label_set_text (GTK_LABEL (widget), text);

  widget = WGET ("id_input_text");
  gtk_entry_set_text (GTK_ENTRY (widget), "");

  gtk_widget_show_all (dialog);

  res = gtk_dialog_run (GTK_DIALOG (dialog));

  widget = WGET ("id_input_text");
  input_text = gtk_entry_get_text (GTK_ENTRY (widget));

  if (res == GTK_RESPONSE_OK)
    ret = g_strdup (input_text);

  gtk_widget_hide (dialog);

  return ret;
}

/**
 * uc_application_project_information_dialog_show:
 * 
 * Show the project properties dialog.
 *
 * Returns: TRUE if project must be saved.
 */
gboolean
uc_application_project_information_dialog_show (void)
{
  GtkWidget *widget = NULL;
  gboolean ret = FALSE;
  gint res = 0;
  GtkWidget *dialog = NULL;
  GtkTextBuffer *buffer = NULL;

  dialog = WGET ("project_information_dialog");

  widget = WGET ("pid_description");
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
  gtk_text_buffer_set_text (buffer, uc_project_get_description (), -1);
  widget = WGET ("pid_title");
  gtk_entry_set_text (GTK_ENTRY (widget), uc_project_get_title ());

  gtk_widget_show_all (dialog);

again:

  res = gtk_dialog_run (GTK_DIALOG (dialog));
  if (res == GTK_RESPONSE_OK)
    {
      if (!uc_project_save_properties ())
	goto again;
      ret = TRUE;
    }
  else
    gtk_widget_hide (dialog);

  return ret;
}

// FIXME
static UCCookiesAction cookie_warning_action;

static gboolean
uc_application_cookie_warning_dialog_not_display_cb (GtkToggleButton *button,
                                                     gpointer user_data)
{
  gboolean val = !gtk_toggle_button_get_active (button);


  switch (cookie_warning_action)
  {
    // Block Add warn
    case UC_COOKIES_ACTION_ADD:
      uc_project_set_cookies_warn_added (val);
      break;
    // Block Update warn
    case UC_COOKIES_ACTION_UPDATE:
      uc_project_set_cookies_warn_updated (val);
      break;
    // Block Delete warn
    case UC_COOKIES_ACTION_DELETE:
      uc_project_set_cookies_warn_deleted (val);
      break;
    default:
      g_assert_not_reached ();
  }

  return FALSE;
}

/**
 * uc_application_cookie_warning_dialog_show:
 * @server: hostname.
 * @page: page name.
 * @label: dialog's label.
 * @name: cookie's name.
 * @value: cookie's value.
 * @path: cookie's path.
 * @expires: expirationdate.
 * @action: cookies action.
 *
 * Dialog for accepting/refusing a cookie.
 *
 * Returns: %TRUE if the cookie must be accepted.
 */
gboolean
uc_application_cookie_warning_dialog_show (const gchar * server,
					   const gchar * page,
					   const gchar * label,
					   gchar ** name,
					   gchar ** value,
					   gchar ** path,
					   gchar ** expires,
					   const UCCookiesAction action)
{
  GtkWidget *widget = NULL;
  GtkWidget *w = NULL;
  gint ret = 0;
  static gulong handler_id = 0;


  cookie_warning_action = action;

  w = WGET ("cookie_warning_dialog");

  widget = WGET ("cwd_bt_ok");
  gtk_window_set_focus (GTK_WINDOW (w), widget);

  widget = WGET ("cwd_server");
  gtk_entry_set_text (GTK_ENTRY (widget), server);
  widget = WGET ("cwd_page");
  gtk_entry_set_text (GTK_ENTRY (widget), page);

  widget = WGET ("cwd_label");
  gtk_label_set_markup (GTK_LABEL (widget), label);
  widget = WGET ("cwd_expires");
  gtk_entry_set_text (GTK_ENTRY (widget), (*expires) ?
		      *expires : _("At the end of this session"));
  widget = WGET ("cwd_name");
  gtk_entry_set_text (GTK_ENTRY (widget), *name);
  widget = WGET ("cwd_value");
  gtk_entry_set_text (GTK_ENTRY (widget), *value);
  widget = WGET ("cwd_path");
  gtk_entry_set_text (GTK_ENTRY (widget), *path);

  widget = WGET ("cwd_stop_display");

  WSENS ("cwd_name", TRUE);
  WSENS ("cwd_path", TRUE);
  WSENS ("cwd_value", TRUE);
  WSENS ("cwd_expires", TRUE);

  if (!handler_id)
    handler_id = g_signal_connect (G_OBJECT (widget), "toggled",
      G_CALLBACK (uc_application_cookie_warning_dialog_not_display_cb), NULL);

  g_signal_handler_block ((gpointer) widget, handler_id);

  if (action == UC_COOKIES_ACTION_ADD)
  {
    gtk_toggle_button_set_active  (GTK_TOGGLE_BUTTON (widget),
                                   !uc_project_get_cookies_warn_added ());
  }
  if (action == UC_COOKIES_ACTION_UPDATE)
  {
    gtk_toggle_button_set_active  (GTK_TOGGLE_BUTTON (widget),
                                   !uc_project_get_cookies_warn_updated ());
    WSENS ("cwd_name", FALSE);
    WSENS ("cwd_path", FALSE);
  }
  else if (action == UC_COOKIES_ACTION_DELETE)
  {
    gtk_toggle_button_set_active  (GTK_TOGGLE_BUTTON (widget),
                                   !uc_project_get_cookies_warn_deleted ());
    WSENS ("cwd_name", FALSE);
    WSENS ("cwd_path", FALSE);
    WSENS ("cwd_value", FALSE);
    WSENS ("cwd_expires", FALSE);
  }

  g_signal_handler_unblock ((gpointer) widget, handler_id);

  ret = gtk_dialog_run (GTK_DIALOG (w));

  /* cancel check */
  if (ret == GTK_RESPONSE_CANCEL)
    uc_check_cancel_set_value (TRUE);

  if (ret == GTK_RESPONSE_ACCEPT && action != UC_COOKIES_ACTION_DELETE)
    {
      g_free (*name), *name = NULL;
      widget = WGET ("cwd_name");
      *name = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));
      g_free (*value), *value = NULL;
      widget = WGET ("cwd_value");
      *value = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));
      g_free (*path), *path = NULL;
      widget = WGET ("cwd_path");
      *path = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));
      widget = WGET ("cwd_expires");
      if (strcmp (gtk_entry_get_text (GTK_ENTRY (widget)),
		  _("At the end of this session")) != 0)
	{
	  g_free (*expires), *expires = NULL;
	  *expires = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));
	}
    }

  gtk_widget_hide (w);

  return (ret == GTK_RESPONSE_ACCEPT);
}


/**
 * uc_application_auth_dialog_show:
 * @title: string returned by the remote host when
 *         requested authentication.
 *
 * Show the basic authentication dialog.
 *
 * Returns: TRUE if both user and password have been
 *          entered.
 *
 */
gboolean
uc_application_auth_dialog_show (const gchar * title, const gchar *host)
{
  GtkWidget *widget = NULL;
  GtkWidget *w = NULL;
  gchar *tmp = NULL;
  gboolean ret = FALSE;


  w = WGET ("auth_dialog");
  if (title)
  {
    gchar **real_title = g_strsplit (title, "=", 2);
    tmp = g_strdup_printf (_("Enter login and password for %s at %s"),
                           real_title[1], host);

    widget = WGET ("ad_title");
    gtk_label_set_text (GTK_LABEL (widget), tmp);

    g_free (tmp), tmp = NULL;
    g_strfreev (real_title), real_title = NULL;
  }

  gtk_widget_show_all (w);

again:
      
  widget = WGET ("ad_stop_display");
  uc_project_set_prompt_auth (
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));

  if (gtk_dialog_run (GTK_DIALOG (w)) == GTK_RESPONSE_OK)
  {
    gchar *auth_user = NULL;
    gchar *auth_password = NULL;
    gchar *auth_line = NULL;


    if (uc_utils_get_auth_fields (glade, WGET ("ad_auth_user"),
                                  WGET ("ad_auth_password"), &auth_user,
                                  &auth_password, &auth_line))
    {
       uc_project_set_auth_line (auth_line);
       g_free (auth_line), auth_line = NULL;
       ret = TRUE;
    }
    else if (uc_project_get_prompt_auth ())
      goto again;
  }

  gtk_widget_hide (w);

  return ret;
}


/**
 * uc_application_new_search_dialog_show:
 * 
 * Show the search dialog.
 */
void
uc_application_new_search_dialog_show (void)
{
  GtkWidget *dialog = NULL;

  dialog = WGET ("new_search_dialog");

/*
  WSENS ("nsd_search_status_code", FALSE);
  WSENS ("nsd_search_links_name", FALSE);
  WSENS ("nsd_search_meta_content", FALSE);
  WSENS ("nsd_search_emails", FALSE);
*/

  gtk_widget_show_all (dialog);
}

/**
 * uc_application_search_dialog_show:
 * 
 * Show the search dialog.
 */
void
uc_application_search_dialog_show (void)
{
  GtkWidget *dialog = NULL;

  dialog = WGET ("search_dialog");

  gtk_widget_show_all (dialog);
}

void
uc_application_w3c_validate (void)
{
  UCLinkProperties *prop = NULL;
  gint32 id = 0;
  gsize size = 0;
  gchar *buffer = NULL;
  gchar *path = NULL;
  gchar *content_type = NULL;
  gboolean validate = TRUE;

  id = uc_check_treeview_get_selected_row_id ();
  if (id <= 0)
    return;

  validate = TRUE;

  prop = uc_lists_checked_links_lookup_by_uid (id);
  content_type = g_hash_table_lookup (prop->header, UC_CONTENT_TYPE);

  path = uc_utils_convert_uid2file (id);
  buffer = uc_utils_get_file_content (path, &size);
  g_free (path), path = NULL;

#ifdef ENABLE_CROCO
  if (validate
      && uc_check_content_type_w3c_accepted ("css", prop->path, content_type))
    {
      UCCroco *croco = NULL;
      croco = uc_uccroco_new ();
      if (croco && uc_uccroco_validate (croco, buffer) >= 0)
	{
	  if (croco->error_count)
	    uc_application_buffer_show (
            _("W3C CSS validation result (libcroco)"),
            (const gchar *) croco->error_buffer->str);
	  else
	    uc_application_buffer_show (
              _	("W3C CSS validation result (libcroco)"),
              _("This is a valid stylesheet."));
	}
      else
	uc_application_dialog_show (
          _("A problem occured while using Croco library.\n"
            "<i>Please check your configuration or your libcroco "
            "version</i>.\n\n"), GTK_MESSAGE_ERROR);

      uc_uccroco_free (&croco);

      validate = FALSE;
    }
#endif

#ifdef ENABLE_TIDY
  if (validate
      && uc_check_content_type_w3c_accepted ("html", prop->path,
					     content_type))
    {
      UCTidy *tidy = NULL;
      tidy = uc_uctidy_new ();
      if (tidy && uc_uctidy_validate (tidy, buffer) >= 0)
	uc_application_buffer_show (
          _("W3C HTML validation result (libtidy)"),
         (const gchar *) tidy->errbuf.bp);
      else
	uc_application_dialog_show (
          _("A problem occured while using Tidy library.\n"
            "<i>Please check your configuration or your libtidy "
            "version</i>.\n\n"), GTK_MESSAGE_ERROR);

      uc_uctidy_free (&tidy);

      validate = FALSE;
    }
#endif

  g_free (buffer), buffer = NULL;
}

/**
 * uc_application_page_information_dialog_show:
 * 
 * Show the properties dialog.
 */
void
uc_application_page_information_dialog_show (void)
{
  gint32 id = 0;
  UCLinkProperties *prop = NULL;
  GtkWidget *widget = NULL;
  GtkWidget *dialog = NULL;

  id = uc_check_treeview_get_selected_row_id ();
  if (id <= 0)
    return;

  prop = uc_lists_checked_links_lookup_by_uid (id);

  /* do not display link information for bookmark's
   * folders */
  if (prop->link_type == LINK_TYPE_BOOKMARK_FOLDER)
    return;

  dialog = WGET ("page_information_dialog");

  uc_application_page_information_dialog_display_header (prop);

  if (prop->metas != NULL)
    {
      widget = WGET ("pid_meta_label");
      gtk_widget_show (widget);

      widget = WGET ("pid_meta_box");
      gtk_widget_show (widget);

      uc_application_page_information_dialog_display_meta_tags (prop);
    }
  else
    {
      widget = WGET ("pid_meta_label");
      gtk_widget_hide (widget);

      widget = WGET ("pid_meta_box");
      gtk_widget_hide (widget);
    }

  widget = WGET ("pid_information_notebook");
  gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), 0);
  uc_utils_clear_container (GTK_CONTAINER
			    (gtk_notebook_get_nth_page
			     (GTK_NOTEBOOK (widget), 1)));
  uc_utils_clear_container (GTK_CONTAINER
			    (gtk_notebook_get_nth_page
			     (GTK_NOTEBOOK (widget), 2)));

  gtk_widget_show (gtk_notebook_get_nth_page (GTK_NOTEBOOK (widget), 1));
  gtk_widget_show (gtk_notebook_get_nth_page (GTK_NOTEBOOK (widget), 2));

  if (prop->link_type == LINK_TYPE_IMAGE ||
      prop->link_type == LINK_TYPE_EMAIL || prop->all_links == NULL)
    {
      gtk_widget_hide (gtk_notebook_get_nth_page (GTK_NOTEBOOK (widget), 2));
      gtk_widget_hide (gtk_notebook_get_nth_page (GTK_NOTEBOOK (widget), 1));
    }
  else
    {
      uc_application_page_information_dialog_display_links (prop);

      if (prop->emails != NULL)
	uc_application_page_information_dialog_display_emails (prop);
      else
	gtk_widget_hide (gtk_notebook_get_nth_page
			 (GTK_NOTEBOOK (widget), 2));
    }

  gtk_widget_show (dialog);
}

/**
 * uc_application_display_search_message:
 * @label_pos: Position of the given message.
 * @message: Message to display.
 * 
 * Display messages in the progess dialog during
 * the search.
 */
void
uc_application_display_search_message (const guint label_pos,
				       const gchar * message)
{
  GtkWidget *widget = NULL;
  gchar *tmp = NULL;

  if (uc_search_get_exit () || !message)
    return;

  tmp = (label_pos != UC_CHECK_MESSAGE_LABEL_FIRST) ?
    uc_utils_string_format4display (message, UC_LABEL_DISPLAY_MAX_LEN) :
    g_strdup (message);

  switch (label_pos)
    {
    case UC_CHECK_MESSAGE_LABEL_FIRST:
      widget = WGET ("sd_search_label_first");
      break;

    case UC_CHECK_MESSAGE_LABEL_SECOND:
      widget = WGET ("sd_search_label_second");
      break;

    default:
      g_free (tmp), tmp = NULL;
      g_return_if_reached ();
    }

  if (label_pos == UC_CHECK_MESSAGE_LABEL_FIRST)
    gtk_label_set_markup (GTK_LABEL (widget), tmp);
  else
    gtk_label_set (GTK_LABEL (widget), tmp);

  g_free (tmp), tmp = NULL;
}

/**
 * uc_application_display_state_message:
 * @label_pos: Position of the given message.
 * @message: Message to display.
 * 
 * Display messages in the progess dialog during
 * the check process
 */
void
uc_application_display_state_message (const guint label_pos,
				      const gchar * message)
{
  GtkWidget *widget = NULL;
  gchar *tmp = NULL;

  if (!message)
    return;

  tmp = (label_pos != UC_CHECK_MESSAGE_LABEL_THIRD) ?
    uc_utils_string_format4display (message, UC_LABEL_DISPLAY_MAX_LEN) :
    g_strdup (message);

  switch (label_pos)
    {
    case UC_CHECK_MESSAGE_LABEL_FIRST:
      widget = WGET ("pd_first_label");
      break;

    case UC_CHECK_MESSAGE_LABEL_SECOND:
      widget = WGET ("pd_second_label");
      break;

    case UC_CHECK_MESSAGE_LABEL_THIRD:
      widget = WGET ("pd_third_label");
      break;

    default:
      g_free (tmp), tmp = NULL;
      g_return_if_reached ();
    }

  if (label_pos == UC_CHECK_MESSAGE_LABEL_THIRD)
    gtk_label_set_markup (GTK_LABEL (widget), tmp);
  else
    gtk_label_set (GTK_LABEL (widget), tmp);

  g_free (tmp), tmp = NULL;
}

/**
 * uc_application_display_informations:
 * @treeview: Tree view to work with.
 * 
 * Search the header of a given url
 * in the urls list to display it.
 */
void
uc_application_display_informations (GtkTreeView * treeview)
{
  gint32 id = 0;
  UCLinkProperties *prop = NULL;
  UCStatusCode *sc = NULL;
  gchar *tmp_str = NULL;
  GtkTextBuffer *buffer_information = NULL;
  GtkTextBuffer *buffer_security = NULL;
  GtkWidget *text_informations = NULL;
  GtkWidget *security = NULL;
  GtkWidget *location = NULL;
  GtkWidget *referrer = NULL;

  id = uc_check_treeview_get_selected_row_id ();
  if (id < 0)
    return;

  prop = uc_lists_checked_links_lookup_by_uid (id);

  text_informations = WGET ("mw_text_informations");
  security = WGET ("mw_security");
  location = WGET ("mw_location");
  referrer = WGET ("mw_referrer");

  /* empty the buffer if the selected item is a bookmark's
   * folder */
  buffer_information =
    gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_informations));
  buffer_security = gtk_text_view_get_buffer (GTK_TEXT_VIEW (security));
  if (prop->link_type != LINK_TYPE_BOOKMARK_FOLDER)
    {
      const gchar *status_code = g_hash_table_lookup (prop->header,
						      UC_HEADER_STATUS);
      sc = uc_application_get_status_code_properties (status_code);

      tmp_str = g_strconcat ("<b><i>", sc->message, "</i></b> (", status_code,
			     ")", NULL);
      gtk_label_set_markup (GTK_LABEL
			    (WGET ("mw_label_informations_message")),
			    tmp_str);
      g_free (tmp_str), tmp_str = NULL;

      if (prop->bad_extensions)
	tmp_str = g_strdup (_("Found Bad extensions.\n"));
      else if (prop->virii)
	tmp_str = g_strconcat (_("Found Virus: "), prop->virname, NULL);
      else
	tmp_str = g_strdup ("");

      gtk_text_buffer_set_text (buffer_security, tmp_str, -1);

      g_free (tmp_str), tmp_str = NULL;

      gtk_entry_set_text (GTK_ENTRY (location), prop->url);
      gtk_entry_set_text (GTK_ENTRY (referrer),
			  (prop->parent && prop->parent->url) ?
			  prop->parent->url : _("No referrer"));

      gtk_text_buffer_set_text (buffer_information, sc->description, -1);
    }
  else
    {
      gtk_label_set (GTK_LABEL (WGET ("mw_label_informations_message")), " ");
      gtk_text_buffer_set_text (buffer_security, " ", -1);
      gtk_entry_set_text (GTK_ENTRY (location), " ");
      gtk_entry_set_text (GTK_ENTRY (referrer), " ");
      gtk_text_buffer_set_text (buffer_information, " ", -1);
    }
}

/**
 * uc_application_treeview_activate_popup:
 * @event: Event.
 * 
 * Display the treview popup menu.
 */
void
uc_application_treeview_activate_popup (GdkEventButton * event)
{
  GtkWidget *popup = NULL;
  gchar *status_code = NULL;
  gchar *content_type = NULL;
  gint32 id = 0;
  UCLinkProperties *prop = NULL;


  id = uc_check_treeview_get_selected_row_id ();
  if (id < 0)
    return;

  prop = uc_lists_checked_links_lookup_by_uid (id);

  // Do not display information for bookmarks folders
  if (prop->link_type == LINK_TYPE_BOOKMARK_FOLDER)
    return;

  status_code = g_hash_table_lookup (prop->header, UC_HEADER_STATUS);
  content_type = g_hash_table_lookup (prop->header, UC_CONTENT_TYPE);

  popup = WGET ("treeview_popup_menu");

  gtk_menu_popup (GTK_MENU (popup), NULL, NULL, NULL, NULL,
		  event->button, event->time);

  // Activate/deactivate appropriate items
  WSENS ("mwpopup_menu_check_email_mx", TRUE);
  WSENS ("mwpopup_view_online", TRUE);
  WSENS ("mwpopup_refresh_link", TRUE);
  WSENS ("mwpopup_view_link_content", TRUE);
  WSENS ("mwpopup_w3c_validate", FALSE);
  WSENS ("mwpopup_image_preview", TRUE);
  WSENS ("mwpopup_view_parent_page_online", TRUE);
  WSENS ("mwpopup_delete_link", FALSE);

  WSENS ("mwpopup_view_bad_extensions",
    (prop->bad_extensions != NULL));
  WSENS ("mwpopup_view_similar_links_locations",
    (prop->similar_links_parents != NULL));
  WSENS ("mwpopup_refresh_branch",
    (prop->childs != NULL));
  WSENS ("mwpopup_refresh_parent",
    (prop->depth_level != 0));

  if (prop->link_type != LINK_TYPE_EMAIL)
    WSENS ("mwpopup_menu_check_email_mx", FALSE);
  else
  {
    WSENS ("mwpopup_view_online", FALSE);
    WSENS ("mwpopup_refresh_link", FALSE);
  }

  if (!uc_check_status_is_good (status_code) ||
      !prop->is_downloadable ||
      (prop->link_type != LINK_TYPE_HREF &&
       prop->link_type != LINK_TYPE_FRAME
       && prop->link_type != LINK_TYPE_CSS))
    WSENS ("mwpopup_view_link_content", FALSE);
  else if (uc_project_get_w3c_checks ("any"))
    {
      if ((uc_project_get_w3c_checks ("css") &&
	   uc_check_content_type_w3c_accepted ("css", prop->path,
					       content_type))
	  || (uc_project_get_w3c_checks ("html")
	      && uc_check_content_type_w3c_accepted ("html", prop->path,
						     content_type)))
	WSENS ("mwpopup_w3c_validate", TRUE);
    }

  if (uc_project_get_speed_check () ||
      !uc_project_get_download_images_content () ||
      (prop->link_type != LINK_TYPE_IMAGE ||
       !uc_check_status_is_good (status_code)))
    WSENS ("mwpopup_image_preview", FALSE);
  

  if (uc_project_get_check_is_bookmarks ())
    {
      WSENS ("mwpopup_refresh_parent", FALSE);
      WSENS ("mwpopup_view_similar_links_locations", FALSE);
      WSENS ("mwpopup_view_parent_page_online", FALSE);
      WSENS ("mwpopup_delete_link", TRUE);
    }

  if (uc_check_status_is_malformed (status_code))
    {
      WSENS ("mwpopup_view_online", FALSE);
      WSENS ("mwpopup_refresh_link", FALSE);
    }

#ifndef ENABLE_GNUTLS
  // Do not refresh HTTPS links if no SSL support
  if (strcmp (prop->protocol, UC_PROTOCOL_HTTPS) == 0)
  {
    WSENS ("mwpopup_refresh_link", FALSE);
    WSENS ("mwpopup_refresh_branch", FALSE);
    WSENS ("mwpopup_refresh_parent", FALSE);
  }
#endif

  gtk_widget_show_all (popup);
}


/**
 * uc_application_remove_paths:
 * 
 * Remove all application paths
 */
void
uc_application_remove_paths (void)
{
  gchar *final_path = NULL;


  final_path = g_strconcat (uc_project_get_working_path (), "/",
                            uc_project_get_cache_name (), NULL);

  uc_utils_rmdirs (final_path, FALSE);

  g_free (final_path), final_path = NULL;
}


/**
 * uc_application_settings_dialog_display_filters_list:
 * @glade: the appropriate glade pointer.
 * @elements: a vector of items to display.
 * @items_size: size of the vector.
 * @tv: tree view to display to work with.
 * @model: list store to work with.
 * 
 * Display list of elements to reject.
 */
static void
uc_application_settings_dialog_display_filters_list (const gchar *
						     glade_name,
						     gpointer elements,
						     guint items_size,
						     GtkTreeView ** tv,
						     GtkListStore ** model)
{
  enum
  {
    NAME_COLUMN,
    N_COLUMNS
  };

  gchar **items = NULL;
  GtkWidget *scroll = NULL;
  GtkTreeIter iter;
  GtkCellRenderer *renderer;
  guint i = 0;

  scroll = WGET (glade_name);

  if (*model != NULL)
    g_object_unref (*model), *model = NULL;

  *model = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING);

  if (elements != NULL)
    {
      items = (gchar **) elements;

      for (i = 0; i < items_size; i++)
	{
	  gtk_list_store_append (*model, &iter);
	  gtk_list_store_set (*model, &iter, NAME_COLUMN,
			      (gchar *) items[i], -1);
	}
    }

  if (*tv && gtk_tree_view_get_model (*tv))
    gtk_widget_destroy (GTK_WIDGET (*tv));

  *tv =
    GTK_TREE_VIEW (gtk_tree_view_new_with_model (GTK_TREE_MODEL (*model)));

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (*tv),
					       -1,
					       _("Name"), renderer,
					       "text", NAME_COLUMN, NULL);
  uc_utils_set_userfriendly_treeview_column (*tv, NAME_COLUMN);

  gtk_tree_view_set_rules_hint (*tv, TRUE);

  gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET (*tv));

  gtk_widget_show_all (scroll);
}

/**
 * uc_application_page_information_dialog_display_header:
 * @prop: #UCLinkProperties node of the element to work with.
 * 
 * Display header information.
 */
static void
uc_application_page_information_dialog_display_header (const
						       UCLinkProperties *
						       prop)
{
  GtkWidget *widget = NULL;
  gchar *tmp = NULL;
  guint status_tmp = 0;

  /* URL */
  widget = WGET ("pid_location");
  tmp = (strlen (prop->url) > 0) ?
    g_strdup (prop->url) : g_strdup (_("Missing URL"));
  gtk_entry_set_text (GTK_ENTRY (widget), tmp);
  g_free (tmp), tmp = NULL;

  /* Link type */
  widget = WGET ("pid_link_type");
  tmp = g_strdup_printf ("(%s)",
			 uc_check_get_link_type_label (prop->link_type));
  gtk_label_set (GTK_LABEL (widget), tmp);
  g_free (tmp), tmp = NULL;
  widget = WGET ("pid_link_type_image");
  gtk_image_set_from_file (GTK_IMAGE (widget),
			   uc_check_get_link_type_icon_path (prop->link_type,
							     prop->protocol));

  /* referring URL */
  widget = WGET ("pid_referrer");

  /* there is no parent url for a local file or bookmark */
  if (!uc_project_get_check_is_bookmarks ())
    tmp = (prop->parent && prop->parent->url) ?
      g_strdup (prop->parent->url) : g_strdup (_("No referrer"));
  else
    tmp = g_strdup (_("No referrer"));
  gtk_entry_set_text (GTK_ENTRY (widget), tmp);
  g_free (tmp), tmp = NULL;

  /* server type */
  widget = WGET ("pid_server_type");
  tmp = g_hash_table_lookup (prop->header, UC_SERVER);
  tmp = (tmp == NULL) ?
    g_strdup (_("Not specified")) :
    uc_utils_get_server_from_header_field (tmp);
  gtk_label_set (GTK_LABEL (widget), tmp);
  g_free (tmp), tmp = NULL;

  /* status */
  widget = WGET ("pid_status_image");
  gtk_image_set_from_pixbuf (GTK_IMAGE (widget), prop->status_icon);
  widget = WGET ("pid_status");
  if ((tmp = g_hash_table_lookup (prop->header, UC_HEADER_STATUS)) == NULL)
    tmp = g_strdup_printf ("(%s)", _("Unknown"));
  else
    {
      status_tmp = atoi (tmp);
      if (status_tmp >= UC_STATUS_CODE_INTERNALS_LIMIT)
	tmp = g_strdup (_("(gURLChecker internals)"));
      else
	tmp = g_strdup_printf ("(%s)", tmp);
    }
  gtk_label_set (GTK_LABEL (widget), tmp);
  g_free (tmp), tmp = NULL;

  /* cached */
  widget = WGET ("pid_source");
  gtk_label_set (GTK_LABEL (widget), (prop->is_downloadable) ?
		 _("Disk Cache") : _("Not cached"));
  g_free (tmp), tmp = NULL;

  /* content length */
  widget = WGET ("pid_size");
  tmp = g_hash_table_lookup (prop->header, UC_CONTENT_LENGTH);
  if (tmp == NULL)
    tmp = g_strdup (_("Not specified"));
  else
    tmp = uc_utils_get_string_from_size (atol (tmp));
  gtk_label_set (GTK_LABEL (widget), tmp);
  g_free (tmp), tmp = NULL;

  /* last modified */
  widget = WGET ("pid_modified");
  tmp = g_strdup (g_hash_table_lookup (prop->header, UC_LAST_MODIFIED));
  if (tmp == NULL)
    tmp = g_strdup (_("Not specified"));
  gtk_label_set (GTK_LABEL (widget), tmp);
  g_free (tmp), tmp = NULL;
}

/**
 * uc_application_page_information_dialog_display_emails:
 * @prop: #UCLinkProperties node of the element to work with.
 * 
 * Display page information emails list.
 */
static void
uc_application_page_information_dialog_display_emails (const
						       UCLinkProperties *
						       prop)
{
  enum
  {
    LABEL_COLUMN,
    URL_COLUMN,
    N_COLUMNS
  };
  GtkTreeView *tv;
  GtkTreeIter iter;
  GtkListStore *model;
  GtkCellRenderer *renderer;
  GList *item = NULL;
  GtkWidget *scroll = NULL;

  scroll = WGET ("pid_emails_scrolled");

  model = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);

  item = g_list_first (prop->emails);
  while (item != NULL)
    {
      UCHTMLTag *link = (UCHTMLTag *) item->data;

      gtk_list_store_append (model, &iter);
      gtk_list_store_set (model, &iter,
			  URL_COLUMN, link->value,
			  LABEL_COLUMN, link->label, -1);

      item = g_list_next (item);
    }

  tv = GTK_TREE_VIEW (gtk_tree_view_new_with_model (GTK_TREE_MODEL (model)));
  g_object_unref (model), model = NULL;

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (tv,
					       -1,
					       _("E-Mail"), renderer,
					       "text", URL_COLUMN, NULL);
  uc_utils_set_userfriendly_treeview_column (tv, URL_COLUMN);

  gtk_tree_view_insert_column_with_attributes (tv,
					       -1,
					       _("Label"), renderer,
					       "text", LABEL_COLUMN, NULL);
  uc_utils_set_userfriendly_treeview_column (tv, LABEL_COLUMN);

  gtk_tree_view_set_rules_hint (tv, TRUE);

  gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET (tv));

  gtk_widget_show_all (scroll);
}

/**
 * uc_application_page_information_dialog_display_links:
 * @prop: #UCLinkProperties node of the element to work with.
 * 
 * Display page information urls list.
 */
static void
uc_application_page_information_dialog_display_links (const
						      UCLinkProperties * prop)
{
  enum
  {
    LABEL_COLUMN,
    URL_COLUMN,
    N_COLUMNS
  };
  GtkTreeView *tv;
  GtkTreeIter iter;
  GtkListStore *model;
  GtkCellRenderer *renderer;
  GList *item = NULL;
  GtkWidget *scroll = NULL;
  gchar *label = NULL;
  gchar *value = NULL;


  scroll = WGET ("pid_links_scrolled");

  model = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
  gtk_list_store_clear (model);

  item = g_list_first (prop->all_links);
  while (item != NULL)
    {
      UCHTMLTag *link = (UCHTMLTag *) item->data;

      label =
	uc_utils_string_format4display (link->label,
					UC_LABEL_DISPLAY_MAX_LEN);
      value =
	uc_utils_string_format4display (link->value,
					UC_LABEL_DISPLAY_MAX_LEN);

      gtk_list_store_append (model, &iter);
      gtk_list_store_set (model, &iter,
			  URL_COLUMN, value, LABEL_COLUMN, label, -1);

      g_free (value), value = NULL;
      g_free (label), label = NULL;

      item = g_list_next (item);
    }

  tv = GTK_TREE_VIEW (gtk_tree_view_new_with_model (GTK_TREE_MODEL (model)));
  g_object_unref (model), model = NULL;

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (tv,
					       -1,
					       _("URL"), renderer,
					       "text", URL_COLUMN, NULL);
  uc_utils_set_userfriendly_treeview_column (tv, URL_COLUMN);

  gtk_tree_view_insert_column_with_attributes (tv,
					       -1,
					       _("Label"), renderer,
					       "text", LABEL_COLUMN, NULL);
  uc_utils_set_userfriendly_treeview_column (tv, LABEL_COLUMN);

  gtk_tree_view_set_rules_hint (tv, TRUE);

  gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET (tv));

  gtk_widget_show_all (scroll);
}

/**
 * uc_application_view_bad_extensions_dialog_show:
 * 
 * Show bad extensions dialog.
 */
void
uc_application_view_bad_extensions_dialog_show (void)
{
  GtkTreeView *tv;
  GtkTreeIter iter;
  GtkListStore *model;
  GtkCellRenderer *renderer;
  GList *item = NULL;
  UCLinkProperties *prop = NULL;
  GtkWidget *scrolled_bad_extensions = NULL;
  gint32 id = 0;
  GtkWidget *dialog = NULL;

  id = uc_check_treeview_get_selected_row_id ();
  if (id < 0)
    {
      uc_application_dialog_show (_
				  ("Please, select a item."),
				  GTK_MESSAGE_WARNING);
      return;
    }

  dialog = WGET ("bad_extensions_dialog");
  scrolled_bad_extensions = WGET ("scrolled_bad_extensions");

  model = gtk_list_store_new (1, G_TYPE_STRING);

  prop = uc_lists_checked_links_lookup_by_uid (id);

  item = g_list_first (prop->bad_extensions);
  while (item != NULL)
    {
      gchar *str = (gchar *) item->data;

      item = g_list_next (item);

      gtk_list_store_append (model, &iter);
      gtk_list_store_set (model, &iter, 0, str, -1);
    }

  tv = GTK_TREE_VIEW (gtk_tree_view_new_with_model (GTK_TREE_MODEL (model)));
  g_object_unref (model), model = NULL;

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (tv,
					       -1, _("URL"),
					       renderer, "text", 0, NULL);
  uc_utils_set_userfriendly_treeview_column (tv, 0);

  gtk_tree_view_set_rules_hint (tv, TRUE);

  uc_utils_clear_container (GTK_CONTAINER (scrolled_bad_extensions));
  gtk_container_add (GTK_CONTAINER (scrolled_bad_extensions),
		     GTK_WIDGET (tv));

  gtk_widget_show_all (dialog);
}

/**
 * uc_application_view_similar_links_dialog_show:
 * 
 * Show similar links dialog.
 */
void
uc_application_view_similar_links_dialog_show (void)
{
  GtkTreeView *tv;
  GtkTreeIter iter;
  GtkListStore *model;
  GtkCellRenderer *renderer;
  GList *item = NULL;
  UCLinkProperties *prop = NULL;
  GtkWidget *scrolled_similar_links = NULL;
  gint32 id = 0;
  GtkWidget *dialog = NULL;

  id = uc_check_treeview_get_selected_row_id ();
  if (id < 0)
    {
      uc_application_dialog_show (_
				  ("Please, select a item."),
				  GTK_MESSAGE_WARNING);
      return;
    }

  dialog = WGET ("similar_links_dialog");
  scrolled_similar_links = WGET ("sld_scrolled_similar_links");

  model = gtk_list_store_new (1, G_TYPE_STRING);

  prop = uc_lists_checked_links_lookup_by_uid (id);

  item = g_list_first (prop->similar_links_parents);
  while (item != NULL)
    {
      gchar *str = (gchar *) item->data;

      item = g_list_next (item);

      gtk_list_store_append (model, &iter);
      gtk_list_store_set (model, &iter, 0, str, -1);
    }

  tv = GTK_TREE_VIEW (gtk_tree_view_new_with_model (GTK_TREE_MODEL (model)));
  g_object_unref (model), model = NULL;

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (tv,
					       -1, _("URL"),
					       renderer, "text", 0, NULL);
  uc_utils_set_userfriendly_treeview_column (tv, 0);

  gtk_tree_view_set_rules_hint (tv, TRUE);

  uc_utils_clear_container (GTK_CONTAINER (scrolled_similar_links));
  gtk_container_add (GTK_CONTAINER (scrolled_similar_links), GTK_WIDGET (tv));

  gtk_widget_show_all (dialog);
}


/**
 * uc_application_launch_web_browser:
 * @url: URL to pass to the web browser.
 * 
 * Launch a web browser to view the selected
 * url
 */
void
uc_application_launch_web_browser (const gchar * url)
{
  gchar *message = NULL;
  GError *err = NULL;
  gchar *command_line = NULL;
  gchar *browser = NULL;
  gint32 ret = 0;


  browser = uc_project_get_browser_path ();
  command_line = g_strconcat (browser, " \"", url, "\"", NULL);
  ret = g_spawn_command_line_async (command_line, &err);
  g_free (command_line), command_line = NULL;

  if (ret == 0)
  {
    message = g_strdup_printf (
      "%s\n\n%s",
      _("Cannot start your web browser\n"
        "The following error occured:\n"),
      err->message);

    uc_application_dialog_show (message, GTK_MESSAGE_ERROR);

    g_clear_error (&err);
    g_free (message), message = NULL;
  }
}


/**
 * uc_application_view_image_dialog_show:
 * 
 * Show view image dialog.
 */
void
uc_application_view_image_dialog_show (void)
{
  gint32 id = 0;
  gchar *src = NULL;
  GtkWidget *dialog = NULL;

  id = uc_check_treeview_get_selected_row_id ();
  if (id < 0)
    {
      uc_application_dialog_show (_
				  ("Please, select a item."),
				  GTK_MESSAGE_WARNING);

      return;
    }

  src = uc_utils_convert_uid2file (id);
  if (src != NULL)
    {
      GtkImage *image_preview = NULL;

      dialog = WGET ("view_image_dialog");

      image_preview = GTK_IMAGE (WGET ("vid_image_preview"));

      gtk_image_set_from_file (image_preview, src);

      g_free (src), src = NULL;

      gtk_widget_show_all (dialog);
    }
  else
    uc_application_dialog_show (_("Either the corresponding "
				  "file is empty or does not exist "
				  "in the cache directory!"),
				GTK_MESSAGE_ERROR);
}

/**
 * uc_application_view_source_dialog_show:
 * 
 * Show view source dialog.
 */
void
uc_application_view_source_dialog_show (void)
{
  gint32 id = 0;
  gchar *src = NULL;
  gsize length = 0;

  id = uc_check_treeview_get_selected_row_id ();
  if (id < 0)
    {
      uc_application_dialog_show (_
				  ("Please, select a item."),
				  GTK_MESSAGE_WARNING);

      return;
    }

  src = uc_cache_get_source (id, &length);
  if (src)
    uc_application_buffer_show (_("View source"), src);
  else
    uc_application_dialog_show (_("Either the corresponding "
				  "file is empty or does not exist "
				  "in the cache directory!"),
				GTK_MESSAGE_ERROR);

  g_free (src), src = NULL;
}

/**
 * uc_application_urls_user_actions_free_cb:
 * 
 * Free the bad URLs actions hash table.
 */
static void
uc_application_urls_user_actions_free_cb (gpointer data)
{
  UCURLsUserActions *a = (UCURLsUserActions *) data;

  g_free (a->id), a->id = NULL;
  g_free (a->label), a->label = NULL;

  g_free (a), a = NULL;
}

/**
 * uc_application_urls_user_actions_init
 * 
 * This function load the actions tp associate with bad URLs.
 */
void
uc_application_urls_user_actions_init (void)
{
  guint i = 0;
  struct
  {
    gchar *id;
    gchar *label;
  }
  UCActions[] =
  {
    {"none", " "},
    {"redirect", _("Redirect")},
    {"edit", _("Edit")},
    {"create-target", _("Create target")}
  };
  const guint actions_len = G_N_ELEMENTS (UCActions);

  if (uc_application_urls_user_actions_hash != NULL)
    g_hash_table_destroy (uc_application_urls_user_actions_hash),
      uc_application_urls_user_actions_hash = NULL;

  uc_application_urls_user_actions_hash =
    g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
			   (GDestroyNotify)
			   uc_application_urls_user_actions_free_cb);

  /* initialization of the hash table for bad URLs actions */
  for (i = 0; i < actions_len; i++)
    {
      UCURLsUserActions *a = NULL;

      a = g_new0 (UCURLsUserActions, 1);
      a->id = g_strdup (UCActions[i].id);
      a->label = g_strdup (UCActions[i].label);
      g_hash_table_replace (uc_application_urls_user_actions_hash,
			    g_strdup (UCActions[i].id), (gpointer) a);
    }
}

/**
 * uc_application_status_code_properties_free_cb:
 * 
 * Free the code properties hash table.
 */
static void
uc_application_status_code_properties_free_cb (gpointer data)
{
  UCStatusCode *sc = (UCStatusCode *) data;

  g_object_unref (sc->icon_file), sc->icon_file = NULL;
  g_free (sc), sc = NULL;
}

/**
 * uc_application_status_code_properties_init:
 * 
 * This function load the properties to associate
 * with a server status code.
 */
void
uc_application_status_code_properties_init (void)
{
  guint i = 0;
  struct
  {
    gchar *code;
    gchar *color;
    gchar *icon;
    gchar *message;
    gchar *description;
  }
  UCHTMLCodes[] =
  {
    /* HOST UNREACHABLE - gURLChecker internal */
    {
    UC_STATUS_CODE_HOST_IS_UNREACHABLE,
	UC_TIMEOUT_LINK_BGCOLOR,
	"/link_status_timeout.png",
	_("Host unreachable"),
	_("The remote host does not respond at all...\nPerhaps "
	    "you would change the timeout in gURLChecker "
	    "preferences, or perhaps it is really down."
	    "\n\nTry to check it later.")},
      /* FILE PROTOCOL - gURLChecker internal */
    {
    UC_STATUS_CODE_FILE_PROTO_ERROR,
	UC_FILE_ERROR_LINK_BGCOLOR,
	"/link_status_file_error.png",
	_("FILE:// protocol"),
	_("File protocol detected.\n"
	    "You have asked me to warn you when I find the \"FILE://\" "
	    "protocol in a link.")},
      /* FTP OK - gURLChecker internal */
    {
    UC_STATUS_CODE_FTP_OK,
	UC_GOOD_LINK_BGCOLOR,
	"/link_status_good.png",
	_("Page available"),
	_("No problem with this page.\nServer is up and document "
	    "can be loaded without any problem.")},
      /* FTP BAD - gURLChecker internal */
    {
    UC_STATUS_CODE_FTP_BAD,
	UC_BAD_LINK_BGCOLOR,
	"/link_status_bad.png",
	_("Page not found"), _("The page does not exist on the server.")},
      /* BAD EMAIL LINK SYNTAX - gURLChecker internal */
    {
    UC_STATUS_CODE_BAD_EMAIL_LINK,
	UC_BAD_EMAIL_LINK_BGCOLOR,
	"/link_status_mail_bad.png",
	_("Bad E-Mail link"),
	_("The E-Mail specified in the link is not well " "formatted.")},
      /* BAD EMAIL LINK MX - gURLChecker internal */
    {
    UC_STATUS_CODE_BAD_EMAIL_LINK_MX,
	UC_BAD_EMAIL_LINK_BGCOLOR,
	"/link_status_mail_bad.png",
	_("Problem with MX"),
	_("The MX of the domain specified for this E-Mail is "
	    "not valid or the domain itself is not valid.")},
      /* GOOD EMAIL LINK - gURLChecker internal */
    {
    UC_STATUS_CODE_GOOD_EMAIL_LINK,
	UC_GOOD_LINK_BGCOLOR,
	"/link_status_mail_good.png",
	_("E-Mail link ok"), _("No problem with this E-Mail.")},
      /* IGNORED ITEM - gURLChecker internal */
    {
    UC_STATUS_CODE_IGNORED_LINK,
	UC_IGNORED_LINK_BGCOLOR,
	"/link_status_ignored.png",
	_("Item ignored"),
	_("You chose to ignore this item while gURLChecker "
	    "was checking it.")},
      /* MALFORMED ITEM - gURLChecker internal */
    {
    UC_STATUS_CODE_MALFORMED_LINK,
	UC_MALFORMED_LINK_BGCOLOR,
	"/link_status_malformed.png",
	_("Malformed link"),
	_("The HTML link is malformed.\nPerhaps HREF value is missing?")},
      /* UNKNOW - gURLChecker internal */
    {
    UC_STATUS_CODE_UNKNOWN,
	UC_BAD_LINK_BGCOLOR,
	"/link_type_unknown.png",
	_("No valid information"),
	_("gURLChecker can't interpret the status code sent "
	    "by the remote host.\nPlease, report the value of the "
	    "\"Status\" header field to the author of gURLChecker.")},
      /* 200 */
    {
    UC_STATUS_CODE_PAGE_OK,
	UC_GOOD_LINK_BGCOLOR,
	"/link_status_good.png",
	_("Page available"),
	_("No problem with this page.\nServer is up and document "
	    "can be loaded without any problem.")},
      /* 301 */
    {
    UC_STATUS_CODE_MOVED_PERMANENTLY,
	UC_GOOD_LINK_BGCOLOR,
	"/link_status_good.png",
	_("Page moved permanently"),
	_("No real problem with this page.\n"
	    "The requested resource just has been assigned a new permanent "
	    "URI.")},
      /* 302 */
    {
    UC_STATUS_CODE_REDIRECTED,
	UC_GOOD_LINK_BGCOLOR,
	"/link_status_good.png",
	_("Page moved Temporarily"),
	_("No problem with this page.\nServer is up and document "
	    "can be loaded without any problem.")},
      /* 303 */
    {
    UC_STATUS_CODE_REDIRECTED_SEE_OTHER,
	UC_GOOD_LINK_BGCOLOR,
	"/link_status_good.png",
	_("The response to the request can be found under a different URI"),
	_("No problem with this page.\nServer is up and document "
	    "can be loaded without any problem.")},
      /* 401 */
    {
    UC_STATUS_CODE_RESTRICTED,
	UC_RESTRICTED_LINK_BGCOLOR,
	"/link_status_restricted.png",
	_("Restricted access"),
	_("This page need authentication to be viewed.")},
      /* 403 */
    {
    "403",
	UC_BAD_LINK_BGCOLOR,
	"/link_status_bad.png",
	_("Access forbidden"),
	_("The remote host has refused to serve us the page.\n"
	    "Given the way gURLChecker process links, this error should "
	    "not occured.\n\n"
	    "Please report this bug to the gURLChecker Team.")},
      /* 204 */
    {
    UC_STATUS_CODE_NO_DATA,
	UC_NO_DATA_LINK_BGCOLOR,
	"/link_status_nodata.png",
	_("Page available, but with no content"),
	_("No problem with this page, but it has no content "
	    "at all...\n\nTry to check it later.")},
      /* 400 */
    {
    "400",
	UC_BAD_LINK_BGCOLOR,
	"/link_status_bad.png",
	_("Malformed URL"),
	_("The request could not be understood by the server "
	    "due to malformed syntax.")},
      /* 404 */
    {
    "404",
	UC_BAD_LINK_BGCOLOR,
	"/link_status_bad.png",
	_("Page not found"), _("The page does not exist on the server.")},
      /* 405 */
    {
    UC_STATUS_CODE_BAD_METHOD,
	UC_BAD_LINK_BGCOLOR,
	"/link_status_bad.png",
	_("Method not allowed"),
	_("The remote host has refused to serve us the page."
	    "\n\nTry to check it later.")},
      /* 500 */
    {
    "500",
	UC_BAD_LINK_BGCOLOR,
	"/link_status_bad.png",
	_("Internal server error"),
	_("The page contain bad code, or the server is "
	    "misconfigured.\n\nTry to check it later.")},
      /* 501 */
    {
    "501",
	UC_BAD_LINK_BGCOLOR,
	"/link_status_bad.png",
	_("Method not supported"),
	_("The remote host has refused to serve us the page.\n"
	    "Given the way gURLChecker process links, this error should "
	    "not occured.\n\n"
	    "Please report this bug to the gURLChecker Team.")},
      /* 502 */
    {
    "502",
	UC_BAD_LINK_BGCOLOR,
	"/link_status_bad.png",
	_("Bad Gateway"),
	_("The server, while acting as a gateway or proxy, received an "
	    "invalid response from the upstream server it accessed in "
	    "attempting to fulfill the request.\n\nTry to check this "
	    "page later.")},
      /* 503 */
    {
    "503",
	UC_BAD_LINK_BGCOLOR,
	"/link_status_timeout.png",
	_("Service unavailable"),
	_("The server is currently unable to handle the request "
	    "due to a temporary overloading or maintenance "
	    "of the server. It is also possible that you are using "
	    "a proxy that can not join the given host.\n\nTry "
	    "to check it later, or change your proxy settings.")},
      /* 504 */
    {
    "504",
	UC_TIMEOUT_LINK_BGCOLOR,
	"/link_status_timeout.png",
	_("Request not completed"),
	_("The server, while acting as a gateway or proxy, did not "
	    "receive a timely response from the upstream server specified "
	    "by the URI (e.g. HTTP, FTP, LDAP) or some other auxiliary "
	    "server (e.g. DNS) it needed to access in attempting to "
	    "complete the request.")},
      /* FTP - 530 */
    {
    UC_STATUS_CODE_FTP_MAX_CLIENTS,
	UC_RESTRICTED_LINK_BGCOLOR,
	"/link_status_restricted.png",
	_("Access forbidden or too many connections"),
	_
	("This server do not accept anonymous users, or the maximum number "
	   "of allowed clients are already connected.\n\n"
	   "Try to check it later.")},
      /* 408 */
    {
  UC_STATUS_CODE_TIMEOUT,
	UC_TIMEOUT_LINK_BGCOLOR,
	"/link_status_timeout.png",
	_("Receiving timout requesting the page"),
	_("Perhaps you would change the timeout in gURLChecker "
	      "preferences. Perhaps the remote host is "
	      "too busy for the moment to responde to "
	      "gURLChecker request, or perhaps the remote host does not "
	      "exist at all.\n\nTry to check it later.")},};
  const guint codes_len = G_N_ELEMENTS (UCHTMLCodes);
  gchar *path = NULL;


  if (uc_application_status_code_properties_hash != NULL)
    g_hash_table_destroy (uc_application_status_code_properties_hash),
      uc_application_status_code_properties_hash = NULL;

  uc_application_status_code_properties_hash =
    g_hash_table_new_full (g_str_hash, g_str_equal, NULL,
			   (GDestroyNotify)
			   uc_application_status_code_properties_free_cb);

  /* initialization of the hash table for HTTP
   * codes properties */
  for (i = 0; i < codes_len; i++)
    {
      UCStatusCode *sc = NULL;

      sc = g_new0 (UCStatusCode, 1);
      path = g_strconcat (UC_PIXMAPS_DIR, UCHTMLCodes[i].icon, NULL);
      sc->icon_file = gdk_pixbuf_new_from_file (path, NULL);
      g_free (path), path = NULL;
      sc->color = UCHTMLCodes[i].color;
      sc->message = UCHTMLCodes[i].message;
      sc->description = UCHTMLCodes[i].description;
      g_hash_table_replace (uc_application_status_code_properties_hash,
			    UCHTMLCodes[i].code, (gpointer) sc);
    }
}

/**
 * uc_application_new_instance_launch:
 * 
 * Launch a new instance of gURLChecker.
 */
void
uc_application_new_instance_launch (void)
{
  gchar *message = NULL;
  GError *err = NULL;
  gchar *argv[] = { PACKAGE_BIN_DIR "/" PACKAGE, NULL };
  gchar **envp = environ;
  GSpawnFlags flags = G_SPAWN_DO_NOT_REAP_CHILD;
  gint child_pid;

  if (g_spawn_async (NULL, argv, envp, flags, NULL, NULL, &child_pid, &err))
    return;

  message = g_strdup_printf ("%s\n\n%s",
			     _
			     ("Cannot start new instance of gURLChecker; "
			      "the following error occured:"), err->message);
  uc_application_dialog_show (message, GTK_MESSAGE_ERROR);
  g_clear_error (&err);

  g_free (message), message = NULL;
}


/**
 * uc_application_settings_dialog_show:
 * 
 * Show the settings dialog box.
 */
void
uc_application_settings_dialog_show (void)
{
  GtkWidget *widget = NULL;
  gchar **items = NULL;
  guint items_size = 0;
  GtkWidget *dialog = NULL;

  dialog = WGET ("settings_dialog");

  /* network timeout */
  widget = WGET ("sd_timeout");
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget),
			     (gdouble) uc_project_get_check_timeout ());

  /* Export labels */
  widget = WGET ("sd_export_labels");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_export_labels ());

  /* Export numbering */
  widget = WGET ("sd_export_numbering");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_export_numbering ());

  /* Export IP */
  widget = WGET ("sd_export_ip");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_export_ip ());

  /* Only export external links */
  widget = WGET ("sd_export_external");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_export_external ());

  /* Activate security checks? */
  widget = WGET ("sd_security_checks");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_security_checks ("any"));
  widget = WGET ("sd_security_checks_bad_extensions");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_security_checks
				("bad_extensions"));
  widget = WGET ("sd_security_checks_exclude_images");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_security_checks
				("exclude_images"));
  widget = WGET ("sd_security_checks_virii_extensions");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_security_checks ("virii"));
#ifndef ENABLE_CLAMAV
  WSENS ("sd_notebook_security_virii", FALSE);
#endif

  /* Activate W3C checks? */
  widget = WGET ("sd_w3c_checks");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_w3c_checks ("any"));
  widget = WGET ("sd_w3c_checks_html");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_w3c_checks ("html"));
#ifndef ENABLE_TIDY
  WSENS ("sd_notebook_w3c_html", FALSE);
#endif

  widget = WGET ("sd_w3c_checks_css");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_w3c_checks ("css"));
#ifndef ENABLE_CROCO
  WSENS ("sd_notebook_w3c_css", FALSE);
#endif

  /* prompt for auth */
  widget = WGET ("sd_prompt_auth");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_prompt_auth ());
  /* debug mode? */
  widget = WGET ("sd_debug_mode");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_debug_mode ());

  /* dump properties? */
  widget = WGET ("sd_dump_properties");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_dump_properties ());

  // Networking depth level
  widget = WGET ("sd_url_depth_level");
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget),
			     (gdouble) uc_project_get_depth_level ());

  // Ignore URL args?
  widget = WGET ("sd_no_urls_args");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
                                uc_project_get_no_urls_args ());

  // Only check given directory and its subdirectories?
  widget = WGET ("sd_check_chroot");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
                                uc_project_get_check_chroot ());

  // Do not check external links?
  widget = WGET ("sd_limit_local");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
                                uc_project_get_limit_local ());

  /* retrieve images content */
  widget = WGET ("sd_images_content");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_download_images_content ());

  /* retrieve archives content */
  widget = WGET ("sd_archives_content");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_download_archives_content ());

  /* networking check wait */
  widget = WGET ("sd_url_wait");
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget),
			     (gdouble) uc_project_get_check_wait ());

  /* documents to reject */
  widget = WGET ("documents_rejected_extensions_entry");
  gtk_entry_set_text (GTK_ENTRY (widget), uc_project_get_reject_documents ());

  /* images to reject */
  widget = WGET ("images_rejected_extensions_entry");
  gtk_entry_set_text (GTK_ENTRY (widget), uc_project_get_reject_images ());

  /* Bad extensions */
  widget = WGET ("security_bad_extensions_entry");
  gtk_entry_set_text (GTK_ENTRY (widget),
                      uc_project_get_security_bad_extensions ());

  /* Virii extensions */
  widget = WGET ("security_virii_extensions_entry");
  gtk_entry_set_text (GTK_ENTRY (widget),
                      uc_project_get_security_virii_extensions ());

  /* w3c HTML extensions */
  widget = WGET ("w3c_html_extensions_entry");
  gtk_entry_set_text (GTK_ENTRY (widget),
                      uc_project_get_w3c_html_extensions ());

  /* w3c CSS extensions */
  widget = WGET ("w3c_css_extensions_entry");
  gtk_entry_set_text (GTK_ENTRY (widget),
                      uc_project_get_w3c_css_extensions ());

  /* directories to reject */
  uc_project_get_reject_directories (&items, &items_size);
  uc_application_settings_dialog_display_filters_list
    ("sd_scrolled_filters_directories", items, items_size,
     &treeview_filter_directories, &treestore_filter_directories);

  /* domains to reject */
  uc_project_get_reject_domains (&items, &items_size);
  uc_application_settings_dialog_display_filters_list
    ("sd_scrolled_filters_domains", items, items_size,
     &treeview_filter_domains, &treestore_filter_domains);

  /* value for domains in timeout */
  widget = WGET ("sd_timeouts_blocked");
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget),
			     (gdouble) uc_project_get_timeouts_blocked ());

  /* accept/not accept file protocol & consider/not consider FILE 
   * protocol like a error */
  widget = WGET ("sd_radio_file_protocol");
  if (uc_project_get_proto_file_check ())
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
  else
    {
      widget = WGET ("sd_radio_file_is_error");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
    }

  /* W3C HTML alerts level */
  if (!strcmp (uc_project_get_w3c_html_level (), "warnings"))
    {
      widget = WGET ("sd_radio_w3c_html_warnings");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
    }
  else if (!strcmp (uc_project_get_w3c_html_level (), "errors"))
    {
      widget = WGET ("sd_radio_w3c_html_errors");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
    }
  else
    g_assert_not_reached ();

  /* accept/not accept cookies */
  widget = WGET ("sd_radio_cookies_accept");
  if (uc_project_get_cookies_accept ())
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);

  widget = WGET ("sd_radio_cookies_none");
  if (!uc_project_get_cookies_accept ())
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);

  /* E-Mail check */
  widget = WGET ("sd_check_email_address");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_proto_mailto ());

  /* warn for cookies */
  widget = WGET ("sd_check_cookies_warn_added");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_cookies_warn_added ());
  widget = WGET ("sd_check_cookies_warn_updated");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_cookies_warn_updated ());
  widget = WGET ("sd_check_cookies_warn_deleted");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_cookies_warn_deleted ());

  /* E-Mail MX check */
  widget = WGET ("sd_check_email_mx");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_proto_mailto_check_mx ());

  gtk_widget_set_sensitive (GTK_WIDGET (widget),
			    uc_project_get_proto_mailto ());

  /* FTP check */
  widget = WGET ("sd_check_ftp_protocol");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_proto_ftp ());

  widget = WGET ("sd_use_passive_ftp");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_passive_ftp ());

  /* HTTPS check */
  widget = WGET ("sd_check_https_protocol");
#ifdef ENABLE_GNUTLS
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_proto_https ());
#else
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);
  WSENS ("sd_check_https_protocol", FALSE);
#endif

  gtk_widget_show_all (dialog);
}

/**
 * uc_application_settings_get_data:
 * 
 * Retrieve user input data settings.
 *
 * Returns: TRUE if all inputs are ok.
 */
gboolean
uc_application_settings_get_data (void)
{
  GtkWidget *widget = NULL;
  gchar *tmp = NULL;
#ifdef ENABLE_CLAMAV
  gchar *error = NULL;
  UCClam *clam = NULL;
#endif
#ifdef ENABLE_CROCO
  UCCroco *croco = NULL;
#endif
#ifdef ENABLE_TIDY
  UCTidy *tidy = NULL;
#endif

  /* network timeout */
  widget = WGET ("sd_timeout");
  uc_project_set_check_timeout (gtk_spin_button_get_value_as_int
				(GTK_SPIN_BUTTON (widget)));
  /* Export labels */
  widget = WGET ("sd_export_labels");
  uc_project_set_export_labels ((guint)
				gtk_toggle_button_get_active
				(GTK_TOGGLE_BUTTON (widget)));
  /* Export numbering */
  widget = WGET ("sd_export_numbering");
  uc_project_set_export_numbering ((guint) gtk_toggle_button_get_active
				   (GTK_TOGGLE_BUTTON (widget)));

  /* Export IP */
  widget = WGET ("sd_export_ip");
  uc_project_set_export_ip ((guint) gtk_toggle_button_get_active
                              (GTK_TOGGLE_BUTTON (widget)));

  /* Only export external links */
  widget = WGET ("sd_export_external");
  uc_project_set_export_external ((guint) gtk_toggle_button_get_active
				   (GTK_TOGGLE_BUTTON (widget)));

  /* Activate security checks? */
  widget = WGET ("sd_security_checks");
  uc_project_set_security_checks ("any",
				  gtk_toggle_button_get_active
				  (GTK_TOGGLE_BUTTON (widget)));
  widget = WGET ("sd_security_checks_bad_extensions");
  uc_project_set_security_checks ("bad_extensions",
				  gtk_toggle_button_get_active
				  (GTK_TOGGLE_BUTTON (widget)));
  widget = WGET ("sd_security_checks_exclude_images");
  uc_project_set_security_checks ("exclude_images",
				  gtk_toggle_button_get_active
				  (GTK_TOGGLE_BUTTON (widget)));
  widget = WGET ("sd_security_checks_virii_extensions");
#ifdef ENABLE_CLAMAV
  if (uc_project_get_security_checks ("any") &&
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)) &&
      !(clam = uc_ucclam_new (&error)))
    {
      gchar *tmp = g_strdup_printf (
        _("A problem occured while initializing Clamav library :\n"
          "<b><i>%s</i></b>\n"
          "<i>Please check your configuration or your libclamav "
          "version</i>.\n\n"
          "<b>Virii will not be detected</b>."), error);
      uc_application_dialog_show (tmp, GTK_MESSAGE_ERROR);
      g_free (tmp), tmp = NULL;
      g_free (error), error = NULL;
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);
    }
#endif
  uc_project_set_security_checks ("virii",
				  gtk_toggle_button_get_active
				  (GTK_TOGGLE_BUTTON (widget)));

  /* Activate W3C checks? */
  widget = WGET ("sd_w3c_checks");
  uc_project_set_w3c_checks ("any",
			     gtk_toggle_button_get_active
			     (GTK_TOGGLE_BUTTON (widget)));

  widget = WGET ("sd_w3c_checks_css");
#ifdef ENABLE_CROCO
  if (uc_project_get_w3c_checks ("any") &&
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)) &&
      !(croco = uc_uccroco_new ()))
    {
      uc_application_dialog_show (
        _("A problem occured while initializing Croco library.\n"
          "<i>Please check your configuration or your libcroco version</i>.\n\n"
          "<b>CSS stylesheets will not be validated</b>."), GTK_MESSAGE_ERROR);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);
    }
#endif
  uc_project_set_w3c_checks ("css",
			     gtk_toggle_button_get_active
			     (GTK_TOGGLE_BUTTON (widget)));

  widget = WGET ("sd_w3c_checks_html");
#ifdef ENABLE_TIDY
  if (uc_project_get_w3c_checks ("any") &&
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)) &&
      !(tidy = uc_uctidy_new ()))
    {
      uc_application_dialog_show (
        _("A problem occured while initializing Tidy library.\n"
          "<i>Please check your configuration or your libtidy version</i>.\n\n"
          "<b>HTML pages will not be validated</b>."), GTK_MESSAGE_ERROR);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);
    }
#endif
  uc_project_set_w3c_checks ("html",
			     gtk_toggle_button_get_active
			     (GTK_TOGGLE_BUTTON (widget)));

  /* prompt for auth */
  widget = WGET ("sd_prompt_auth");
  uc_project_set_prompt_auth ((guint)
			      gtk_toggle_button_get_active
			      (GTK_TOGGLE_BUTTON (widget)));

  /* active debug mode? */
  widget = WGET ("sd_debug_mode");
  uc_project_set_debug_mode ((guint)
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
							   (widget)));

  /* dump properties? */
  widget = WGET ("sd_dump_properties");
  uc_project_set_dump_properties ((guint)
				  gtk_toggle_button_get_active
				  (GTK_TOGGLE_BUTTON (widget)));

  // Networking depth level
  widget = WGET ("sd_url_depth_level");
  uc_project_set_depth_level (gtk_spin_button_get_value_as_int
			      (GTK_SPIN_BUTTON (widget)));

  // Ignore URL args?
  widget = WGET ("sd_no_urls_args");
  uc_project_set_no_urls_args (
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));

  // Only check given directory and its subdirectories?
  widget = WGET ("sd_check_chroot");
  uc_project_set_check_chroot (
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));

  // Do not check external links?
  widget = WGET ("sd_limit_local");
  uc_project_set_limit_local (
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));

  /* retrieving images content */
  widget = WGET ("sd_images_content");
  uc_project_set_download_images_content ((guint)
					  gtk_toggle_button_get_active
					  (GTK_TOGGLE_BUTTON (widget)));

  /* retrieving archives content */
  widget = WGET ("sd_archives_content");
  uc_project_set_download_archives_content ((guint)
					    gtk_toggle_button_get_active
					    (GTK_TOGGLE_BUTTON (widget)));

  /* networking check wait */
  widget = WGET ("sd_url_wait");
  uc_project_set_check_wait (gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
							       (widget)));

  /* documents to reject */
  widget = WGET ("documents_rejected_extensions_entry");
  tmp = (gchar *) gtk_entry_get_text (GTK_ENTRY (widget));
  uc_project_set_reject_documents (tmp);

  /* images to reject */
  widget = WGET ("images_rejected_extensions_entry");
  tmp = (gchar *) gtk_entry_get_text (GTK_ENTRY (widget));
  uc_project_set_reject_images (tmp);

  /* Bad extensions */
  widget = WGET ("security_bad_extensions_entry");
  tmp = (gchar *) gtk_entry_get_text (GTK_ENTRY (widget));
  uc_project_set_security_bad_extensions (tmp);

  /* Virii extensions */
  widget = WGET ("security_virii_extensions_entry");
  tmp = (gchar *) gtk_entry_get_text (GTK_ENTRY (widget));
  uc_project_set_security_virii_extensions (tmp);

  /* w3c HTML extensions */
  widget = WGET ("w3c_html_extensions_entry");
  tmp = (gchar *) gtk_entry_get_text (GTK_ENTRY (widget));
  uc_project_set_w3c_html_extensions (tmp);

  /* w3c CSS extensions */
  widget = WGET ("w3c_css_extensions_entry");
  tmp = (gchar *) gtk_entry_get_text (GTK_ENTRY (widget));
  uc_project_set_w3c_css_extensions (tmp);

  /* directories filter */
  uc_application_filters_save_settings (treeview_filter_directories,
					"directories");

  /* domains filter */
  uc_application_filters_save_settings (treeview_filter_domains, "domains");

  /* value for domains in timeout */
  widget = WGET ("sd_timeouts_blocked");
  uc_project_set_timeouts_blocked (gtk_spin_button_get_value_as_int
				   (GTK_SPIN_BUTTON (widget)));

  /* check the FILE protocol & accept/not accept FILE protocol */
  widget = WGET ("sd_radio_file_protocol");
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      uc_project_set_proto_file_check (1);
      uc_project_set_proto_file_is_error (0);
    }
  else
    {
      uc_project_set_proto_file_check (0);
      uc_project_set_proto_file_is_error (1);
    }

  /* accept/not accept cookies */
  widget = WGET ("sd_radio_cookies_accept");
  uc_project_set_cookies_accept (gtk_toggle_button_get_active
				 (GTK_TOGGLE_BUTTON (widget)));

  /* W3C HTML alerts level */
  widget = WGET ("sd_radio_w3c_html_warnings");
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    uc_project_set_w3c_html_level ("warnings");
  widget = WGET ("sd_radio_w3c_html_errors");
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    uc_project_set_w3c_html_level ("errors");

  /* check E-Mails */
  widget = WGET ("sd_check_email_address");
  uc_project_set_proto_mailto ((guint)
			       gtk_toggle_button_get_active
			       (GTK_TOGGLE_BUTTON (widget)));

  /* warn for cookies */
  widget = WGET ("sd_check_cookies_warn_added");
  uc_project_set_cookies_warn_added ((guint)
				     gtk_toggle_button_get_active
				     (GTK_TOGGLE_BUTTON (widget)));
  widget = WGET ("sd_check_cookies_warn_updated");
  uc_project_set_cookies_warn_updated ((guint)
				       gtk_toggle_button_get_active
				       (GTK_TOGGLE_BUTTON (widget)));
  widget = WGET ("sd_check_cookies_warn_deleted");
  uc_project_set_cookies_warn_deleted ((guint)
				       gtk_toggle_button_get_active
				       (GTK_TOGGLE_BUTTON (widget)));

  /* check MX */
  widget = WGET ("sd_check_email_mx");
  uc_project_set_proto_mailto_check_mx ((guint)
					gtk_toggle_button_get_active
					(GTK_TOGGLE_BUTTON (widget)));

  /* check FTP */
  widget = WGET ("sd_check_ftp_protocol");
  uc_project_set_proto_ftp ((guint)
			    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
							  (widget)));

  widget = WGET ("sd_use_passive_ftp");
  uc_project_set_passive_ftp ((guint)
			      gtk_toggle_button_get_active
			      (GTK_TOGGLE_BUTTON (widget)));

  /* check HTTPS */
  widget = WGET ("sd_check_https_protocol");
  uc_project_set_proto_https ((guint)
			      gtk_toggle_button_get_active
			      (GTK_TOGGLE_BUTTON (widget)));


  uc_project_general_settings_save ();

  /* FIXME */
  if (uc_project_get_url () && !uc_project_get_check_is_bookmarks ())
    uc_project_set_save (TRUE);

  return TRUE;
}

/**
 * uc_application_filters_save_settings:
 * @treeview: Tree view to work with.
 * @path: Filter type ("domains" or "directories").
 * 
 * Save filters settings.
 */
static void
uc_application_filters_save_settings (GtkTreeView * treeview,
				      const gchar * path)
{
  GtkTreeModel *tree_store = NULL;
  gchar **filters_vector = NULL;
  GtkTreeIter iter;
  GString *filters = g_string_new ("");
  guint size = 0;

  tree_store = gtk_tree_view_get_model (treeview);
  if (gtk_tree_model_get_iter_first (tree_store, &iter))
    do
      {
	GValue *value = NULL;

	value = g_new0 (GValue, 1);
	gtk_tree_model_get_value (tree_store, &iter, 0, value);

	g_string_append_printf (filters, ",%s",
				(gchar *) g_value_get_string (value));

	g_value_unset (value);
	size++;
      }
    while (gtk_tree_model_iter_next (tree_store, &iter));

  if (size > 0)
    g_string_erase (filters, 0, 1);

  filters_vector = g_strsplit (filters->str, ",", -1);

  if (!strcmp (path, "directories"))
    uc_project_set_reject_directories (filters->str);
  else if (!strcmp (path, "domains"))
    uc_project_set_reject_domains (filters->str);

  g_strfreev (filters_vector), filters_vector = NULL;
  g_string_free (filters, TRUE), filters = NULL;
}

/**
 * uc_application_progress_dialog_show:
 * 
 * Show the progess dialog.
 */
void
uc_application_progress_dialog_show (void)
{
  GtkWidget *widget = NULL;
  GtkWidget *dialog = NULL;

  dialog = WGET ("progress_dialog");

  widget = WGET ("pd_dynamic_values_security_checks");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				uc_project_get_security_checks ("any"));

  widget = WGET ("pd_dynamic_values_time_between");
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget),
			     (gdouble) uc_project_get_check_wait ());

  widget = WGET ("pd_dynamic_values_timeout");
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget),
			     (gdouble) uc_project_get_check_timeout ());

  uc_application_display_state_message (UC_CHECK_MESSAGE_LABEL_FIRST,
                                        _("Check in progress..."));
  uc_application_display_state_message (UC_CHECK_MESSAGE_LABEL_SECOND, "");
  uc_application_display_state_message (UC_CHECK_MESSAGE_LABEL_THIRD, "");

  gtk_widget_show_all (dialog);
}

/**
 * uc_application_progress_dialog_set_modal:
 */
void
uc_application_progress_dialog_set_modal (const gboolean modal)
{
  static GtkTreePath *treepath = NULL;

  if (modal)
    {
      if (treepath)
	{
	  gtk_widget_set_sensitive (WGET ("menu_bar"), TRUE);
	  gtk_widget_set_sensitive (WGET ("tool_bar"), TRUE);
	  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), TRUE);

	  gtk_tree_view_set_cursor (GTK_TREE_VIEW (treeview),
				    treepath, NULL, FALSE);
	  gtk_tree_path_free (treepath), treepath = NULL;
	}
    }
  else if (treeview != NULL)
    {
      if (treepath != NULL)
	gtk_tree_path_free (treepath), treepath = NULL;

      gtk_tree_view_get_cursor (treeview, &treepath, NULL);

      gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);

      gtk_widget_set_sensitive (WGET ("menu_bar"), FALSE);
      gtk_widget_set_sensitive (WGET ("tool_bar"), FALSE);

      gtk_window_set_focus (GTK_WINDOW (WGET ("main_window")),
			    GTK_WIDGET (treeview));
    }

  gtk_window_set_modal (GTK_WINDOW (WGET ("progress_dialog")), modal);
}

/**
 * uc_application_about_dialog_show:
 * 
 * Show the about box.
 */
void
uc_application_about_dialog_show (void)
{
  GtkAboutDialog *about_dialog = NULL;
  GdkPixbuf *pixbuf = NULL;
  gchar *license = NULL;
  const gchar *artists[] = {
    "Jan Henrik Helmers\n"
      "Emmanuel Saracco",
    NULL
  };
  const gchar *authors[] = {
    _("Emmanuel Saracco\n"
      "\n"
      "Contributors:\n"
      "Stephane Loeuillet"),
    NULL
  };
  const gchar *documenters[] = {
    _("Manpage:"),
    "Daniel Pecos",
    "Emmanuel Saracco",
    "",
    _("Code:"),
    "Emmanuel Saracco",
    NULL
  };
  gchar *translators =
    _("Takeshi Aihana, Japanese\n"
      "Alexander Hunziker, German\n"
      "Florent Jugla, Esperanto\n"
      "Daniel Pecos, Spanish\n"
      "Gunner Poulsen, Danish\n"
      "Emmanuel Saracco, French\n"
      "Angelo Theodorou, Italian\n");

  g_file_get_contents (PACKAGE_DATA_DIR "/doc/" PACKAGE "/COPYING",
		       &license, NULL, NULL);
  pixbuf =
    gdk_pixbuf_new_from_file (UC_PIXMAPS_DIR "/gurlchecker_icon.png", NULL);

  about_dialog = GTK_ABOUT_DIALOG (gtk_about_dialog_new ());
  gtk_window_set_position (GTK_WINDOW (about_dialog),
                           GTK_WIN_POS_CENTER_ALWAYS);
  gtk_about_dialog_set_version (about_dialog, UC_VERSION);
  gtk_about_dialog_set_copyright (about_dialog,
                                  "(C) 2002-2013 Emmanuel Saracco");
  gtk_about_dialog_set_comments (about_dialog,
    _("gURLChecker is a graphical web links checker.\n"
      "It can work on a whole site, a single local page "
      "or a browser bookmarks file"));
  gtk_about_dialog_set_license (about_dialog, license);
  gtk_about_dialog_set_wrap_license (about_dialog, TRUE);
  gtk_about_dialog_set_website (about_dialog,
				"http://gurlchecker.labs.libre-entreprise.org");
  gtk_about_dialog_set_authors (about_dialog, authors);
  gtk_about_dialog_set_documenters (about_dialog, documenters);
  gtk_about_dialog_set_artists (about_dialog, artists);
  gtk_about_dialog_set_translator_credits (about_dialog, translators);
  gtk_about_dialog_set_logo (about_dialog, pixbuf);

  g_object_unref (pixbuf), pixbuf = NULL;
  g_free (license), license = NULL;

  gtk_dialog_run (GTK_DIALOG (about_dialog));
  gtk_widget_destroy (GTK_WIDGET (about_dialog));
}

/**
 * uc_application_dialog_yes_no_show:
 * @message: The message to display.
 * @msg_type: Type of the message.
 * 
 * Print a yes/no choice dialog.
 *  
 * Returns: TRUE if user clicked on the OK button.
 */
gint
uc_application_dialog_yes_no_show (const gchar * message,
				   const GtkMessageType msg_type)
{
  gint ret = 0;
  GtkWidget *dialog = NULL;


  dialog = gtk_message_dialog_new_with_markup (
    NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, msg_type,
    GTK_BUTTONS_NONE, message);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
			  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			  GTK_STOCK_NO, GTK_RESPONSE_NO,
			  GTK_STOCK_YES, GTK_RESPONSE_YES, NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  ret = gtk_dialog_run (GTK_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return ret;
}

void
uc_application_buffer_show (const gchar * title, const gchar * message)
{
  GtkWidget *dialog = NULL;
  GtkWidget *w = NULL;
  GtkTextBuffer *buffer = NULL;
  gchar *tmp = NULL;

  dialog = WGET ("show_buffer_dialog");
  gtk_window_set_title (GTK_WINDOW (dialog), title);

  w = WGET ("text");
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w));
  tmp = uc_utils_to_utf8 ((gchar *) message);
  gtk_text_buffer_set_text (buffer, tmp, -1);
  g_free (tmp), tmp = NULL;

  gtk_widget_show_all (dialog);

  gtk_dialog_run (GTK_DIALOG (dialog));

  gtk_widget_hide (dialog);
}

/**
 * uc_application_dialog_show:
 * @message: The message to display.
 * @msg_type: Type of the message.
 * 
 * Print a alert box.
 */
void
uc_application_dialog_show (const gchar * message,
			    const GtkMessageType msg_type)
{
  GtkWidget *dialog = NULL;


  dialog = gtk_message_dialog_new_with_markup (
    NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, msg_type,
    GTK_BUTTONS_OK, message);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  gtk_dialog_run (GTK_DIALOG (dialog));

  gtk_widget_destroy (dialog);
}


/**
 * uc_application_new_local_file_dialog_show:
 * 
 * Show the new local file project dialog box.
 */
void
uc_application_new_local_file_dialog_show (void)
{
  GtkWidget *widget = NULL;
  GtkWidget *dialog = NULL;


  if (!uc_project_save_all ())
    return;

  dialog = WGET ("new_local_file_project_dialog");

  widget = WGET ("nlfpd_project_url");

  if (uc_project_get_url () != NULL)
    gtk_entry_set_text (
      GTK_ENTRY (gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (widget))),
      uc_project_get_url ());

  // Do not retreive page content?
  widget = WGET ("nlfpd_speed_check");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);

  // Only check given directory and its subdirectories?
  widget = WGET ("nlfpd_check_chroot");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
                                uc_project_get_check_chroot ());

  // Do not check external links?
  widget = WGET ("nlfpd_limit_local");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
                                uc_project_get_limit_local ());

  gtk_widget_show_all (dialog);
}


/**
 * uc_application_new_web_dialog_show:
 * 
 * Show the new web project dialog box.
 */
void
uc_application_new_web_dialog_show (void)
{
  GtkWidget *widget = NULL;
  GtkWidget *dialog = NULL;


  if (!uc_project_save_all ())
    return;

  dialog = WGET ("new_web_project_dialog");

  widget = WGET ("nwpd_project_url");

  if (uc_project_get_url () != NULL)
    gtk_entry_set_text (
      GTK_ENTRY (gnome_entry_gtk_entry (GNOME_ENTRY (widget))),
      uc_project_get_url ());

  // Do not retreive page content?
  widget = WGET ("nwpd_speed_check");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);

  // Ignore URL args?
  widget = WGET ("nwpd_no_urls_args");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
                                uc_project_get_no_urls_args ());

  // Only check given directory and its subdirectories?
  widget = WGET ("nwpd_check_chroot");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
                                uc_project_get_check_chroot ());

  // Do not check external links?
  widget = WGET ("nwpd_limit_local");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
                                uc_project_get_limit_local ());

  gtk_widget_show_all (dialog);
}

/**
 * uc_application_new_bookmarks_dialog_show:
 * 
 * Show the new bookmarks project dialog box.
 */
void
uc_application_new_bookmarks_dialog_show (void)
{
  GtkWidget *widget = NULL;
  GtkWidget *dialog = NULL;
  GtkTextBuffer *buffer = NULL;


  if (!uc_project_save_all ())
    return;

  dialog = WGET ("new_bookmarks_project_dialog");

  widget = WGET ("nbpd_project_bookmarks_file");

  if (uc_project_get_check_is_bookmarks ())
    gnome_file_entry_set_default_path (GNOME_FILE_ENTRY (widget),
				       uc_project_get_bookmarks_file ());

  widget = WGET ("nbpd_info");

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
#if defined(ENABLE_SQLITE3) && defined(ENABLE_JSON)
  gtk_text_buffer_set_text (buffer, 
    _("You can select a local or remote bookmarks file.\n"
      "This file have to be either a XBEL, Opera, Google Chrome, "
      "or Firefox database file"), -1);
#elif defined(ENABLE_SQLITE3)
  gtk_text_buffer_set_text (buffer, 
    _("You can select a local or remote bookmarks file.\n"
      "This file have to be either a XBEL, Opera, or a Firefox database file"),
       -1);
#elif defined(ENABLE_JSON)
  gtk_text_buffer_set_text (buffer, 
    _("You can select a local or remote bookmarks file.\n"
      "This file have to be either a XBEL, Opera, or a Google Chrome file"),
      -1);
#else
  gtk_text_buffer_set_text (buffer, 
    _("You can select a local or remote bookmarks file.\n"
      "This file have to be in XBEL or Opera format"), -1);
#endif

  gtk_widget_show_all (dialog);
}

/**
 * uc_application_add_filter_domain_add:
 * 
 * Add a domain filter.
 */
void
uc_application_add_filter_domain_add (void)
{
  gchar *text = NULL;

  text = uc_application_input_dialog (_("New domain to reject"),
				      _("Add a domain to reject"));

  if (text != NULL && g_strstrip (text) && strlen (text) > 0)
    {
      GtkTreeIter iter;

      /* very basic check of the domain name syntax */
      if ((strlen (text) < 4) ||
	  (!strchr (text, '.')) ||
	  (text[strlen (text) - 1] == '.') ||
	  (text[strlen (text) - 2] == '.') || (text[0] == '.'))
	{
	  uc_application_dialog_show (_("This domain name is not valid."),
                                      GTK_MESSAGE_ERROR);
	  uc_application_add_filter_domain_add ();
	}
      else
	{
	  gtk_list_store_append (treestore_filter_domains, &iter);
	  gtk_list_store_set (treestore_filter_domains, &iter, 0, text, -1);
	}
    }

  g_free (text), text = NULL;
}

/**
 * uc_application_treeview_get_selected_iter:
 * @tv: Tree view to work with.
 * @iter: Iterater to fill.
 * 
 * set the iterator for the current
 * selection in a given treeview.
 *
 * Returns: TRUE if it has filled the given iter with selection.
 */
gboolean
uc_application_treeview_get_selected_iter (const GtkTreeView * tv,
					   GtkTreeIter * iter)
{
  GtkTreeModel *model = NULL;
  GtkTreeSelection *select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
  gint32 ret = -1;

  if (gtk_tree_selection_get_selected (select, &model, iter))
    {
      gtk_tree_model_get (model, iter, 0, &ret, -1);

      return TRUE;
    }

  return FALSE;
}

/**
 * uc_application_add_filter_domain_remove:
 * 
 * Remove a domain filter.
 */
void
uc_application_add_filter_domain_remove (void)
{
  GtkTreeIter iter;

  if (uc_application_treeview_get_selected_iter
      (treeview_filter_domains, &iter))
    gtk_list_store_remove (treestore_filter_domains, &iter);
  else
    uc_application_dialog_show (_("Please, select a item."), GTK_MESSAGE_ERROR);
}

/**
 * uc_application_add_filter_directory_add:
 * 
 * Add a directory filter.
 */
void
uc_application_add_filter_directory_add (void)
{
  gchar *text = NULL;

  text = uc_application_input_dialog (_("New directory to reject"),
				      _("Add a directory to reject"));

  if (text != NULL && g_strstrip (text) && strlen (text) > 0)
    {
      GtkTreeIter iter;
      gchar *tmp_path = NULL;

      /* force directory to be "slashed" */
      if (text[strlen (text) - 1] == '/')
	text[strlen (text) - 1] = '\0';

      tmp_path = (text[0] == '/') ?
	g_strconcat (text, "/", NULL) : g_strconcat ("/", text, "/", NULL);

      gtk_list_store_append (treestore_filter_directories, &iter);
      gtk_list_store_set (treestore_filter_directories, &iter, 0, tmp_path,
			  -1);
      g_free (tmp_path), tmp_path = NULL;
    }

  g_free (text), text = NULL;
}

/**
 * uc_application_add_filter_directory_remove:
 * 
 * Remove a directory filter.
 */
void
uc_application_add_filter_directory_remove (void)
{
  GtkTreeIter iter;

  if (uc_application_treeview_get_selected_iter
      (treeview_filter_directories, &iter))
    gtk_list_store_remove (treestore_filter_directories, &iter);
  else
    uc_application_dialog_show (_("Please, select a item."), GTK_MESSAGE_ERROR);
}

/**
 * uc_application_search_get_data:
 * 
 * Retreive the string to search and the search
 * properties.
 */
void
uc_application_search_get_data (void)
{
  GtkWidget *widget = NULL;
  gchar *tmp = NULL;

  widget = WGET ("nsd_search_string");
  tmp = (gchar *)
    gtk_entry_get_text (GTK_ENTRY
			(gnome_entry_gtk_entry (GNOME_ENTRY (widget))));

  if (tmp != NULL)
    {
      g_strstrip (tmp);

      if (strlen (tmp) > 0)
	{
          // Add search key to Search history
	  gnome_entry_append_history (GNOME_ENTRY (widget), TRUE, tmp);

	  uc_search_free ();
	  search_state.string = g_strdup (tmp);

	  /* FIXME
	     search_state.case_sensitive =
	     (gtk_toggle_button_get_active
	     (GTK_TOGGLE_BUTTON
	     (WGET ("search_case sensitive"))));
	   */
	  search_state.case_sensitive = TRUE;

	  search_state.status_code =
	    (gtk_toggle_button_get_active
	     (GTK_TOGGLE_BUTTON (WGET ("nsd_search_status_code"))));
	  search_state.pages_content =
	    (gtk_toggle_button_get_active
	     (GTK_TOGGLE_BUTTON (WGET ("nsd_search_pages_content"))));
	  search_state.links_name =
	    (gtk_toggle_button_get_active
	     (GTK_TOGGLE_BUTTON (WGET ("nsd_search_links_name"))));
	  search_state.meta_content =
	    (gtk_toggle_button_get_active
	     (GTK_TOGGLE_BUTTON (WGET ("nsd_search_meta_content"))));
	  search_state.emails =
	    (gtk_toggle_button_get_active
	     (GTK_TOGGLE_BUTTON (WGET ("nsd_search_emails"))));

	  gtk_widget_hide (WGET ("new_search_dialog"));
	  UC_UPDATE_UI;
	  uc_search_begin ();

	  return;
	}
    }

  uc_application_dialog_show (_("Please, enter a string to search."),
			      GTK_MESSAGE_ERROR);
}


/**
 * uc_application_project_get_data:
 * @type: FIXME.
 * 
 * Retrieving url to check and some check
 * properties.
 *
 * Returns: %TRUE if data input is ok.
 */
gboolean
uc_application_project_get_data (const UCProjectType type)
{
  GtkWidget *widget = NULL;
  gchar *auth_user = NULL;
  gchar *auth_password = NULL;
  gchar *auth_line = NULL;
  gchar *tmp = NULL;
  gchar *tmp_url = NULL;
  gchar *err_msg = _("Please, check that there is no mismatch in "
                     "your URL and try again.");


  uc_project_set_type (type);

  if (type == UC_PROJECT_TYPE_WEB_SITE)
  {
    widget = WGET ("nwpd_project_url");
    tmp = g_strdup (gtk_entry_get_text (GTK_ENTRY (gnome_entry_gtk_entry (
                                                     GNOME_ENTRY (widget)))));
  }
  else
  {
    widget = WGET ("nlfpd_project_url");
    tmp = g_strdup (gtk_entry_get_text (
                      GTK_ENTRY (gnome_file_entry_gtk_entry (
                                   GNOME_FILE_ENTRY (widget)))));
  }

  if (tmp != NULL)
    {
      g_strstrip (tmp);

#ifndef ENABLE_GNUTLS
      if (uc_utils_memcasecmp (tmp, UC_PROTOCOL_HTTPS))
        err_msg = 
          _("Sorry, but this version of gurlchecker have no SSL support");
      else
#endif
      if (strlen (tmp) > 0)
	{
	  tmp_url =
	    uc_url_add_protocol ((uc_project_get_type () ==
				  UC_PROJECT_TYPE_WEB_SITE) ?
				 UC_PROTOCOL_HTTP : UC_PROTOCOL_FILE, tmp);

	  if (uc_url_is_valid (tmp_url))
	    {
	      uc_project_new ();
	      uc_project_set_url (tmp_url);
	      uc_project_set_type (type);

	      if (uc_project_get_type () == UC_PROJECT_TYPE_WEB_SITE)
		{
                  // Add path in Web projects history
                  gnome_entry_append_history (GNOME_ENTRY (widget), TRUE,
                                              tmp_url);

		  // Manage authentication
		  uc_project_set_auth_line (NULL);
		  if (uc_utils_get_auth_fields (glade,
						WGET ("nwpd_auth_user"),
						WGET ("nwpd_auth_password"),
						&auth_user, &auth_password,
						&auth_line))
		    {
		      uc_project_set_auth_line (auth_line);
		      g_free (auth_line), auth_line = NULL;
		    }

		  /* ignore URLs arguments */
		  widget = WGET ("nwpd_no_urls_args");
		  uc_project_set_no_urls_args (
                    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
		}
	      else
		{
                  // Add path in Local files projects history
                  gnome_entry_append_history (
                    GNOME_ENTRY (gnome_file_entry_gnome_entry (
                                   GNOME_FILE_ENTRY (widget))), TRUE, tmp);

		  /* force check for files, even if user chose "consider as 
		   * error" in preferences */
		  uc_utils_swap_file_proto_option (TRUE);
		}

	      /* do not retrieve content (speed up the check) */
	      if (type == UC_PROJECT_TYPE_WEB_SITE)
		widget = WGET ("nwpd_speed_check");
	      else
		widget = WGET ("nlfpd_speed_check");
	      uc_project_set_speed_check (
                gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));

	      /* only check this directory and its subdirectories? */
	      if (type == UC_PROJECT_TYPE_WEB_SITE)
		widget = WGET ("nwpd_check_chroot");
	      else
		widget = WGET ("nlfpd_check_chroot");
	      uc_project_set_check_chroot (
                gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));

	      /* only check local links */
	      if (type == UC_PROJECT_TYPE_WEB_SITE)
		widget = WGET ("nwpd_limit_local");
	      else
		widget = WGET ("nlfpd_limit_local");
	      uc_project_set_limit_local (
                gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));

	      widget = WGET ("mw_project_type");
	      if (uc_project_get_type () == UC_PROJECT_TYPE_LOCAL_FILE)
		gtk_label_set_markup (
                  GTK_LABEL (widget), 
                  _("[<b>Local file</b>] Page information"));
	      else
		gtk_label_set_markup (
                  GTK_LABEL (widget), _("[<b>Web site</b>] Page information"));

	      if (type == UC_PROJECT_TYPE_WEB_SITE)
		gtk_widget_hide (WGET ("new_web_project_dialog"));
	      else
		gtk_widget_hide (WGET ("new_local_file_project_dialog"));

	      UC_UPDATE_UI;

	      uc_web_site_begin_check ();

	      uc_project_set_speed_check (FALSE);

	      /* restore original options for file protocol */
	      if (uc_project_get_type () == UC_PROJECT_TYPE_LOCAL_FILE)
		uc_utils_swap_file_proto_option (FALSE);

	      g_free (tmp_url), tmp_url = NULL;
	      g_free (tmp), tmp = NULL;

	      return TRUE;
	    }
	}
    }

  uc_application_dialog_show (err_msg, GTK_MESSAGE_ERROR);

  g_free (tmp_url), tmp_url = NULL;
  g_free (tmp), tmp = NULL;

  return FALSE;
}


/**
 * uc_application_get_bookmarks_project_data:
 * 
 * Retrieving bookmarks file to check.
 */
void
uc_application_get_bookmarks_project_data (void)
{
  GtkWidget *widget = NULL;
  gchar *file = NULL;
  gchar *file_real = NULL;
  gboolean remote_file = FALSE;
  UCBookmarksType btype = UC_BOOKMARKS_TYPE_NONE;

  widget = WGET ("nbpd_project_bookmarks_file");

  file = gnome_file_entry_get_full_path (GNOME_FILE_ENTRY (widget), FALSE);

  if (file == NULL || strlen (file) == 0)
    {
      uc_application_dialog_show (
        _("Please choose a bookmarks file to check."), GTK_MESSAGE_ERROR);

      return;
    }

  /* Extract URL from path, or leave path */
  if (!(file_real = uc_url_extract_url_from_local_path (file)))
    file_real = g_strdup (file);

  /* This is a URL instead of a local file */
  if (uc_url_is_valid (file_real))
    {
      UCLinkProperties *prop = NULL;
      gchar *file_content = NULL;
      gchar *project_host = NULL;
      gchar *host = NULL;
      gchar *port = NULL;
      gchar *path = NULL;
      gchar *args = NULL;
      gchar *src = NULL;

      /* Get the file on the network and save it on local */
      project_host = uc_url_get_hostname (NULL, file_real);
      uc_project_set_current_host (project_host);

      uc_url_parse (project_host, "", file_real, &host, &port, &path, &args);
      g_free (project_host), project_host = NULL;

      prop = uc_check_link_properties_node_new ();
      prop->protocol = uc_url_get_protocol (file_real);
      prop->h_name = host;
      prop->port = uc_url_get_port (file_real);
      prop->path = path;
      prop->args = args;
      prop->is_downloadable = TRUE;
      prop->header = uc_check_url_get_header (prop);

      file_content = uc_check_url_get_content (prop, "", "", "", "");

      g_free (file_content), file_content = NULL;
      g_free (port), port = NULL;
      g_free (file_real), file_real = NULL;

      src = uc_utils_convert_uid2file (prop->uid);

      if (src && g_file_test (src, G_FILE_TEST_EXISTS))
	{
	  file_real =
	    g_strdup_printf ("/tmp/.gurlchecker_bookmarks_%u-%u.tmp",
			     prop->uid, getpid ());
	  remote_file = TRUE;
	  uc_utils_copy (src, file_real);
	}
      else
	uc_application_dialog_show (
          _("A problem occured when retrieving remote bookmarks file.\n"
            "<i>Try to pump up the timeout delay</i>."), GTK_MESSAGE_ERROR);

      g_free (src), src = NULL;

      uc_lists_checked_links_node_free (NULL, &prop);

      if (!file_real)
	return;
    }
  /* This is a local file */
  else if (!g_file_test (file_real, G_FILE_TEST_EXISTS))
    {
      uc_application_dialog_show (
        _("Either the file is not correct or your don't have appropriate "
          "rights on it.\n"
          "<b>Please choose a valid bookmarks file</b>."), GTK_MESSAGE_ERROR);

      g_free (file_real), file_real = NULL;

      return;
    }

  // For the moment we accept only XBEL, Google Chrome JSON, Opera and
  // Firefox sqlite3 format
  btype = uc_bookmarks_guess_type (file_real);
  if (btype == UC_BOOKMARKS_TYPE_NONE)
  {
#if defined(ENABLE_SQLITE3) && defined(ENABLE_JSON)
    uc_application_dialog_show (
      _("The file format is not correct.\n"
        "<i>gurlchecker can deal only with <b>XBEL</b>, <b>Opera</b>, "
        "<b>Google Chrome</b>, and <b>Firefox</b> database "
        "file</i>."),
      GTK_MESSAGE_ERROR);
#elif defined(ENABLE_SQLITE3)
    uc_application_dialog_show (
      _("The file format is not correct.\n"
        "<i>This version of gurlchecker can deal only with <b>XBEL</b>, "
        "<b>Opera</b> and <b>Firefox</b> database file</i>."),
      GTK_MESSAGE_ERROR);

#elif defined(ENABLE_JSON)
    uc_application_dialog_show (
      _("The file format is not correct.\n"
        "<i>This version of gurlchecker can deal only with <b>XBEL</b>, "
        "<b>Opera</b> and <b>Google Chrome</b> file</i>."),
      GTK_MESSAGE_ERROR);
#else
    uc_application_dialog_show (
      _("The file format is not correct.\n"
        "<i>This version of gurlchecker can deal only with <b>XBEL</b> and "
        "<b>Opera</b> file</i>."),
      GTK_MESSAGE_ERROR);
#endif

    g_free (file_real), file_real = NULL;

    return;
  }

  uc_project_new ();
  uc_project_set_type (UC_PROJECT_TYPE_BOOKMARKS);

  widget = WGET ("mw_project_type");
  if (remote_file)
    gtk_label_set_markup (GTK_LABEL (widget),
			  _("[<b>Remote bookmarks</b>] Page information"));
  else
    gtk_label_set_markup (GTK_LABEL (widget),
			  _("[<b>Local bookmarks</b>] Page information"));

  widget = WGET ("nbpd_project_bookmarks_file");

  // Add path in Bookmarks projects history
  gnome_entry_append_history (GNOME_ENTRY (gnome_file_entry_gnome_entry (
                                             GNOME_FILE_ENTRY (widget))),
                              TRUE, file_real);

  /* do not retrieve content (speed up the check) */
  widget = WGET ("nbpd_speed_check");
  uc_project_set_speed_check ((guint)
			      gtk_toggle_button_get_active
			      (GTK_TOGGLE_BUTTON (widget)));

  uc_project_set_bookmarks_file (file_real);
  uc_project_set_bookmarks_type (btype);

  gtk_widget_hide (WGET ("new_bookmarks_project_dialog"));
  UC_UPDATE_UI;
  uc_bookmarks_begin_check ();

  if (remote_file)
    unlink (file_real);
  g_free (file_real), file_real = NULL;

  // FIXME
  // For the moment we are not able to manage projects
  // for bookmarks files...
  uc_project_set_save (FALSE);
  uc_project_set_save_bookmarks (FALSE);
}

/**
 * uc_application_draw_main_frames:
 * 
 * Draw the two principals main frames
 * for displaying urls list and headers.
 */
void
uc_application_draw_main_frames (void)
{
  GtkWidget *main_frames = NULL;
  GtkWidget *right_frames = NULL;
  GtkWidget *urls_list = NULL;
  GtkWidget *scrolled_window = NULL;

  main_frames = WGET ("mw_main_frames");
  right_frames = WGET ("mw_right_frames");
  scrolled_window = WGET ("mw_scrolled_window");

  UC_GET_WIDGET ("urls_list", WGET ("main_window"), urls_list);
  if (urls_list != NULL)
    gtk_widget_destroy (urls_list), urls_list = NULL;

  urls_list = gtk_tree_view_new ();
  gtk_widget_show_all (urls_list);

  gtk_container_add (GTK_CONTAINER (scrolled_window), urls_list);
  UC_SET_WIDGET ("urls_list", WGET ("main_window"), urls_list);

  g_signal_connect (G_OBJECT (urls_list), "cursor_changed",
		    G_CALLBACK (on_urls_list_cursor_changed), NULL);

  g_signal_connect (G_OBJECT (urls_list), "button_press_event",
		    G_CALLBACK (on_url_list_mouse_clicked), NULL);

  g_signal_connect (G_OBJECT (urls_list), "button_release_event",
		    G_CALLBACK (on_url_list_mouse_clicked), NULL);

  g_signal_connect (G_OBJECT (urls_list),
		    "motion-notify-event",
		    G_CALLBACK (on_main_treeview_motion_notify_event), NULL);

  g_signal_connect (G_OBJECT (urls_list),
		    "leave_notify_event",
		    G_CALLBACK (on_main_treeview_leave_notify_event), NULL);

  g_signal_connect (G_OBJECT (urls_list),
		    "enter_notify_event",
		    G_CALLBACK (on_main_treeview_enter_notify_event), NULL);

  gtk_widget_show_all (right_frames);
  gtk_widget_show_all (main_frames);
}

static gboolean
uc_application_get_urls_user_action_value_cb (gpointer key, gpointer value,
					      gpointer data)
{
  return (strcmp (((UCURLsUserActions *) value)->label, (gchar *) data) == 0);
}

/**
 * uc_application_get_urls_user_action_by_value:
 * @value: user action value.
 * 
 * Get a action by its value.
 *
 * Returns: A #UCURLsUserActions action node associated with the given value.
 */
UCURLsUserActions *
uc_application_get_urls_user_action_by_value (gchar * value)
{
  UCURLsUserActions *a = NULL;


  if (value != NULL)
    a = g_hash_table_find (uc_application_urls_user_actions_hash,
			   uc_application_get_urls_user_action_value_cb,
			   (gpointer) value);
  if (a == NULL)
    {
      if (value != NULL && strlen (value) > 0)
	{
	  a = g_new0 (UCURLsUserActions, 1);
	  a->id = g_strdup (value);
	  a->label = g_strdup (value);
	  g_hash_table_replace (uc_application_urls_user_actions_hash,
				g_strdup (value), (gpointer) a);
	}
      else
	a =
	  g_hash_table_lookup (uc_application_urls_user_actions_hash, "none");
    }

  return a;
}

/**
 * uc_application_get_urls_user_action:
 * @key: user action key.
 * 
 * Get a action by its key.
 *
 * Returns: A #UCURLsUserActions action node associated with the given id.
 */
UCURLsUserActions *
uc_application_get_urls_user_action (gchar * key)
{
  UCURLsUserActions *a = NULL;

  if (key != NULL)
    a = g_hash_table_lookup (uc_application_urls_user_actions_hash, key);

  if (a == NULL)
    {
      if (key != NULL && strlen (key) > 0)
	{
	  a = g_new0 (UCURLsUserActions, 1);
	  a->id = g_strdup (key);
	  a->label = g_strdup (key);
	  g_hash_table_replace (uc_application_urls_user_actions_hash,
				g_strdup (key), (gpointer) a);
	}
      else
	a =
	  g_hash_table_lookup (uc_application_urls_user_actions_hash, "none");
    }

  return a;
}

/**
 * uc_application_get_status_code_properties:
 * @status_code: Status code to work with.
 * 
 * Return properties for a given status code.
 *
 * Returns: #UCStatusCode propertie node associated with the given status code.
 */
UCStatusCode *
uc_application_get_status_code_properties (const gchar * status_code)
{
  UCStatusCode *sc = NULL;

  if (status_code != NULL)
    sc = g_hash_table_lookup
      (uc_application_status_code_properties_hash, status_code);

  if (sc == NULL)
    sc = g_hash_table_lookup (uc_application_status_code_properties_hash,
			      UC_STATUS_CODE_UNKNOWN);

  return sc;
}

/**
 * uc_application_quit:
 * 
 * Do a clean application exit.
 *
 * returns: ~TRUE if we must quit application.
 */
gboolean
uc_application_quit (void)
{
  if (!uc_project_save_all ())
    return TRUE;
  else if (uc_project_get_id ())
    uc_project_report_save ();

  uc_application_clean_data ();

  return FALSE;
}

/**
 * uc_application_clean_data:
 * 
 * General clean up routine call at application
 * exit.
 */
void
uc_application_clean_data (void)
{
  uc_application_set_status_bar (0, _("Cleaning..."));
  UC_UPDATE_UI;

  uc_application_remove_paths ();
  uc_project_free ();
  uc_cookies_free ();
  uc_search_free ();
  uc_lists_checked_links_free ();
  uc_lists_already_checked_free ();
  if (uc_project_get_check_is_bookmarks ())
    uc_bookmarks_free ();
  uc_timeout_domains_free ();
  g_hash_table_destroy (uc_application_status_code_properties_hash),
    uc_application_status_code_properties_hash = NULL;

  if (treestore_filter_directories != NULL)
    g_object_unref (treestore_filter_directories),
      treestore_filter_directories = NULL;
  if (treestore_filter_domains != NULL)
    g_object_unref (treestore_filter_domains),
      treestore_filter_domains = NULL;
  if (treestore_search != NULL)
    g_object_unref (treestore_search), treestore_search = NULL;
  if (treestore != NULL)
    g_object_unref (treestore), treestore = NULL;

  if (security_alert_icon)
    g_object_unref (security_alert_icon), security_alert_icon = NULL;
  if (w3c_alert_icon)
    g_object_unref (w3c_alert_icon), w3c_alert_icon = NULL;
  if (empty_icon)
    g_object_unref (empty_icon), empty_icon = NULL;
}

/**
 * uc_application_globals_init:
 *
 * Initialisation of all globals.
 */
void
uc_application_globals_init (void)
{
  /* global pointer for glade xml file */
  glade = NULL;

  /* treeview pointers */
  treeview = NULL;
  treeview_filter_directories = NULL;
  treeview_projects = NULL;
  treeview_filter_domains = NULL;
  treeview_search = NULL;

  /* treestore for the url treeview list */
  treestore = NULL;
  treestore_filter_directories = NULL;
  treestore_filter_domains = NULL;
  treestore_search = NULL;

  UC_DISPLAY_SET_ALL;

  /* Globals icons */
  empty_icon =
    gdk_pixbuf_new_from_file (UC_PIXMAPS_DIR "/empty_icon.png", NULL);
  security_alert_icon =
    gdk_pixbuf_new_from_file (UC_PIXMAPS_DIR "/security_alert.png", NULL);
  w3c_alert_icon =
    gdk_pixbuf_new_from_file (UC_PIXMAPS_DIR "/w3c_alert.png", NULL);

  uc_refresh_page = FALSE;

  glade = glade_xml_new (UC_GLADE_XML_FILENAME, NULL, NULL);
  glade_xml_signal_autoconnect (glade);

  uc_application_status_code_properties_init ();
  uc_application_urls_user_actions_init ();

  uc_application_build_projects_treeview ();
}

/**
 * uc_application_init:
 * @url: the command line URL, or %NULL
 * @auth_user: user for authentication.
 * @auth_password: password for authentication.
 * @no_urls_args: if %TRUE, do not check URL with arguments
 * 
 * gURLChecker initialization.
 */
void
uc_application_init (gchar * url, gchar * auth_user,
		     gchar * auth_password, gboolean no_urls_args)
{
  uc_conn_init ();
  uc_timeout_domains_init ();
  uc_tooltips_init ();
  uc_application_menu_init ();
  uc_project_free ();
  uc_check_reset ();
  uc_project_xml_load_settings (NULL);
  uc_project_projects_list_load ();

  uc_application_draw_main_frames ();
  UC_UPDATE_UI;

  /* if user specified a url to check on
   * command line, go and check it */
  if (url)
    {
      uc_project_set_url (url);
      if (auth_user && auth_password)
	{
	  gchar *auth_line = NULL;

	  auth_line = uc_utils_build_auth_line (auth_user, auth_password);
	  uc_project_set_auth_line (auth_line);
	  g_free (auth_line), auth_line = NULL;
	}
      uc_project_set_no_urls_args (no_urls_args);
      uc_web_site_begin_check ();
    }
}

/**
 * uc_application_make_paths:
 * 
 * Create the directories needed by gURLChecker.
 */
void
uc_application_make_paths (void)
{
  gchar *path = NULL;

  path = g_strconcat (uc_project_get_working_path (), "/",
		      uc_project_get_cache_name (), NULL);
  uc_utils_mkdirs (path, TRUE);
  g_free (path), path = NULL;

  path = g_strconcat (uc_project_get_working_path (), "/projects", NULL);
  uc_utils_mkdirs (path, TRUE);
  g_free (path), path = NULL;
}

/**
 * uc_application_menu_init:
 * 
 * First menu items management.
 */
static void
uc_application_menu_init (void)
{
  /* no treeview, then no display options */
  WSENS ("mwm_display", FALSE);
  WSENS ("mwm_find", FALSE);

  uc_project_set_save (FALSE);
  uc_project_set_save_bookmarks (FALSE);

  WSENS ("bt_ignore_progress_dialog", TRUE);
  WSENS ("bt_suspend_progress_dialog", TRUE);

  WSENS ("mwm_delete_all_bad_links", FALSE);
  WSENS ("mwm_report_export", FALSE);
  WSENS ("mwm_delete_project", FALSE);
  WSENS ("mwm_refresh_all", FALSE);
  WSENS ("mwm_refresh_main_page", FALSE);
  WSENS ("mwm_project_properties", FALSE);
#ifndef ENABLE_GNUTLS
  WSENS ("mwm_https_protocol", FALSE);
#endif
#ifndef ENABLE_CLAMAV
  WSENS ("mwm_display_security_alerts", FALSE);
#endif
#ifndef ENABLE_TIDY
  WSENS ("mwm_display_w3c_alerts", FALSE);
#endif
}

/**
 * uc_application_set_status_bar:
 * @progress: 0, or a value if we want progress bar on the
 *            status bar.
 * @msg: message to display in satus bar.
 * 
 * Refresh application status bar.
 */
void
uc_application_set_status_bar (const gfloat progress, const gchar * msg)
{
  GtkWidget *widget = NULL;

  widget = WGET ("status_bar");

  if (progress > 0)
    gnome_appbar_set_progress_percentage (GNOME_APPBAR (widget), progress);
  if (msg != NULL)
    gnome_appbar_push (GNOME_APPBAR (widget), msg);
}

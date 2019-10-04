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
#include "lists.h"
#include "check.h"
#include "project.h"
#include "tooltips.h"
#include "search.h"
#include "bookmarks.h"
#include "web_site.h"
#include "report.h"
#include "utils.h"

#include "callbacks.h"


void
on_settings_cookies_toggled_cb (GtkToggleButton * button, gpointer user_data)
{
  if (strcmp (gtk_widget_get_name (GTK_WIDGET (button)),
              "sd_radio_cookies_accept") == 0)
    WSENS ("sd_frame_alerts", TRUE);
  else
    WSENS ("sd_frame_alerts", FALSE);
}


void
on_settings_w3c_toggled_cb (GtkToggleButton * button, gpointer user_data)
{
  WSENS ("sd_notebook_w3c",
	 gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)));
}


void
on_settings_security_toggled_cb (GtkToggleButton * button, gpointer user_data)
{
  WSENS ("sd_notebook_security",
	 gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)));
}


gboolean on_widget_deactivate_brelease_event (GtkWidget *widget,
                                              GdkEvent *event,
                                              gpointer user_data)
{
  return (event->type == GDK_BUTTON_RELEASE);
}


gboolean on_display_checkbox_status_event (GtkWidget *widget, GdkEvent *event,
                                           gpointer user_data)
{
  const gchar *wname = NULL;
  UCDisplayStatus status;


  if (event->type == GDK_BUTTON_RELEASE)
  {
    wname = gtk_widget_get_name (widget);

    if (strcmp (wname, "mwm_display_goodlinks") == 0)
      status = UC_DISPLAY_STATUS_GOODLINKS;
    else if (strcmp (wname, "mwm_display_badlinks") == 0)
      status = UC_DISPLAY_STATUS_BADLINKS;
    else if (strcmp (wname, "mwm_display_timeouts") == 0)
      status = UC_DISPLAY_STATUS_TIMEOUTS;
    else if (strcmp (wname, "mwm_display_malformedlinks") == 0)
      status = UC_DISPLAY_STATUS_MALFORMEDLINKS;
    else if (strcmp (wname, "mwm_display_security_alerts") == 0)
      status = UC_DISPLAY_STATUS_SECURITY_ALERTS;
    else if (strcmp (wname, "mwm_display_w3c_alerts") == 0)
      status = UC_DISPLAY_STATUS_W3C_ALERTS;
    else
      g_assert_not_reached ();

    if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget)))
    {
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), TRUE);
      uc_application_reset_menu_types ();
      uc_application_reset_menu_protocols ();
      UC_DISPLAY_STATUS_SET (status);
    }
    else
    {
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), FALSE);
      UC_DISPLAY_STATUS_UNSET (status);
    }
  
    uc_application_main_tree_apply_filters ("mwm_link_status");

    return TRUE;
  }

  return FALSE;
}


gboolean on_display_checkbox_type_event (GtkWidget *widget, GdkEvent *event,
                                         gpointer user_data)
{
  const gchar *wname = NULL;
  UCDisplayType type;


  if (event->type == GDK_BUTTON_RELEASE)
  {
    wname = gtk_widget_get_name (widget);

    if (strcmp (wname, "mwm_document_type") == 0)
      type = UC_DISPLAY_TYPE_HREF;
    else if (strcmp (wname, "mwm_image_type") == 0)
      type = UC_DISPLAY_TYPE_IMAGE;
    else if (strcmp (wname, "mwm_frame_type") == 0)
      type = UC_DISPLAY_TYPE_FRAME;
    else if (strcmp (wname, "mwm_email_type") == 0)
      type = UC_DISPLAY_TYPE_EMAIL;
    else if (strcmp (wname, "mwm_stylesheet_type") == 0)
      type = UC_DISPLAY_TYPE_CSS;
    else
      g_assert_not_reached ();

    if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget)))
    {
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), TRUE);
      uc_application_reset_menu_status ();
      uc_application_reset_menu_protocols ();
      UC_DISPLAY_TYPE_SET (type);
    }
    else
    {
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), FALSE);
      UC_DISPLAY_TYPE_UNSET (type);
    }
  
    uc_application_main_tree_apply_filters ("mwm_link_type");

    return TRUE;
  }

  return FALSE;
}


gboolean on_display_checkbox_protocol_event (GtkWidget *widget, GdkEvent *event,
                                             gpointer user_data)
{
  const gchar *wname = NULL;
  UCDisplayProto proto;


  if (event->type == GDK_BUTTON_RELEASE)
  {
    wname = gtk_widget_get_name (widget);

    if (strcmp (wname, "mwm_http_protocol") == 0)
      proto = UC_DISPLAY_PROTO_HTTP;
    else if (strcmp (wname, "mwm_file_protocol") == 0)
      proto = UC_DISPLAY_PROTO_FILE;
    else if (strcmp (wname, "mwm_https_protocol") == 0)
      proto = UC_DISPLAY_PROTO_HTTPS;
    else if (strcmp (wname, "mwm_ftp_protocol") == 0)
      proto = UC_DISPLAY_PROTO_FTP;
    else
      g_assert_not_reached ();

    if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget)))
    {
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), TRUE);
      uc_application_reset_menu_status ();
      uc_application_reset_menu_types ();
      UC_DISPLAY_PROTO_SET (proto);
    }
    else
    {
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), FALSE);
      UC_DISPLAY_PROTO_UNSET (proto);
    }
  
    uc_application_main_tree_apply_filters ("mwm_protocol");

    return TRUE;
  }

  return FALSE;
}


void
on_search_close_clicked (GtkButton * button, gpointer user_data)
{
  uc_search_set_exit (TRUE);

  if (treestore_search != NULL)
    g_object_unref (G_OBJECT (treestore_search)), treestore_search = NULL;

  WSENS ("mwm_find", TRUE);
  WSENS ("mwm_project", TRUE);
  WSENS ("mw_bt_new", TRUE);
  WSENS ("mw_bt_open", TRUE);
}

void
on_view_link_content_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  uc_application_view_source_dialog_show ();
}

void
on_w3c_validate_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  uc_application_w3c_validate ();
}

void
on_quit_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  if (!uc_application_quit ())
    gtk_exit (0);
}

void
on_about_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  uc_application_about_dialog_show ();
}


void
on_settings_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  uc_application_settings_dialog_show ();
}


void
on_display_all_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  uc_application_main_tree_display_all ();
}

void
on_display_collapse_all_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  uc_application_main_tree_collapse_all ();
}

void
on_display_expand_all_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  uc_application_main_tree_expand_all ();
}

void
on_bt_ok_settings_dialog_clicked (GtkButton * button, gpointer user_data)
{
  if (uc_application_settings_get_data ())
    gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}

void
on_bt_new_project_clicked (GtkButton * button, gpointer user_data)
{
  uc_application_new_web_dialog_show ();
}

void
on_bt_apply_settings_dialog_clicked (GtkButton * button, gpointer user_data)
{
  uc_application_settings_get_data ();
}

void
on_treeview_projects_cursor_changed (GtkTreeView * selection,
				     gpointer user_data)
{
  uc_project_display_informations (selection);
}

void
on_urls_list_cursor_changed (GtkTreeView * selection, gpointer user_data)
{
  uc_application_display_informations (selection);
}

void
on_search_list_cursor_changed (GtkTreeView * selection, gpointer user_data)
{
  UC_UPDATE_UI;
  uc_search_main_select (selection);
}

gboolean
on_url_list_mouse_clicked (GtkWidget * widget, GdkEventButton * event,
			   gpointer data)
{
  /* show page properties on left
   * double-click */
  if (event->type == GDK_2BUTTON_PRESS)
    uc_application_page_information_dialog_show ();

  /* show contextual popup menu on right
   * button release */
  else if ((event->type == GDK_BUTTON_RELEASE) && (event->button == 3))
    uc_application_treeview_activate_popup (event);

  return FALSE;
}

gboolean
on_project_list_mouse_clicked (GtkWidget * widget, GdkEventButton * event,
			       gpointer data)
{
  gint32 id = 0;

  /* open project on left double-click */
  if (event->type == GDK_2BUTTON_PRESS)
    {
      id = uc_project_treeview_get_selected_row_id ();
      if (id > 0)
	{
	  gtk_widget_hide (gtk_widget_get_toplevel (widget));
	  uc_project_open (id);
	}
    }

  return FALSE;
}

void
on_bt_cancel_progress_dialog_clicked (GtkButton * button, gpointer user_data)
{
  uc_application_progress_dialog_set_modal (TRUE);
  uc_check_cancel_set_value (TRUE);
}

void
on_bt_ignore_progress_dialog_clicked (GtkButton * button, gpointer user_data)
{
  uc_check_ignore_item_set_value (TRUE);
}

void
on_progress_dialog_close (GtkDialog * dialog, gpointer user_data)
{
  uc_check_cancel_set_value (TRUE);
}

void
on_dialog_close_activate (GtkButton * button, gpointer user_data)
{
  gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}

gboolean
on_dialog_delete_event (GtkWidget * widget,
			GdkEvent * event, gpointer user_data)
{
  const gchar *name = NULL;

  name = gtk_widget_get_name (widget);

  if (!strcmp (name, "relations_dialog"))
    {
      uc_utils_clear_container (GTK_CONTAINER (widget));
    }
  else if (!strcmp (name, "open_project_dialog"))
    {
      uc_application_set_status_bar (0, "");
    }
  else if (!strcmp (name, "progress_dialog"))
    {
      uc_check_cancel_set_value (TRUE);
    }
  else if (!strcmp (name, "search_dialog"))
    {
      on_search_close_clicked (NULL, NULL);
    }

  gtk_widget_hide (widget);

  return TRUE;
}

gboolean
on_progress_dialog_delete_event (GtkWidget * widget,
				 GdkEvent * event, gpointer user_data)
{
  uc_check_cancel_set_value (TRUE);

  return TRUE;
}

void
on_bt_basic_display_format_clicked (GtkButton * button, gpointer user_data)
{
  uc_application_display_informations (GTK_TREE_VIEW (user_data));
}

void
on_bt_detailed_display_format_clicked (GtkButton * button, gpointer user_data)
{
  uc_application_display_informations (GTK_TREE_VIEW (user_data));
}

gboolean
on_key_press_event (GtkWidget * widget,
		    GdkEventKey * event, gpointer user_data)
{
  const gchar *b = NULL;
  const gchar *e = NULL;
  const gchar *name = NULL;
  gchar *prefix = NULL;
  gchar *bt_ok_name = NULL;

  if (event->keyval != GDK_Return)
    return FALSE;

  /* Retrieve widget name prefix to guess the "Ok" button name */
  if (!(name = gtk_widget_get_name (widget)) ||
      !(b = name) ||
      !(e = strchr (name, '_')) || !(prefix = uc_utils_strdup_delim (b, e)))
    {
      g_warning ("Error while proccessing ENTER key signal event.");
      g_free (prefix), prefix = NULL;
      return FALSE;
    }

  bt_ok_name = g_strdup_printf ("%s_bt_ok", prefix);

  gtk_widget_activate (WGET (bt_ok_name));

  g_free (bt_ok_name), bt_ok_name = NULL;
  g_free (prefix), prefix = NULL;

  return TRUE;
}

void
on_new_instance_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  uc_application_new_instance_launch ();
}

void
on_report_export_html_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  uc_report_export (UC_EXPORT_HTML, NULL, UC_EXPORT_LIST_NORMAL);
}

void
on_bt_export_csv_similar_links_clicked (GtkButton * button, gpointer user_data)
{
  UCLinkProperties *prop = NULL;
  gint32 id = 0;


  id = uc_check_treeview_get_selected_row_id ();
  if (id < 0)
    return;

  prop = uc_lists_checked_links_lookup_by_uid (id);

  uc_report_export (UC_EXPORT_CSV, prop->similar_links_parents,
                    UC_EXPORT_LIST_SIMPLE);
}

void
on_bt_export_csv_page_links_clicked (GtkButton * button, gpointer user_data)
{
  UCLinkProperties *prop = NULL;
  gint32 id = 0;


  id = uc_check_treeview_get_selected_row_id ();
  if (id < 0)
    return;

  prop = uc_lists_checked_links_lookup_by_uid (id);

  uc_report_export (UC_EXPORT_CSV, prop->all_links, UC_EXPORT_LIST_SIMPLE);
}

void
on_report_export_csv_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  uc_report_export (UC_EXPORT_CSV, uc_lists_checked_links_get (),
                    UC_EXPORT_LIST_NORMAL);
}

void on_page_information_dialog_switch_page (GtkNotebook *notebook,
                                             GtkNotebookPage *page,
                                             guint page_num,
                                             gpointer user_data) 
{
  // Set visible "CSV export" button if we ar on "Links" page
  if (page_num == 1)
    gtk_widget_show (WGET ("bt_page_information_dialog_export_csv"));
  else
    gtk_widget_hide (WGET ("bt_page_information_dialog_export_csv"));
}

void
on_bt_suspend_progress_dialog_clicked (GtkButton * button, gpointer user_data)
{
  uc_check_suspend_continue ();
}

void on_view_bad_extensions_activate
  (GtkMenuItem * menuitem, gpointer user_data)
{
  uc_application_view_bad_extensions_dialog_show ();
}

void on_view_similar_links_locations_activate
  (GtkMenuItem * menuitem, gpointer user_data)
{
  uc_application_view_similar_links_dialog_show ();
}

void
on_view_online_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  UCLinkProperties *prop = NULL;
  gint32 id = 0;

  id = uc_check_treeview_get_selected_row_id ();
  if (id > 0)
    {
      prop = uc_lists_checked_links_lookup_by_uid (id);
      uc_application_launch_web_browser (prop->url);
    }
  else
    uc_application_dialog_show (_
				("Please, select a item."),
				GTK_MESSAGE_WARNING);
}

void
on_page_properties_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  gint32 id = 0;

  id = uc_check_treeview_get_selected_row_id ();
  if (id > 0)
    uc_application_page_information_dialog_show ();
  else
    uc_application_dialog_show (_
				("Please, select a item."),
				GTK_MESSAGE_WARNING);
}

void
on_view_parent_page_online_activate (GtkMenuItem * menuitem,
				     gpointer user_data)
{
  UCLinkProperties *prop = NULL;
  gchar *parent_url = NULL;
  gint32 id = 0;

  id = uc_check_treeview_get_selected_row_id ();
  if (id > 0)
    {
      prop = uc_lists_checked_links_lookup_by_uid (id);
      parent_url =
	(prop->parent) ? (prop->parent)->url : uc_project_get_url ();
      uc_application_launch_web_browser (parent_url);
    }
  else
    uc_application_dialog_show (_
				("Please, select a item."),
				GTK_MESSAGE_WARNING);
}

void
on_bt_add_filter_directory_clicked (GtkButton * button, gpointer user_data)
{
  uc_application_add_filter_directory_add ();
}

void
on_bt_add_filter_domain_clicked (GtkButton * button, gpointer user_data)
{
  uc_application_add_filter_domain_add ();
}

void
on_bt_remove_filter_domain_clicked (GtkButton * button, gpointer user_data)
{
  uc_application_add_filter_domain_remove ();
}

void
on_bt_remove_filter_directory_clicked (GtkButton * button, gpointer user_data)
{
  uc_application_add_filter_directory_remove ();
}

void
on_bt_clear_filter_directory_clicked (GtkButton * button, gpointer user_data)
{
  gtk_list_store_clear (treestore_filter_directories);
}

void
on_bt_clear_filter_domain_clicked (GtkButton * button, gpointer user_data)
{
  gtk_list_store_clear (treestore_filter_domains);
}

void
on_check_email_address_clicked (GtkButton * button, gpointer user_data)
{
  GtkWidget *widget1 = NULL;
  GtkWidget *widget2 = NULL;
  gboolean active = FALSE;

  widget1 = WGET ("sd_check_email_address");
  widget2 = WGET ("sd_check_email_mx");

  active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget1));
  if (!active)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget2), FALSE);

  WSENS ("sd_check_email_mx", active);
}

void
on_menu_check_email_mx_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  uc_check_mx_is_valid ();
}

void
on_image_preview_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  uc_application_view_image_dialog_show ();
}

void
  on_dynamic_values_time_between_value_changed
  (GtkSpinButton * spinbutton, gpointer user_data)
{
  GtkWidget *widget = NULL;

  widget = WGET ("pd_dynamic_values_time_between");
  uc_project_set_check_wait (gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
							       (widget)));
}

void
  on_dynamic_values_security_checks_value_toggled
  (GtkToggleButton * togglebutton, gpointer user_data)
{
  GtkWidget *widget = NULL;

  widget = WGET ("pd_dynamic_values_security_checks");
  uc_project_set_security_checks ("any", gtk_toggle_button_get_active
				  (GTK_TOGGLE_BUTTON (widget)));
}

void
  on_dynamic_values_timeout_value_changed
  (GtkSpinButton * spinbutton, gpointer user_data)
{
  GtkWidget *widget = NULL;

  widget = WGET ("pd_dynamic_values_timeout");
  uc_project_set_check_timeout (gtk_spin_button_get_value_as_int
				(GTK_SPIN_BUTTON (widget)));
}

void
on_search_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  uc_application_new_search_dialog_show ();
}

void
on_new_search_button_clicked (GtkButton * button, gpointer user_data)
{
  uc_application_search_get_data ();
}

void
on_clear_project_history_clicked (GtkButton * button, gpointer user_data)
{
  gnome_entry_clear_history (
    GNOME_ENTRY (gnome_entry_new ("web_project")));
  gnome_entry_clear_history (
    GNOME_ENTRY (gnome_entry_new ("bookmarks_project")));
  gnome_entry_clear_history (
    GNOME_ENTRY (gnome_entry_new ("local_file_project")));
}

void
on_clear_search_history_clicked (GtkButton * button, gpointer user_data)
{
  gnome_entry_clear_history (GNOME_ENTRY (gnome_entry_new ("search")));
}

void
on_clear_export_history_clicked (GtkButton * button, gpointer user_data)
{
  gnome_entry_clear_history (GNOME_ENTRY (gnome_entry_new ("export_path")));
}

void
on_new_search_check_button_toggled (GtkToggleButton * togglebutton,
				    gpointer user_data)
{
  uc_search_check_verification ();
}

void
on_web_site_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  uc_application_new_web_dialog_show ();
}

void
on_local_file_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  uc_application_new_local_file_dialog_show ();
}

void
on_bookmarks_file_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  uc_application_new_bookmarks_dialog_show ();
}

void
on_bt_ok_new_bookmarks_project_clicked (GtkButton * button,
					gpointer user_data)
{
  static gboolean running = FALSE;

  if (!running)
    {
      running = TRUE;
      uc_application_get_bookmarks_project_data ();
      running = FALSE;
    }
}


void
on_delete_all_bad_links_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  gint res = 0;


  res = uc_application_dialog_yes_no_show (
      _("All bookmarks referring to not found page will be removed.\n"
        "<b>Do you want to continue</b>?"), GTK_MESSAGE_QUESTION);
  if (res != GTK_RESPONSE_YES)
    return;

  uc_application_main_tree_display_all ();
  gtk_tree_view_expand_all (treeview);

  uc_bookmarks_delete_bad_links (uc_lists_checked_links_get ());
  uc_project_set_save_bookmarks (TRUE);

  uc_application_main_tree_display_all ();
  gtk_tree_view_expand_all (treeview);
}


void
on_delete_link_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  gint32 id = 0;
  gint res = 0;
  GtkTreeIter iter;
  UCLinkProperties *prop = NULL;


  id = uc_check_treeview_get_selected_row_id ();
  if (id > 0)
  {
    res = uc_application_dialog_yes_no_show (
        _("Selected link will be removed.\n"
          "<b>Do you want to continue</b>?"), GTK_MESSAGE_QUESTION);
    if (res != GTK_RESPONSE_YES)
      return;

    uc_application_treeview_get_selected_iter (treeview, &iter);
    prop = uc_lists_checked_links_lookup_by_uid (id);

    uc_bookmarks_delete_link (prop);
    uc_project_set_save_bookmarks (TRUE);

    gtk_tree_store_remove (treestore, &iter);
  }
  else
    uc_application_dialog_show (
      _("Please, select a item."), GTK_MESSAGE_WARNING);
}

void
on_save_bookmarks_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  uc_bookmarks_save_changes ();
}

void
on_save_project_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  uc_project_save ();
}

void
on_open_project_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  uc_application_open_project_dialog_show ();
}

void
on_project_properties_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  (gboolean) uc_application_project_information_dialog_show ();
}

void
on_menu_refresh_all_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  // FIXME
  if (uc_project_get_check_is_bookmarks ())
    g_warning ("function not yet implemented!");
  else
    uc_web_site_refresh_all ();
}

void
on_menu_refresh_branch_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  gint32 id = 0;

  id = uc_check_treeview_get_selected_row_id ();
  if (id > 0)
    uc_web_site_refresh_branch (id);
  else
    uc_application_dialog_show (_("Please, select a item."),
                                GTK_MESSAGE_WARNING);
}

void
on_menu_refresh_link_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  gint32 id = 0;

  id = uc_check_treeview_get_selected_row_id ();
  if (id > 0)
    uc_web_site_refresh_link (id);
  else
    uc_application_dialog_show (_("Please, select a item."),
                                GTK_MESSAGE_WARNING);
}

void
on_bt_close_open_project_clicked (GtkButton * button, gpointer user_data)
{
  gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));
  uc_application_set_status_bar (0, "");
}

void
on_bt_ok_open_project_clicked (GtkButton * button, gpointer user_data)
{
  gint32 id = 0;

  id = uc_project_treeview_get_selected_row_id ();
  if (id > 0)
    {
      gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));
      uc_project_open (id);
    }
  else
    uc_application_dialog_show (_("Please, select a project."),
                                GTK_MESSAGE_WARNING);
}

void
on_bt_delete_open_project_clicked (GtkButton * button, gpointer user_data)
{
  gint32 id = 0;

  id = uc_project_treeview_get_selected_row_id ();
  if (id > 0)
    {
      if (uc_application_dialog_yes_no_show
	  (_("Are you sure you want to delete " "this project?"),
	   GTK_MESSAGE_QUESTION) == GTK_RESPONSE_YES)
	uc_project_delete (id);
    }
  else
    uc_application_dialog_show (_("Please, select a project."),
				GTK_MESSAGE_WARNING);
}

void
on_bt_ok_new_web_project_clicked (GtkButton * button, gpointer user_data)
{
  uc_application_project_get_data (UC_PROJECT_TYPE_WEB_SITE);
}

void
on_bt_ok_new_local_file_project_clicked (GtkButton * button,
					 gpointer user_data)
{
  uc_application_project_get_data (UC_PROJECT_TYPE_LOCAL_FILE);
}

gboolean
on_main_window_delete_event (GtkWidget * widget,
			     GdkEvent * event, gpointer user_data)
{
  if (!uc_application_quit ())
    gtk_exit (0);

  return TRUE;
}

gboolean
on_main_treeview_motion_notify_event (GtkWidget * widget,
				      GdkEventButton * event, gpointer data)
{
  uc_tooltips_main_set_mouse_coord (event->x, event->y);

  return FALSE;
}

/**
 * on_main_treeview_enter_notify_event:
 * @widget: Widget.
 * @event: Event.
 * @user_data: Nothing.
 *
 * When mouse enter in the tree view zone. Mainly used by main tree view
 * tooltips.
 * 
 * Returns: Always FALSE.
 */
gboolean
on_main_treeview_enter_notify_event (GtkWidget * widget,
				     GdkEventCrossing * event,
				     gpointer user_data)
{
  uc_tooltips_main_set_frozen (FALSE);

  return FALSE;
}

/**
 * on_main_treeview_leave_notify_event:
 * @widget: Widget.
 * @event: Event.
 * @user_data: Noting.
 *
 * When mouse leace the tree view zone. Mainly used by main tree view 
 * tooltips.
 *
 * Returns: Always FALSE.
 */
gboolean
on_main_treeview_leave_notify_event (GtkWidget * widget,
				     GdkEventCrossing * event,
				     gpointer user_data)
{
  uc_tooltips_main_set_frozen (TRUE);

  if (uc_tooltips_main_get_active ())
    {
      uc_tooltips_main_set_active (FALSE);
      uc_tooltips_main_destroy ();
    }

  return FALSE;
}

void
on_display_tooltips_toggled (GtkToggleButton * togglebutton,
			     gpointer user_data)
{
  uc_tooltips_main_set_display (gtk_toggle_button_get_active (togglebutton));
}

void
on_delete_project_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  if (uc_application_dialog_yes_no_show
      (_("Are you sure you want to delete this project?"),
       GTK_MESSAGE_QUESTION) == GTK_RESPONSE_YES)
    uc_project_delete (uc_project_get_id ());
}

void
on_menu_refresh_parent_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  gint32 id = 0;

  id = uc_check_treeview_get_selected_row_id ();
  if (id > 0)
    uc_web_site_refresh_parent (id);
  else
    uc_application_dialog_show (_
				("Please, select a item."),
				GTK_MESSAGE_WARNING);
}

void
on_menu_refresh_main_page_activate (GtkMenuItem * menuitem,
				    gpointer user_data)
{
  uc_web_site_refresh_link (uc_check_get_main_page_id ());
}

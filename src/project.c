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
#include "html_parser.h"
#include "report.h"
#include "lists.h"
#include "utils.h"
#include "url.h"
#include "bookmarks.h"
#include "cookies.h"

#include "project.h"


struct
{
  guint32 id;
  gchar *title;
  gchar *description;
  guint tooltips_delay;
  UCProjectType type;
  G_CONST_RETURN gchar *local_charset;
  gboolean check_is_current;
  gboolean check_is_main;
  gboolean save;
  gboolean save_bookmarks;
  gchar *url;
  gchar *bookmarks_file;
  UCBookmarksType bookmarks_type;
  gchar *report_export_path;
  gchar *current_host;
  gchar *current_port;
  guint check_wait;
  guint depth_level;
  guint timeouts_blocked;
  gchar *browser_path;
  gchar *working_path;
  gchar *cache_name;
  gboolean proto_file_is_error;
  gboolean proto_file_check;
  gboolean proto_mailto;
  gboolean cookies_accept;
  gboolean cookies_warn_added;
  gboolean cookies_warn_updated;
  gboolean cookies_warn_deleted;
  gboolean proto_ftp;
  gboolean passive_ftp;
  gboolean proto_https;
  gboolean proto_mailto_check_mx;
  gboolean check_chroot;
  gboolean limit_local;
  gboolean speed_check;
  gboolean use_proxy;
  gchar *proxy_host;
  guint proxy_port;
  gchar *chroot_path;
  gboolean images_content;
  gboolean archives_content;
  guint check_timeout;
  gboolean prompt_auth;
  gboolean w3c_checks;
  gboolean w3c_checks_html;
  gchar *w3c_html_level;
  gboolean w3c_checks_css;
  gboolean security_checks;
  gboolean security_checks_bad_extensions;
  gboolean security_checks_exclude_images;
  gboolean security_checks_virii;
  gboolean export_labels;
  gboolean export_numbering;
  gboolean export_external;
  gboolean export_ip;
  gboolean debug_mode;
  gboolean dump_properties;
  gchar *reject_images;
  gchar *reject_documents;
  gchar *html_extensions;
  gchar *css_extensions;
  gchar *virii_extensions;
  gchar *bad_extensions;
  gchar **reject_directories;
  gchar **reject_domains;
  gboolean auth_save;
  gboolean no_urls_args;
  gchar *auth_line;
  GHashTable *cookies;
}
static project;

/*
 * struct containing path for "project type"
 * images
 */
struct
{
  UCProjectType type;
  gchar *icon_file;
}
static uc_project_type_icon[] = {
  {UC_PROJECT_TYPE_WEB_SITE, UC_PIXMAPS_DIR "/menu_new_web_site.png"},
  {UC_PROJECT_TYPE_LOCAL_FILE, UC_PIXMAPS_DIR "/menu_new_local_file.png"},
  {UC_PROJECT_TYPE_BOOKMARKS, UC_PIXMAPS_DIR "/menu_new_bookmarks_file.png"}
};

static GList *uc_project_projects_list = NULL;

/* this flag is TRUE when speed up check option is active
 * and first page have not yet been retreived */
static gboolean uc_project_speed_check_set_download_content_value = TRUE;

static gchar *uc_project_xml_filters_get_value (xmlDocPtr doc,
						xmlNodePtr cur);
static void uc_project_xml_filters_tag (xmlDocPtr doc, xmlNodePtr cur);
static void uc_project_serialize_filters (xmlTextWriterPtr writer);
static void uc_project_serialize_header_cb (gpointer key, gpointer value,
					    gpointer data);
static void uc_project_serialize_header (xmlTextWriterPtr writer,
					 GHashTable * header);
static void uc_project_serialize_checked_links (xmlTextWriterPtr writer,
						GList * list);
static void uc_project_serialize_tags (xmlTextWriterPtr writer,
				       UCHTMLTag * main_tag, GList * tags);
static guint uc_project_get_new_project_id (void);
static void uc_project_projects_list_free (void);
static void uc_project_projects_list_node_free (UCProjectProjects **pl);
static UCProjectProjects *uc_project_projects_list_lookup_by_uid (const gint
								  id);
static GList *uc_project_xml_get_tags (const gchar * docname);
static GList *uc_project_xml_parse_project (GList * list, xmlDocPtr doc,
					    xmlNodePtr cur);
static GList *uc_project_xml_project_tag (GList * list, xmlDocPtr doc,
					  xmlNodePtr cur);
static GList *uc_project_xml_website_tag (GList * list, xmlDocPtr doc,
					  xmlNodePtr cur);
static GList *uc_project_xml_page_tag (UCLinkProperties * parent,
				       GList * list, xmlDocPtr doc,
				       xmlNodePtr cur);
static GList *uc_project_xml_tags_tag (GList * list, xmlDocPtr doc,
				       xmlNodePtr cur);
static GList *uc_project_xml_tag_tag (GList * list, xmlDocPtr doc,
				      xmlNodePtr cur);
static void uc_project_xml_properties_tag (UCLinkProperties * prop,
					   xmlDocPtr doc, xmlNodePtr cur);
static GHashTable *uc_project_xml_header_tag (GHashTable * table,
					      xmlDocPtr doc, xmlNodePtr cur);
static GList *uc_project_xml_similar_links_tag (GList * list, xmlDocPtr doc,
						xmlNodePtr cur);
static GList *uc_project_xml_bad_extensions_tag (GList * list, xmlDocPtr doc,
						 xmlNodePtr cur);
static GList *uc_project_xml_childs_tag (UCLinkProperties * prop,
					 GList * list, xmlDocPtr doc,
					 xmlNodePtr cur);
static void uc_project_xml_dispatch_links (UCLinkProperties * prop);
static void uc_project_xml_report_tag (xmlDocPtr doc, xmlNodePtr cur);
static void uc_project_projects_list_delete (const gint32 id);
static void uc_project_general_settings_create_default (void);
static void uc_project_proxy_init (void);
static void uc_project_already_checked_links_append_all (GList * list);
static void
uc_project_xml_write_attribute (xmlTextWriterPtr writer,
				const gchar * name, gchar * strval,
				const guint numval, const gboolean yesno);
static void
uc_project_serialize_similar_links (xmlTextWriterPtr writer,
				    GList * similar_links);
static void
uc_project_serialize_bad_extensions (xmlTextWriterPtr writer,
				     GList * bad_extensions);
static void
uc_project_xml_write_element (xmlTextWriterPtr writer,
			      const gchar * name, gchar * strval,
			      const guint numval);
static GList *uc_project_xml_load_links (const UCProjectProjects * p);

/**
 * uc_project_get_type_icon:
 * @project_type: a #UCProjectType type.
 * 
 * Returns the corresponding icon for a given link type.
 * 
 * Returns: A #GdkPixbuf icon.
 */
GdkPixbuf *
uc_project_get_type_icon (const UCProjectType project_type)
{
  guint i = 0;

  for (i = 0; i < UC_PROJECT_TYPES_LEN - 1; i++)
    if (uc_project_type_icon[i].type == project_type)
      return gdk_pixbuf_new_from_file (uc_project_type_icon[i].icon_file,
				       NULL);

  return gdk_pixbuf_new_from_file (UC_PIXMAPS_DIR "/list_unknown.png", NULL);
}

/**
 * uc_project_delete:
 * @id: id of the project to delete.
 *
 * Delete a gurlchecker project.
 */
void
uc_project_delete (const gint32 id)
{
  GtkWidget *widget = NULL;
  GtkTextBuffer *text_buffer = NULL;
  gchar *path = NULL;
  GtkTreeIter iter;

  /* delete item on list */
  uc_project_projects_list_delete (id);

  /* delete associated files and dirs on disk */
  path =
    g_strdup_printf ("%s/projects/%u/project.xml",
		     uc_project_get_working_path (), id);
  unlink (path);
  g_free (path), path = NULL;

  path =
    g_strdup_printf ("%s/projects/%u/settings.xml",
		     uc_project_get_working_path (), id);
  unlink (path);
  g_free (path), path = NULL;

  path =
    g_strdup_printf ("%s/projects/%u/documents/",
		     uc_project_get_working_path (), id);
  uc_utils_rmfiles (path);
  rmdir (path);
  g_free (path), path = NULL;

  path =
    g_strdup_printf ("%s/projects/%u/", uc_project_get_working_path (), id);
  rmdir (path);
  g_free (path), path = NULL;

  /* save new projects XML file */
  uc_project_save_index ();

  widget = WGET ("opd_treeview");
  if (uc_application_treeview_get_selected_iter
      (GTK_TREE_VIEW (widget), &iter))
    {
      gtk_list_store_remove (GTK_LIST_STORE
			     (gtk_tree_view_get_model
			      (GTK_TREE_VIEW (widget))), &iter);

      /* reset properties display */
      widget = WGET ("opd_dates");
      gtk_label_set_text (GTK_LABEL (widget), "\n");

      widget = WGET ("opd_location");
      text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
      gtk_text_buffer_set_text (text_buffer, "", -1);
      gtk_text_view_set_buffer (GTK_TEXT_VIEW (widget), text_buffer);

      widget = WGET ("opd_title");
      text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
      gtk_text_buffer_set_text (text_buffer, "", -1);
      gtk_text_view_set_buffer (GTK_TEXT_VIEW (widget), text_buffer);

      widget = WGET ("opd_description");
      text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
      gtk_text_buffer_set_text (text_buffer, "", -1);
      gtk_text_view_set_buffer (GTK_TEXT_VIEW (widget), text_buffer);
    }

  /* id we are deleting the current project,
   * clean UI */
  if (id == uc_project_get_id ())
    uc_application_init (NULL, NULL, NULL, FALSE);
}


static void
uc_project_already_checked_links_append_all (GList * list)
{
  GList *item = NULL;


  item = g_list_first (list);
  while (item != NULL)
  {
    gchar *normalized_url = NULL;
    UCLinkProperties *prop = (UCLinkProperties *) item->data;


    normalized_url = uc_url_normalize (uc_project_get_current_host (),
                                       (prop->parent != NULL) ?
                                         prop->parent->path : "", prop->url);
    prop->normalized_url = normalized_url;
    uc_lists_already_checked_links_append (normalized_url, prop);

    if (prop->childs != NULL)
      uc_project_already_checked_links_append_all (prop->childs);

    item = g_list_next (item);
  }
}


/**
 * uc_project_open:
 * @id: id of the project to open.
 *
 * Open a existant gurlchecker project.
 * 
 * See: uc_project_xml_load_links (),
 *      uc_project_xml_load_settings ()
 * 
 * Returns: %TRUE if the project have been open correctly.
 */
gboolean
uc_project_open (const gint32 id)
{
  UCProjectProjects *p = NULL;
  gchar *tmp = NULL;
  gchar *path = NULL;
  gchar *path2 = NULL;
  GtkWidget *widget = NULL;
  GtkTreeIter iter;

  uc_project_new ();

  WSENS ("menu_bar", FALSE);
  WSENS ("tool_bar", FALSE);

  uc_application_draw_main_frames ();
  UC_UPDATE_UI;

  UC_GET_WIDGET ("urls_list", WGET ("main_window"), widget);
  treeview = GTK_TREE_VIEW (widget);

  /* open project and load XML elements */
  p = uc_project_projects_list_lookup_by_uid (id);

  /* load specific settings for this new project */
  uc_project_xml_load_settings (p);

  /* status bar message */
  tmp = g_strdup_printf (_("Loading [%s] project..."), p->title);
  uc_application_set_status_bar (0, tmp);
  g_free (tmp), tmp = NULL;
  UC_UPDATE_UI;

  /* load links */
  uc_lists_checked_links_set (uc_project_xml_load_links (p));

  /* copy project files in the current cache
   * file */
  path = g_strconcat (uc_project_get_working_path (), "/",
		      uc_project_get_cache_name (), NULL);
  path2 = g_strdup_printf ("%s/projects/%u/documents/",
			   uc_project_get_working_path (),
			   uc_project_get_id ());
  uc_utils_copy_files (path2, path);
  g_free (path2), path2 = NULL;
  g_free (path), path = NULL;

  // rebuild already checked global list and normalized_url field
  // (this one for each page)
  uc_project_already_checked_links_append_all (uc_lists_checked_links_get ());

  /* build tree view and display it */
  treestore = gtk_tree_store_new (N_COLUMNS,
				  G_TYPE_INT,
				  GDK_TYPE_PIXBUF,
				  GDK_TYPE_PIXBUF,
				  GDK_TYPE_PIXBUF,
				  GDK_TYPE_PIXBUF,
				  G_TYPE_STRING,
				  G_TYPE_STRING, G_TYPE_STRING,
				  G_TYPE_STRING);

  UC_DISPLAY_SET_ALL;
  uc_check_display_list (uc_lists_checked_links_get (), NULL, iter);

  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (treestore));

  uc_application_build_url_treeview ();

  /* activate some menu items */

  /* FIXME
   * for the moment we are not able to manage projects
   * for bookmarks files... */
  WSENS ("mwm_project_properties", !uc_project_get_check_is_bookmarks ());
  WSENS ("mwm_refresh_all", TRUE);
  WSENS ("mwm_refresh_main_page", !uc_project_get_check_is_bookmarks ());
  WSENS ("mwm_delete_project", TRUE);
  WSENS ("mwm_find", TRUE);
  WSENS ("mwm_display", TRUE);
  WSENS ("mwm_report_export", TRUE);
#ifndef ENABLE_GNUTLS
  if (strstr (p->location, UC_PROTOCOL_HTTPS))
  WSENS ("mwm_refresh_main_page", FALSE);
#endif
#ifdef ENABLE_CLAMAV
  WSENS ("mwm_display_security_alerts", !uc_project_get_speed_check ());
#endif

  uc_report_display_update ();

  /* main title */
  tmp = g_strconcat ("gURLChecker ", UC_VERSION, " - ", p->title,
		     " [", p->location, "]", NULL);
  gtk_window_set_title (GTK_WINDOW (WGET ("main_window")), tmp);
  g_free (tmp), tmp = NULL;

  widget = WGET ("mw_label_ellapsed_time");
  gtk_label_set_markup (GTK_LABEL (widget), "");

  uc_application_set_status_bar (0, "");

  WSENS ("menu_bar", TRUE);
  WSENS ("tool_bar", TRUE);

  widget = WGET ("mw_project_type");
  if (uc_project_get_type () == UC_PROJECT_TYPE_LOCAL_FILE)
    gtk_label_set_markup (GTK_LABEL (widget),
			  _("[<b>Local file</b>] Page information"));
  else
    gtk_label_set_markup (GTK_LABEL (widget),
			  _("[<b>Web site</b>] Page information"));

  return TRUE;
}

/**
 * uc_project_treeview_get_selected_row_id:
 *
 * Identify the project corresponding to the currently selected row
 * and return its id.
 * 
 * Returns: id of the project.
 */
gint
uc_project_treeview_get_selected_row_id (void)
{
  GtkTreeIter iter;
  GtkTreeModel *model = NULL;
  GtkTreeSelection *select = NULL;
  GtkWidget *widget = NULL;
  gint32 ret = -1;

  widget = WGET ("opd_treeview");

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
  if (select == NULL)
    return ret;

  if (gtk_tree_selection_get_selected (select, &model, &iter))
    gtk_tree_model_get (model, &iter, 0, &ret, -1);

  return ret;
}

/**
 * uc_project_projects_list_delete:
 * @id: id of a project.
 *
 * Delete a project from the internal project's list.
 */
static void
uc_project_projects_list_delete (const gint32 id)
{
  UCProjectProjects *pl = NULL;

  
  pl = uc_project_projects_list_lookup_by_uid (id);
  if (pl != NULL)
    uc_project_projects_list_node_free (&pl);
}

/**
 * uc_project_projects_list_lookup_by_uid:
 * @id: id of a project.
 *
 * Lookup for a project in the internal project's list and return
 * the corresponding #UCProjectProjects node.
 * 
 * Returns: The project object.
 */
static UCProjectProjects *
uc_project_projects_list_lookup_by_uid (const gint32 id)
{
  GList *item = NULL;

  if (uc_project_projects_list == NULL)
    uc_project_projects_list_load ();

  item = g_list_first (uc_project_get_projects_list ());
  while (item != NULL)
    {
      UCProjectProjects *pl = (UCProjectProjects *) item->data;

      if (pl->id == id)
	return pl;

      item = g_list_next (item);
    }

  return NULL;
}

/**
 * uc_project_display_informations:
 * @treeview: the treeview where projects are listed.
 *
 * Retreive the currently selected project on the list and display its
 * informations in the project's dialog.
 */
void
uc_project_display_informations (GtkTreeView * treeview)
{
  gint32 id = 0;
  UCProjectProjects *pl = NULL;
  GtkWidget *widget = NULL;
  GtkTextBuffer *text_buffer = NULL;
  gchar *tmp = NULL;
  gchar creation_date[UC_BUFFER_DATE_LEN + 1] = { 0 };
  gchar update_date[UC_BUFFER_DATE_LEN + 1] = { 0 };

  id = uc_project_treeview_get_selected_row_id ();
  if (id < 0)
    {
      uc_application_dialog_show (_
				  ("Please, select a item."),
				  GTK_MESSAGE_WARNING);

      return;
    }

  pl = uc_project_projects_list_lookup_by_uid (id);

  strftime (creation_date, UC_BUFFER_DATE_LEN, "%Y-%m-%d",
            localtime (&pl->create));
  strftime (update_date, UC_BUFFER_DATE_LEN, "%Y-%m-%d",
            localtime (&pl->update));

  widget = WGET ("opd_dates");
  tmp = g_strdup_printf (_("<i>Created on</i> %s\n"
			   "<i>Updated on</i> %s"), creation_date,
			 update_date);
  gtk_label_set_markup (GTK_LABEL (widget), tmp);
  g_free (tmp), tmp = NULL;

  widget = WGET ("opd_location");
  text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
  gtk_text_buffer_set_text (text_buffer, pl->location, -1);
  gtk_text_view_set_buffer (GTK_TEXT_VIEW (widget), text_buffer);

  widget = WGET ("opd_title");
  text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
  gtk_text_buffer_set_text (text_buffer, pl->title, -1);
  gtk_text_view_set_buffer (GTK_TEXT_VIEW (widget), text_buffer);

  widget = WGET ("opd_description");
  text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
  gtk_text_buffer_set_text (text_buffer, pl->description, -1);
  gtk_text_view_set_buffer (GTK_TEXT_VIEW (widget), text_buffer);

  tmp = g_strdup_printf ("[%s], %s", pl->title, pl->location);
  uc_application_set_status_bar (0, tmp);
  g_free (tmp), tmp = NULL;
}

/**
 * uc_project_save_properties:
 *
 * Save properties for the current project.
 * 
 * Returns: %TRUE if all required properties for a project have been
 *          enter.
 */
gboolean
uc_project_save_properties (void)
{
  GtkWidget *widget = NULL;
  gchar *tmp = NULL;
  GtkTextIter start;
  GtkTextIter end;
  GtkTextBuffer *buffer = NULL;

  widget = WGET ("pid_title");
  tmp = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));
  if (tmp == NULL || strlen (g_strstrip (tmp)) == 0)
    {
      uc_application_dialog_show (_("Please, enter a title for the current "
				    "project."), GTK_MESSAGE_ERROR);

      g_free (tmp), tmp = NULL;

      return FALSE;
    }
  g_free (tmp), tmp = NULL;
  uc_project_set_title (gtk_entry_get_text (GTK_ENTRY (widget)));

  widget = WGET ("pid_description");
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
  gtk_text_buffer_get_bounds (buffer, &start, &end);
  tmp = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
  uc_project_set_description (tmp);
  g_free (tmp), tmp = NULL;

  gtk_widget_hide (WGET ("project_information_dialog"));

  if (!uc_project_get_check_is_bookmarks ())
  {
    uc_project_set_save (TRUE);
    WSENS ("mwm_delete_project", TRUE);
  }

  return TRUE;
}

/**
 * uc_project_serialize_similar_links:
 * @writer: #xmlTextWriterPtr from libxml2 API.
 * @similar_links: slimilar links list.
 * 
 * Save nodes of the "similar links" internal list in XML format on
 * the disk.
 */
static void
uc_project_serialize_similar_links (xmlTextWriterPtr writer,
				    GList * similar_links)
{
  GList *item = NULL;

  xmlTextWriterStartElement (writer, BAD_CAST "similar_links");

  item = g_list_first (similar_links);
  while (item != NULL)
    {
      uc_project_xml_write_element (writer, "item", (gchar *) item->data, 0);

      item = g_list_next (item);
    }

  xmlTextWriterEndElement (writer);
}

/**
 * uc_project_serialize_bad_extensions:
 * @writer: #xmlTextWriterPtr from libxml2 API.
 * @bad_extensions: Bad extensions list.
 * 
 * Save nodes of the "Bad extensions" internal list in XML format on
 * the disk.
 */
static void
uc_project_serialize_bad_extensions (xmlTextWriterPtr writer,
				     GList * bad_extensions)
{
  GList *item = NULL;

  xmlTextWriterStartElement (writer, BAD_CAST "bad_extensions");

  item = g_list_first (bad_extensions);
  while (item != NULL)
    {
      uc_project_xml_write_element (writer, "item", (gchar *) item->data, 0);

      item = g_list_next (item);
    }

  xmlTextWriterEndElement (writer);
}

/**
 * uc_project_serialize_tags:
 * @writer: #xmlTextWriterPtr from libxml2 API.
 * @main_tag: main tag (link node).
 * @tags: list of the main tag childs.
 *
 * Save the tag and all its childs in XML format on the disk.
 * A #UCHTMLTag tag contains properties of a link.
 */
static void
uc_project_serialize_tags (xmlTextWriterPtr writer, UCHTMLTag * main_tag,
			   GList * tags)
{
  GList *item = NULL;

  xmlTextWriterStartElement (writer, BAD_CAST "tag");
  if (main_tag != NULL)
    {
      uc_project_xml_write_attribute (writer, "depth", NULL,
				      main_tag->depth, FALSE);
      uc_project_xml_write_attribute (writer, "type", NULL,
				      main_tag->type, FALSE);
      uc_project_xml_write_attribute (writer, "label", main_tag->label, 0,
				      FALSE);
      uc_project_xml_write_attribute (writer, "value", main_tag->value, 0,
				      FALSE);
    }

  xmlTextWriterEndElement (writer);

  xmlTextWriterStartElement (writer, BAD_CAST "tags");

  item = g_list_first (tags);
  while (item != NULL)
    {
      UCHTMLTag *tag = (UCHTMLTag *) item->data;

      xmlTextWriterStartElement (writer, BAD_CAST "tag");
      uc_project_xml_write_attribute (writer, "depth", NULL, tag->depth,
				      FALSE);
      uc_project_xml_write_attribute (writer, "type", NULL, tag->type, FALSE);
      uc_project_xml_write_attribute (writer, "label", tag->label, 0, FALSE);
      uc_project_xml_write_attribute (writer, "value", tag->value, 0, FALSE);
      xmlTextWriterEndElement (writer);

      item = g_list_next (item);
    }

  xmlTextWriterEndElement (writer);
}

static void
uc_project_serialize_header_cb (gpointer key, gpointer value, gpointer data)
{
  xmlTextWriterPtr writer = data;

  xmlTextWriterStartElement (writer, BAD_CAST "item");
  uc_project_xml_write_attribute (writer, "name", (gchar *) key, 0, FALSE);
  uc_project_xml_write_attribute (writer, "value", (gchar *) value, 0, FALSE);
  xmlTextWriterEndElement (writer);
}

/**
 * uc_project_serialize_header:
 * @writer: #xmlTextWriterPtr from libxml2 API.
 * @header: HTTP header to save (a #GHashTable).
 *
 * Save HTTP header in XML format on the disk.
 */
static void
uc_project_serialize_header (xmlTextWriterPtr writer, GHashTable * header)
{
  xmlTextWriterStartElement (writer, BAD_CAST "header");
  g_hash_table_foreach ((gpointer) header, uc_project_serialize_header_cb,
			writer);
  xmlTextWriterEndElement (writer);
}

/**
 * uc_project_serialize_filters:
 * @writer: #xmlTextWriterPtr from libxml2 API.
 *
 * gurlchecker URLs filters. they are saved in XML format on the disk.
 */
static void
uc_project_serialize_filters (xmlTextWriterPtr writer)
{
  gchar **tmp = NULL;
  gchar *tmp1 = NULL;
  guint size = 0;
  guint i = 0;

  xmlTextWriterStartElement (writer, BAD_CAST "filters");

  xmlTextWriterStartElement (writer, BAD_CAST "directories");
  uc_project_get_reject_directories (&tmp, &size);
  for (i = 0; i < size; i++)
    uc_project_xml_write_element (writer, "item", tmp[i], 0);
  xmlTextWriterEndElement (writer);

  xmlTextWriterStartElement (writer, BAD_CAST "domains");
  uc_project_get_reject_domains (&tmp, &size);
  for (i = 0; i < size; i++)
    uc_project_xml_write_element (writer, "item", tmp[i], 0);
  xmlTextWriterEndElement (writer);

  xmlTextWriterStartElement (writer, BAD_CAST "documents");
  tmp1 = uc_project_get_reject_documents ();
  tmp = g_strsplit (tmp1, ",", -1);
  for (i = 0; i < uc_utils_vector_length (tmp); i++)
    uc_project_xml_write_element (writer, "item", tmp[i], 0);
  xmlTextWriterEndElement (writer);
  g_strfreev (tmp), tmp = NULL;

  xmlTextWriterStartElement (writer, BAD_CAST "images");
  tmp1 = uc_project_get_reject_images ();
  tmp = g_strsplit (tmp1, ",", -1);
  for (i = 0; i < uc_utils_vector_length (tmp); i++)
    uc_project_xml_write_element (writer, "item", tmp[i], 0);
  xmlTextWriterEndElement (writer);
  g_strfreev (tmp), tmp = NULL;

  xmlTextWriterStartElement (writer, BAD_CAST "virii_extensions");
  tmp1 = uc_project_get_security_virii_extensions ();
  tmp = g_strsplit (tmp1, ",", -1);
  for (i = 0; i < uc_utils_vector_length (tmp); i++)
    uc_project_xml_write_element (writer, "item", tmp[i], 0);
  xmlTextWriterEndElement (writer);
  g_strfreev (tmp), tmp = NULL;

  xmlTextWriterStartElement (writer, BAD_CAST "html_extensions");
  tmp1 = uc_project_get_w3c_html_extensions ();
  tmp = g_strsplit (tmp1, ",", -1);
  for (i = 0; i < uc_utils_vector_length (tmp); i++)
    uc_project_xml_write_element (writer, "item", tmp[i], 0);
  xmlTextWriterEndElement (writer);
  g_strfreev (tmp), tmp = NULL;

  xmlTextWriterStartElement (writer, BAD_CAST "css_extensions");
  tmp1 = uc_project_get_w3c_css_extensions ();
  tmp = g_strsplit (tmp1, ",", -1);
  for (i = 0; i < uc_utils_vector_length (tmp); i++)
    uc_project_xml_write_element (writer, "item", tmp[i], 0);
  xmlTextWriterEndElement (writer);
  g_strfreev (tmp), tmp = NULL;

  xmlTextWriterStartElement (writer, BAD_CAST "bad_extensions");
  tmp1 = uc_project_get_security_bad_extensions ();
  tmp = g_strsplit (tmp1, ",", -1);
  for (i = 0; i < uc_utils_vector_length (tmp); i++)
    uc_project_xml_write_element (writer, "item", tmp[i], 0);
  xmlTextWriterEndElement (writer);
  g_strfreev (tmp), tmp = NULL;

  xmlTextWriterEndElement (writer);
}

/**
 * uc_project_serialize_checked_links:
 * @writer: #xmlTextWriterPtr from libxml2 API.
 * @list: a #GList of already checked links.
 *
 * Save internal list of "already checked links" in XML format on the disk.
 */
static void
uc_project_serialize_checked_links (xmlTextWriterPtr writer, GList * list)
{
  GList *item = NULL;

  item = g_list_first (list);
  while (item != NULL)
    {
      UCLinkProperties *prop = (UCLinkProperties *) item->data;

      xmlTextWriterStartElement (writer, BAD_CAST "page");
      uc_project_xml_write_attribute (writer, "id", NULL, prop->uid, FALSE);
      uc_project_xml_write_attribute (writer, "pid", NULL,
				      (prop->parent) ? prop->parent->uid : 0,
				      FALSE);
      uc_project_xml_write_attribute (writer, "depth", NULL,
				      prop->depth_level, FALSE);
      uc_project_xml_write_attribute (writer, "parsable", NULL,
				      prop->is_parsable, TRUE);
      uc_project_xml_write_attribute (writer, "downloadable", NULL,
				      prop->is_downloadable, TRUE);
      uc_project_xml_write_attribute (writer, "checked", NULL, prop->checked,
				      TRUE);
      uc_project_xml_write_attribute (writer, "virii", NULL, prop->virii,
				      TRUE);
      uc_project_xml_write_attribute (writer, "virname",
				      (prop->virname ==
				       NULL) ? "" : prop->virname, 0, FALSE);
      uc_project_xml_write_attribute (writer, "user_action",
				      (prop->user_action ==
				       NULL) ? "" : prop->user_action,
				      0, FALSE);
      uc_project_xml_write_attribute (writer, "w3c_valid", NULL,
				      prop->w3c_valid, TRUE);
      uc_project_xml_write_attribute (writer, "title",
				      (prop->label ==
				       NULL) ? "" : prop->label, 0, FALSE);
      uc_project_xml_write_attribute (writer, "href", prop->url, 0, FALSE);
      uc_project_xml_write_attribute (writer, "type", NULL, prop->link_type,
				      FALSE);
      uc_project_xml_write_attribute (writer, "original_value",
				      prop->link_value, 0, FALSE);
      uc_project_xml_write_attribute (writer, "current_path",
				      prop->current_path, 0, FALSE);
      uc_project_xml_write_attribute (writer, "domain", prop->domain, 0,
				      FALSE);

      xmlTextWriterStartElement (writer, BAD_CAST "properties");

      uc_project_serialize_header (writer, prop->header);

      xmlTextWriterStartElement (writer, BAD_CAST "url");
      uc_project_xml_write_attribute (writer, "proto", prop->protocol,
				      0, FALSE);
      uc_project_xml_write_attribute (writer, "ip_addr",
                                              prop->ip_addr, 0, FALSE);
      uc_project_xml_write_attribute (writer, "host", prop->h_name, 0, FALSE);
      uc_project_xml_write_attribute (writer, "port", prop->port, 0, FALSE);
      uc_project_xml_write_attribute (writer, "path", prop->path, 0, FALSE);
      uc_project_xml_write_attribute (writer, "args", prop->args, 0, FALSE);
      xmlTextWriterEndElement (writer);

      xmlTextWriterEndElement (writer);

      uc_project_serialize_tags (writer, (UCHTMLTag *) prop->tag, prop->tags);

      uc_project_serialize_similar_links (writer,
					  prop->similar_links_parents);

      uc_project_serialize_bad_extensions (writer, prop->bad_extensions);

      xmlTextWriterStartElement (writer, BAD_CAST "childs");

      if (prop->childs)
	uc_project_serialize_checked_links (writer, prop->childs);

      xmlTextWriterEndElement (writer);
      xmlTextWriterEndElement (writer);

      item = g_list_next (item);
    }
}

/**
 * uc_project_save_index:
 *
 * Save the gurlchecker project's list in XML format on the disk.
 */
void
uc_project_save_index (void)
{
  gchar *path = NULL;
  GList *item = NULL;
  gchar *tmp = NULL;
  xmlTextWriterPtr writer = NULL;
  xmlDocPtr doc = NULL;

  path = g_strconcat (uc_project_get_working_path (), "/projects.xml", NULL);

  writer = xmlNewTextWriterDoc (&doc, 0);
  xmlTextWriterStartDocument (writer, NULL, "UTF-8", NULL);

  xmlTextWriterStartElement (writer, BAD_CAST "projects");

  item = g_list_first (uc_project_get_projects_list ());
  while (item != NULL)
    {
      UCProjectProjects *pl = (UCProjectProjects *) item->data;

      xmlTextWriterStartElement (writer, BAD_CAST "project");

      uc_project_xml_write_attribute (writer, "id", NULL, pl->id, FALSE);
      uc_project_xml_write_attribute (writer, "type", NULL, pl->type, FALSE);

      tmp = g_strdup_printf ("%f", (gdouble) pl->create);
      xmlTextWriterWriteAttribute (writer, BAD_CAST "create",
				   BAD_CAST (xmlChar *) tmp);
      g_free (tmp), tmp = NULL;
      tmp = g_strdup_printf ("%f", (gdouble) pl->update);
      xmlTextWriterWriteAttribute (writer, BAD_CAST "update",
				   BAD_CAST (xmlChar *) tmp);
      g_free (tmp), tmp = NULL;

      uc_project_xml_write_element (writer, "location", pl->location, 0);
      uc_project_xml_write_element (writer, "title", pl->title, 0);
      uc_project_xml_write_element (writer, "description", pl->description,
				    0);

      xmlTextWriterEndElement (writer);

      item = g_list_next (item);
    }

  xmlTextWriterEndElement (writer);

  xmlTextWriterEndDocument (writer);

  xmlFreeTextWriter (writer), writer = NULL;
  xmlSaveFormatFileEnc (path, doc, "UTF-8", 1);
  xmlFreeDoc (doc), doc = NULL;

  g_free (path), path = NULL;
}


/**
 * uc_project_general_settings_create_default:
 *
 * Create main gurlchecker default settings in XML format on the disk.
 * Used when gurlchecker is used for the first time.
 */
static void
uc_project_general_settings_create_default (void)
{
  xmlTextWriterPtr writer = NULL;
  xmlDocPtr doc = NULL;
  gchar *path = NULL;


  path = g_strdup_printf ("%s/settings.xml", uc_project_get_working_path ());

  writer = xmlNewTextWriterDoc (&doc, 0);
  xmlTextWriterStartDocument (writer, NULL, "UTF-8", NULL);

  xmlTextWriterStartElement (writer, BAD_CAST "projects");
  xmlTextWriterStartElement (writer, BAD_CAST "project");

  uc_project_xml_write_attribute (writer, "working_path",
				  uc_project_get_working_path (), 0, FALSE);
  uc_project_xml_write_attribute (writer, "cookies_accept", NULL, 1, TRUE);
  uc_project_xml_write_attribute (writer, "cookies_warn_added", NULL, 1,
				  TRUE);
  uc_project_xml_write_attribute (writer, "cookies_warn_updated", NULL, 0,
				  TRUE);
  uc_project_xml_write_attribute (writer, "cookied_warn_deleted", NULL, 0,
				  TRUE);
  uc_project_xml_write_attribute (writer, "proto_file_is_error", NULL, 1,
				  TRUE);
  uc_project_xml_write_attribute (writer, "proto_file_check", NULL, 0, TRUE);
  uc_project_xml_write_attribute (writer, "proto_mailto", NULL, 0, TRUE);
  uc_project_xml_write_attribute (writer, "proto_mailto_check_mx", NULL,
				  0, TRUE);
  uc_project_xml_write_attribute (writer, "proto_https", NULL, 1, TRUE);
  uc_project_xml_write_attribute (writer, "proto_ftp", NULL, 1, TRUE);
  uc_project_xml_write_attribute (writer, "passive_ftp", NULL, 1, TRUE);
  uc_project_xml_write_attribute (writer, "check_chroot", NULL, 0, TRUE);
  uc_project_xml_write_attribute (writer, "limit_local", NULL, 1, TRUE);
  uc_project_xml_write_attribute (writer, "no_urls_args", NULL, 1, TRUE);
  uc_project_xml_write_attribute (writer, "use_proxy", NULL,
				  uc_project_get_use_proxy (), TRUE);
  uc_project_xml_write_attribute (writer, "proxy_host",
				  uc_project_get_proxy_host (), 0, FALSE);
  uc_project_xml_write_attribute (writer, "proxy_port", NULL,
				  uc_project_get_proxy_port (), FALSE);
  uc_project_xml_write_attribute (writer, "chroot_path", "", 0, FALSE);
  uc_project_xml_write_attribute (writer, "download_images_content", NULL,
				  0, TRUE);
  uc_project_xml_write_attribute (writer, "download_archives_content", NULL,
				  0, TRUE);
  uc_project_xml_write_attribute (writer, "check_timeout", NULL,
				  UC_CHECK_TIMEOUT_DEFAULT, FALSE);
  uc_project_xml_write_attribute (writer, "depth_level", NULL,
				  UC_MAX_DEPTH_LEVEL, FALSE);
  uc_project_xml_write_attribute (writer, "check_wait", NULL,
				  UC_CHECK_WAIT_DEFAULT, FALSE);
  uc_project_xml_write_attribute (writer, "timeouts_blocked", NULL,
				  UC_TIMEOUTS_BLOCKED_DEFAULT, FALSE);
  uc_project_xml_write_attribute (writer, "prompt_auth", NULL, 1, TRUE);
  uc_project_xml_write_attribute (writer, "w3c_checks", NULL, 0, TRUE);
  uc_project_xml_write_attribute (writer, "w3c_checks_html", NULL, 1, TRUE);
  uc_project_xml_write_attribute (writer, "w3c_html_level", "warnings", 0,
				  FALSE);
  uc_project_xml_write_attribute (writer, "w3c_checks_css", NULL, 1, TRUE);
  uc_project_xml_write_attribute (writer, "security_checks", NULL, 0, TRUE);
  uc_project_xml_write_attribute (writer, "security_checks_bad_extensions",
				  NULL, 1, TRUE);
  uc_project_xml_write_attribute (writer, "security_checks_exclude_images",
				  NULL, 1, TRUE);
  uc_project_xml_write_attribute (writer, "security_checks_virii",
				  NULL, 1, TRUE);
  uc_project_xml_write_attribute (writer, "export_labels", NULL, 1, TRUE);
  uc_project_xml_write_attribute (writer, "export_numbering", NULL, 1, TRUE);
  uc_project_xml_write_attribute (writer, "export_external", NULL, 1, TRUE);
  uc_project_xml_write_attribute (writer, "export_ip", NULL, 1, TRUE);
  uc_project_xml_write_attribute (writer, "debug_mode", NULL, 0, TRUE);
  uc_project_xml_write_attribute (writer, "dump_properties", NULL, 0, TRUE);

  uc_project_serialize_filters (writer);

  xmlTextWriterEndElement (writer);
  xmlTextWriterEndElement (writer);

  xmlTextWriterEndDocument (writer);

  xmlFreeTextWriter (writer), writer = NULL;
  xmlSaveFormatFileEnc (path, doc, "UTF-8", 1);
  xmlFreeDoc (doc), doc = NULL;

  g_free (path), path = NULL;
}


/**
 * uc_project_xml_write_attribute:
 * @writer: #xmlTextWriterPtr from libxml2 API.
 * @name: name of the attribute.
 * @strval: string value of the attribute @name if it is a string.
 *          %NULL otherwise.
 * @numval: number value of the attribute @name if it is a numeric.
 * @yesno: if the attribute @name is a boolean, %TRUE or %FALSE.
 *
 * Write a XML attribute depending of its type (strin/number).
 */
static void
uc_project_xml_write_attribute (xmlTextWriterPtr writer,
				const gchar * name, gchar * strval,
				const guint numval, const gboolean yesno)
{
  gchar *tmp = NULL;

  if (strval != NULL)
    tmp = uc_utils_to_utf8 (strval);
  else
    tmp = (yesno) ?
      g_strdup (((numval) ? "yes" : "no")) : g_strdup_printf ("%u", numval);

  xmlTextWriterWriteAttribute (writer, BAD_CAST (xmlChar *) name,
			       BAD_CAST (xmlChar *) tmp);
  g_free (tmp), tmp = NULL;
}

/**
 * uc_project_xml_write_element:
 * @writer: #xmlTextWriterPtr from libxml2 API.
 * @name: name of the attribute.
 * @strval: string value of the attribute @name if it is a string.
 *          %NULL otherwise.
 * @numval: number value of the attribute @name if it is a numeric.
 *
 * Write a XML element depending of its type (strin/number).
 */
static void
uc_project_xml_write_element (xmlTextWriterPtr writer,
			      const gchar * name, gchar * strval,
			      const guint numval)
{
  gchar *tmp = NULL;

  if (strval != NULL)
    tmp = uc_utils_to_utf8 (strval);
  else
    tmp = g_strdup_printf ("%u", numval);

  xmlTextWriterWriteElement (writer, BAD_CAST (xmlChar *) name,
			     BAD_CAST (xmlChar *) tmp);
  g_free (tmp), tmp = NULL;
}

/**
 * uc_project_general_settings_save:
 * 
 * Save the main gurlchecker settings in XML format on the disk.
 */
void
uc_project_general_settings_save (void)
{
  xmlTextWriterPtr writer = NULL;
  xmlDocPtr doc = NULL;
  gchar *tmp = NULL;
  gchar *path = NULL;

  path = g_strdup_printf ("%s/settings.xml", uc_project_get_working_path ());

  writer = xmlNewTextWriterDoc (&doc, 0);
  xmlTextWriterStartDocument (writer, NULL, "UTF-8", NULL);

  xmlTextWriterStartElement (writer, BAD_CAST "projects");
  xmlTextWriterStartElement (writer, BAD_CAST "project");

  uc_project_xml_write_attribute (writer, "working_path",
				  uc_project_get_working_path (), 0, FALSE);
  uc_project_xml_write_attribute (writer, "proto_file_is_error", NULL,
				  uc_project_get_proto_file_is_error (),
				  TRUE);
  uc_project_xml_write_attribute (writer, "cookies_accept", NULL,
				  uc_project_get_cookies_accept (), TRUE);
  uc_project_xml_write_attribute (writer, "cookies_warn_added", NULL,
				  uc_project_get_cookies_warn_added (), TRUE);
  uc_project_xml_write_attribute (writer, "cookies_warn_updated", NULL,
				  uc_project_get_cookies_warn_updated (),
				  TRUE);
  uc_project_xml_write_attribute (writer, "cookies_warn_deleted", NULL,
				  uc_project_get_cookies_warn_deleted (),
				  TRUE);
  uc_project_xml_write_attribute (writer, "proto_file_check", NULL,
				  uc_project_get_proto_file_check (), TRUE);
  uc_project_xml_write_attribute (writer, "proto_mailto", NULL,
				  uc_project_get_proto_mailto (), TRUE);
  uc_project_xml_write_attribute (writer, "proto_https", NULL,
				  uc_project_get_proto_https (), TRUE);
  uc_project_xml_write_attribute (writer, "proto_ftp", NULL,
				  uc_project_get_proto_ftp (), TRUE);
  uc_project_xml_write_attribute (writer, "passive_ftp", NULL,
				  uc_project_get_passive_ftp (), TRUE);
  uc_project_xml_write_attribute (writer, "proto_mailto_check_mx", NULL,
				  uc_project_get_proto_mailto_check_mx (),
				  TRUE);
  uc_project_xml_write_attribute (writer, "check_chroot", NULL,
				  uc_project_get_check_chroot (), TRUE);
  uc_project_xml_write_attribute (writer, "limit_local", NULL,
				  uc_project_get_limit_local (), TRUE);
  uc_project_xml_write_attribute (writer, "no_urls_args", NULL,
				  uc_project_get_no_urls_args (), TRUE);
  uc_project_xml_write_attribute (writer, "use_proxy", NULL,
				  uc_project_get_use_proxy (), TRUE);
  uc_project_xml_write_attribute (writer, "proxy_host",
				  uc_project_get_proxy_host (), 0, FALSE);
  uc_project_xml_write_attribute (writer, "proxy_port", NULL,
				  uc_project_get_proxy_port (), FALSE);
  uc_project_xml_write_attribute (writer, "chroot_path",
				  (tmp =
				   uc_project_get_chroot_path ())? tmp : "",
				  0, FALSE);
  uc_project_xml_write_attribute (writer, "download_images_content", NULL,
				  uc_project_get_download_images_content (),
				  TRUE);
  uc_project_xml_write_attribute (writer, "download_archives_content", NULL,
				  uc_project_get_download_archives_content (),
				  TRUE);
  uc_project_xml_write_attribute (writer, "check_timeout", NULL,
				  uc_project_get_check_timeout (), FALSE);
  uc_project_xml_write_attribute (writer, "depth_level", NULL,
				  uc_project_get_depth_level (), FALSE);
  uc_project_xml_write_attribute (writer, "check_wait", NULL,
				  uc_project_get_check_wait (), FALSE);
  uc_project_xml_write_attribute (writer, "timeouts_blocked", NULL,
				  uc_project_get_timeouts_blocked (), FALSE);
  uc_project_xml_write_attribute (writer, "prompt_auth", NULL,
				  uc_project_get_prompt_auth (), TRUE);
  uc_project_xml_write_attribute (writer, "security_checks", NULL,
				  uc_project_get_security_checks ("any"),
				  TRUE);
  uc_project_xml_write_attribute (writer, "security_checks_bad_extensions",
				  NULL,
				  uc_project_get_security_checks
				  ("bad_extensions"), TRUE);
  uc_project_xml_write_attribute (writer, "security_checks_exclude_images",
				  NULL,
				  uc_project_get_security_checks
				  ("exclude_images"), TRUE);
  uc_project_xml_write_attribute (writer, "security_checks_virii",
				  NULL,
				  uc_project_get_security_checks
				  ("virii"), TRUE);
  uc_project_xml_write_attribute (writer, "w3c_checks", NULL,
				  uc_project_get_w3c_checks ("any"), TRUE);
  uc_project_xml_write_attribute (writer, "w3c_checks_html",
				  NULL,
				  uc_project_get_w3c_checks ("html"), TRUE);
  uc_project_xml_write_attribute (writer, "w3c_html_level",
				  (gchar *) uc_project_get_w3c_html_level (),
				  0, FALSE);
  uc_project_xml_write_attribute (writer, "w3c_checks_css", NULL,
				  uc_project_get_w3c_checks ("css"), TRUE);
  uc_project_xml_write_attribute (writer, "export_labels", NULL,
				  uc_project_get_export_labels (), TRUE);
  uc_project_xml_write_attribute (writer, "export_numbering", NULL,
				  uc_project_get_export_numbering (), TRUE);
  uc_project_xml_write_attribute (writer, "export_external", NULL,
				  uc_project_get_export_external (), TRUE);
  uc_project_xml_write_attribute (writer, "export_ip", NULL,
				  uc_project_get_export_ip (), TRUE);
  uc_project_xml_write_attribute (writer, "debug_mode", NULL,
				  uc_project_get_debug_mode (), TRUE);
  uc_project_xml_write_attribute (writer, "dump_properties", NULL,
				  uc_project_get_dump_properties (), TRUE);

  uc_project_serialize_filters (writer);

  xmlTextWriterEndElement (writer);
  xmlTextWriterEndElement (writer);

  xmlTextWriterEndDocument (writer);

  xmlFreeTextWriter (writer), writer = NULL;
  xmlSaveFormatFileEnc (path, doc, "UTF-8", 1);
  xmlFreeDoc (doc), doc = NULL;

  g_free (path), path = NULL;
}

void
uc_project_report_save (void)
{
  gchar *path = NULL;
  xmlTextWriterPtr writer = NULL;
  xmlDocPtr doc = NULL;

  path = g_strdup_printf ("%s/projects/%u/project.xml",
			  uc_project_get_working_path (),
			  uc_project_get_id ());

  writer = xmlNewTextWriterDoc (&doc, 0);
  xmlTextWriterStartDocument (writer, NULL, "UTF-8", NULL);

  xmlTextWriterStartElement (writer, BAD_CAST "projects");
  xmlTextWriterStartElement (writer, BAD_CAST "project");

  xmlTextWriterStartElement (writer, BAD_CAST "website");
  uc_project_xml_write_attribute (writer, "report_export_path",
				  uc_project_get_report_export_path (), 0,
				  FALSE);
  uc_project_xml_write_attribute (writer, "url", uc_project_get_url (), 0,
				  FALSE);
  uc_project_xml_write_attribute (writer, "current_host",
				  uc_project_get_current_host (), 0, FALSE);
  uc_project_xml_write_attribute (writer, "current_port",
				  uc_project_get_current_port (), 0, FALSE);

  uc_project_serialize_checked_links (writer, uc_lists_checked_links_get ());
  xmlTextWriterEndElement (writer);

  xmlTextWriterStartElement (writer, BAD_CAST "report");
  xmlTextWriterStartElement (writer, BAD_CAST "links");
  uc_project_xml_write_attribute (writer, "all",
				  NULL, uc_report_get_alllinks (), FALSE);
  uc_project_xml_write_attribute (writer, "checked",
				  NULL, uc_report_get_checkedlinks (), FALSE);
  uc_project_xml_write_attribute (writer, "bad",
				  NULL, uc_report_get_badlinks (), FALSE);
  uc_project_xml_write_attribute (writer, "malformed",
				  NULL, uc_report_get_malformedlinks (),
				  FALSE);
  uc_project_xml_write_attribute (writer, "good", NULL,
				  uc_report_get_goodlinks (), FALSE);
  uc_project_xml_write_attribute (writer, "timeout", NULL,
				  uc_report_get_timedoutlinks (), FALSE);
  uc_project_xml_write_attribute (writer, "ignored", NULL,
				  uc_report_get_ignoredlinks (), FALSE);
  xmlTextWriterEndElement (writer);
  xmlTextWriterEndElement (writer);

  xmlTextWriterEndElement (writer);
  xmlTextWriterEndElement (writer);

  xmlTextWriterEndDocument (writer);

  xmlFreeTextWriter (writer), writer = NULL;
  xmlSaveFormatFileEnc (path, doc, "UTF-8", 1);
  xmlFreeDoc (doc), doc = NULL;

  g_free (path), path = NULL;
}

gboolean
uc_project_save_all (void)
{
  if (uc_project_get_save () || uc_project_get_save_bookmarks ())
    {
      gint res = 0;

      if (uc_project_get_save ())
	res = uc_project_save ();
      else if (uc_project_get_save_bookmarks ())
	res = uc_bookmarks_save_changes ();

      if (res == GTK_RESPONSE_YES || res == GTK_RESPONSE_NO)
	;
      else
	return FALSE;
    }

  return TRUE;
}

/**
 * uc_project_save:
 *
 * Save the entire current project (settings, links etc.) in XML format
 * on the disk.
 *
 * Returns: %TRUE if project must be saved.
 */
gint
uc_project_save (void)
{
  gchar *path = NULL;
  gchar *path2 = NULL;
  time_t t;
  guint project_id = 0;
  gint res = 0;
  UCProjectProjects *node = NULL;

  if (uc_project_get_check_is_bookmarks ())
    return uc_bookmarks_save_changes ();

  res = uc_application_dialog_yes_no_show (_("Save the current project ?"),
					   GTK_MESSAGE_QUESTION);
  if (res != GTK_RESPONSE_YES)
    return res;

  if (strlen (uc_project_get_title ()) == 0 &&
      !uc_application_project_information_dialog_show ())
    return GTK_RESPONSE_CANCEL;

  uc_application_set_status_bar (0, _("Saving project..."));
  UC_UPDATE_UI;

  time (&t);
  project_id = uc_project_get_id ();

  if (project_id == 0)
    {
      node = g_new0 (UCProjectProjects, 1);
      node->id = uc_project_get_new_project_id ();
      node->type = uc_project_get_type ();
      node->create = t;
      node->update = t;
      node->location = (uc_project_get_check_is_bookmarks ()) ?
	g_strdup (uc_project_get_bookmarks_file ()) :
	g_strdup (uc_project_get_url ());
      node->title = g_strdup (uc_project_get_title ());
      node->description = g_strdup (uc_project_get_description ());
      uc_project_projects_list =
	g_list_append (uc_project_projects_list, node);

      uc_project_set_id (node->id);
    }
  else
    {
      node = uc_project_projects_list_lookup_by_uid (project_id);

      g_free (node->title), node->title = NULL;
      g_free (node->description), node->description = NULL;
      node->title = g_strdup (uc_project_get_title ());
      node->description = g_strdup (uc_project_get_description ());
      node->update = t;
    }

  uc_project_save_index ();
  uc_project_general_settings_save ();

  path = g_strdup_printf ("%s/projects/%u/documents/",
			  uc_project_get_working_path (),
			  uc_project_get_id ());
  uc_utils_mkdirs (path, TRUE);
  g_free (path), path = NULL;

  path = g_strdup_printf ("%s/settings.xml", uc_project_get_working_path ());
  path2 = g_strdup_printf ("%s/projects/%u/settings.xml",
			   uc_project_get_working_path (),
			   uc_project_get_id ());

  uc_utils_copy (path, path2);

  g_free (path2), path2 = NULL;
  g_free (path), path = NULL;

  uc_project_report_save ();

  path = g_strconcat (uc_project_get_working_path (), "/",
		      uc_project_get_cache_name (), NULL);
  path2 = g_strdup_printf ("%s/projects/%u/documents/",
			   uc_project_get_working_path (),
			   uc_project_get_id ());
  uc_utils_copy_files (path, path2);
  g_free (path), path = NULL;
  g_free (path2), path2 = NULL;

  uc_project_set_save (FALSE);
  uc_project_set_save_bookmarks (FALSE);
  WSENS ("mwm_report_export", TRUE);

  uc_application_set_status_bar (0, "");
  UC_UPDATE_UI;

  return GTK_RESPONSE_YES;
}

/**
 * uc_project_projects_list_node_free:
 * @pl: #UCProjectProjects node of the project to free.
 *
 * Free a projects' list node.
 */
static void
uc_project_projects_list_node_free (UCProjectProjects ** pl)
{
  g_free ((*pl)->location), (*pl)->location = NULL;
  g_free ((*pl)->title), (*pl)->title = NULL;
  g_free ((*pl)->description), (*pl)->description = NULL;

  uc_project_projects_list =
    g_list_remove (uc_project_projects_list, (gpointer) *pl);

  g_free (*pl), *pl = NULL;
}


/**
 * uc_project_projects_list_free:
 *
 * Free the entire projects' list.
 */
static void
uc_project_projects_list_free (void)
{
  GList *item = NULL;


  if (uc_project_projects_list == NULL)
    return;

  item = g_list_first (uc_project_get_projects_list ());
  while (item != NULL)
  {
    UCProjectProjects *pl = (UCProjectProjects *) item->data;


    item = g_list_next (item);

    uc_project_projects_list_node_free (&pl);
  }

  g_list_free (uc_project_projects_list), uc_project_projects_list = NULL;
}


/**
 * uc_project_projects_list_load:
 *
 * Load the projects' list.
 *
 * See: uc_project_xml_get_tags ()
 */
void
uc_project_projects_list_load (void)
{
  gchar *path = NULL;

  if (uc_project_projects_list != NULL)
    uc_project_projects_list_free ();

  path = g_strconcat (uc_project_get_working_path (), "/projects.xml", NULL);
  uc_project_projects_list = uc_project_xml_get_tags (path);
  g_free (path), path = NULL;
}

/**
 * uc_project_get_projects_list:
 *
 * Return the gurlchecker projects' list.
 *
 * Returns: A #GList pointer on the internal projects' list.
 */
GList *
uc_project_get_projects_list (void)
{
  return uc_project_projects_list;
}

/**
 * uc_project_get_new_project_id:
 *
 * Return a new project id.
 * 
 * Returns: The new id for the new project.
 */
static guint
uc_project_get_new_project_id (void)
{
  GList *item = NULL;
  guint max = 0;

  if (uc_project_projects_list == NULL)
    return 1;

  item = g_list_first (uc_project_get_projects_list ());
  while (item != NULL)
    {
      UCProjectProjects *lp = (UCProjectProjects *) item->data;

      if (lp->id > max)
	max = lp->id;

      item = g_list_next (item);
    }

  return max + 1;
}

/**
 * uc_project_new:
 * @id: id of the project.
 *
 * Load a project and its properties from the disk.
 *
 * See: uc_project_projects_list_load ()
 */
void
uc_project_new (void)
{
  uc_application_init (NULL, NULL, NULL, FALSE);
}

/**
 * uc_project_free:
 * @type: type of the project.
 *
 * Free internals lists and variables to prepare a new project.
 */
void
uc_project_free (void)
{
  g_strfreev (project.reject_directories), project.reject_directories = NULL;
  g_strfreev (project.reject_domains), project.reject_domains = NULL;

  g_free (project.reject_documents), project.reject_documents = NULL;
  g_free (project.reject_images), project.reject_images = NULL;
  g_free (project.html_extensions), project.html_extensions = NULL;
  g_free (project.css_extensions), project.css_extensions = NULL;
  g_free (project.virii_extensions), project.virii_extensions = NULL;
  g_free (project.bad_extensions), project.bad_extensions = NULL;

  g_free (project.browser_path), project.browser_path = NULL;
  g_free (project.description), project.description = NULL;
  g_free (project.title), project.title = NULL;
  g_free (project.url), project.url = NULL;
  g_free (project.bookmarks_file), project.bookmarks_file = NULL;
  g_free (project.report_export_path), project.report_export_path = NULL;
  g_free (project.current_host), project.current_host = NULL;
  g_free (project.current_port), project.current_port = NULL;
  g_free (project.working_path), project.working_path = NULL;
  g_free (project.cache_name), project.cache_name = NULL;
  g_free (project.w3c_html_level), project.w3c_html_level = NULL;

  g_free (project.proxy_host), project.proxy_host = NULL;
  g_free (project.auth_line), project.auth_line = NULL;
  g_free (project.chroot_path), project.chroot_path = NULL;

  uc_project_projects_list_free ();

  project.id = 0;
  project.local_charset = NULL;

  uc_project_set_save_bookmarks (FALSE);
}

/**
 * uc_project_get_browser_path:
 *
 * Check for the browser path and return it.
 * 
 * Returns: The browser path (no new memory allocation).
 */
gchar *
uc_project_get_browser_path (void)
{
  gchar *browser = NULL;


  if (project.browser_path == NULL)
  {
    browser = uc_utils_get_gnome_browser_conf ();
    uc_project_set_browser_path (browser);
    g_free (browser), browser = NULL;
  }

  return project.browser_path;
}

/**
 * uc_project_get_type:
 *
 * Return the current project type.
 * 
 * Returns: The current project type.
 */
UCProjectType
uc_project_get_type (void)
{
  return project.type;
}

/**
 * uc_project_get_save:
 *
 * Check if the project should be saved.
 * 
 * Returns: %TRUE if the project should be saved.
 */
gboolean
uc_project_get_save (void)
{
  return project.save;
}

/**
 * uc_project_get_id:
 *
 * Return the id of the current project.
 * 
 * Returns: id of the project.
 */
guint
uc_project_get_id (void)
{
  return project.id;
}

/**
 * uc_project_get_reject_directories:
 * @items: the vector to fill.
 * @item_size: the size to fill.
 *
 * Fill the @items vector with the directories to avoid during
 * the check (those directories will not be checked).
 *
 * There is no memory allocation.
 *
 * See: uc_project_get_reject_domains (),
 *      uc_project_get_reject_documents (),
 *      uc_project_get_reject_images (),
 *      uc_project_get_security_bad_extensions (),
 *      uc_project_get_security_virii_extensions (),
 *      uc_project_get_w3c_html_extensions (),
 *      uc_project_get_w3c_css_extensions ()
 */
void
uc_project_get_reject_directories (gchar *** items, guint * item_size)
{
  if (project.reject_directories != NULL)
    {
      *items = project.reject_directories;
      *item_size = uc_utils_vector_length (project.reject_directories);
    }
  else
    {
      *items = NULL;
      *item_size = 0;
    }
}

/**
 * uc_project_get_reject_domains:
 * @items: the vector to fill.
 * @item_size: the size to fill.
 *
 * Fill the @items vector with the domains to avoid during
 * the check (those domains will not be checked).
 *
 * There is no memory allocation.
 *
 * See: uc_project_get_reject_directories (),
 *      uc_project_get_reject_documents (),
 *      uc_project_get_reject_images (),
 *      uc_project_get_security_bad_extensions (),
 *      uc_project_get_security_virii_extensions (),
 *      uc_project_get_w3c_html_extensions (),
 *      uc_project_get_w3c_css_extensions ()
 */
void
uc_project_get_reject_domains (gchar *** items, guint * item_size)
{
  if (project.reject_domains != NULL)
    {
      *items = project.reject_domains;
      *item_size = uc_utils_vector_length (project.reject_domains);
    }
  else
    {
      *items = NULL;
      *item_size = 0;
    }
}

/**
 * uc_project_get_security_virii_extensions:
 *
 * Return a string with comma separated extensions to scan during the check.
 *
 * There is no memory allocation.
 *
 * See: uc_project_get_reject_directories (),
 *      uc_project_get_reject_domains (),
 *      uc_project_get_reject_documents (),
 *      uc_project_get_reject_images (),
 *      uc_project_get_security_bad_extensions (),
 *      uc_project_get_w3c_html_extensions (),
 *      uc_project_get_w3c_css_extensions ()
 *
 * Returns: A string with comma separated extensions.
 */
gchar *
uc_project_get_security_virii_extensions (void)
{
  return (project.virii_extensions != NULL)?
    project.virii_extensions:UC_DEFAULT_SECURITY_VIRII_EXTENSIONS;
}

/**
 * uc_project_get_w3c_html_extensions:
 *
 * Return a string with comma separated extensions of HTML documents to
 * validate during the scan.
 *
 * There is no memory allocation.
 *
 * See: uc_project_get_reject_directories (),
 *      uc_project_get_reject_domains (),
 *      uc_project_get_reject_documents (),
 *      uc_project_get_reject_images (),
 *      uc_project_get_security_bad_extensions (),
 *      uc_project_get_security_virii_extensions (),
 *      uc_project_get_w3c_css_extensions ()
 *
 * Returns: A string with comma separated extensions.
 */
gchar *
uc_project_get_w3c_html_extensions (void)
{
  return (project.html_extensions != NULL)?
    project.html_extensions:UC_DEFAULT_W3C_HTML_EXTENSIONS;
}


/**
 * uc_project_get_w3c_css_extensions:
 *
 * Return a string with comma separated extensions of CSS files to validate
 * during the scan.
 *
 * There is no memory allocation.
 *
 * See: uc_project_get_reject_directories (),
 *      uc_project_get_reject_domains (),
 *      uc_project_get_reject_documents (),
 *      uc_project_get_reject_images (),
 *      uc_project_get_security_bad_extensions (),
 *      uc_project_get_security_virii_extensions (),
 *      uc_project_get_w3c_html_extensions ()
 *
 * Returns: A string with comma separated extensions.
 */
gchar *
uc_project_get_w3c_css_extensions (void)
{
  return (project.css_extensions != NULL)?
    project.css_extensions:UC_DEFAULT_W3C_CSS_EXTENSIONS;
}

/**
 * uc_project_get_reject_documents:
 *
 * Return a string with comma separated extensions to ignore during the scan.
 *
 * See: uc_project_get_reject_directories (),
 *      uc_project_get_reject_domains (),
 *      uc_project_get_reject_images (),
 *      uc_project_get_security_bad_extensions (),
 *      uc_project_get_security_virii_extensions (),
 *      uc_project_get_w3c_html_extensions (),
 *      uc_project_get_w3c_css_extensions ()
 *
 * Returns: A string with comma separated extensions.
 */
gchar *
uc_project_get_reject_documents (void)
{
  if (project.reject_documents == NULL)
    uc_project_set_reject_documents ("");

  return project.reject_documents;
}

/**
 * uc_project_get_reject_images:
 *
  Return a string with comma separated extensions to ignore during the scan.
 *
 * There is no memory allocation.
 *
 * See: uc_project_get_reject_directories (),
 *      uc_project_get_reject_domains (),
 *      uc_project_get_reject_documents (),
 *      uc_project_get_security_bad_extensions (),
 *      uc_project_get_security_virii_extensions (),
 *      uc_project_get_w3c_html_extensions (),
 *      uc_project_get_w3c_css_extensions ()
 *
 * Returns: A string with comma separated extensions.
 */
gchar *
uc_project_get_reject_images (void)
{
  if (project.reject_images == NULL)
    uc_project_set_reject_images ("");

  return project.reject_images;
}

/**
 * uc_project_get_security_bad_extensions:
 *
 * Return a string with comma separated extensions of files to search during
 * the security scan.
 *
 * There is no memory allocation.
 *
 * See: uc_project_get_reject_directories (),
 *      uc_project_get_reject_domains (),
 *      uc_project_get_reject_documents (),
 *      uc_project_get_reject_images (),
 *      uc_project_get_security_virii_extensions (),
 *      uc_project_get_w3c_html_extensions (),
 *      uc_project_get_w3c_css_extensions ()
 *
 * Returns: A string with comma separated extensions.
 */
gchar *
uc_project_get_security_bad_extensions (void)
{
  return (project.bad_extensions != NULL)?
    project.bad_extensions:UC_DEFAULT_SECURITY_BAD_EXTENSIONS;
}


static void
uc_project_proxy_init (void)
{
  gchar *host = NULL;
  guint port = 0;

  uc_utils_get_gnome_proxy_conf (&host, &port);
  uc_project_set_use_proxy ((strlen (host) > 0));

  if (uc_project_get_use_proxy ())
    {
      uc_project_set_proxy_host (host);
      uc_project_set_proxy_port (port);
    }
  else
    {
      uc_project_set_proxy_host ("");
      uc_project_set_proxy_port (0);
    }

  g_free (host), host = NULL;
}

GHashTable *
uc_project_get_cookies (void)
{
  if (project.cookies == NULL)
    project.cookies = g_hash_table_new_full (g_str_hash, g_str_equal,
					     g_free, NULL);
  return project.cookies;
}

gchar *
uc_project_get_title (void)
{
  return (project.title) ? project.title : "";
}

gchar *
uc_project_get_description (void)
{
  return (project.description) ? project.description : "";
}

gboolean
uc_project_get_save_bookmarks (void)
{
  return project.save_bookmarks;
}

gboolean
uc_project_get_save_project (void)
{
  return project.save;
}

G_CONST_RETURN gchar *
uc_project_get_local_charset (void)
{
  return project.local_charset;
}

gboolean
uc_project_get_speed_check (void)
{
  return project.speed_check;
}

gboolean
uc_project_get_check_is_current (void)
{
  return project.check_is_current;
}

gboolean
uc_project_get_check_is_main (void)
{
  return project.check_is_main;
}

gboolean
uc_project_get_check_is_bookmarks (void)
{
  return (project.bookmarks_type != UC_BOOKMARKS_TYPE_NONE);
}

gchar *
uc_project_get_url (void)
{
  return project.url;
}

gchar *
uc_project_get_bookmarks_file (void)
{
  return project.bookmarks_file;
}

UCBookmarksType
uc_project_get_bookmarks_type (void)
{
  return project.bookmarks_type;
}

gchar *
uc_project_get_report_export_path (void)
{
  gchar *home = NULL;

  if (project.report_export_path == NULL)
    {
      home = getenv ("HOME");
      g_assert (home != NULL);

      uc_project_set_report_export_path (home);
    }

  return project.report_export_path;
}

gchar *
uc_project_get_current_host (void)
{
  return project.current_host;
}

gchar *
uc_project_get_current_port (void)
{
  if (!project.current_port)
    uc_project_set_current_port (UC_URL_DEFAULT_PORT);

  return project.current_port;
}

guint
uc_project_get_check_wait (void)
{
  return project.check_wait;
}

guint
uc_project_get_timeouts_blocked (void)
{
  return project.timeouts_blocked;
}

guint
uc_project_get_depth_level (void)
{
  return project.depth_level;
}

gchar *
uc_project_get_working_path (void)
{
  gchar *home = NULL;

  if (project.working_path == NULL)
    {
      home = getenv ("HOME");
      g_assert (home != NULL);

      home = g_strconcat (home, "/.gurlchecker", NULL);
      uc_project_set_working_path (home);
      g_free (home), home = NULL;
    }

  return project.working_path;
}

gchar *
uc_project_get_cache_name (void)
{
  gchar *cache = NULL;

  if (project.cache_name == NULL)
    {
      cache = g_strdup_printf ("cache/%s_%d", PACKAGE, getpid ());
      uc_project_set_cache_name (cache);
      g_free (cache), cache = NULL;
    }

  return project.cache_name;
}

gboolean
uc_project_get_proto_file_is_error (void)
{
  return project.proto_file_is_error;
}

gboolean
uc_project_get_proto_file_check (void)
{
  return project.proto_file_check;
}

gboolean
uc_project_get_cookies_accept (void)
{
  return project.cookies_accept;
}

const gchar *
uc_project_get_w3c_html_level (void)
{
  if (!project.w3c_html_level)
    uc_project_set_w3c_html_level ("warnings");

  return project.w3c_html_level;
}

gboolean
uc_project_get_proto_mailto (void)
{
  return project.proto_mailto;
}

gboolean
uc_project_get_proto_https (void)
{
#ifdef ENABLE_GNUTLS
  return project.proto_https;
#else
  return 0;
#endif
}

gboolean
uc_project_get_proto_ftp (void)
{
  return project.proto_ftp;
}

gboolean
uc_project_get_passive_ftp (void)
{
  return project.passive_ftp;
}

gboolean
uc_project_speed_check_get_download_content (void)
{
  return (!uc_project_get_check_is_bookmarks ()) ?
    uc_project_speed_check_set_download_content_value : FALSE;
}

gboolean
uc_project_get_cookies_warn_added (void)
{
  return project.cookies_warn_added;
}

gboolean
uc_project_get_cookies_warn_updated (void)
{
  return project.cookies_warn_updated;
}

gboolean
uc_project_get_cookies_warn_deleted (void)
{
  return project.cookies_warn_deleted;
}

gboolean
uc_project_get_proto_mailto_check_mx (void)
{
  return project.proto_mailto_check_mx;
}

gboolean
uc_project_get_check_chroot (void)
{
  return project.check_chroot;
}

gboolean
uc_project_get_limit_local (void)
{
  return project.limit_local;
}

gboolean
uc_project_get_use_proxy (void)
{
  return project.use_proxy;
}

gchar *
uc_project_get_proxy_host (void)
{
  uc_project_proxy_init ();

  return project.proxy_host;
}

guint
uc_project_get_proxy_port (void)
{
  uc_project_proxy_init ();

  return project.proxy_port;
}

gchar *
uc_project_get_auth_line (void)
{
  return project.auth_line;
}

gchar *
uc_project_get_chroot_path (void)
{
  return project.chroot_path;
}

gboolean
uc_project_get_no_urls_args (void)
{
  return project.no_urls_args;
}

gboolean
uc_project_get_download_images_content (void)
{
  return project.images_content;
}

gboolean
uc_project_get_download_archives_content (void)
{
  return project.archives_content;
}

guint
uc_project_get_check_timeout (void)
{
  return project.check_timeout;
}

gboolean
uc_project_get_prompt_auth (void)
{
  return project.prompt_auth;
}

gboolean
uc_project_get_w3c_checks (const gchar * type)
{
  gboolean ret = FALSE;

  if (!strcmp (type, "css"))
    ret = project.w3c_checks_css;
  else if (!strcmp (type, "html"))
#ifdef ENABLE_TIDY
    ret = project.w3c_checks_html;
#else
    ret = FALSE;
#endif
  else if (!strcmp (type, "any"))
    ret = project.w3c_checks;
  else
    g_assert_not_reached ();

  return ret;
}

gboolean
uc_project_get_security_checks (const gchar * type)
{
  gboolean ret = FALSE;

  if (!strcmp (type, "bad_extensions"))
    ret = project.security_checks_bad_extensions;
  else if (!strcmp (type, "exclude_images"))
    ret = project.security_checks_exclude_images;
  else if (!strcmp (type, "virii"))
#ifdef ENABLE_CLAMAV
    ret = project.security_checks_virii;
#else
    ret = FALSE;
#endif
  else if (!strcmp (type, "any"))
    ret = project.security_checks;
  else
    g_assert_not_reached ();

  return ret;
}

gboolean
uc_project_get_export_labels (void)
{
  return project.export_labels;
}

gboolean
uc_project_get_export_numbering (void)
{
  return project.export_numbering;
}

gboolean
uc_project_get_export_external (void)
{
  return project.export_external;
}


gboolean
uc_project_get_export_ip (void)
{
  return project.export_ip;
}


gboolean
uc_project_get_debug_mode (void)
{
  return project.debug_mode;
}

gboolean
uc_project_get_dump_properties (void)
{
  return project.dump_properties;
}

guint
uc_project_get_tooltips_delay (void)
{
  return project.tooltips_delay;
}

void
uc_project_set_cookies (GHashTable * value)
{
  project.cookies = value;
}

void
uc_project_set_reject_directories (const gchar * items)
{
  g_strfreev (project.reject_directories), project.reject_directories = NULL;
  project.reject_directories = g_strsplit (items, ",", -1);
}

void
uc_project_set_reject_domains (const gchar * items)
{
  g_strfreev (project.reject_domains), project.reject_domains = NULL;
  project.reject_domains = g_strsplit (items, ",", -1);
}

void
uc_project_set_security_virii_extensions (const gchar * items)
{
  g_free (project.virii_extensions), project.virii_extensions = NULL;
  project.virii_extensions = g_strdup (items);
}

void
uc_project_set_w3c_html_extensions (const gchar * items)
{
  g_free (project.html_extensions), project.html_extensions = NULL;
  project.html_extensions = g_strdup (items);
}

void
uc_project_set_w3c_css_extensions (const gchar * items)
{
  g_free (project.css_extensions), project.css_extensions = NULL;
  project.css_extensions = g_strdup (items);
}

void
uc_project_set_reject_documents (const gchar * items)
{
  g_free (project.reject_documents), project.reject_documents = NULL;
  project.reject_documents = g_strdup (items);
}

void
uc_project_set_reject_images (const gchar * items)
{
  g_free (project.reject_images), project.reject_images = NULL;
  project.reject_images = g_strdup (items);
}

void
uc_project_set_security_bad_extensions (const gchar * items)
{
  g_free (project.bad_extensions), project.bad_extensions = NULL;
  project.bad_extensions = g_strdup (items);
}

void
uc_project_set_type (const UCProjectType type)
{
  project.type = type;
}

void
uc_project_set_id (const guint id)
{
  project.id = id;
}

void
uc_project_speed_check_set_download_content (const gboolean value)
{
  uc_project_speed_check_set_download_content_value = value;
}

void
uc_project_set_speed_check (const gboolean value)
{
  uc_project_speed_check_set_download_content (value);

  project.speed_check = value;
}


void
uc_project_set_browser_path (const gchar * value)
{
  gchar *p = NULL;


  g_free (project.browser_path), project.browser_path = NULL;
  p = project.browser_path = g_strdup (value);
 
  // Remove extra characters added by GNOME after navigator options
  // if any
  do
  {
    if (*p == '"')
      *p = ' ';
    else if (*p == '%' && *(p + 1) == 's')
    {
      *p = ' ';
      *(p + 1) = ' ';
    }
  }
  while (*(++p));
}


void
uc_project_set_description (const gchar * value)
{
  g_free (project.description), project.description = NULL;
  project.description = g_strdup (value);
}


void
uc_project_set_title (const gchar * value)
{
  g_free (project.title), project.title = NULL;
  project.title = g_strdup (value);
}

void
uc_project_set_save_bookmarks (const gboolean value)
{
  project.save_bookmarks = value;
  WSENS ("mwm_save_bookmarks", value);
  WSENS ("mw_bt_save_project", value);
}

void
uc_project_set_save (const gboolean value)
{
  project.save = value;
  WSENS ("mwm_save_project", value);
  WSENS ("mw_bt_save_project", value);
}

void
uc_project_set_local_charset (G_CONST_RETURN gchar * value)
{
  project.local_charset = value;
}

void
uc_project_set_timeouts_blocked (const guint value)
{
  project.timeouts_blocked = value;
}

void
uc_project_set_check_is_current (const gboolean value)
{
  project.check_is_current = value;
}

void
uc_project_set_check_is_main (const gboolean value)
{
  project.check_is_main = value;
}

void
uc_project_set_url (const gchar * value)
{
  g_free (project.url), project.url = NULL;
  project.url = g_strdup (value);
}

void
uc_project_set_bookmarks_file (const gchar * value)
{
  g_free (project.bookmarks_file), project.bookmarks_file = NULL;
  project.bookmarks_file = g_strdup (value);
}

void
uc_project_set_bookmarks_type (const UCBookmarksType value)
{
  project.bookmarks_type = value;
}

void
uc_project_set_report_export_path (const gchar * value)
{
  g_free (project.report_export_path), project.report_export_path = NULL;
  project.report_export_path = g_strdup (value);
}

void
uc_project_set_current_host (const gchar * value)
{
  g_free (project.current_host), project.current_host = NULL;
  project.current_host = g_strdup (value);
}

void
uc_project_set_current_port (const gchar * value)
{
  g_free (project.current_port), project.current_port = NULL;
  project.current_port = g_strdup (value);
}

void
uc_project_set_check_wait (const guint value)
{
  project.check_wait = value;
}

void
uc_project_set_depth_level (const guint value)
{
  project.depth_level = value;
}

void
uc_project_set_working_path (const gchar * value)
{
  g_free (project.working_path), project.working_path = NULL;
  project.working_path = g_strdup (value);
}

void
uc_project_set_cache_name (const gchar * value)
{
  g_free (project.cache_name), project.cache_name = NULL;
  project.cache_name = g_strdup (value);
}

void
uc_project_set_proto_file_is_error (const gboolean value)
{
  project.proto_file_is_error = value;
}

void
uc_project_set_proto_file_check (const gboolean value)
{
  project.proto_file_check = value;
}

void
uc_project_set_cookies_accept (const gboolean value)
{
  project.cookies_accept = value;
}

void
uc_project_set_w3c_html_level (const gchar * value)
{
  g_free (project.w3c_html_level), project.w3c_html_level = NULL;

  project.w3c_html_level = g_strdup (value);
}

void
uc_project_set_proto_mailto (const gboolean value)
{
  project.proto_mailto = value;
}

void
uc_project_set_proto_ftp (const gboolean value)
{
  project.proto_ftp = value;
}

void
uc_project_set_passive_ftp (const gboolean value)
{
  project.passive_ftp = value;
}

void
uc_project_set_proto_https (const gboolean value)
{
  project.proto_https = value;
}

void
uc_project_set_cookies_warn_added (const gboolean value)
{
  project.cookies_warn_added = value;
}

void
uc_project_set_cookies_warn_updated (const gboolean value)
{
  project.cookies_warn_updated = value;
}

void
uc_project_set_cookies_warn_deleted (const gboolean value)
{
  project.cookies_warn_deleted = value;
}

void
uc_project_set_proto_mailto_check_mx (const gboolean value)
{
  project.proto_mailto_check_mx = value;
}

void
uc_project_set_check_chroot (const gboolean value)
{
  project.check_chroot = value;
}

void
uc_project_set_limit_local (const gboolean value)
{
  project.limit_local = value;
}

void
uc_project_set_use_proxy (const gboolean value)
{
  project.use_proxy = value;
}

void
uc_project_set_proxy_host (const gchar * value)
{
  g_free (project.proxy_host), project.proxy_host = NULL;
  project.proxy_host = g_strdup (value);
}

void
uc_project_set_proxy_port (const guint value)
{
  project.proxy_port = value;
}

void
uc_project_set_auth_line (const gchar * value)
{
  g_free (project.auth_line), project.auth_line = NULL;
  project.auth_line = g_strdup (value);
}

void
uc_project_set_chroot_path (const gchar * value)
{
  g_free (project.chroot_path), project.chroot_path = NULL;
  project.chroot_path = g_strdup (value);
}

void
uc_project_set_no_urls_args (const gboolean value)
{
  project.no_urls_args = value;
}

void
uc_project_set_download_images_content (const gboolean value)
{
  project.images_content = value;
}

void
uc_project_set_download_archives_content (const gboolean value)
{
  project.archives_content = value;
}

void
uc_project_set_check_timeout (const guint value)
{
  project.check_timeout = value;
}

void
uc_project_set_prompt_auth (const gboolean value)
{
  project.prompt_auth = value;
}

void
uc_project_set_security_checks (const gchar * type, const gboolean value)
{
  if (!strcmp (type, "bad_extensions"))
    project.security_checks_bad_extensions = value;
  else if (!strcmp (type, "exclude_images"))
    project.security_checks_exclude_images = value;
  else if (!strcmp (type, "virii"))
    project.security_checks_virii = value;
  else if (!strcmp (type, "any"))
    project.security_checks = value;
  else
    g_assert_not_reached ();
}

void
uc_project_set_w3c_checks (const gchar * type, const gboolean value)
{
  if (!strcmp (type, "css"))
    project.w3c_checks_css = value;
  else if (!strcmp (type, "html"))
    project.w3c_checks_html = value;
  else if (!strcmp (type, "any"))
    project.w3c_checks = value;
  else
    g_assert_not_reached ();
}

void
uc_project_set_export_labels (const gboolean value)
{
  project.export_labels = value;
}

void
uc_project_set_export_numbering (const gboolean value)
{
  project.export_numbering = value;
}

void
uc_project_set_export_external (const gboolean value)
{
  project.export_external = value;
}


void
uc_project_set_export_ip (const gboolean value)
{
  project.export_ip = value;
}


void
uc_project_set_debug_mode (const gboolean value)
{
  project.debug_mode = value;
}

void
uc_project_set_tooltips_delay (const guint value)
{
  project.tooltips_delay = value;
}

void
uc_project_set_dump_properties (const gboolean value)
{
  project.dump_properties = value;
}

static GList *
uc_project_xml_parse_project (GList * list, xmlDocPtr doc, xmlNodePtr cur)
{
  xmlChar *value = NULL;
  UCProjectProjects *item = NULL;
  xmlChar *tmp = NULL;

  item = g_new0 (UCProjectProjects, 1);
  item->location = NULL;
  item->title = NULL;
  item->description = NULL;
  item->id = atoi ((char *) (tmp = xmlGetProp (cur, BAD_CAST "id")));
  xmlFree (tmp), tmp = NULL;
  item->type = atoi ((char *) (tmp = xmlGetProp (cur, BAD_CAST "type")));
  xmlFree (tmp), tmp = NULL;
  item->create = atol ((char *) (tmp = xmlGetProp (cur, BAD_CAST "create")));
  xmlFree (tmp), tmp = NULL;
  item->update = atol ((char *) (tmp = xmlGetProp (cur, BAD_CAST "update")));
  xmlFree (tmp), tmp = NULL;

  cur = cur->xmlChildrenNode;
  while (cur != NULL
	 && (item->description == NULL ||
	     item->title == NULL || item->location == NULL))
    {
      if (!xmlStrcmp (cur->name, BAD_CAST "title"))
	{
	  value = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
	  item->title = g_strdup ((gchar *) value);
	  xmlFree (value), value = NULL;
	}
      else if (!xmlStrcmp (cur->name, BAD_CAST "description"))
	{
	  value = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
	  item->description =
	    (value) ? g_strdup ((gchar *) value) : g_strdup ("");
	  xmlFree (value), value = NULL;
	}
      else if (!xmlStrcmp (cur->name, BAD_CAST "location"))
	{
	  value = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
	  item->location = g_strdup ((gchar *) value);
	  xmlFree (value), value = NULL;
	}

      cur = cur->next;
    }

  list = g_list_append (list, item);

  return list;
}

static GList *
uc_project_xml_get_tags (const gchar * docname)
{
  GList *list = NULL;
  xmlNodePtr cur = NULL;
  xmlDocPtr doc = NULL;

  if (!g_file_test (docname, G_FILE_TEST_EXISTS))
    return list;

  doc = xmlParseFile (docname);
  cur = xmlDocGetRootElement (doc);

  g_assert (cur != NULL);

  cur = cur->xmlChildrenNode;
  while (cur != NULL)
    {
      if (!xmlStrcmp (cur->name, BAD_CAST "project"))
	list = uc_project_xml_parse_project (list, doc, cur);

      cur = cur->next;
    }

  xmlFreeDoc (doc), doc = NULL;

  return list;
}

static GHashTable *
uc_project_xml_header_tag (GHashTable * table, xmlDocPtr doc, xmlNodePtr cur)
{
  xmlChar *key = NULL;
  xmlChar *value = NULL;

  table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  cur = cur->xmlChildrenNode;
  while (cur != NULL)
    {
      if (!xmlStrcmp (cur->name, BAD_CAST "item"))
	{
	  key = xmlGetProp (cur, BAD_CAST "name");
	  value = xmlGetProp (cur, BAD_CAST "value");
	  g_hash_table_replace (table, g_strdup ((gchar *) key),
				g_strdup ((gchar *) value));
	  xmlFree (value), value = NULL;
	  xmlFree (key), key = NULL;
	}

      cur = cur->next;
    }

  return table;
}

static void
uc_project_xml_properties_tag (UCLinkProperties * prop, xmlDocPtr doc,
			       xmlNodePtr cur)
{
  xmlChar *tmp = NULL;

  cur = cur->xmlChildrenNode;
  while (cur != NULL)
    {
      if (!xmlStrcmp (cur->name, BAD_CAST "header"))
	prop->header = uc_project_xml_header_tag (prop->header, doc, cur);
      else if (!xmlStrcmp (cur->name, BAD_CAST "url"))
	{
	  prop->protocol =
	    g_strdup ((gchar *) (tmp = xmlGetProp (cur, BAD_CAST "proto")));
	  xmlFree (tmp), tmp = NULL;
	  prop->link_icon = uc_check_get_link_type_icon (prop->link_type,
							 prop->protocol);
	  prop->ip_addr =
	    g_strdup ((gchar *) (tmp = xmlGetProp (cur, BAD_CAST "ip_addr")));
	  xmlFree (tmp), tmp = NULL;
	  prop->h_name =
	    g_strdup ((gchar *) (tmp = xmlGetProp (cur, BAD_CAST "host")));
	  xmlFree (tmp), tmp = NULL;
	  prop->port =
	    g_strdup ((gchar *) (tmp = xmlGetProp (cur, BAD_CAST "port")));
	  xmlFree (tmp), tmp = NULL;
	  prop->path =
	    g_strdup ((gchar *) (tmp = xmlGetProp (cur, BAD_CAST "path")));
	  xmlFree (tmp), tmp = NULL;
	  prop->args =
	    g_strdup ((gchar *) (tmp = xmlGetProp (cur, BAD_CAST "args")));
	  xmlFree (tmp), tmp = NULL;
	}

      cur = cur->next;
    }
}

static GList *
uc_project_xml_tag_tag (GList * list, xmlDocPtr doc, xmlNodePtr cur)
{
  UCHTMLTag *tag = NULL;
  xmlChar *tmp = NULL;

  tag = uc_html_parser_node_new ();
  tag->depth = atoi ((char *) (tmp = xmlGetProp (cur, BAD_CAST "depth")));
  xmlFree (tmp), tmp = NULL;
  tag->type = atoi ((char *) (tmp = xmlGetProp (cur, BAD_CAST "type")));
  xmlFree (tmp), tmp = NULL;

  tag->label =
    g_strdup ((gchar *) (tmp = xmlGetProp (cur, BAD_CAST "label")));
  xmlFree (tmp), tmp = NULL;
  tag->value =
    g_strdup ((gchar *) (tmp = xmlGetProp (cur, BAD_CAST "value")));
  xmlFree (tmp), tmp = NULL;

  list = g_list_append (list, tag);

  return list;
}

static GList *
uc_project_xml_similar_links_tag (GList * list, xmlDocPtr doc, xmlNodePtr cur)
{
  xmlChar *value = NULL;

  cur = cur->xmlChildrenNode;
  while (cur != NULL)
    {
      if (!xmlStrcmp (cur->name, BAD_CAST "item"))
	{
	  value = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
	  list = g_list_append (list, g_strdup ((gchar *) value));
	  xmlFree (value), value = NULL;
	}

      cur = cur->next;
    }

  return list;
}

static GList *
uc_project_xml_bad_extensions_tag (GList * list, xmlDocPtr doc,
				   xmlNodePtr cur)
{
  xmlChar *value = NULL;

  cur = cur->xmlChildrenNode;
  while (cur != NULL)
    {
      if (!xmlStrcmp (cur->name, BAD_CAST "item"))
	{
	  value = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
	  list = g_list_append (list, g_strdup ((gchar *) value));
	  xmlFree (value), value = NULL;
	}

      cur = cur->next;
    }

  return list;
}

static GList *
uc_project_xml_tags_tag (GList * list, xmlDocPtr doc, xmlNodePtr cur)
{
  cur = cur->xmlChildrenNode;
  while (cur != NULL)
    {
      if (!xmlStrcmp (cur->name, BAD_CAST "tag"))
	list = uc_project_xml_tag_tag (list, doc, cur);

      cur = cur->next;
    }

  return list;
}

static GList *
uc_project_xml_childs_tag (UCLinkProperties * prop, GList * list,
			   xmlDocPtr doc, xmlNodePtr cur)
{
  cur = cur->xmlChildrenNode;
  while (cur != NULL)
    {
      if (!xmlStrcmp (cur->name, BAD_CAST "page"))
	list = uc_project_xml_page_tag (prop, list, doc, cur);

      cur = cur->next;
    }

  return list;
}

static GList *
uc_project_xml_page_tag (UCLinkProperties * parent, GList * list,
			 xmlDocPtr doc, xmlNodePtr cur)
{
  UCLinkProperties *prop = NULL;
  UCStatusCode *sc = NULL;
  xmlChar *tmp = NULL;

  prop = uc_check_link_properties_node_new ();
  prop->depth_level =
    atoi ((char *) (tmp = xmlGetProp (cur, BAD_CAST "depth")));
  xmlFree (tmp), tmp = NULL;
  prop->uid = atoi ((char *) (tmp = xmlGetProp (cur, BAD_CAST "id")));
  xmlFree (tmp), tmp = NULL;
  prop->parent = parent;
  prop->is_parsable =
    uc_utils_get_yesno ((gchar *) (tmp =
				   xmlGetProp (cur, BAD_CAST "parsable")));
  xmlFree (tmp), tmp = NULL;
  prop->is_downloadable = uc_utils_get_yesno ((gchar *) (tmp =
							 xmlGetProp (cur,
								     BAD_CAST
								     "downloadable")));
  xmlFree (tmp), tmp = NULL;
  prop->virii = uc_utils_get_yesno ((gchar *) (tmp =
					       xmlGetProp (cur,
							   BAD_CAST
							   "virii")));
  xmlFree (tmp), tmp = NULL;
  tmp = xmlGetProp (cur, BAD_CAST "virname");
  prop->virname = (tmp == NULL) ? g_strdup ("") : g_strdup ((gchar *) tmp);
  xmlFree (tmp), tmp = NULL;
  tmp = xmlGetProp (cur, BAD_CAST "user_action");
  prop->user_action =
    (tmp == NULL) ? g_strdup ("") : g_strdup ((gchar *) tmp);
  xmlFree (tmp), tmp = NULL;
  prop->w3c_valid = uc_utils_get_yesno ((gchar *) (tmp =
						   xmlGetProp (cur,
							       BAD_CAST
							       "w3c_valid")));
  xmlFree (tmp), tmp = NULL;
  prop->checked =
    uc_utils_get_yesno ((gchar *) (tmp =
				   xmlGetProp (cur, BAD_CAST "checked")));
  xmlFree (tmp), tmp = NULL;

  tmp = xmlGetProp (cur, BAD_CAST "title");
  prop->label = (tmp == NULL) ? g_strdup ("") : g_strdup ((gchar *) tmp);
  xmlFree (tmp), tmp = NULL;
  tmp = xmlGetProp (cur, BAD_CAST "href");
  prop->url = (tmp == NULL) ? g_strdup ("") : g_strdup ((gchar *) tmp);
  xmlFree (tmp), tmp = NULL;
  prop->link_type = atoi ((char *) (tmp = xmlGetProp (cur, BAD_CAST "type")));
  xmlFree (tmp), tmp = NULL;
  tmp = xmlGetProp (cur, BAD_CAST "original_value");
  prop->link_value = (tmp == NULL) ? g_strdup ("") : g_strdup ((gchar *) tmp);
  xmlFree (tmp), tmp = NULL;
  tmp = xmlGetProp (cur, BAD_CAST "current_path");
  prop->current_path =
    (tmp == NULL) ? g_strdup ("") : g_strdup ((gchar *) tmp);
  xmlFree (tmp), tmp = NULL;
  tmp = xmlGetProp (cur, BAD_CAST "domain");
  prop->domain = (tmp == NULL) ? g_strdup ("") : g_strdup ((gchar *) tmp);
  xmlFree (tmp), tmp = NULL;

  cur = cur->xmlChildrenNode;
  while (cur != NULL)
    {
      if (!xmlStrcmp (cur->name, BAD_CAST "properties"))
	uc_project_xml_properties_tag (prop, doc, cur);
      else if (!xmlStrcmp (cur->name, BAD_CAST "tag"))
	{
	  GList *tl = NULL;
	  UCHTMLTag *tag = NULL;

	  tl = uc_project_xml_tag_tag (tl, doc, cur);
	  if (tl)
	    tag = (UCHTMLTag *) tl->data;

	  prop->tag = tag;
	}
      else if (!xmlStrcmp (cur->name, BAD_CAST "tags"))
	prop->tags = uc_project_xml_tags_tag (prop->tags, doc, cur);
      else if (!xmlStrcmp (cur->name, BAD_CAST "similar_links"))
	{
	  prop->similar_links_parents =
	    uc_project_xml_similar_links_tag (prop->similar_links_parents,
					      doc, cur);
	}
      else if (!xmlStrcmp (cur->name, BAD_CAST "bad_extensions"))
	{
	  prop->bad_extensions =
	    uc_project_xml_bad_extensions_tag (prop->bad_extensions,
					       doc, cur);
	}
      else if (!xmlStrcmp (cur->name, BAD_CAST "childs"))
	prop->childs =
	  uc_project_xml_childs_tag (prop, prop->childs, doc, cur);

      cur = cur->next;
    }

  sc = uc_application_get_status_code_properties ((gchar *)
						  g_hash_table_lookup
						  (prop->header,
						   UC_HEADER_STATUS));

  prop->status_icon = sc->icon_file;
  uc_project_xml_dispatch_links (prop);
  list = g_list_append (list, prop);

  return list;
}

static gchar *
uc_project_xml_filters_get_value (xmlDocPtr doc, xmlNodePtr cur)
{
  xmlChar *value = NULL;
  GString *filters = g_string_new ("");

  cur = cur->xmlChildrenNode;
  while (cur != NULL)
    {
      if (!xmlStrcmp (cur->name, BAD_CAST "item"))
	{
	  value = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
	  g_string_append_printf (filters, ",%s", g_strstrip ((gchar *) value));
	  xmlFree (value), value = NULL;
	}

      cur = cur->next;
    }

  if (filters->len > 0)
    g_string_erase (filters, 0, 1);

  return g_string_free (filters, FALSE);
}

static void
uc_project_xml_filters_tag (xmlDocPtr doc, xmlNodePtr cur)
{
  cur = cur->xmlChildrenNode;
  while (cur != NULL)
    {
      if (!xmlStrcmp (cur->name, BAD_CAST "directories"))
	uc_project_set_reject_directories (
          uc_project_xml_filters_get_value (doc, cur));
      else if (!xmlStrcmp (cur->name, BAD_CAST "domains"))
	uc_project_set_reject_domains (
          uc_project_xml_filters_get_value (doc, cur));
      else if (!xmlStrcmp (cur->name, BAD_CAST "documents"))
	uc_project_set_reject_documents (
          uc_project_xml_filters_get_value (doc, cur));
      else if (!xmlStrcmp (cur->name, BAD_CAST "images"))
	uc_project_set_reject_images (
          uc_project_xml_filters_get_value (doc, cur));
      else if (!xmlStrcmp (cur->name, BAD_CAST "virii_extensions"))
	uc_project_set_security_virii_extensions (
          uc_project_xml_filters_get_value (doc, cur));
      else if (!xmlStrcmp (cur->name, BAD_CAST "bad_extensions"))
	uc_project_set_security_bad_extensions (
          uc_project_xml_filters_get_value (doc, cur));
      else if (!xmlStrcmp (cur->name, BAD_CAST "html_extensions"))
	uc_project_set_w3c_html_extensions (
          uc_project_xml_filters_get_value (doc, cur));
      else if (!xmlStrcmp (cur->name, BAD_CAST "css_extensions"))
	uc_project_set_w3c_css_extensions (
          uc_project_xml_filters_get_value (doc, cur));

      cur = cur->next;
    }
}

/**
 * uc_project_xml_website_tag:
 * @list: #GList of links.
 * @doc: current #xmlDocPtr document.
 * @cur: current #xmlNodePtr cursor.
 *
 * Load all XML data from the disk for the current website project.
 * 
 * See: uc_project_xml_project_tag ()
 * 
 * Returns: a #GList of the links' list.
 */
static GList *
uc_project_xml_website_tag (GList * list, xmlDocPtr doc, xmlNodePtr cur)
{
  xmlChar *tmp = NULL;

  uc_project_set_report_export_path ((gchar *) (tmp =
						xmlGetProp (cur,
							    BAD_CAST
							    "report_export_path")));
  xmlFree (tmp), tmp = NULL;
  uc_project_set_url ((gchar *) (tmp = xmlGetProp (cur, BAD_CAST "url")));
  xmlFree (tmp), tmp = NULL;
  uc_project_set_current_host ((gchar *) (tmp =
					  xmlGetProp (cur,
						      BAD_CAST
						      "current_host")));
  xmlFree (tmp), tmp = NULL;
  uc_project_set_current_port ((gchar *) (tmp =
					  xmlGetProp (cur,
						      BAD_CAST
						      "current_port")));
  xmlFree (tmp), tmp = NULL;

  cur = cur->xmlChildrenNode;
  while (cur != NULL)
    {
      if (!xmlStrcmp (cur->name, BAD_CAST "page"))
	list = uc_project_xml_page_tag (NULL, list, doc, cur);

      cur = cur->next;
    }

  return list;
}

/**
 * uc_project_xml_report_tag:
 * @doc: current #xmlDocPtr document.
 * @cur: current #xmlNodePtr cursor.
 *
 * Load all XML data from the disk for the current project's report.
 *
 * See: uc_project_xml_project_tag ()
 */
static void
uc_project_xml_report_tag (xmlDocPtr doc, xmlNodePtr cur)
{
  xmlChar *tmp[7];
  guint i = 0;

  cur = cur->xmlChildrenNode;
  while (cur != NULL && xmlStrcmp (cur->name, BAD_CAST "links"))
    cur = cur->next;

  uc_check_report_force_values (atoi
				((char *) (tmp[0] =
					   xmlGetProp (cur, BAD_CAST "all"))),
				atoi ((char *) (tmp[1] =
						xmlGetProp (cur,
							    BAD_CAST
							    "checked"))),
				atoi ((char *) (tmp[2] =
						xmlGetProp (cur,
							    BAD_CAST "bad"))),
				atoi ((char *) (tmp[3] =
						xmlGetProp (cur,
							    BAD_CAST
							    "malformed"))),
				atoi ((char *) (tmp[4] =
						xmlGetProp (cur,
							    BAD_CAST
							    "good"))),
				atoi ((char *) (tmp[5] =
						xmlGetProp (cur,
							    BAD_CAST
							    "ignored"))),
				atoi ((char *) (tmp[6] =
						xmlGetProp (cur,
							    BAD_CAST
							    "timeout"))), 0);

  for (i = 0; i < 7; i++)
    xmlFree (tmp[i]);
}

/**
 * uc_project_xml_project_tag:
 * @list: #GList of links.
 * @doc: current #xmlDocPtr document.
 * @cur: current #xmlNodePtr cursor.
 *
 * Load all XML data from the disk for the current project.
 * 
 * See: uc_project_xml_report_tag (),
 *      uc_project_xml_website_tag ()
 * 
 * Returns: a #GList of the links' list.
 */
static GList *
uc_project_xml_project_tag (GList * list, xmlDocPtr doc, xmlNodePtr cur)
{
  cur = cur->xmlChildrenNode;
  while (cur != NULL)
    {
      if (!xmlStrcmp (cur->name, BAD_CAST "website"))
	list = uc_project_xml_website_tag (list, doc, cur);
      else if (!xmlStrcmp (cur->name, BAD_CAST "report"))
	uc_project_xml_report_tag (doc, cur);

      cur = cur->next;
    }

  return list;
}

/**
 * uc_project_xml_load_settings:
 * @p: #UCProjectProjects node of the project to load.
 *
 * Load settings for a given project.
 * 
 * See: uc_project_open (),
 *      uc_project_xml_load_links ()
 */
void
uc_project_xml_load_settings (const UCProjectProjects * p)
{
  xmlDocPtr doc = NULL;
  xmlNodePtr cur = NULL;
  xmlChar *tmp = NULL;
  gchar *path = NULL;
  G_CONST_RETURN gchar *charset;

  g_get_charset (&charset);
  uc_project_set_local_charset (charset);

  if (p == NULL)
    {
      path =
	g_strdup_printf ("%s/settings.xml", uc_project_get_working_path ());

      if (!g_file_test (path, G_FILE_TEST_EXISTS))
	uc_project_general_settings_create_default ();
    }
  else
    path = g_strdup_printf ("%s/projects/%u/settings.xml",
			    uc_project_get_working_path (), p->id);

  g_assert (g_file_test (path, G_FILE_TEST_EXISTS));

  doc = xmlParseFile (path);
  cur = xmlDocGetRootElement (doc);
  g_assert (cur != NULL);

  cur = cur->xmlChildrenNode;
  while (cur != NULL)
    {
      if (!xmlStrcmp (cur->name, BAD_CAST "project"))
	{
	  xmlNodePtr cur1 = NULL;

	  if ((tmp = xmlGetProp (cur, BAD_CAST "working_path")))
	    {
	      uc_project_set_working_path ((gchar *) tmp);
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "proto_file_is_error")))
	    {
	      uc_project_set_proto_file_is_error (uc_utils_get_yesno
						  ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "proto_file_check")))
	    {
	      uc_project_set_proto_file_check (uc_utils_get_yesno
					       ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "cookies_accept")))
	    {
	      uc_project_set_cookies_accept (uc_utils_get_yesno
					     ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "w3c_html_level")))
	    {
	      uc_project_set_w3c_html_level ((gchar *) tmp);
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "proto_mailto")))
	    {
	      uc_project_set_proto_mailto (uc_utils_get_yesno
					   ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "proto_https")))
	    {
	      uc_project_set_proto_https (uc_utils_get_yesno ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "proto_ftp")))
	    {
	      uc_project_set_proto_ftp (uc_utils_get_yesno ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "passive_ftp")))
	    {
	      uc_project_set_passive_ftp (uc_utils_get_yesno ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "cookies_warn_added")))
	    {
	      uc_project_set_cookies_warn_added (uc_utils_get_yesno
						 ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "cookies_warn_updated")))
	    {
	      uc_project_set_cookies_warn_updated (uc_utils_get_yesno
						   ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "cookies_warn_deleted")))
	    {
	      uc_project_set_cookies_warn_deleted (uc_utils_get_yesno
						   ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "proto_mailto_check_mx")))
	    {
	      uc_project_set_proto_mailto_check_mx (uc_utils_get_yesno
						    ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "check_chroot")))
	    {
	      uc_project_set_check_chroot (uc_utils_get_yesno
					   ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "limit_local")))
	    {
	      uc_project_set_limit_local (uc_utils_get_yesno ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "no_urls_args")))
	    {
	      uc_project_set_no_urls_args (uc_utils_get_yesno ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "speed_check")))
	    {
	      uc_project_set_speed_check (uc_utils_get_yesno ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "use_proxy")))
	    {
	      uc_project_set_use_proxy (uc_utils_get_yesno ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "proxy_port")))
	    {
	      uc_project_set_proxy_port (atoi ((char *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "chroot_path")))
	    {
	      uc_project_set_chroot_path ((gchar *) tmp);
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "download_images_content")))
	    {
	      uc_project_set_download_images_content (uc_utils_get_yesno
						      ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "download_archives_content")))
	    {
	      uc_project_set_download_archives_content (uc_utils_get_yesno
							((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "check_timeout")))
	    {
	      uc_project_set_check_timeout (atoi ((char *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "depth_level")))
	    {
	      uc_project_set_depth_level (atoi ((char *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "check_wait")))
	    {
	      uc_project_set_check_wait (atoi ((char *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "timeouts_blocked")))
	    {
	      uc_project_set_timeouts_blocked (atoi ((char *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "prompt_auth")))
	    {
	      uc_project_set_prompt_auth (uc_utils_get_yesno ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "w3c_checks")))
	    {
	      uc_project_set_w3c_checks ("any", uc_utils_get_yesno
					 ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "w3c_checks_css")))
	    {
	      uc_project_set_w3c_checks ("css",
					 uc_utils_get_yesno ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "w3c_checks_html")))
	    {
	      uc_project_set_w3c_checks ("html",
					 uc_utils_get_yesno ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "security_checks")))
	    {
	      uc_project_set_security_checks ("any", uc_utils_get_yesno
					      ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp =
	       xmlGetProp (cur, BAD_CAST "security_checks_bad_extensions")))
	    {
	      uc_project_set_security_checks ("bad_extensions",
					      uc_utils_get_yesno ((gchar *)
								  tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp =
	       xmlGetProp (cur, BAD_CAST "security_checks_exclude_images")))
	    {
	      uc_project_set_security_checks ("exclude_images",
					      uc_utils_get_yesno ((gchar *)
								  tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "security_checks_virii")))
	    {
	      uc_project_set_security_checks ("virii",
					      uc_utils_get_yesno ((gchar *)
								  tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "export_labels")))
	    {
	      uc_project_set_export_labels (uc_utils_get_yesno
					    ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "export_numbering")))
	    {
	      uc_project_set_export_numbering (uc_utils_get_yesno
					       ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "export_external")))
	    {
	      uc_project_set_export_external (uc_utils_get_yesno
					      ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "export_ip")))
	    {
	      uc_project_set_export_ip (uc_utils_get_yesno ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "debug_mode")))
	    {
	      uc_project_set_debug_mode (uc_utils_get_yesno ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  if ((tmp = xmlGetProp (cur, BAD_CAST "dump_properties")))
	    {
	      uc_project_set_dump_properties (uc_utils_get_yesno
					      ((gchar *) tmp));
	      xmlFree (tmp), tmp = NULL;
	    }

	  cur1 = cur->xmlChildrenNode;
	  while (cur1 != NULL)
	    {
	      if (!xmlStrcmp (cur1->name, BAD_CAST "filters"))
		{
		  uc_project_xml_filters_tag (doc, cur1);
		  break;
		}

	      cur1 = cur1->next;
	    }
	}

      cur = cur->next;
    }

  g_free (path), path = NULL;

  xmlFreeDoc (doc), doc = NULL;
}

/**
 * uc_project_xml_load_links:
 * @p: #UCProjectProjects node of the project to load.
 *
 * Load all project's links from the disk.
 * 
 * See: uc_project_open (),
 *      uc_project_xml_load_settings ()
 *
 * Returns: A #GList of all project's links.
 */
static GList *
uc_project_xml_load_links (const UCProjectProjects * p)
{
  GList *list = NULL;
  xmlNodePtr cur = NULL;
  gchar *path = NULL;
  xmlDocPtr doc = NULL;

  uc_project_set_id (p->id);
  uc_project_set_title (p->title);
  uc_project_set_description (p->description);

  path = g_strdup_printf ("%s/projects/%u/project.xml",
			  uc_project_get_working_path (), p->id);

  g_assert (g_file_test (path, G_FILE_TEST_EXISTS));

  doc = xmlParseFile (path);
  cur = xmlDocGetRootElement (doc);

  g_assert (cur != NULL);

  cur = cur->xmlChildrenNode;
  while (cur != NULL)
    {
      if (!xmlStrcmp (cur->name, BAD_CAST "project"))
	list = uc_project_xml_project_tag (list, doc, cur);

      cur = cur->next;
    }

  g_free (path), path = NULL;
  xmlFreeDoc (doc), doc = NULL;

  return list;
}

static void
uc_project_xml_dispatch_links (UCLinkProperties * prop)
{
  GList *item = NULL;

  item = g_list_first (prop->tags);
  while (item != NULL)
    {
      UCHTMLTag *tag = (UCHTMLTag *) item->data;

      if (tag->type == LINK_TYPE_META)
	prop->metas = g_list_append (prop->metas, tag);
      else if (tag->type == LINK_TYPE_EMAIL)
	prop->emails = g_list_append (prop->emails, tag);
      else if (tag->type != LINK_TYPE_NONE)
	prop->all_links = g_list_append (prop->all_links, tag);

      item = g_list_next (item);
    }
}

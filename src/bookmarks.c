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
#include "lists.h"
#include "report.h"
#include "html_parser.h"
#include "utils.h"
#include "url.h"
#include "timeout.h"

#include "bookmarks.h"


static UCLinkProperties *parents[UC_MAX_DEPTH_LEVEL];

static UCLinkProperties *uc_bookmarks_register_folder (
         const UCLinkProperties *parent, const xmlNodePtr parent_xml_node,
         const guint32 folder_id, const guint32 id, const guint depth,
         const gchar *label);
static UCLinkProperties *uc_bookmarks_register_link (const guint depth,
                                                     const gchar *label,
                                                     const gchar *value);
static GList *uc_bookmarks_get_links (void);
static void uc_bookmarks_reassign_parents (GList * list,
					   UCLinkProperties * parent);

// Firefox
#ifdef ENABLE_SQLITE3

#define FF_QUERY_GET_ROOT \
  "SELECT folder_id FROM moz_bookmarks_roots WHERE root_name='menu'"

#define FF_QUERY_GET_CHILDS \
  "SELECT \
     mb.id AS mb_id, \
     mb.parent AS folder_id, \
     mb.title, \
     mp.id, \
     mp.title AS label, \
     mp.url \
   FROM \
     moz_bookmarks AS mb LEFT JOIN moz_places AS mp \
      ON mb.fk = mp.id \
   WHERE mb.title <> '' \
     AND mb.parent = %i"

#define FF_QUERY_DELETE_ITEM \
  "DELETE FROM moz_bookmarks WHERE fk = %i"


static sqlite3 *sqlite3_cnx = NULL;

static GList *uc_bookmarks_firefox_get_content (void);
static gint uc_bookmarks_firefox_cb (gpointer data, gint argc,
                                gchar **argv, gchar **azColName);
static void uc_bookmarks_firefox_delete_item (const guint32 id);
static void uc_bookmarks_sqlite3_close (void);
static gint uc_bookmarks_sqlite3_open (const gchar *file);
static gint uc_bookmarks_sqlite3_query (const gchar *query);
#endif

// Google Chrome
#ifdef ENABLE_JSON
static GList *uc_bookmarks_googlechrome_get_content (const gchar * docname);
static GList *uc_bookmarks_googlechrome_parse (JsonNode *node,
                                          UCLinkProperties *parent,
                                          GList **list);
static void uc_bookmarks_googlechrome_delete_item (JsonArray *array,
                                              const guint32 id);

static JsonParser *uc_bookmarks_googlechrome_doc = NULL;
#endif

static guint depth = 0;

// XBEL
static xmlDocPtr uc_bookmarks_xbel_doc = NULL;

static xmlDocPtr uc_bookmarks_xbel_parse_file (const gchar * docname);
static GList *uc_bookmarks_xbel_parse_bookmark (guint depth, GList * list,
					       xmlDocPtr doc, xmlNodePtr cur);
static GList *uc_bookmarks_xbel_parse_folder (guint depth, GList * list,
					     xmlDocPtr doc, xmlNodePtr cur);
static GList *uc_bookmarks_xbel_get_content (const gchar * docname);

// Opera
static gchar **uc_bookmarks_opera_doc = NULL;

static GList *uc_bookmarks_opera_get_content (const gchar * docname);
static void uc_bookmarks_opera_delete_item (const guint32 id);
static gchar *uc_bookmarks_opera_file_get_value (gchar **arr, gchar *key);
static GList *uc_bookmarks_opera_parse_folder (GList * list, gchar **arr);
static void uc_bookmarks_opera_save (void);


///////////////////////// Common bookmarks functions //////////////////////////


static UCLinkProperties *
uc_bookmarks_register_folder (const UCLinkProperties *parent,
                              const xmlNodePtr parent_xml_node,
                              const guint32 folder_id, const guint32 id,
                              const guint depth, const gchar *label)
{
  UCLinkProperties *prop = NULL;


  prop = uc_check_link_properties_node_new ();
  prop->parent = (UCLinkProperties *) parent;
  prop->xml_node = (xmlNodePtr) parent_xml_node;
  prop->bookmark_folder_id = folder_id;
  prop->bookmark_id = id;
  prop->depth_level = depth;
  prop->label = g_strdup (label);
  prop->link_value = g_strdup ("");
  prop->link_icon =
    gdk_pixbuf_new_from_file (UC_PIXMAPS_DIR "/link_type_folder.png", NULL);
  prop->link_type = LINK_TYPE_BOOKMARK_FOLDER;
  prop->header = g_hash_table_new_full (g_str_hash, g_str_equal,
                                        g_free, g_free);

  uc_check_link_already_checked_with_insert (prop, "");

  return prop;
}


static UCLinkProperties *
uc_bookmarks_register_link (const guint depth, const gchar *label,
                            const gchar *value)
{
  UCLinkProperties *prop = NULL;
  UCHTMLTag *tag = NULL;
  gchar *hostname = NULL;
  gboolean accept;
  gchar *proto = NULL;


  tag = uc_html_parser_node_new ();
  tag->depth = depth;
  tag->label = g_strdup (label);
  tag->value = g_strdup (value);
  proto = uc_url_get_protocol (tag->value);
  tag->type = (strcmp (proto, UC_PROTOCOL_FILE) == 0) ?
                LINK_TYPE_FILE_HREF : LINK_TYPE_HREF;
  g_free (proto), proto = NULL;

  uc_check_currentitem_init (NULL, "", tag, tag->value, FALSE);

  hostname = uc_url_get_hostname ("", tag->value);

  if (!uc_timeout_domains_is_blocked (hostname))
  {
    prop = uc_check_link_get_properties (depth, "", "", tag, NULL, &accept, 0);
    if (prop != NULL && accept)
    {
      prop->normalized_url = g_strdup (prop->url);

      // Run security tests if requested
      if (uc_project_get_security_checks ("any"))
        uc_check_run_security_checks (prop);

      // Run w3c tests if requested
      if (uc_project_get_w3c_checks ("any"))
        uc_check_run_w3c_checks (prop);
    }
  }
  else if (prop != NULL)
  {
    // FIXME
    g_warning (">> FIXME DEBUG : prop != NULL (should never append)\n");
  }
  else
  {
    // FIXME
    g_warning (">> FIXME DEBUG : Freeing tag\n");
    uc_html_parser_node_free (&tag);
  }

  g_free (hostname), hostname = NULL;

  return prop;
}


/**
 * uc_bookmarks_get_links:
 * 
 * Get all the links of a given bookmarks
 * file.
 *
 * Returns: A #GList of #UCLinkPropeties node from bookmarks file.
 */
static GList *
uc_bookmarks_get_links (void)
{
  GList *list = NULL;


  switch (uc_project_get_bookmarks_type ())
  {
#ifdef ENABLE_JSON
    // Google Chrome
    case UC_BOOKMARKS_TYPE_GOOGLECHROME:
      list = uc_bookmarks_googlechrome_get_content (
               uc_project_get_bookmarks_file ());
      break;
#endif

    // XBEL
    case UC_BOOKMARKS_TYPE_XBEL:
      list = uc_bookmarks_xbel_get_content (uc_project_get_bookmarks_file ());
      break;

    // Opera
    case UC_BOOKMARKS_TYPE_OPERA:
      list = uc_bookmarks_opera_get_content (uc_project_get_bookmarks_file ());
      break;

#ifdef ENABLE_SQLITE3
    // FF sqlite3
    case UC_BOOKMARKS_TYPE_FIREFOX:
      list = uc_bookmarks_firefox_get_content ();
      break;
#endif

    // Default
    case UC_BOOKMARKS_TYPE_NONE:
    default:
      g_assert_not_reached ();
  }

  return list;
}


/**
 * uc_bookmarks_reassign_parents:
 * @list: Bookmarks list.
 * @parent: Parent node.
 *
 * Reorganize parents/childs in the URLs list. 
 */
static void
uc_bookmarks_reassign_parents (GList * list, UCLinkProperties * parent)
{
  GList *item = NULL;


  item = g_list_first (list);
  while (item != NULL)
  {
    UCLinkProperties *prop = (UCLinkProperties *) item->data;

    item = g_list_next (item);

    prop->parent = parent;

    if (prop->childs)
      uc_bookmarks_reassign_parents (prop->childs, prop);
  }
}


/**
 * uc_bookmarks_save_changes:
 *
 * Save bookmarks changes.
 *
 * Returns: %TRUE if the save must be done.
 */
gint
uc_bookmarks_save_changes (void)
{
  gchar *new_path = NULL;
  gint res = 0;


  if (!g_file_test (uc_project_get_bookmarks_file (), G_FILE_TEST_EXISTS))
    {
      gchar *filename = NULL;


      res = uc_application_dialog_yes_no_show (
        _("Should remote bookmarks be saved on your machine?"),
        GTK_MESSAGE_QUESTION);
      if (res != GTK_RESPONSE_YES)
	return res;

      filename = uc_application_input_file_dialog (
                   _("Save bookmarks file"),
                   _("Input filename"));
      if (!filename)
	return GTK_RESPONSE_CANCEL;

      xmlSaveFile (filename, uc_bookmarks_xbel_doc);
      if ((!g_file_test (filename, G_FILE_TEST_EXISTS)))
      {
        uc_application_dialog_show (
          _("The filename is not in a correct format.\n"
            "<b>Nothing has been done</b>."), GTK_MESSAGE_ERROR);
        g_free (filename), filename = NULL;
        return GTK_RESPONSE_CANCEL;
      }

      g_free (filename), filename = NULL;

      uc_project_set_save_bookmarks (FALSE);
    }
  else
  {
    switch (uc_project_get_bookmarks_type ())
    {
#ifdef ENABLE_JSON
      // Google Chrome
      case UC_BOOKMARKS_TYPE_GOOGLECHROME:
#endif
      // Opera
      case UC_BOOKMARKS_TYPE_OPERA:
      // XBEL
      case UC_BOOKMARKS_TYPE_XBEL:

        res = uc_application_dialog_yes_no_show (
                _("Bookmarks changes will be saved.\n"
                  "<i>Your old bookmarks file will be renamed with \".uc\" "
                  "extension</i>.\n"
                  "<b>Do you want to save</b>?"), GTK_MESSAGE_QUESTION);
        if (res != GTK_RESPONSE_YES)
          return res;
    
        new_path = g_strconcat (uc_project_get_bookmarks_file (), ".uc", NULL);
    
        if (rename (uc_project_get_bookmarks_file (), new_path) == 0)
        {
#ifdef ENABLE_JSON
          if (uc_project_get_bookmarks_type () ==
                UC_BOOKMARKS_TYPE_GOOGLECHROME)
          {
            JsonGenerator *gen = json_generator_new ();

            json_generator_set_root (gen, json_parser_get_root (
                                            uc_bookmarks_googlechrome_doc));
            json_generator_to_file (gen, uc_project_get_bookmarks_file (),
                                    NULL);

            g_object_unref (gen);
          }
          else
#endif
          if (uc_project_get_bookmarks_type () == UC_BOOKMARKS_TYPE_OPERA)
            uc_bookmarks_opera_save ();
          else
            xmlSaveFile (uc_project_get_bookmarks_file (),
                         uc_bookmarks_xbel_doc);
          uc_project_set_save_bookmarks (FALSE);
        }
        else
          uc_application_dialog_show (
            _("A error occured.\n"
              "<b>Nothing has been done</b>."), GTK_MESSAGE_ERROR);
    
        g_free (new_path), new_path = NULL;

        break;
  
#ifdef ENABLE_SQLITE3
      // FF sqlite3
      case UC_BOOKMARKS_TYPE_FIREFOX:

        res = uc_application_dialog_yes_no_show (
                _("Bookmarks changes will be applied.\n"
                  "<i>We recommand you to manually backup your Firefox "
                  "database before applying changes!</i>\n"
                  "<b>Do you want to apply</b>?"), GTK_MESSAGE_QUESTION);
        if (res != GTK_RESPONSE_YES)
          return res;

        uc_bookmarks_sqlite3_query ("COMMIT");
        uc_bookmarks_sqlite3_query ("BEGIN");
        uc_project_set_save_bookmarks (FALSE);

        break;
#endif
  
      // Default
      case UC_BOOKMARKS_TYPE_NONE:
      default:
        g_assert_not_reached ();
    }
  }

  return GTK_RESPONSE_YES;
}


void
uc_bookmarks_delete_bad_links (GList *list)
{
  GList *item = NULL;
  UCLinkProperties *prop = NULL;
  gchar *status_code = NULL;


  item = g_list_first (list);
  while (item != NULL)
  {
    prop = (UCLinkProperties *) item->data;

    item = g_list_next (item);

    if (prop->link_type == LINK_TYPE_META || prop->to_delete ||
        prop->link_type == LINK_TYPE_NONE)
      continue;

    if (prop->link_type == LINK_TYPE_BOOKMARK_FOLDER)
    {
      if (prop->childs != NULL)
        uc_bookmarks_delete_bad_links (prop->childs);
    }
    else
    {
      status_code = g_hash_table_lookup (prop->header, UC_HEADER_STATUS);

      if (strcmp (status_code, UC_STATUS_CODE_PAGE_NOT_FOUND) == 0)
        uc_bookmarks_delete_link (prop);
    }
  }
}


/**
 * uc_bookmarks_delete_link:
 * @prop: #UCLinkProperties node to work with.
 * 
 * Remove a node on the treeview list.
 * The removal is virtual in the local lists (to_delete = TRUE)
 */
void
uc_bookmarks_delete_link (UCLinkProperties *prop)
{
  prop->to_delete = TRUE;

  switch (uc_project_get_bookmarks_type ())
  {
    // XBEL
    case UC_BOOKMARKS_TYPE_XBEL:
      xmlUnlinkNode (prop->xml_node);
      xmlFreeNode (prop->xml_node), prop->xml_node = NULL;
      break;

#ifdef ENABLE_SQLITE3
    // FF sqlite3
    case UC_BOOKMARKS_TYPE_FIREFOX:
      uc_bookmarks_firefox_delete_item (prop->bookmark_id);
      break;
#endif

#ifdef ENABLE_JSON
    // Google Chrome
    case UC_BOOKMARKS_TYPE_GOOGLECHROME:
      uc_bookmarks_googlechrome_delete_item (prop->json_array,
                                             prop->bookmark_id);
      break;
#endif

    // Opera
    case UC_BOOKMARKS_TYPE_OPERA:
      uc_bookmarks_opera_delete_item (prop->bookmark_id);
      break;

    // Default
    case UC_BOOKMARKS_TYPE_NONE:
    default:
      g_assert_not_reached ();
  }
}


void
uc_bookmarks_free (void)
{
  guint i = 0;

  
  for (i = 0; i < UC_MAX_DEPTH_LEVEL; i++)
    parents[i] = NULL;

  if (uc_bookmarks_xbel_doc != NULL)
    xmlFreeDoc (uc_bookmarks_xbel_doc), uc_bookmarks_xbel_doc = NULL;

  if (uc_bookmarks_opera_doc != NULL)
    g_strfreev (uc_bookmarks_opera_doc), uc_bookmarks_opera_doc = NULL;

#ifdef ENABLE_SQLITE3
  uc_bookmarks_sqlite3_close ();
#endif

#ifdef ENABLE_JSON
  if (uc_bookmarks_googlechrome_doc != NULL)
    g_object_unref (uc_bookmarks_googlechrome_doc),
      uc_bookmarks_googlechrome_doc = NULL;
#endif

  uc_project_set_bookmarks_type (UC_BOOKMARKS_TYPE_NONE);
}


/**
 * uc_bookmarks_begin_check:
 * 
 * Begin the bookmarks check.
 */
void
uc_bookmarks_begin_check (void)
{
  guint timer_id = 0;
  gchar *tmp = NULL;
  GtkWidget *widget = NULL;
  GtkTreeIter iter;

  uc_application_draw_main_frames ();
  uc_application_progress_dialog_show ();
  uc_application_display_state_message (UC_CHECK_MESSAGE_LABEL_FIRST,
                                        _("Check in progress..."));
  UC_UPDATE_UI;

  UC_GET_WIDGET ("urls_list", WGET ("main_window"), widget);
  treeview = GTK_TREE_VIEW (widget);

  treestore = gtk_tree_store_new (N_COLUMNS,
				  G_TYPE_INT,
				  GDK_TYPE_PIXBUF,
				  GDK_TYPE_PIXBUF,
				  GDK_TYPE_PIXBUF,
				  GDK_TYPE_PIXBUF,
				  G_TYPE_STRING,
				  G_TYPE_STRING, G_TYPE_STRING,
				  G_TYPE_STRING);

  uc_application_build_url_treeview ();
  gtk_widget_show (widget);

  tmp = g_strconcat ("gURLChecker ", UC_VERSION, " - ",
		     uc_project_get_bookmarks_file (), NULL);
  gtk_window_set_title (GTK_WINDOW (WGET ("main_window")), tmp);
  g_free (tmp), tmp = NULL;

  /* init timer */
  timer_id = g_timeout_add (1000, &uc_report_timer_callback, NULL);

  uc_project_set_check_is_current (TRUE);
  uc_project_set_check_is_main (TRUE);
  uc_project_set_current_host ("");
  uc_project_set_current_port (UC_URL_DEFAULT_PORT);
  uc_lists_checked_links_set (uc_bookmarks_get_links ());
  uc_project_set_check_is_main (FALSE);
  uc_project_set_check_is_current (FALSE);

  g_source_remove (timer_id), timer_id = 0;

  gtk_tree_store_clear (treestore);

  UC_DISPLAY_SET_ALL;
  uc_check_display_list (uc_lists_checked_links_get (), NULL, iter);

  gtk_widget_hide (WGET ("progress_dialog"));

  /* Update some menu items */
  WSENS ("mwm_delete_all_bad_links", TRUE);
  WSENS ("mwm_display", TRUE);
  WSENS ("mwm_delete_project", FALSE);
  WSENS ("mwm_refresh_all", FALSE);
  WSENS ("mwm_refresh_main_page", FALSE);
  WSENS ("mwm_find", !uc_project_get_speed_check ());
  WSENS ("mwm_display_security_alerts", !uc_project_get_speed_check ());

  uc_check_cancel_set_value (FALSE);
}


UCBookmarksType
uc_bookmarks_guess_type (const gchar *file)
{
  UCBookmarksType btype = UC_BOOKMARKS_TYPE_NONE;

  if (uc_bookmarks_format_is_opera (file))
    btype = UC_BOOKMARKS_TYPE_OPERA;
  else if (uc_bookmarks_format_is_xbel (file))
    btype = UC_BOOKMARKS_TYPE_XBEL;
#ifdef ENABLE_SQLITE3
  else if (uc_bookmarks_format_is_firefox (file))
    btype = UC_BOOKMARKS_TYPE_FIREFOX;
#endif
#ifdef ENABLE_JSON
  else if (uc_bookmarks_format_is_googlechrome (file))
    btype = UC_BOOKMARKS_TYPE_GOOGLECHROME;
#endif
  
  return btype;
}


////////////////////////////// Firefox ////////////////////////////////////////


gboolean
uc_bookmarks_format_is_firefox (const gchar *file)
{
  gboolean ret = FALSE;
#ifdef ENABLE_SQLITE3
  gchar *error = NULL;


  ret = (uc_bookmarks_sqlite3_open (file) == SQLITE_OK);
  if (!ret)
    g_warning ("Bookmarks: can't open FF sqlite3 database: %s\n",
               sqlite3_errmsg (sqlite3_cnx));
  else
  {
    if (sqlite3_exec (sqlite3_cnx, "SELECT id FROM moz_places LIMIT 1",
                      NULL, NULL, &error) != SQLITE_OK)
    {
      ret = FALSE;
      sqlite3_free (error), error = NULL;
    }
  }
  uc_bookmarks_sqlite3_close ();
#endif

  return ret;
}


#ifdef ENABLE_SQLITE3
static void
uc_bookmarks_sqlite3_close (void)
{
  if (sqlite3_cnx != NULL)
  {
    sqlite3_close (sqlite3_cnx);
    sqlite3_cnx = NULL;
  }
}


static gint
uc_bookmarks_sqlite3_open (const gchar *file)
{
  gint ret = -1;

 
  if (sqlite3_cnx != NULL)
    ret = SQLITE_OK;
  else
    ret = sqlite3_open_v2 ((file)?file:uc_project_get_bookmarks_file (),
                           &sqlite3_cnx, SQLITE_OPEN_READWRITE, NULL);

  return ret;
}


static gint
uc_bookmarks_sqlite3_query (const gchar *query)
{
  sqlite3_stmt *stmt = NULL;
  gint rc;


  sqlite3_prepare_v2 (sqlite3_cnx, query, strlen (query), &stmt, NULL);
  rc = sqlite3_step (stmt);
  sqlite3_finalize (stmt);

  return rc;
}


static void
uc_bookmarks_firefox_delete_item (const guint32 id)
{
  gchar *query = NULL;


  query = g_strdup_printf (FF_QUERY_DELETE_ITEM, id);
  uc_bookmarks_sqlite3_query (query);
  g_free (query), query = NULL;
}


static gint
uc_bookmarks_firefox_cb (gpointer data, gint argc, gchar **argv,
                         gchar **azColName)
{
  gint i = 0;
  UCLinkProperties *prop = NULL;
  gchar *hostname = NULL;
  gchar *key = NULL;
  gchar *value = NULL;
  gchar *query = NULL;
  gchar *error = NULL;
  gint folder_id = 0;
  gchar *folder_name = NULL;
  gint id = 0;
  gint mb_id = 0;
  GList **list = (GList **) data;


  if (uc_check_cancel_get_value ())
    return SQLITE_OK;

  uc_report_display_update ();
  uc_check_wait ();

  for (i = 0; i < argc; i++)
  {
    if (strcmp (azColName[i], "label") == 0)
      key = g_strdup ((argv[i]) ? argv[i] : "");
    if (strcmp (azColName[i], "title") == 0)
      folder_name = g_strdup ((argv[i]) ? argv[i] : "");
    else if (strcmp (azColName[i], "url") == 0)
      value = g_strdup ((argv[i]) ? argv[i] : "");
    else if (strcmp (azColName[i], "folder_id") == 0)
      folder_id = atoi ((argv[i]) ? argv[i] : "0");
    else if (strcmp (azColName[i], "id") == 0)
      id = atoi ((argv[i]) ? argv[i] : "0");
    else if (strcmp (azColName[i], "mb_id") == 0)
      mb_id = atoi ((argv[i]) ? argv[i] : "0");
  }

  // Folder
  if (strlen (key) == 0)
  {
    prop = uc_bookmarks_register_folder (NULL, NULL, folder_id, id, depth,
                                         folder_name);
    *list = g_list_append (*list, uc_check_register_link ("", prop));

    ++depth;

    query = g_strdup_printf (FF_QUERY_GET_CHILDS, mb_id);
    sqlite3_exec (sqlite3_cnx, query, uc_bookmarks_firefox_cb, &prop->childs,
                  &error);
    g_free (query), query = NULL;

    --depth;
  }
  // Link
  else
  {
    prop = uc_bookmarks_register_link (depth, key, value);

    if (prop != NULL)
    {
      prop->bookmark_folder_id = folder_id;
      prop->bookmark_id = id;

      *list = g_list_append (*list, uc_check_register_link (prop->url, prop));
    }
  }

  g_free (key), key = NULL;
  g_free (value), value = NULL;
  g_free (folder_name), folder_name = NULL;
  g_free (hostname), hostname = NULL;

  return SQLITE_OK;
}


static GList *
uc_bookmarks_firefox_get_content (void)
{
  sqlite3_stmt *stmt = NULL;
  gchar *error = NULL;
  gchar *query = NULL;
  gint root_id;
  GList *list = NULL;


  uc_bookmarks_sqlite3_open (NULL);
  uc_bookmarks_sqlite3_query ("BEGIN");

  // Get id off bookmarks root "menu"
  sqlite3_prepare_v2 (sqlite3_cnx, FF_QUERY_GET_ROOT,
                      strlen (FF_QUERY_GET_ROOT), &stmt, NULL);
  sqlite3_step (stmt);
  root_id = sqlite3_column_int (stmt, 0);
  sqlite3_finalize (stmt);

  // Get list of links
  query = g_strdup_printf (FF_QUERY_GET_CHILDS, root_id);
  sqlite3_exec (sqlite3_cnx, query, uc_bookmarks_firefox_cb, &list,
                &error);
  g_free (query), query = NULL;

  uc_bookmarks_reassign_parents (list, NULL);

  return list;
}
#endif // ENABLE_SQLITE3


////////////////////////////// XBEL ///////////////////////////////////////////


/**
 * uc_bookmarks_format_is_xbel:
 * @file: The file to check.
 * 
 * Check the format of a given XBEL file.
 * 
 * Returns: TRUE if the given file is in XBEL format.
 */
gboolean
uc_bookmarks_format_is_xbel (const gchar * file)
{
  gchar *buffer = NULL;
  gboolean ret = FALSE;
  gsize length = 0;


  g_file_get_contents (file, &buffer, &length, NULL);
  ret =
    ((strstr (buffer, "<xbel ") != NULL) ||
     (strstr (buffer, "<XBEL ") != NULL) ||
     (strstr (buffer, " xbel>") != NULL) ||
     (strstr (buffer, " XBEL>") != NULL));
  g_free (buffer), buffer = NULL;

  return ret;
}


/**
 * uc_bookmarks_xbel_parse_bookmark:
 * @depth: Depth of the node.
 * @list: The list to work with.
 * @doc: The XML doc to work with.
 * @cur: The XML cursor to work with.
 * 
 * Parse bookmark entity.
 *
 * Returns: A #GList of #UCLinkProperties nodes.
 */
static GList *
uc_bookmarks_xbel_parse_bookmark (guint depth,
				 GList * list, xmlDocPtr doc, xmlNodePtr cur)
{
  xmlChar *key = NULL;
  xmlChar *value = NULL;
  xmlNodePtr parent = NULL;


  uc_report_display_update ();
  uc_check_wait ();

  value = xmlGetProp (cur, (const xmlChar *) "href");

  parent = cur;
  cur = cur->xmlChildrenNode;
  while (cur != NULL && !uc_check_cancel_get_value ())
  {
    if (!xmlStrcmp (cur->name, (const xmlChar *) "title"))
    {
      UCLinkProperties *prop = NULL;


      key = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
      prop = uc_bookmarks_register_link (depth, (gchar *) key, (gchar *) value);
      xmlFree (key), key = NULL;

      if (prop != NULL)
      {
        prop->xml_node = parent;

        list = g_list_append (list, uc_check_register_link (prop->url, prop));
      }
      break;
    }

    cur = cur->next;
  }

  xmlFree (value), value = NULL;

  return list;
}

/**
 * uc_bookmarks_xbel_parse_folder:
 * @depth: Depth of the node.
 * @list: The list to work with.
 * @doc: The XML doc to work with.
 * @cur: The XML cursor to work with.
 * 
 * Parse folder entity.
 *
 * Returns: A #GList of #UCLinkProperties nodes.
 */
static GList *
uc_bookmarks_xbel_parse_folder (guint depth,
			       GList * list, xmlDocPtr doc, xmlNodePtr cur)
{
  xmlChar *key = NULL;
  xmlNodePtr parent = NULL;

  uc_report_display_update ();

  if (depth == uc_project_get_depth_level ())
    return list;

  parent = cur;
  cur = cur->xmlChildrenNode;
  while (cur != NULL && !uc_check_cancel_get_value ())
    {
      if (!xmlStrcmp (cur->name, (const xmlChar *) "title"))
	{
	  UCLinkProperties *prop = NULL;


	  key = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
          prop = uc_bookmarks_register_folder (NULL, parent, 0, 0, depth, 
                                               (gchar *) key);
	  xmlFree (key), key = NULL;

	  list = g_list_append (list, uc_check_register_link ("", prop));

	  parents[depth] = prop;
	}
      else if (!xmlStrcmp (cur->name, (const xmlChar *) "bookmark"))
	parents[depth]->childs =
	  uc_bookmarks_xbel_parse_bookmark (depth + 1,
					   parents[depth]->childs, doc, cur);
      else if (!xmlStrcmp (cur->name, (const xmlChar *) "folder"))
	parents[depth]->childs =
	  uc_bookmarks_xbel_parse_folder (depth + 1, parents[depth]->childs,
					 doc, cur);

      cur = cur->next;
    }

  return list;
}


/**
 * uc_bookmarks_xbel_parse_file:
 * @docname: Path of the document to open.
 * 
 * Open xml file and parse it with DOM.
 * if there is a encoding problem, rebuild the given file by specifying
 * the encoding based on current locales and try to parse it again.
 *
 * Returns: A #xmlDocPtr reference.
 */
static xmlDocPtr
uc_bookmarks_xbel_parse_file (const gchar * docname)
{
  xmlDocPtr doc = NULL;

  doc = xmlParseFile (docname);
  if (doc == NULL)
    {
      FILE *fd = NULL;
      gchar *buffer = NULL;
      gchar **tab = NULL;
      gchar *filename = NULL;
      guint32 i = 0;

      filename = g_strdup_printf ("%s/%s/%s.%u",
				  uc_project_get_working_path (),
				  uc_project_get_cache_name (),
				  strrchr (docname, '/'), getpid ());
      fd = fopen (filename, "w");
      g_assert (fd != NULL);

      g_file_get_contents (docname, &buffer, NULL, NULL);
      g_assert (buffer != NULL);

      tab = g_strsplit (buffer, "\n", 0);

      while (tab[i])
	{
	  if (strstr (tab[i], "<?xml ") != NULL)
	    {
	      g_free (tab[i]), tab[i] = NULL;

	      tab[i] =
		g_strdup_printf ("<?xml version=\"1.0\" encoding=\"%s\"?>",
				 uc_project_get_local_charset ());

	      break;
	    }
	  i++;
	}

      if (tab[i] == NULL)
	fprintf (fd,
		 "<?xml version=\"1.0\" encoding=\"%s\"?>\n"
		 "%s", uc_project_get_local_charset (), buffer);
      else
	{
	  g_free (buffer), buffer = NULL;
	  buffer = g_strjoinv ("\n", tab);
	  fprintf (fd, "%s", buffer);
	}

      fclose (fd);

      doc = xmlParseFile (filename);
      unlink (filename);

      g_free (filename), filename = NULL;
      g_strfreev (tab), tab = NULL;
      g_free (buffer), buffer = NULL;
    }

  return doc;
}


/**
 * uc_bookmarks_xbel_get_content:
 * @docname: Path of the document to open.
 * 
 * Retreive URLs from a given file.
 * 
 * Returns: A #GList of #UCLinkPropeties node from bookmarks file.
 */
static GList *
uc_bookmarks_xbel_get_content (const gchar * docname)
{
  GList *list = NULL;
  xmlNodePtr cur = NULL;

  uc_bookmarks_xbel_doc = uc_bookmarks_xbel_parse_file (docname);
  if (uc_bookmarks_xbel_doc == NULL)
    {
      uc_application_dialog_show (_("Either the given XML file is not well "
				    "formed or its encoding can not "
				    "be found...\n"
				    "<i>If you are checking a remote bookmarks "
				    "file, try to pump up the timeout "
				    "delay</i>.\n"
				    "<b>Check has been aborted</b>."),
				  GTK_MESSAGE_ERROR);

      return NULL;
    }

  cur = xmlDocGetRootElement (uc_bookmarks_xbel_doc);
  if (cur == NULL)
    {
      uc_application_dialog_show (_("The given XML file is empty.\n"),
				  GTK_MESSAGE_WARNING);

      return NULL;
    }

  cur = cur->xmlChildrenNode;
  while (cur != NULL && !uc_check_cancel_get_value ())
    {
      if (!xmlStrcmp (cur->name, (const xmlChar *) "bookmark"))
	list =
	  uc_bookmarks_xbel_parse_bookmark (0, list, uc_bookmarks_xbel_doc,
					   cur);
      else if (!xmlStrcmp (cur->name, (const xmlChar *) "folder"))
	list =
	  uc_bookmarks_xbel_parse_folder (0, list, uc_bookmarks_xbel_doc, cur);

      cur = cur->next;
    }

  uc_bookmarks_reassign_parents (list, NULL);

  return list;
}


////////////////////////////// Google Chrome  /////////////////////////////////


/**
 * uc_bookmarks_format_googlechrome:
 * @file: The file to check.
 * 
 * Check the format of a given Google Chrome.
 * 
 * Returns: TRUE if the given file is in Google Chrome format.
 */
gboolean
uc_bookmarks_format_is_googlechrome (const gchar * file)
{
  gboolean ret = FALSE;
#ifdef ENABLE_JSON
  GError *error = NULL;
  JsonParser *parser = NULL;


  parser = json_parser_new ();
  ret = json_parser_load_from_file (parser, file, &error);
  g_object_unref (parser);
#endif

  return ret;
}


#ifdef ENABLE_JSON
static void
uc_bookmarks_googlechrome_delete_item (JsonArray *array, const guint32 id)
{
  GList *l = NULL;
  GList *items = NULL;
  guint i = 0;


   l = items = json_array_get_elements (array);

   while (items != NULL)
   {
     if (atoi (json_node_get_string (
                 json_object_get_member (json_node_get_object (items->data),
                                         "id"))) == id)
       json_array_remove_element (array, i);

     items = items->next;
     i++;
   }

  g_list_free (l);
}


/**
 * uc_bookmarks_googlechrome_get_content:
 * @docname: Path of the document to open.
 * 
 * Retreive URLs from a given file.
 * 
 * Returns: A #GList of #UCLinkPropeties node from bookmarks file.
 */
static GList *
uc_bookmarks_googlechrome_get_content (const gchar * docname)
{
  GList *list = NULL;
  JsonNode *root;
  GError *error = NULL;


  uc_bookmarks_googlechrome_doc = json_parser_new ();
  json_parser_load_from_file (uc_bookmarks_googlechrome_doc, docname, &error);
  root = json_parser_get_root (uc_bookmarks_googlechrome_doc);

  root = json_object_get_member(json_node_get_object (root), "roots");
  list = uc_bookmarks_googlechrome_parse (root, NULL, &list);

  return list;
}


static GList *
uc_bookmarks_googlechrome_parse (JsonNode *node, UCLinkProperties *parent,
                            GList **list)
{
  GList *items = NULL;
  GList *l = NULL;
  JsonObject *object = NULL;
  JsonArray *array = NULL;
  UCLinkProperties *p = parent;

  
  switch (JSON_NODE_TYPE (node))
  {
    case JSON_NODE_OBJECT:
      object = json_node_get_object (node);

      l = items = json_object_get_members(object);

      while (items != NULL)
      {
        JsonNode *node = json_object_get_member (object, items->data);
        *list = uc_bookmarks_googlechrome_parse (node, parent, list);
        items = items->next;
      }
    
      g_list_free (l);
      break;

    case JSON_NODE_ARRAY:
      array = json_node_get_array (node);

      l = items = json_array_get_elements (array);

      while (items != NULL && !uc_check_cancel_get_value ())
      {
        JsonNode *element = items->data;
        JsonObject *obj = json_node_get_object(element);
        UCLinkProperties *prop = NULL;
    

        uc_report_display_update ();
        uc_check_wait ();
    
        if (strcmp (json_node_get_string (
                      json_object_get_member (obj, "type")), "folder") == 0)
        {
          const gchar *folder_id = json_node_get_string (
                                     json_object_get_member (obj, "id"));
          const gchar *folder_name = json_node_get_string (
                                       json_object_get_member (obj, "name"));

          prop = uc_bookmarks_register_folder (p, NULL, atoi (folder_id), 0,
                                               depth, folder_name);

          *list = g_list_append (*list, uc_check_register_link ("", prop));

          p = prop;

          ++depth;
          prop->childs = uc_bookmarks_googlechrome_parse (element, p,
                                                          &prop->childs);
          --depth;
        }
        else if (strcmp (json_node_get_string (
                           json_object_get_member (obj, "type")), "url") == 0)
        {
          const gchar *id = json_node_get_string (
                              json_object_get_member (obj, "id"));
          const gchar *key = json_node_get_string (
                               json_object_get_member (obj, "name"));
          const gchar *value = json_node_get_string (
                                 json_object_get_member (obj, "url"));


          prop = uc_bookmarks_register_link (depth, key, value);

          if (prop != NULL)
          {
             prop->parent = p;
             prop->bookmark_folder_id = (p) ? p->bookmark_folder_id : 0;
             prop->bookmark_id = atoi (id);
             prop->json_array = array;

             *list = g_list_append (*list,
                                    uc_check_register_link (prop->url, prop));
          }
        } 
        else
          *list = uc_bookmarks_googlechrome_parse (element, p, list);
    
        items = items->next;
      }
    
      g_list_free (l);
      break;

    default:
      ;
  }

  return *list;
}
#endif // ENABLE_JSON


////////////////////////////// Opera //////////////////////////////////////////


/**
 * uc_bookmarks_format_is_opera:
 * @file: The file to check.
 * 
 * Check the format of a given Opera file.
 * 
 * Returns: TRUE if the given file is in Opera format.
 */
gboolean
uc_bookmarks_format_is_opera (const gchar * file)
{
  gchar *buffer = NULL;
  gboolean ret = FALSE;
  gsize length = 0;


  g_file_get_contents (file, &buffer, &length, NULL);
  ret = (strstr (buffer, "Opera Hotlist") != NULL);
  g_free (buffer), buffer = NULL;

  return ret;
}


static void
uc_bookmarks_opera_delete_item (const guint32 id)
{
  guint32 i = 0;


  while (uc_bookmarks_opera_doc[i])
  {
    if (strcmp (uc_bookmarks_opera_doc[i], "#URL") == 0 &&
        atoi (uc_bookmarks_opera_file_get_value (
                &uc_bookmarks_opera_doc[i], "ID")) == id)
    {
      while (*uc_bookmarks_opera_doc[i])
      {
        *uc_bookmarks_opera_doc[i] = 'D';
        ++i;
      }
      break;
    }

    ++i;
  }
}


static gchar *
uc_bookmarks_opera_file_get_value (gchar **arr, gchar *key)
{
  gint i = 0;
  gchar *lkey = NULL;
  gchar *ret = NULL;


  lkey = g_strconcat ("\t", key, "=", NULL);

  while (*arr[i] && 
         (ret = strstr (arr[i++], lkey)) == NULL);

  if (ret != NULL)
    ret += strlen (lkey);

  g_free (lkey), lkey = NULL;

  return ret;
}


static void
uc_bookmarks_opera_save (void)
{
  guint32 i = 0;
  FILE *fd;


  fd = fopen (uc_project_get_bookmarks_file (), "w");

  while (uc_bookmarks_opera_doc[i])
  {
    if (*uc_bookmarks_opera_doc[i] != 'D')
    {
      gchar *line = g_strconcat (uc_bookmarks_opera_doc[i], "\n", NULL);
      fwrite (line, 1, strlen (line), fd);
      g_free (line), line = NULL;
    }
    ++i;
  }

  fclose (fd);
}


/**
 * uc_bookmarks_opera_parse_folder:
 * @list: The list to work with.
 * @arr: Opera file splitted in lines.
 * 
 * Parse Opera file.
 *
 * Returns: A #GList of #UCLinkProperties nodes.
 */
static GList *
uc_bookmarks_opera_parse_folder (GList * list, gchar **arr)
{
  guint32 i = 0;
  gboolean trash_folder = FALSE;


  uc_report_display_update ();

  if (depth == uc_project_get_depth_level ())
    return list;

  while (arr[i] && !uc_check_cancel_get_value ())
  {
    UCLinkProperties *prop = NULL;


    // Folder in
    if (strcmp (arr[i], "#FOLDER") == 0)
    {
      // Ignore trash folder
      trash_folder = 
        (uc_bookmarks_opera_file_get_value (&arr[i], "TRASH FOLDER") != NULL);

      if (!trash_folder)
      {
         prop = uc_bookmarks_register_folder (
           parents[depth], NULL, 
           atoi (uc_bookmarks_opera_file_get_value (&arr[i], "ID")), 0, depth,
           uc_bookmarks_opera_file_get_value (&arr[i], "NAME"));

        if (parents[depth])
          parents[depth]->childs = 
            g_list_append (parents[depth]->childs,
                           uc_check_register_link ("", prop));
        else
          list = g_list_append (list, uc_check_register_link ("", prop));
  
        parents[++depth] = prop;
      }
    }
    // Bookmark
    else if (strcmp (arr[i], "#URL") == 0)
    {
      prop = uc_bookmarks_register_link (
               depth, 
               uc_bookmarks_opera_file_get_value (&arr[i], "NAME"),
               uc_bookmarks_opera_file_get_value (&arr[i], "URL"));

      if (prop != NULL)
      {
        prop->bookmark_id = 
          atoi (uc_bookmarks_opera_file_get_value (&arr[i], "ID"));

        if (parents[depth])
        {
          prop->parent = parents[depth];
          prop->bookmark_folder_id = prop->parent->bookmark_folder_id;

          parents[depth]->childs =
            g_list_append (parents[depth]->childs,
                           uc_check_register_link (prop->url, prop));
        }
         else
           list = g_list_append (list,
                                 uc_check_register_link (prop->url, prop));
      }
    }
    // Folder out
    else if (*arr[i] == '-' && !trash_folder)
      --depth;

    ++i;
  }

  return list;
}


static GList *
uc_bookmarks_opera_get_content (const gchar * docname)
{
  GList *list = NULL;
  gchar *buffer = NULL;
  gsize length = 0;


  g_file_get_contents (docname, &buffer, &length, NULL);

  uc_bookmarks_opera_doc = g_strsplit (buffer, "\n", 0);

  list = uc_bookmarks_opera_parse_folder (list, uc_bookmarks_opera_doc);

  g_free (buffer), buffer = NULL;

  return list;
}

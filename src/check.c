/* 
 * Copyright (C) 2002-2011
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

#include "report.h"
#include "cache.h"
#include "application.h"
#include "connection.h"
#include "lists.h"
#include "html_parser.h"
#include "project.h"
#include "search.h"
#include "url.h"
#include "utils.h"
#include "timeout.h"
#include "ucclam.h"
#include "uccroco.h"
#include "uctidy.h"
#include "cookies.h"
#include "bookmarks.h"

#include "check.h"


#define UC_CHECK_WAIT_END_OTHER_CONDITIONS \
	( \
		(uc_check_cancel_get_value ()) || \
		(uc_check_ignore_item_get_value ()) \
	)


/*
 * struct containing path for "type"
 * images
 */
typedef struct _UCLinkTypeIcon UCLinkTypeIcon;
static struct _UCLinkTypeIcon
{
  UCLinkType type;
  gchar *icon_file;
}
uc_check_link_type_icon[] =
{
  {LINK_TYPE_HREF, UC_PIXMAPS_DIR "/link_type_href.png"},
  {LINK_TYPE_HREF_SSL, UC_PIXMAPS_DIR "/link_type_href_ssl.png"},
  {LINK_TYPE_HREF_FTP, UC_PIXMAPS_DIR "/link_type_href_ftp.png"},
  {LINK_TYPE_FILE_HREF, UC_PIXMAPS_DIR "/link_type_href_local.png"},
  {LINK_TYPE_FILE_IMAGE, UC_PIXMAPS_DIR "/link_type_image_local.png"},
  {LINK_TYPE_EMAIL, UC_PIXMAPS_DIR "/link_type_mail.png"},
  {LINK_TYPE_IMAGE, UC_PIXMAPS_DIR "/link_type_image.png"},
  {LINK_TYPE_IMAGE_SSL, UC_PIXMAPS_DIR "/link_type_image_ssl.png"},
  {LINK_TYPE_IMAGE_FTP, UC_PIXMAPS_DIR "/link_type_image_ftp.png"},
  {LINK_TYPE_FRAME, UC_PIXMAPS_DIR "/link_type_frame.png"},
  {LINK_TYPE_FRAME_SSL, UC_PIXMAPS_DIR "/link_type_frame_ssl.png"},
  {LINK_TYPE_CSS, UC_PIXMAPS_DIR "/link_type_css.png"},
  {LINK_TYPE_CSS_SSL, UC_PIXMAPS_DIR "/link_type_css_ssl.png"},
  {LINK_TYPE_NONE, UC_PIXMAPS_DIR "/link_type_unknown.png"}
};


/* struct to point on the current url or host elements
 * (for timeouts callback) */
static struct _UCCurrentItem
{
  gchar *current_host;
  UCHTMLTag *tag;
  UCLinkProperties *parent;
  gchar *url;
  gboolean is_first;
}
currentItem;


/* global variable to know if user want to refresh a link */
static gboolean uc_check_refresh_link_value = FALSE;

/* unique id for a glist properties node */
static guint32 uc_check_node_uid = 1;

/* if TRUE, wait until it is FALSE */
static gboolean uc_check_wait_value = FALSE;

static gboolean uc_check_suspend_value = FALSE;

/* if FALSE, abort the check */
static gboolean uc_check_cancel_value = FALSE;

/* if FALSE, abort the check of current url*/
static gboolean uc_check_ignore_item_value = FALSE;

static void uc_check_right_frames_reset (void);
static gboolean uc_check_content_type_accepted (gchar * path,
						gchar * content_type);
static gboolean uc_check_protocol_accepted (gchar * protocol);
static gboolean uc_check_content_type_parsable (const gchar * content_type);
static gboolean uc_check_content_type_downloadable (gchar * content_type);
static gboolean uc_check_apply_filters (const UCLinkProperties * prop);
static gboolean uc_check_domain_accepted (gchar * domain);
static gboolean uc_check_directory_accepted (const gchar * directory);
static gboolean uc_check_chroot_authorized (const gchar * host,
					    const gchar * proto,
					    const gchar * path);
static gboolean
uc_check_link_get_properties_proto_mailto (UCLinkProperties * prop);
static UCHTTPCheckReturn
uc_check_link_get_properties_proto_http (UCLinkProperties * prop,
					 const gchar * url,
					 const gchar * current_path,
					 gchar * current_host,
					 UCHTMLTag * tag,
					 UCLinkProperties * old_prop);
static gboolean
uc_check_link_get_properties_proto_file (UCLinkProperties *prop,
                                         const gchar *url,
                                         const gchar *current_path,
                                         const gchar *current_host,
					 const UCLinkType type);
static gboolean uc_check_wait_callback (gpointer data);
static gchar *uc_check_build_referer_field (void);
static void
uc_check_refresh_report_real (GList * list, guint32 * bad,
			      guint32 * malformed, guint32 * good,
			      guint32 * timeout, guint32 * ignore);
static GHashTable *uc_check_url_build_header (const gchar * status_code);
static gboolean uc_check_is_security_alert (const UCLinkProperties * prop);
static GList *uc_check_get_bad_extensions (UCLinkProperties * prop);
static gboolean uc_check_get_virii (UCLinkProperties * prop);
static gboolean uc_check_content_type_virii (gchar * content_type);
static gboolean uc_check_content_type_bad_extensions (const UCLinkType type);
static UCLinkProperties *uc_check_link_already_checked (const gchar * url);
static gboolean uc_check_get_w3c_html_validate (UCLinkProperties * prop);
static gboolean uc_check_get_w3c_css_validate (UCLinkProperties * prop);
static void uc_check_display_list_current_item (const UCLinkProperties *prop);


gboolean
uc_check_html_is_valid (const gchar * buffer)
{
  gboolean ret = FALSE;

#ifdef ENABLE_TIDY
  UCTidy *tidy;

  if ((tidy = uc_uctidy_new ()))
    {
      ret = (uc_uctidy_validate (tidy, buffer) == 0);
      uc_uctidy_free (&tidy);
    }
#endif

  return ret;
}

/**
 * uc_check_run_w3c_checks:
 * @prop: Current node.
 *
 * Run all w3c checks on the given node.
 */
void
uc_check_run_w3c_checks (UCLinkProperties * prop)
{
  gchar *content_type = NULL;
  gchar *status = NULL;


  prop->w3c_valid = TRUE;

  status = g_hash_table_lookup (prop->header, UC_HEADER_STATUS);
  if (status && !uc_check_status_is_email (status)
      && uc_check_status_is_good (status))
    {
      content_type = g_hash_table_lookup (prop->header, UC_CONTENT_TYPE);

      /* HTML validation */
      if (uc_check_content_type_w3c_accepted
	  ("html", prop->path, content_type))
	{
	  if (uc_project_get_w3c_checks ("html"))
	    prop->w3c_valid = uc_check_get_w3c_html_validate (prop);
	}
      /* CSS validation */
      else
	if (uc_check_content_type_w3c_accepted
	    ("css", prop->path, content_type))
	{
	  if (uc_project_get_w3c_checks ("css"))
	    prop->w3c_valid = uc_check_get_w3c_css_validate (prop);
	}
    }
}


/**
 * uc_check_run_security_checks:
 * @prop: Current node.
 *
 * Run all security checks on the given node.
 */
void
uc_check_run_security_checks (UCLinkProperties * prop)
{
  // Bad file extensions test
  if (uc_project_get_security_checks ("bad_extensions"))
    {
      if (uc_check_content_type_bad_extensions (prop->link_type))
	prop->bad_extensions = uc_check_get_bad_extensions (prop);
    }

  // Scan for virii (only if page content have been downloaded)
  if (!uc_project_get_speed_check () &&
      uc_project_get_security_checks ("virii"))
  {
    if (uc_check_content_type_virii (
          g_hash_table_lookup (prop->header, UC_CONTENT_TYPE)))
      prop->virii = uc_check_get_virii (prop);
  }
}


static gboolean
uc_check_is_security_alert (const UCLinkProperties * prop)
{
  return (prop->bad_extensions != NULL || prop->virii);
}

gboolean
uc_check_is_w3c_alert (const UCLinkProperties * prop)
{
  return (!prop->w3c_valid);
}

/**
 * uc_check_status_is_good:
 * @status: status code to test.
 * 
 * Test a given status code to see if it come from
 * a good link.
 * 
 * Returns: %TRUE if we consider that the given
 * status code is good.
 */
gboolean
uc_check_status_is_good (const gchar * status)
{
  if (!status)
    return FALSE;

  return (status[0] == '2' || (!strcmp (status, UC_STATUS_CODE_FTP_OK) ||
			       !strcmp (status, UC_STATUS_CODE_REDIRECTED) ||
			       !strcmp (status,
					UC_STATUS_CODE_REDIRECTED_SEE_OTHER)
			       || !strcmp (status,
					   UC_STATUS_CODE_MOVED_PERMANENTLY)
			       || !strcmp (status,
					   UC_STATUS_CODE_GOOD_EMAIL_LINK)));
}

/**
 * uc_check_status_is_bad:
 * @status: status code to test.
 * 
 * Test a given status code to see if it come from
 * a bad link.
 * 
 * Returns: %TRUE if we consider that the given
 * status code is bad.
 */
gboolean
uc_check_status_is_bad (const gchar * status)
{
  if (!status)
    return TRUE;

  return (status[0] != '2'
	  && status[0] != '9'
	  && status[0] != '8'
	  && strcmp (status, UC_STATUS_CODE_REDIRECTED)
	  && strcmp (status, UC_STATUS_CODE_REDIRECTED_SEE_OTHER)
	  && strcmp (status, UC_STATUS_CODE_MOVED_PERMANENTLY)
	  && strcmp (status, "503")
	  && strcmp (status, UC_STATUS_CODE_TIMEOUT)
	  && strcmp (status, UC_STATUS_CODE_GOOD_EMAIL_LINK)
	  && strcmp (status, UC_STATUS_CODE_FTP_OK));
}

/**
 * uc_check_status_is_timeout:
 * @status: status code to test.
 * 
 * Test a given status code to see if it come from
 * a timedouted link.
 *
 * Returns: %TRUE if we consider that the given
 * status code is timeout.
 */
gboolean
uc_check_status_is_timeout (const gchar * status)
{
  if (!status)
    return TRUE;

  return ((status[0] == '9' ||
	   !strcmp (status, "503") ||
	   !strcmp (status, UC_STATUS_CODE_TIMEOUT))
	  &&
	  (strcmp (status, UC_STATUS_CODE_IGNORED_LINK) &&
	   strcmp (status, UC_STATUS_CODE_MALFORMED_LINK) &&
	   strcmp (status, UC_STATUS_CODE_FILE_PROTO_ERROR)));
}

/**
 * uc_check_status_is_email:
 * @status: status code to test.
 * 
 * Test a given status code to see if it come from
 * a E-Mail link.
 * 
 * Returns: %TRUE if the given status is the status
 * of a E-Mail link.
 */
gboolean
uc_check_status_is_email (const gchar * status)
{
  if (!status)
    return FALSE;

  return (!strcmp (status, UC_STATUS_CODE_BAD_EMAIL_LINK) ||
	  !strcmp (status, UC_STATUS_CODE_BAD_EMAIL_LINK_MX) ||
	  !strcmp (status, UC_STATUS_CODE_GOOD_EMAIL_LINK));
}

/**
 * uc_check_status_is_ignored:
 * @status: status code to test.
 * 
 * Test a given status code to see if it come from
 * a ignored link.
 * 
 * Returns: %TRUE if the link has been ignored
 * by the user.
 */
gboolean
uc_check_status_is_ignored (const gchar * status)
{
  if (!status)
    return TRUE;
  return (!strcmp (status, UC_STATUS_CODE_IGNORED_LINK));
}

/**
 * uc_check_status_is_malformed:
 * @status: status code to test.
 * 
 * Test a given status code to see if it come from
 * a malformed link.
 *
 * Returns: %TRUE if the link is malformed.
 */
gboolean
uc_check_status_is_malformed (const gchar * status)
{
  if (!status)
    return TRUE;

  return (!strcmp (status, UC_STATUS_CODE_MALFORMED_LINK) ||
	  !strcmp (status, UC_STATUS_CODE_FILE_PROTO_ERROR));
}

/**
 * uc_check_get_link_type_label:
 * @type: link type.
 *
 * Associate a label with a given link type.
 *
 * Returns: The label corresponding to the given link type.
 */
gchar *
uc_check_get_link_type_label (const UCLinkType type)
{
  switch (type)
    {
    case LINK_TYPE_HREF:
      return _("Hypertext Link");
    case LINK_TYPE_FILE_HREF:
      return _("Local Hypertext Link");
    case LINK_TYPE_FILE_IMAGE:
      return _("Local Image");
    case LINK_TYPE_EMAIL:
      return _("E-Mail");
    case LINK_TYPE_IMAGE:
      return _("Image");
    case LINK_TYPE_FRAME:
      return _("Frame link");
    case LINK_TYPE_CSS:
      return _("StyleSheet link");
    default:
      return _("Unknown Type");
    }
}

/**
 * uc_check_get_link_type_icon:
 * @link_type: link type.
 * @proto: protocol of the link.
 *
 * Associate a icon with a given link type.
 *
 * See: uc_check_get_link_type_for_icon(),
 *      uc_check_get_link_type_icon_path()
 * 
 * Returns: A #GdkPixbuf object corresponding to the given link type.
 */
GdkPixbuf *
uc_check_get_link_type_icon (const UCLinkType link_type, const gchar * proto)
{
  guint i = 0;
  UCLinkType lt = LINK_TYPE_NONE;
  gchar *type = uc_check_get_link_type_for_icon (proto);

  if (!strcmp (type, "ssl"))
    switch (link_type)
      {
      case LINK_TYPE_HREF:
	lt = LINK_TYPE_HREF_SSL;
	break;
      case LINK_TYPE_IMAGE:
	lt = LINK_TYPE_IMAGE_SSL;
	break;
      case LINK_TYPE_FRAME:
	lt = LINK_TYPE_FRAME_SSL;
	break;
      case LINK_TYPE_CSS:
	lt = LINK_TYPE_CSS_SSL;
	break;
      default:
	lt = link_type;
      }
  else if (!strcmp (type, "ftp"))
    switch (link_type)
      {
      case LINK_TYPE_HREF:
	lt = LINK_TYPE_HREF_FTP;
	break;
      case LINK_TYPE_IMAGE:
	lt = LINK_TYPE_IMAGE_FTP;
	break;
      default:
	lt = link_type;
      }
  else
    lt = link_type;

  for (i = 0; i < sizeof (uc_check_link_type_icon) / 
                  sizeof (UCLinkTypeIcon); i++)
  {
    if (uc_check_link_type_icon[i].type == lt)
      return gdk_pixbuf_new_from_file
	(uc_check_link_type_icon[i].icon_file, NULL);
  }

  return gdk_pixbuf_new_from_file (UC_PIXMAPS_DIR "/list_unknown.png", NULL);
}

/**
 * uc_check_get_link_type_for_icon:
 * @proto: protocol.
 *
 * Check if we are dealing with a SSL protocol (HTTPS) or
 * a FTP protocol.
 *
 * Returns: The protocol we are dealing with.
 */
gchar *
uc_check_get_link_type_for_icon (const gchar * proto)
{
  gchar *ret = NULL;

  if (!strcmp (proto, UC_PROTOCOL_HTTPS))
    ret = "ssl";
  else if (!strcmp (proto, UC_PROTOCOL_FTP))
    ret = "ftp";
  else
    ret = "";

  return ret;
}

/**
 * uc_check_get_link_type_icon_path:
 * @link_type: link type.
 * @proto: protocol of the link.
 *
 * Associate a icon path with a given link type.
 *
 * See: uc_check_get_link_type_for_icon(),
 *      uc_check_get_link_type_icon()
 * 
 * Returns: The icon path corresponding to the given link type.
 */
gchar *
uc_check_get_link_type_icon_path (const UCLinkType link_type,
				  const gchar * proto)
{
  guint i = 0;
  UCLinkType lt = LINK_TYPE_NONE;
  gchar *type = uc_check_get_link_type_for_icon (proto);

  if (!strcmp (type, "ssl"))
    switch (link_type)
      {
      case LINK_TYPE_HREF:
	lt = LINK_TYPE_HREF_SSL;
	break;
      case LINK_TYPE_IMAGE:
	lt = LINK_TYPE_IMAGE_SSL;
	break;
      case LINK_TYPE_FRAME:
	lt = LINK_TYPE_FRAME_SSL;
	break;
      case LINK_TYPE_CSS:
	lt = LINK_TYPE_CSS_SSL;
	break;
      default:
	lt = link_type;
      }
  else if (!strcmp (type, "ftp"))
    switch (link_type)
      {
      case LINK_TYPE_HREF:
	lt = LINK_TYPE_HREF_FTP;
	break;
      case LINK_TYPE_IMAGE:
	lt = LINK_TYPE_IMAGE_FTP;
	break;
      default:
	lt = link_type;
      }
  else
    lt = link_type;

  for (i = 0; i < sizeof (uc_check_link_type_icon) / 
                  sizeof (UCLinkTypeIcon); i++)
  {
    if (uc_check_link_type_icon[i].type == lt)
      return uc_check_link_type_icon[i].icon_file;
  }

  return UC_PIXMAPS_DIR "/list_unknown.png";
}


static void
uc_check_display_list_current_item (const UCLinkProperties * prop)
{
  gchar *lm_date = NULL;
  gchar *label = NULL;
  gchar *value = NULL;
  GtkTreePath *treepath = NULL;
  GtkTreeIter c_iter;
  UCURLsUserActions *action = NULL;


  /*
   * we do not display meta links nor nodes to remove from
   * bookmarks */
  if (prop->link_type == LINK_TYPE_META || prop->to_delete)
    return;

  // Do not display empty bookmarks folder
  if (prop->link_type == LINK_TYPE_BOOKMARK_FOLDER &&
      prop->childs == NULL)
    return;

  lm_date = g_hash_table_lookup (prop->header, UC_LAST_MODIFIED);
  if (lm_date == NULL)
    lm_date = "-";

  action = uc_application_get_urls_user_action (prop->user_action);

  label = uc_utils_string_format4display (prop->label,
					  UC_LABEL_DISPLAY_MAX_LEN);
  value = uc_utils_string_format4display (prop->link_value,
					  UC_LABEL_DISPLAY_MAX_LEN);

  gtk_tree_store_append (treestore, &c_iter, NULL);

  gtk_tree_store_set (treestore, &c_iter,
    UID_COLUMN, prop->uid,
    LINK_STATUS_ICON_COLUMN, prop->status_icon,
    LINK_ICON_COLUMN, prop->link_icon,
    SECURITY_ALERT_ICON_COLUMN,
      (uc_check_is_security_alert (prop))?security_alert_icon:empty_icon,
    W3C_ALERT_ICON_COLUMN,
      (uc_check_is_w3c_alert (prop))?w3c_alert_icon:empty_icon,
    ACTION_COLUMN, action->label,
    LABEL_COLUMN, label,
    URL_COLUMN, value,
    LM_COLUMN, lm_date, -1);

  gtk_tree_view_get_cursor (treeview, &treepath, NULL);

  if (treepath == NULL)
    treepath = gtk_tree_path_new_from_string ("0");
  else
    gtk_tree_path_next (treepath);

  gtk_tree_view_set_cursor (GTK_TREE_VIEW (treeview), treepath, NULL, FALSE);

  gtk_tree_path_free (treepath), treepath = NULL;
  g_free (value), value = NULL;
  g_free (label), label = NULL;
}


/**
 * uc_check_display_list:
 * @list: #GList of the links to display.
 * @path: value to build tree view path.
 * @iter: A GtkTreeIter variable.
 * 
 * Display recursively urls in the treeview.
 */
void
uc_check_display_list (GList * list, gchar * path, GtkTreeIter iter)
{
  UCLinkProperties *prop = NULL;
  UCURLsUserActions *action = NULL;
  GList *item = NULL;
  gchar *lm_date = NULL;
  gchar *label = NULL;
  gchar *value = NULL;
  guint32 pos = 0;
  GtkTreeIter c_iter;
  gboolean display = FALSE;
  gboolean display_all = UC_DISPLAY_STATUS_IS_NONE &&
                         UC_DISPLAY_TYPE_IS_NONE &&
                         UC_DISPLAY_PROTO_IS_NONE;


  if (!display_all)
    c_iter = iter;

  item = g_list_first (list);
  while (item != NULL)
  {
    prop = (UCLinkProperties *) item->data;

    item = g_list_next (item);

     // We do not display meta links nor nodes to remove from
     // bookmarks */
    if (prop->link_type == LINK_TYPE_META || prop->to_delete)
      continue;

    // Do not display empty bookmarks folder
    if (prop->link_type == LINK_TYPE_BOOKMARK_FOLDER &&
        prop->childs == NULL)
      continue;

    if (!display_all)
    {
      if (prop->path != NULL)
      {
        // Handle STATUS filters
        if (!UC_DISPLAY_STATUS_IS_NONE)
        {
          const gchar *status = g_hash_table_lookup (prop->header,
                                                     UC_HEADER_STATUS);
     
  
          display = status != NULL && 
            ((UC_DISPLAY_STATUS_IS (UC_DISPLAY_STATUS_BADLINKS) &&
              uc_check_status_is_bad (status))
  
            || (UC_DISPLAY_STATUS_IS (UC_DISPLAY_STATUS_GOODLINKS) &&
                uc_check_status_is_good (status))
    
            || (UC_DISPLAY_STATUS_IS (UC_DISPLAY_STATUS_MALFORMEDLINKS) &&
                uc_check_status_is_malformed (status))
    
            || (UC_DISPLAY_STATUS_IS (UC_DISPLAY_STATUS_TIMEOUTS) &&
                uc_check_status_is_timeout (status))
    
            || (UC_DISPLAY_STATUS_IS (UC_DISPLAY_STATUS_SECURITY_ALERTS) &&
                uc_check_is_security_alert (prop))
    
            || (UC_DISPLAY_STATUS_IS (UC_DISPLAY_STATUS_W3C_ALERTS) &&
                uc_check_is_w3c_alert (prop)));
        }
        // Handle TYPE filters
        else if (!UC_DISPLAY_TYPE_IS_NONE)
        {
          UCLinkType type = prop->link_type;
  
  
          display =  
            ((UC_DISPLAY_TYPE_IS (UC_DISPLAY_TYPE_HREF) &&
              type == LINK_TYPE_HREF)
  
            || (UC_DISPLAY_TYPE_IS (UC_DISPLAY_TYPE_IMAGE) &&
                type == LINK_TYPE_IMAGE)
  
            || (UC_DISPLAY_TYPE_IS (UC_DISPLAY_TYPE_FRAME) &&
                type == LINK_TYPE_FRAME)
  
            || (UC_DISPLAY_TYPE_IS (UC_DISPLAY_TYPE_EMAIL) &&
                type == LINK_TYPE_EMAIL)
  
            || (UC_DISPLAY_TYPE_IS (UC_DISPLAY_TYPE_CSS) &&
                type == LINK_TYPE_CSS));
        }
        // Handle PROTOCOLS filters
        else if (!UC_DISPLAY_PROTO_IS_NONE)
        {
          const gchar *protocol = prop->protocol;
  
  
          display =  
            ((UC_DISPLAY_PROTO_IS (UC_DISPLAY_PROTO_HTTP) &&
              strcmp (protocol, UC_PROTOCOL_HTTP) == 0)
  
            || (UC_DISPLAY_PROTO_IS (UC_DISPLAY_PROTO_FILE) &&
                strcmp (protocol, UC_PROTOCOL_FILE) == 0)

            || (UC_DISPLAY_PROTO_IS (UC_DISPLAY_PROTO_HTTPS) &&
                strcmp (protocol, UC_PROTOCOL_HTTPS) == 0)
  
          || (UC_DISPLAY_PROTO_IS (UC_DISPLAY_PROTO_FTP) &&
                strcmp (protocol, UC_PROTOCOL_FTP) == 0));
        }
      }
    }
    else
    {
      display = TRUE;

      // Here we build the treeview path to automatically select
      // a row according to the search result selection
      g_free (prop->treeview_path), prop->treeview_path = NULL;
      prop->treeview_path = (prop->depth_level == 0) ?
          g_strdup_printf ("%u", pos) : g_strdup_printf ("%s:%u", path, pos);
    }

    if (display)
    {
      lm_date = g_hash_table_lookup (prop->header, UC_LAST_MODIFIED);
      if (lm_date == NULL)
        lm_date = "-";

      action = uc_application_get_urls_user_action (prop->user_action);
      label = uc_utils_string_format4display (prop->label,
                                              UC_LABEL_DISPLAY_MAX_LEN);
      value = uc_utils_string_format4display (prop->link_value,
                                              UC_LABEL_DISPLAY_MAX_LEN);

      if (display_all)
        gtk_tree_store_append (treestore, &c_iter,
          (prop->depth_level) ? &prop->parent->iter : NULL);
      else
        gtk_tree_store_append (treestore, &c_iter, NULL);

      gtk_tree_store_set (treestore, &c_iter,
        UID_COLUMN, prop->uid,
        LINK_STATUS_ICON_COLUMN, prop->status_icon,
        LINK_ICON_COLUMN, prop->link_icon,
        SECURITY_ALERT_ICON_COLUMN,
          (uc_check_is_security_alert (prop))?security_alert_icon:empty_icon,
        W3C_ALERT_ICON_COLUMN,
          (uc_check_is_w3c_alert (prop))?w3c_alert_icon:empty_icon,
        ACTION_COLUMN, action->label,
        LABEL_COLUMN, label,
        URL_COLUMN, value,
        LM_COLUMN, lm_date, -1);

      g_free (value), value = NULL;
      g_free (label), label = NULL;

      if (display_all)
        prop->iter = c_iter;
    }

    if (prop->childs != NULL)
      uc_check_display_list (prop->childs, prop->treeview_path, c_iter);

    if (display_all)
      pos++;
  }
}


/**
 * uc_check_treeview_get_selected_row_id:
 * 
 * Get the current selected row in the main tree view.
 * 
 * Returns: The id of the current selected row in the url
 * treeview.
 */
gint
uc_check_treeview_get_selected_row_id (void)
{
  GtkTreeIter iter;
  GtkTreeModel *model = NULL;
  GtkTreeSelection *select = NULL;
  gint32 ret = -1;

  if (treeview != NULL && GTK_IS_TREE_VIEW (treeview) &&
      !uc_lists_checked_links_is_empty ())
    {
      select = gtk_tree_view_get_selection (treeview);

      if (select != NULL &&
	  gtk_tree_selection_get_selected (select, &model, &iter))
	gtk_tree_model_get (model, &iter, 0, &ret, -1);
    }

  return ret;
}

/**
 * uc_check_url_get_content:
 * @prop: #UCLinkProperties node of a link.
 * @current_path: current path.
 * @current_port: current port.
 * @current_args: current arguments.
 * @current_host: current hostname.
 *
 * See: uc_check_url_get_header(),
 *      uc_check_url_build_header(),
 *      uc_server_get_response()
 * 
 * Returns: The page content of a given link.
 */
gchar *
uc_check_url_get_content (UCLinkProperties * prop,
			  const gchar * current_path,
			  const gchar * current_port,
			  const gchar * current_args, gchar * current_host)
{
  gchar *hostname = NULL;
  gchar *referer = NULL;
  gchar *request = NULL;
  gchar *cookies = NULL;
  GArray *response = NULL;
  gchar *path = NULL;
  gchar *ret = NULL;
  gboolean is_local = FALSE;

  uc_application_display_state_message (UC_CHECK_MESSAGE_LABEL_THIRD,
					_("Retrieving source..."));
  UC_UPDATE_UI;

  hostname = (strlen (prop->h_name) ? prop->h_name : current_host);
  is_local = !strcmp (prop->h_name, uc_project_get_current_host ());
  referer = uc_check_build_referer_field ();
  cookies = (is_local && uc_project_get_cookies_accept ())?
    uc_cookies_get_header_field (prop->path) : g_strdup ("");

  /* build request */
  request = (uc_project_get_use_proxy ())?
    g_strdup_printf ("GET %s://%s%s%s HTTP/1.0\r\n"
		     "User-Agent: %s\r\n"
		     "Host: %s\r\n"
		     "%s"
		     "%s"
		     "Accept: */*\r\n\r\n",
		     prop->protocol,
		     hostname, prop->path, prop->args, UC_USER_AGENT,
		     hostname,
		     (uc_project_get_auth_line ())?
		     uc_project_get_auth_line () : "",
		     referer) : g_strdup_printf ("GET %s%s HTTP/1.0\r\n"
						 "User-Agent: %s\r\n"
						 "Host: %s\r\n"
						 "%s"
						 "%s"
						 "%s"
						 "Accept: */*\r\n\r\n",
						 prop->path, prop->args,
						 UC_USER_AGENT, hostname,
						 (uc_project_get_auth_line
						  ())?
						 uc_project_get_auth_line () :
						 "", cookies, referer);
  g_free (cookies), cookies = NULL;
  g_free (referer), referer = NULL;

  /* get HTTP response */
  response =
    uc_server_get_response (prop->protocol, hostname, atoi (prop->port),
			    request, FALSE, NULL);
  g_free (request), request = NULL;

  /* exit if no result */
  if (response == NULL)
    return NULL;

  /* save content in cache */
  if (response->data != NULL)
    {
      if (prop->is_downloadable)
	{
	  FILE *fd;
	  const gchar *begin = NULL;
	  gchar *m = NULL;

	  path = uc_utils_convert_uid2file (prop->uid);
	  fd = fopen (path, "wb");
	  g_free (path), path = NULL;

	  begin = m = g_malloc0 (response->len + 1);
	  memcpy (m, response->data, response->len);

	  begin += prop->header_size + 4;
	  fwrite (begin, 1, response->len - prop->header_size - 4, fd);
	  fclose (fd);

	  ret = g_strdup (begin);

	  g_free (m), m = NULL;
	}

      /* if no header length has been specify in HTTP header,
       * set it */
      if (g_hash_table_lookup (prop->header, UC_CONTENT_LENGTH) == NULL)
	g_hash_table_replace (prop->header, g_strdup (UC_CONTENT_LENGTH),
			      g_strdup_printf ("%u",
					       response->len -
					       prop->header_size));
    }

  g_array_free (response, TRUE), response = NULL;

  /* return page content */
  return ret;
}

/**
 * uc_check_url_build_header:
 * @status_code: status code to insert in the header to build.
 *
 * Build a HTTP header for some gURLChecker internals types.
 *
 * See: %UC_STATUS_CODE_IGNORED_LINK,
 *      %UC_STATUS_CODE_MALFORMED_LINK,
 *      %UC_STATUS_CODE_TIMEOUT
 * 
 * Returns: A HTTP header in a #GHashTable.
 */
static GHashTable *
uc_check_url_build_header (const gchar * status_code)
{
  GHashTable *tmp = NULL;

  tmp = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  g_hash_table_replace (tmp, g_strdup (UC_HEADER_STATUS),
			g_strdup (status_code));
  g_hash_table_replace (tmp, g_strdup (UC_CONTENT_TYPE),
			g_strdup (UC_CONTENT_TYPE_HTML));

  return tmp;
}

/**
 * uc_check_suspend_continue:
 *
 * Wait until the check can be alive again.
 */
void
uc_check_suspend_continue (void)
{
  GtkWidget *widget = NULL;

  uc_check_suspend_value = (!uc_check_suspend_get_value ());

  widget = WGET ("bt_suspend_progress_dialog");

  if (uc_check_suspend_get_value ())
    {
      WSENS ("bt_ignore_progress_dialog", FALSE);
      gtk_button_set_label (GTK_BUTTON (widget), _("Continue"));
      uc_application_progress_dialog_set_modal (FALSE);
    }
  else
    {
      WSENS ("bt_ignore_progress_dialog", TRUE);
      gtk_button_set_label (GTK_BUTTON (widget), _("Suspend"));
      uc_application_progress_dialog_set_modal (TRUE);
    }

  while (uc_check_suspend_get_value () && !uc_check_cancel_get_value ())
    {
      UC_UPDATE_UI;
      g_usleep (500);
    }
}

/**
 * uc_check_url_get_header:
 * @prop: #UCLinkProperties node of a link.
 *
 * See: uc_check_url_get_content(),
 *      uc_check_url_build_header(),
 *      uc_server_get_response()
 * 
 * Returns: The HTTP header of a link in a #GHashTable.
 */
GHashTable *
uc_check_url_get_header (UCLinkProperties * prop)
{
  GHashTable *header = NULL;
  gchar **lines = NULL;
  gchar *referer = NULL;
  gchar *cookies = NULL;
  gchar *request = NULL;
  GArray *array = NULL;
  gchar *response = NULL;
  gchar *uri = NULL;
  gchar str[UC_HEADER_STATUS_CODE_LEN + 1] = { 0 };
  guint i = 0;
  gboolean is_local = FALSE;

  is_local = !strcmp (prop->h_name, uc_project_get_current_host ());

  if (!strcmp (prop->protocol, UC_PROTOCOL_FTP))
    {
      uc_application_display_state_message
	(UC_CHECK_MESSAGE_LABEL_THIRD, _("Checking file (FTP)..."));

      /* request = path */
      request = g_strdup (prop->path);
    }
  else
    {
      if (prop->link_type == LINK_TYPES_SECURITY_TEST_BAD_EXTENSIONS)
	;
      else
	uc_application_display_state_message
	  (UC_CHECK_MESSAGE_LABEL_THIRD, _("Retrieving header..."));

      UC_UPDATE_UI;

      referer = uc_check_build_referer_field ();
      cookies = (is_local && uc_project_get_cookies_accept ())?
	uc_cookies_get_header_field (prop->path) : g_strdup ("");

      if (uc_project_get_use_proxy ())
        uri = g_strdup_printf ("%s://%s%s%s", 
                               prop->protocol, prop->h_name, 
                               prop->path, prop->args);
      else
        uri = g_strdup_printf ("%s%s", prop->path, prop->args);

      /* build request */
      request = g_strdup_printf (
        "GET %s HTTP/1.0\r\n"
        "User-Agent: %s\r\n"
        "Host: %s\r\n"
        "%s"
        "%s"
        "%s"
        "Accept: */*\r\n\r\n",
        uri,
        UC_USER_AGENT, prop->h_name,
        (uc_project_get_auth_line ())?uc_project_get_auth_line ():"",
        cookies,
        referer);

      g_free (uri), uri = NULL;
      g_free (cookies), cookies = NULL;
      g_free (referer), referer = NULL;
    }

  /* get response */
  array =
    uc_server_get_response (prop->protocol, prop->h_name, atoi (prop->port),
			    request, TRUE, &prop->ip_addr);
  g_free (request), request = NULL;

  /* exit if no result */
  if (array == NULL || array->data == NULL)
    {
      if (array)
	g_array_free (array, TRUE), array = NULL;
      return NULL;
    }

  response = g_new0 (gchar, array->len + 1);
  memcpy (response, array->data, array->len);
  response[array->len] = '\0';

  /* create hash table for header fields */
  header = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

  if (!strcmp (prop->protocol, UC_PROTOCOL_FTP))
    {
      g_hash_table_replace (header, g_strdup (UC_HEADER_STATUS),
			    g_strdup (response));
      g_hash_table_replace (header, g_strdup (UC_CONTENT_TYPE),
			    g_strdup (UC_CONTENT_TYPE_FTP));
    }
  else
    {
      /* get HTTP header code */
      memcpy (&str, &response[UC_HEADER_STATUS_CODE_FIRST_POS],
	      UC_HEADER_STATUS_CODE_LEN);
      g_hash_table_replace (header, g_strdup (UC_HEADER_STATUS),
			    g_strdup (str));

      /* fill header size */
      prop->header_size = strlen (response);

      /* get HTTP header fields */
      i = 0;
      lines = g_strsplit (response, "\r\n", 0);
      while (lines[i] != NULL && strchr (lines[i], '<') == NULL)
	{
	  gchar **key_value = g_strsplit (lines[i], ":", 2);
	  gchar *cookie_field = NULL;

	  if (key_value[0] != NULL && key_value[1] != NULL)
	    {
	      g_strstrip (key_value[0]);
	      g_strstrip (key_value[1]);

	      /* Status header field has already been set */
	      if (strcasecmp (key_value[0], UC_HEADER_STATUS) != 0)
		{
		  g_hash_table_replace (header,
					cookie_field =
					g_utf8_strdown (key_value[0], -1),
					g_strdup (key_value[1]));

		  /* If this is cookie field, register the cookie */
		  if (is_local && !uc_check_cancel_get_value () &&
		      uc_project_get_cookies_accept () &&
		      !strcmp (cookie_field, UC_SET_COOKIE))
		    uc_cookies_add (prop, key_value[1]);
		}
	    }

	  g_strfreev (key_value), key_value = NULL;
	  i++;
	}
      g_strfreev (lines), lines = NULL;
    }

  g_array_free (array, TRUE), array = NULL;
  g_free (response), response = NULL;

  return header;
}

/**
 * uc_check_link_already_checked:
 * @url: the url to check.
 *
 * Check if a given url already has been checked.
 * 
 * See: uc_check_link_already_checked_with_insert(),
 *      uc_check_register_link(),
 *      uc_lists_already_checked_links_append()    
 * 
 * Returns: The #UCLinkProperties node of the url if it has been already 
 * checked, or %NULL.
 */
static UCLinkProperties *
uc_check_link_already_checked (const gchar * url)
{
  return (UCLinkProperties *) uc_lists_checked_links_lookup (url);
}

/**
 * uc_check_link_already_checked_with_insert:
 * @prop: the #UCLinkPropertie node of the link to insert.
 * @url: the url to insert.
 *
 * Check if a given url already has been checked. If not, it insert it in
 * the already checked urls list.
 * 
 * See: uc_check_link_already_checked-),
 *      uc_check_register_link(),
 *      uc_lists_already_checked_links_append()    
 * 
 * Returns: %TRUE if the given url was already been checked.
 */
gboolean
uc_check_link_already_checked_with_insert (UCLinkProperties * prop,
					   gchar * url)
{
  UCLinkProperties *prop_tmp = NULL;
  gboolean already_checked = FALSE;

  prop_tmp = uc_check_link_already_checked (url);

  /* if we are checking bookmarks, do not check for already
   * checked links */
  if (uc_project_get_check_is_bookmarks ())
    return FALSE;

  already_checked = (prop_tmp != NULL);

  if (already_checked &&
      !uc_lists_similar_links_exist (prop_tmp->similar_links_parents,
				     prop->url))
    uc_lists_similar_links_append ((gpointer) prop_tmp, prop->url);

  if (already_checked)
    {
      uc_application_display_state_message (UC_CHECK_MESSAGE_LABEL_THIRD,
					    _("Item already checked..."));
      UC_UPDATE_UI;

      uc_utils_debug ("        (already checked) %s\n", (url)?url:"");
    }

  return already_checked;
}

/**
 * uc_check_register_link:
 * @normalized_url: the url of the link.
 * @lp: the #UCLinkProperties of the link.
 * 
 * Register a link in the checked urls list and update the check report.
 *
 * Returns: a %gpointer on the new item.
 */
gpointer
uc_check_register_link (const gchar * normalized_url, UCLinkProperties * lp)
{
  gchar *status = NULL;

  /* append link to the already checked temporary 
   * list */
  if (uc_lists_already_checked_links_append (normalized_url, (gpointer) lp))
    if (uc_project_get_check_is_main ())
      uc_check_display_list_current_item (lp);

  /* we do not register bookmark's folders */
  if (lp->link_type != LINK_TYPE_BOOKMARK_FOLDER &&
      !uc_check_refresh_link_get_value ())
    {
      status = g_hash_table_lookup (lp->header, UC_HEADER_STATUS);

      /* timeouts */
      if (uc_check_status_is_timeout (status))
	uc_report_set_timedoutlinks (-1);

      /* good links */
      else if (uc_check_status_is_good (status))
	uc_report_set_goodlinks (-1);

      /* bad links */
      else if (uc_check_status_is_bad (status))
	uc_report_set_badlinks (-1);

      /* display debug informations if requested */
      uc_utils_debug ("        (%s) %s\n",
        (status)?status:"", (normalized_url)?normalized_url:"");
    }

  uc_utils_debug_dump_link_properties (lp);

  return (gpointer) lp;
}

/**
 * uc_check_mx_is_valid:
 * 
 * Test if the MX of the selected E-Mail link is valid.
 * launch a popup to inform the user of its state
 */
void
uc_check_mx_is_valid (void)
{
  UCLinkProperties *prop = NULL;
  gchar *mx = NULL;
  gint32 id = 0;

  id = uc_check_treeview_get_selected_row_id ();
  if (id < 0)
    {
      uc_application_dialog_show (_
				  ("Please, select a item."),
				  GTK_MESSAGE_WARNING);
      return;
    }

  prop = uc_lists_checked_links_lookup_by_uid (id);

  if (prop->domain != NULL && strlen (prop->domain) > 0)
    {
      gchar *msg = NULL;

      mx = uc_utils_get_mx (prop->domain);
      if (uc_utils_mx_is_valid (mx))
	{
	  msg = g_strdup_printf (_("MX host %s for this "
				   "domain is alive and listen on "
				   "port 25."), mx);
	  uc_application_dialog_show (msg, GTK_MESSAGE_INFO);
	}
      else
	uc_application_dialog_show (_("MX host does not respond or "
				      "the E-Mail's domain is not valid."),
				    GTK_MESSAGE_WARNING);
      g_free (mx), mx = NULL;
    }
  else
    uc_application_dialog_show (_("The E-Mail's domain is not valid."),
				GTK_MESSAGE_WARNING);
}

/**
 * uc_check_directory_accepted:
 * @path: the url to check.
 * 
 * Check if a given directory should be filtered or not.
 * 
 * Returns: TRUE is there is no filter for the given directory.
 */
static gboolean
uc_check_directory_accepted (const gchar * path)
{
  gchar **items = NULL;
  gchar *tmp = NULL;
  guint size = 0;
  guint i = 0;
  gboolean found = FALSE;

  /* root can not be filtered */
  if (!strcmp (path, "/"))
    return TRUE;

  tmp = (path[strlen (path) - 1] != '/') ?
    g_strconcat (path, "/", NULL) : g_strdup (path);

  uc_project_get_reject_directories (&items, &size);
  while (i < size && !(found = (strstr (tmp, items[i++]) != NULL)));

  g_free (tmp), tmp = NULL;

  return (!found);
}

/*
 * return TRUE is the domain there is no filter for 
 * the given hostname
 */
static gboolean
uc_check_domain_accepted (gchar * domain)
{
  gchar **items = NULL;
  guint size = 0;
  guint i = 0;
  gchar *tmp = NULL;
  gboolean found = FALSE;

  /* checked host is not filtered */
  if (!strcmp (uc_project_get_current_host (), domain))
    return TRUE;

  /* extract the domain of the complete hostname */
  tmp = (strchr (domain, '.') != strrchr (domain, '.')) ?
    strchr (domain, '.') + 1 : domain;

  uc_project_get_reject_domains (&items, &size);
  while (i < size && !(found = (!strcmp (tmp, items[i++]))));

  return (!found);
}


/*
 * return TRUE if the given path is the check has been
 * restricted to the project directory and its subdirectories and
 * if the given path is in the current or in the subdirectories
 */
static gboolean
uc_check_chroot_authorized (const gchar * host, const gchar * proto,
			    const gchar * path)
{
  gboolean ret = TRUE;
  gboolean same_host = FALSE;
  gboolean same_path = FALSE;


  same_host = uc_utils_memcasecmp (host, uc_project_get_current_host ());
  same_path = uc_utils_memcasecmp (path, uc_project_get_chroot_path ());

  if (!uc_project_get_limit_local ())
  {
    if (uc_project_get_check_chroot () && same_host)
      ret = same_path;
  }
  else if (uc_project_get_type () == UC_PROJECT_TYPE_LOCAL_FILE &&
	   strcmp (proto, UC_PROTOCOL_FILE) != 0)
    ret = FALSE;
  else if (!uc_project_get_check_chroot ())
    ret = same_host;
  else
    ret = (same_host && same_path);

  return ret;
}


/*
 * check if the given url is ok with
 * the filters
 */
static gboolean
uc_check_apply_filters (const UCLinkProperties * prop)
{
  gchar *content_type = NULL;

  return (prop != NULL &&
	  prop->header != NULL &&
	  (!strcmp (prop->protocol, UC_PROTOCOL_MAILTO) ||
	   ((content_type =
	     g_hash_table_lookup (prop->header, UC_CONTENT_TYPE)) != NULL &&
	    uc_check_content_type_accepted (prop->path, content_type))) &&
	  uc_check_directory_accepted (prop->url) &&
	  uc_check_domain_accepted (prop->h_name));
}

/*
 * set it to TRUE if user is rescanning a link to refresh its
 * status
 */
void
uc_check_refresh_link_set_value (const gboolean value)
{
  uc_check_refresh_link_value = value;
}

/*
 * return TRUE if user is rescanning a link to refresh its
 * status
 */
gboolean
uc_check_refresh_link_get_value (void)
{
  return uc_check_refresh_link_value;
}

/*
 * set the cancel check flag.
 * this flag is tested during the check to know
 * if the check must aborting or continuing
 */
void
uc_check_cancel_set_value (const gboolean value)
{
  uc_check_cancel_value = value;
}

/*
 * return TRUE is the check is to suspend and
 * FALSE otherwise
 */
gboolean
uc_check_suspend_get_value (void)
{
  return uc_check_suspend_value;
}

/*
 * return TRUE is the check is to cancel and
 * FALSE otherwise
 */
gboolean
uc_check_cancel_get_value (void)
{
  return uc_check_cancel_value;
}

/*
 * return TRUE is the current item must be
 * ignored
 */
gboolean
uc_check_ignore_item_get_value (void)
{
  return uc_check_ignore_item_value;
}


/*
 * set ignore item value 
 */
void
uc_check_ignore_item_set_value (const gboolean value)
{
  uc_check_ignore_item_value = value;
}


/*
 * reset all items in the right frames
 */
static void
uc_check_right_frames_reset (void)
{
  GtkWidget *w = NULL;


  w = WGET ("mw_label_informations_message");
  if (w != NULL)
    gtk_label_set_text (GTK_LABEL (w), "");

  w = WGET ("mw_text_informations");
  if (w != NULL)
    gtk_text_buffer_set_text (gtk_text_view_get_buffer
			      (GTK_TEXT_VIEW (w)), "", -1);
  w = WGET ("mw_location");
  if (w != NULL)
    gtk_entry_set_text (GTK_ENTRY (w), "");

  w = WGET ("mw_referrer");
  if (w != NULL)
    gtk_entry_set_text (GTK_ENTRY (w), "");

  uc_check_report_reset (-1);
}


/*
 * this function must be called before
 * any check, to initialize some elements
 */
void
uc_check_reset (void)
{
  gchar *title = NULL;


  uc_application_set_status_bar (0, _("Cleaning..."));
  UC_UPDATE_UI;

  if (treestore != NULL)
  {
    g_object_unref (G_OBJECT (treestore)), treestore = NULL;
    treeview = NULL;
  }

  uc_lists_refresh_preserved_links_reset ();
  uc_timeout_domains_free ();
  uc_application_remove_paths ();
  uc_application_make_paths ();
  uc_lists_checked_links_free ();
  uc_lists_already_checked_free ();
  uc_search_free ();
  uc_check_right_frames_reset ();
  if (uc_project_get_check_is_bookmarks ())
    uc_bookmarks_free ();

  uc_check_node_uid = 1;
  uc_refresh_page = FALSE;
  uc_check_suspend_value = FALSE;
  uc_project_set_save (FALSE);
  uc_check_cancel_set_value (FALSE);
  uc_check_ignore_item_set_value (FALSE);
  uc_application_set_status_bar (0, "");

  title = g_strconcat ("gURLChecker ", UC_VERSION, NULL);
  gtk_window_set_title (GTK_WINDOW (WGET ("main_window")), title);
  g_free (title), title = NULL;
}


/*
 * return TRUE if the given content-type is parsable
 * (if it could contain links)
 */
static gboolean
uc_check_content_type_parsable (const gchar * content_type)
{
  return (content_type != NULL && (strstr (content_type, "htm") != NULL ||
				   strstr (content_type, "php") != NULL ||
				   strstr (content_type, "css") != NULL));
}

/*
 * return TRUE if the given content-type is dowloadable
 */
static gboolean
uc_check_content_type_downloadable (gchar * content_type)
{
  return (content_type != NULL &&
	  ((uc_project_get_download_images_content () &&
	    strstr (content_type, "image") != NULL)
	   ||
	   (!uc_check_content_type_parsable (content_type) &&
	    (uc_project_get_download_archives_content () ||
	     ((uc_project_get_security_checks ("any") &&
	       uc_project_get_security_checks ("virii")) &&
	      uc_check_content_type_virii (content_type))))));
}


/*
 * return TRUE if the given content-type is authorized
 * to be checked.
 * At first we rely on the extension. If it there is no extension, then
 * look at the mime type.
 */
static gboolean
uc_check_content_type_accepted (gchar * path, gchar * content_type)
{
  gboolean accepted = TRUE;
  gchar *src = NULL;
  gchar *dest_doc = NULL;
  gchar *dest_img = NULL;
  gchar *p = NULL;


  if (path == NULL && content_type == NULL)
    return FALSE;

  // Guess extension
  if ((src = strrchr (path, '.')) == NULL)
  {
    if (content_type && ((src = strrchr (content_type, '/')) != NULL))
    {
      if ((p = strchr (src, ';')))
        *p = '\0';
    }
    else
      return FALSE;
  }
  ++src;

  // Prepare strings to be compared
  // Hacky, but simple
  src = g_strdup_printf (",%s,", src);
  dest_doc = g_strdup_printf (",%s,", uc_project_get_reject_documents ());
  dest_img = g_strdup_printf (",%s,", uc_project_get_reject_images ());

  accepted = (strstr (dest_doc, src) == NULL && strstr (dest_img, src) == NULL);

  g_free (src), src = NULL;
  g_free (dest_doc), dest_doc = NULL;
  g_free (dest_img), dest_img = NULL;

  return accepted;
}


static gboolean
uc_check_content_type_bad_extensions (const UCLinkType type)
{
  return (type != LINK_TYPE_EMAIL &&
	  type != LINK_TYPE_IMAGE_FTP &&
	  type != LINK_TYPE_HREF_FTP &&
	  !(uc_project_get_security_checks ("exclude_images") &&
	    (type == LINK_TYPE_IMAGE ||
	     type == LINK_TYPE_FILE_IMAGE || type == LINK_TYPE_IMAGE_SSL)));
}


static gboolean
uc_check_content_type_virii (gchar * content_type)
{
  gboolean accepted = TRUE;
  gchar *src = content_type;
  gchar *dest = NULL;
  gchar *p = NULL;


  if (content_type == NULL)
    return FALSE;

  // Guess extension
  if (src && ((p = strchr (content_type, '/')) != NULL))
      *p = '\0';
  else
    return FALSE;

  // Prepare strings to be compared
  // Hacky, but simple
  src = g_strdup_printf (",%s,", src);
  dest = g_strdup_printf (",%s,", uc_project_get_security_virii_extensions ());
  accepted = (strstr (dest, src) != NULL);

  g_free (src), src = NULL;
  g_free (dest), dest = NULL;

  return accepted;
}


gboolean
uc_check_content_type_w3c_accepted (const gchar * type, gchar * path,
				    gchar * content_type)
{
  gboolean accepted = TRUE;
  gchar *src = NULL;
  gchar *dest = NULL;
  gchar *p = NULL;


  if (path == NULL && content_type == NULL)
    return FALSE;

  // Guess extension
  if ((src = strrchr (path, '.')) == NULL)
  {
    if (content_type && ((src = strrchr (content_type, '/')) != NULL))
    {
      if ((p = strchr (src, ';')))
        *p = '\0';
    }
    else
      return FALSE;
  }
  ++src;

  // Prepare strings to be compared
  // Hacky, but simple
  src = g_strdup_printf (",%s,", src);

  if (strcmp (type, "html") == 0)
    dest = g_strdup_printf (",%s,", uc_project_get_w3c_html_extensions ());
  else if (strcmp (type, "css") == 0)
    dest = g_strdup_printf (",%s,", uc_project_get_w3c_css_extensions ());
  else
    g_assert_not_reached ();

  accepted = (strstr (dest, src) != NULL);

  g_free (src), src = NULL;
  g_free (dest), dest = NULL;

  return accepted;
}


static gboolean
uc_check_get_virii (UCLinkProperties * prop)
{
  gboolean ret = FALSE;
#ifdef ENABLE_CLAMAV
  UCClam *clam = NULL;
  gchar *path = NULL;


  uc_application_display_state_message (
    UC_CHECK_MESSAGE_LABEL_THIRD, _("<b>SECURITY</b>: Checking for virii..."));
  UC_UPDATE_UI;

  path = uc_utils_convert_uid2file (prop->uid);

  if ((clam = uc_ucclam_new (NULL)))
    {
      ret = uc_ucclam_scan (clam, path, &prop->virname);
      uc_ucclam_free (&clam);
    }

  g_free (path), path = NULL;
#endif

  return ret;
}


static gboolean
uc_check_get_w3c_css_validate (UCLinkProperties * prop)
{
  gboolean ret = TRUE;
#ifdef ENABLE_CROCO
  UCCroco *croco = NULL;
  gchar *path = NULL;
  gchar *buffer = NULL;
  gsize size = 0;


  uc_application_display_state_message (
    UC_CHECK_MESSAGE_LABEL_THIRD, _("<b>W3C</b>: Validate CSS stylesheet..."));
  UC_UPDATE_UI;

  path = uc_utils_convert_uid2file (prop->uid);
  buffer = uc_utils_get_file_content (path, &size);

  if ((croco = uc_uccroco_new ()))
    {
      ret = uc_uccroco_validate (croco, buffer);
      uc_uccroco_free (&croco);
    }

  g_free (buffer), buffer = NULL;
  g_free (path), path = NULL;
#endif

  return ret;
}


static gboolean
uc_check_get_w3c_html_validate (UCLinkProperties * prop)
{
  gboolean ret = FALSE;
#ifdef ENABLE_TIDY
  UCTidy *tidy = NULL;
  gchar *path = NULL;
  gchar *buffer = NULL;
  gsize size = 0;


  uc_application_display_state_message (
    UC_CHECK_MESSAGE_LABEL_THIRD, _("<b>W3C</b>: Validate HTML page..."));
  UC_UPDATE_UI;

  path = uc_utils_convert_uid2file (prop->uid);
  buffer = uc_utils_get_file_content (path, &size);

  if ((tidy = uc_uctidy_new ()))
    {
      ret = (uc_uctidy_validate (tidy, buffer) == 0);
      uc_uctidy_free (&tidy);
    }

  g_free (buffer), buffer = NULL;
  g_free (path), path = NULL;
#endif

  return ret;
}


/*
 * Check existence of bad extensions on the server for a given node. return a
 * list of existing extensions.
 */
static GList *
uc_check_get_bad_extensions (UCLinkProperties * prop)
{
  gchar **bad_extensions;
  guint bad_extensions_size = 0;
  guint i = 0;
  guint j = 0;
  gchar *status = NULL;
  UCLinkProperties *prop_tmp = NULL;
  gchar *extension = NULL;
  gboolean ignore_case = FALSE;
  gchar *tmp = NULL;


  if (prop->bad_extensions != NULL)
    {
      g_list_foreach (prop->bad_extensions, (GFunc) g_free, NULL);
      g_list_free (prop->bad_extensions), prop->bad_extensions = NULL;
    }

  if (prop->path == NULL ||
      prop->path[strlen (prop->path) - 1] == '/')
    return NULL;

    // * For website projects, check for bad extension only for links
    //   in main site pages.
    // * For bookmarks, dont' bother
    if (!uc_project_get_check_is_bookmarks () &&
      g_ascii_strcasecmp (prop->h_name, uc_project_get_current_host ()) != 0)
    return NULL;

  tmp = uc_project_get_security_bad_extensions ();
  bad_extensions = g_strsplit (tmp, ",", -1);
  bad_extensions_size = uc_utils_vector_length (bad_extensions);

  // Lower case/Upper case
  for (i = 0; i < 2; i++)
  {
    for (j = 0; j < bad_extensions_size; j++)
      {
        // There is no need for lower/upper check for non alpha characters
        ignore_case = uc_utils_str_is_not_alphanum (bad_extensions[j]);

        if (ignore_case && i == 1)
          continue;

	prop_tmp = uc_check_link_properties_node_new ();
	prop_tmp->link_type = LINK_TYPES_SECURITY_TEST_BAD_EXTENSIONS;
	prop_tmp->protocol = g_strdup (prop->protocol);
	prop_tmp->h_name = g_strdup (prop->h_name);
	prop_tmp->port = g_strdup (prop->port);
	extension = (i == 1)?
          g_ascii_strup (bad_extensions[j], -1):g_strdup (bad_extensions[j]);

        prop_tmp->path = g_strdup_printf ("%s%s%s",
          prop->path,
          (ignore_case ||
           extension[0] == ':' ||
           extension[0] == '%' ||
           extension[0] == '?')?"":".", extension);

	prop_tmp->args = g_strdup ("");

	tmp = g_strdup_printf (
          _("<b>SECURITY</b>: Checking for bad extensions [<b>%s</b>]..."),
          extension);
	uc_application_display_state_message (UC_CHECK_MESSAGE_LABEL_THIRD,
					      tmp);
	UC_UPDATE_UI;

	g_free (tmp), tmp = NULL;
	g_free (extension), extension = NULL;

	if ((prop_tmp->header = uc_check_url_get_header (prop_tmp)) &&
	    (status =
	     g_hash_table_lookup (prop_tmp->header, UC_HEADER_STATUS))
	    && uc_check_status_is_good (status))
	  {
	    if ((tmp = strrchr (prop_tmp->path, '/')) == NULL)
	      tmp = prop_tmp->path;
	    prop->bad_extensions =
	      g_list_prepend (prop->bad_extensions, g_strdup (tmp));

	    uc_utils_debug ("[SECURITY] Bad extension : %s\n", (tmp)?tmp:"");
	  }

        uc_lists_checked_links_node_free (NULL, &prop_tmp);
      }
    }

  g_strfreev (bad_extensions), bad_extensions = NULL;

  return prop->bad_extensions;
}


/*
 * Return TRUE if the given protocol is authorized
 * to be checked.
 */
static gboolean
uc_check_protocol_accepted (gchar * protocol)
{
  gchar **accepted = UC_CHECK_PROTOCOLS_GET_ACCEPTED;
  guint i = 0;
  gboolean found = FALSE;

  while (i < UC_CHECK_PROTOCOLS_ACCEPTED_LEN &&
	 !(found = (!strcmp (accepted[i++], protocol))));

  g_strfreev (accepted), accepted = NULL;

  /* FIXME */
  if (!strcmp (protocol, UC_PROTOCOL_MAILTO)
      && uc_project_get_proto_mailto ())
    found = TRUE;
  else if (!strcmp (protocol, UC_PROTOCOL_FTP)
	   && !uc_project_get_proto_ftp ())
    found = FALSE;
  else if (!strcmp (protocol, UC_PROTOCOL_HTTPS) &&
	   !uc_project_get_proto_https ())
    found = FALSE;
  else if (!strcmp (protocol, UC_PROTOCOL_FILE) &&
	   (uc_project_get_proto_file_is_error ()
	    || uc_project_get_proto_file_check ()))
    found = TRUE;

  return found;
}

/*
 * callback for uc_check_wait ()
 */
static gboolean
uc_check_wait_callback (gpointer data)
{
  uc_check_wait_value = FALSE;

  return TRUE;
}

/*
 * wait
 */
void
uc_check_wait (void)
{
  gchar *buffer = NULL;
  guint id = 0;

  if (!uc_project_get_check_wait ())
    return;

  buffer = g_strdup_printf (_("Sleeping %u second(s)..."),
			    uc_project_get_check_wait ());
  uc_application_display_state_message (UC_CHECK_MESSAGE_LABEL_THIRD, buffer);
  g_free (buffer), buffer = NULL;

  uc_check_wait_value = TRUE;
  id = g_timeout_add (uc_project_get_check_wait () * 1000,
		      &uc_check_wait_callback, NULL);

  /* sleeping a while... */
  while (uc_check_wait_value
	 && (!uc_check_cancel_get_value () &&
	     !uc_check_ignore_item_get_value ()))
    {
      UC_UPDATE_UI;
      g_usleep (500);
    }

  g_source_remove (id), id = 0;
}

static gchar *
uc_check_build_referer_field (void)
{
  return (currentItem.parent != NULL) ?
    g_strdup_printf ("Referer: %s\r\n", currentItem.parent->url) :
    g_strdup ("");
}

/*
 * init a curentItem structure
 */
void
uc_check_currentitem_init (UCLinkProperties * parent, gchar * current_host,
			   UCHTMLTag * tag, gchar * url, gboolean is_first)
{
  currentItem.is_first = is_first;
  currentItem.parent = parent;
  currentItem.current_host = current_host;
  currentItem.tag = tag;
  currentItem.url = url;
}


/*
 * make and return a basic UCLinkProperties node
 */
UCLinkProperties *
uc_check_link_properties_node_new (void)
{
  UCLinkProperties *prop = NULL;

  prop = g_new0 (UCLinkProperties, 1);
  prop->uid = uc_check_node_uid++;
  prop->treeview_path = NULL;
  prop->current_path = NULL;
  prop->tag = (gpointer) currentItem.tag;
  prop->parent = (currentItem.parent != NULL &&
		  !uc_check_is_main_page (currentItem.parent)) ?
    currentItem.parent : NULL;
  prop->link_type = LINK_TYPE_NONE;
  prop->link_icon = NULL;
  prop->status_icon = NULL;
  prop->user_action = NULL;
  prop->link_value = NULL;
  prop->url = NULL;
  prop->normalized_url = NULL;
  prop->label = NULL;
  prop->ip_addr = NULL;
  prop->h_name = NULL;
  prop->port = NULL;
  prop->path = NULL;
  prop->args = NULL;
  prop->domain = NULL;
  prop->header = NULL;
  prop->header_size = 0;
  prop->depth_level = 0;
  prop->metas = NULL;
  prop->emails = NULL;
  prop->all_links = NULL;
  prop->childs = NULL;
  prop->similar_links_parents = NULL;
  prop->bad_extensions = NULL;
  prop->virii = FALSE;
  prop->virname = NULL;
  prop->w3c_valid = TRUE;
  prop->tags = NULL;
  prop->is_parsable = FALSE;
  prop->is_downloadable = FALSE;
  prop->xml_node = NULL;
#ifdef ENABLE_JSON
  prop->json_array = NULL;
#endif
  prop->bookmark_folder_id = 0;
  prop->bookmark_id = 0;
  prop->to_delete = FALSE;
  prop->checked = FALSE;

  return (gpointer) prop;
}

/*
 * process a MAILTO link
 */
static gboolean
uc_check_link_get_properties_proto_mailto (UCLinkProperties * prop)
{
  gchar *email = NULL;
  gchar *tmp = NULL;
  gchar *user = NULL;
  gchar *domain = NULL;

  uc_application_display_state_message (UC_CHECK_MESSAGE_LABEL_THIRD,
					_("Checking E-Mail..."));
  UC_UPDATE_UI;

  /* retrieve the host name of the E-Mail link */
  uc_utils_split_email (prop->path, &user, &domain);
  prop->domain = domain;
  prop->link_type = LINK_TYPE_EMAIL;
  prop->url = (strlen (user) > 0 || strlen (domain) > 0) ?
    g_strconcat (UC_PROTOCOL_MAILTO, ":", user, "@", domain, prop->args,
		 NULL) : g_strconcat (UC_PROTOCOL_MAILTO, ":", prop->args,
				      NULL);
  prop->header = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
					g_free);
  g_free (user), user = NULL;

  email = prop->url;

  /* If there is no E-Mail but a URL like "?subject" etc. , 
   * consider it is ok */
  if (uc_utils_memcasecmp (email, "mailto:?"))
    tmp = UC_STATUS_CODE_GOOD_EMAIL_LINK;
  /* Check the E-Mail */
  else
    switch (uc_utils_email_is_valid
	    (email, (gboolean) uc_project_get_proto_mailto_check_mx ()))
      {
      case UC_EMAIL_SYNTAX_BAD:
	tmp = UC_STATUS_CODE_BAD_EMAIL_LINK;
	break;

      case UC_EMAIL_SYNTAX_MX_BAD:
	tmp = UC_STATUS_CODE_BAD_EMAIL_LINK_MX;
	break;

      case UC_EMAIL_OK:
	tmp = UC_STATUS_CODE_GOOD_EMAIL_LINK;
	break;

	/* syntax and mx are bad */
      default:
	tmp = UC_STATUS_CODE_BAD_EMAIL_LINK;
      }

  g_hash_table_replace (prop->header, g_strdup (UC_HEADER_STATUS),
			g_strdup (tmp));

  return FALSE;
}


/*
 * process a FILE link
 */
static gboolean
uc_check_link_get_properties_proto_file (UCLinkProperties *prop,
                                         const gchar * url,
                                         const gchar * current_path,
                                         const gchar * current_host,
                                         const UCLinkType type)
{
  GHashTable *tmp = NULL;
  gchar *str = NULL;
  gsize length = 0;


  uc_application_display_state_message (UC_CHECK_MESSAGE_LABEL_THIRD,
					_("Checking local file..."));
  UC_UPDATE_UI;

  tmp = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

  if (uc_project_get_proto_file_is_error ())
    g_hash_table_replace (tmp, g_strdup (UC_HEADER_STATUS),
			  g_strdup (UC_STATUS_CODE_FILE_PROTO_ERROR));
  else
    {
      if (g_file_test (prop->path, G_FILE_TEST_EXISTS) &&
	  (str = uc_utils_get_file_content (prop->path, &length)))
	{
	  /* FIXME */
	  uc_cache_append_source (prop->uid, str, strlen (str));
	  g_hash_table_replace (
            tmp, g_strdup (UC_CONTENT_LENGTH),
            g_strdup_printf ("%lu", (unsigned long) strlen (str)));
	  g_hash_table_replace (tmp, g_strdup (UC_HEADER_STATUS),
				g_strdup (UC_STATUS_CODE_PAGE_OK));

	  if (prop->link_type != LINK_TYPE_FILE_IMAGE)
	    {
	      GList *item = NULL;

	      prop->tags =
		(prop->link_type == LINK_TYPE_CSS ||
		 prop->link_type == LINK_TYPE_CSS_SSL) ?
		uc_css_parser_get_tags (str) : uc_html_parser_get_tags (str);

	      item = g_list_first (prop->tags);
	      while (item != NULL)
		{
		  UCHTMLTag *tag = (UCHTMLTag *) item->data;
                  gchar *normalized_url = NULL;
                  UCLinkProperties *prop_tmp = NULL;


		  UC_UPDATE_UI;

		  item = g_list_next (item);
 
                  normalized_url = uc_url_normalize (current_host,
                                                     current_path, tag->value);

                  // Exclude already checked links
                  if ((prop_tmp = 
                         uc_check_link_already_checked (normalized_url)))
                  {
                    if (!uc_lists_similar_links_exist (
                          prop_tmp->similar_links_parents, normalized_url))
                    {
                      uc_lists_similar_links_append ((gpointer) prop_tmp,
                                                     normalized_url);
                    }

                    uc_lists_html_tags_node_free (&prop->tags, &tag);
                  }
                  else
                  {            
                    if (tag->type == LINK_TYPE_META)
                      prop->metas = g_list_append (prop->metas, (gpointer) tag);
                    else if (tag->type == LINK_TYPE_EMAIL)
                      prop->emails =
                        g_list_append (prop->emails, (gpointer) tag);
                    else if (tag->type != LINK_TYPE_NONE)
                      prop->all_links =
                        g_list_append (prop->all_links, (gpointer) tag);
                  }

                  g_free (normalized_url), normalized_url = NULL;
		}
	    }

	  g_free (str), str = NULL;
	}
      else
	g_hash_table_replace (tmp,
			      g_strdup (UC_HEADER_STATUS),
			      g_strdup (UC_STATUS_CODE_PAGE_NOT_FOUND));
    }

  g_hash_table_replace (tmp, g_strdup (UC_CONTENT_TYPE),
			g_strdup (UC_CONTENT_TYPE_HTML));

  prop->header = tmp;
  prop->domain = g_strdup ("");
  prop->url = g_strdup (url);
  prop->link_type = type;

  return FALSE;
}


void
uc_check_refresh_report (void)
{
  guint32 bad = 0;
  guint32 malformed = 0;
  guint32 good = 0;
  guint32 timeout = 0;
  guint32 ignore = 0;

  uc_application_set_status_bar (0, _("Updating report..."));

  uc_check_report_reset (0);
  uc_check_refresh_report_real (uc_lists_checked_links_get (),
				&bad, &malformed, &good, &timeout, &ignore);

  uc_report_set_badlinks (bad);
  uc_report_set_malformedlinks (malformed);
  uc_report_set_goodlinks (good);
  uc_report_set_timedoutlinks (timeout);
  uc_report_set_ignoredlinks (ignore);

  uc_report_display_update ();

  uc_application_set_status_bar (0, "");
}

static void
uc_check_refresh_report_real (GList * list, guint32 * bad,
			      guint32 * malformed, guint32 * good,
			      guint32 * timeout, guint32 * ignore)
{
  GList *item = NULL;
  gchar *status = NULL;

  item = g_list_first (list);
  while (item != NULL)
    {
      UCLinkProperties *prop = (UCLinkProperties *) item->data;

      item = g_list_next (item);

      /* we do not register bookmark's folders */
      if (prop->link_type != LINK_TYPE_BOOKMARK_FOLDER &&
	  !prop->to_delete && !uc_check_is_main_page (prop))
	{
	  status = g_hash_table_lookup (prop->header, UC_HEADER_STATUS);

	  /* timeouts */
	  if (uc_check_status_is_timeout (status))
	    (*timeout)++;

	  /* good links */
	  else if (uc_check_status_is_good (status))
	    (*good)++;

	  /* bad links */
	  else if (uc_check_status_is_bad (status))
	    (*bad)++;

	  /* malformed links */
	  else if (uc_check_status_is_malformed (status))
	    (*malformed)++;

	  else if (uc_check_status_is_ignored (status))
	    (*ignore)++;
	}

      if (prop->childs != NULL)
	uc_check_refresh_report_real (prop->childs,
				      &(*bad), &(*malformed), &(*good),
				      &(*timeout), &(*ignore));
    }
}

/*
 * process a HTTP link
 */
static UCHTTPCheckReturn
uc_check_link_get_properties_proto_http (UCLinkProperties * prop,
					 const gchar * url,
					 const gchar * current_path,
					 gchar * current_host,
					 UCHTMLTag * tag,
					 UCLinkProperties * old_prop)
{
  gchar *str = NULL;
  gchar *status_code = NULL;
  GList *item = NULL;
  gchar *content_type = NULL;
  gchar *path_args = NULL;
  gchar *tmp = NULL;

  if (strlen (prop->h_name) > 0)
    {
      tmp = g_strconcat (prop->path, prop->args, NULL);
      path_args = uc_utils_to_utf8 (tmp);
      g_free (tmp), tmp = NULL;

      /* Has a specific port been specified */
      prop->url = (!strcmp (prop->port, UC_URL_DEFAULT_PORT) ||
		   !strcmp (prop->port, UC_URL_DEFAULT_SSL_PORT)) ?
	g_strconcat (prop->protocol,
		     "://", prop->h_name, path_args, NULL) :
	g_strconcat (prop->protocol,
		     "://", prop->h_name, ":", prop->port, path_args, NULL);

      g_free (path_args), path_args = NULL;
    }
  /* malformed tag */
  else
    {
      if (prop->url == NULL)
	prop->url = g_strdup ("");
      return UC_HTTP_CHECK_RETURN_MALFORMED;
    }

  /* cancel if bad header */
  if ((prop->header = uc_check_url_get_header ((gpointer) prop)) == NULL)
    return UC_HTTP_CHECK_RETURN_TIMEOUT;

  /* if we are rescanning a link, test if it has changed.
   * if yes, do not go further */
  if (uc_check_refresh_link_get_value () && old_prop != NULL)
    {
      gchar *lm_date_old = NULL;
      gchar *lm_date_new = NULL;

      lm_date_old = g_hash_table_lookup (old_prop->header, UC_LAST_MODIFIED);
      lm_date_new = g_hash_table_lookup (prop->header, UC_LAST_MODIFIED);

      /* if lm dates are the same, keep this url/prop for reinjecting it
       * at the end of the update in the prop's childs */
      if (lm_date_old != NULL && lm_date_new != NULL &&
	  !strcmp (lm_date_old, lm_date_new))
	{
	  uc_lists_refresh_preserved_links_append (old_prop);
	  uc_lists_already_checked_links_append (old_prop->normalized_url,
						 old_prop);
	  return UC_HTTP_CHECK_RETURN_SAME;
	}
    }

  /* cancel if content type not accepted */
  content_type = g_hash_table_lookup (prop->header, UC_CONTENT_TYPE);
  if (!uc_check_content_type_accepted (prop->path, content_type))
    return UC_HTTP_CHECK_RETURN_BAD;

  prop->link_type = tag->type;
  prop->domain = g_strdup ("");

  status_code = g_hash_table_lookup (prop->header, UC_HEADER_STATUS);
  if (uc_check_status_is_good (status_code))
    {
      if (!uc_project_get_speed_check () ||
	  uc_project_speed_check_get_download_content ())
	{
	  /* if redirection, go on and download the 
	   * content */
	  if (status_code[0] != '3')
	    uc_project_speed_check_set_download_content (FALSE);

	  prop->is_parsable = uc_check_content_type_parsable (content_type);

	  prop->is_downloadable =
	    (prop->is_parsable ||
	     uc_check_content_type_downloadable (content_type));

	  if (uc_project_get_url () != NULL)
	    prop->is_parsable = (prop->is_parsable &&
				 g_ascii_strcasecmp (prop->h_name,
						     uc_project_get_current_host
						     ()) == 0);
	}
      else
	{
	  prop->is_parsable = FALSE;
	  prop->is_downloadable = FALSE;
	}
    }
  else
    return UC_HTTP_CHECK_RETURN_OK;

  if (uc_url_is_faked (prop, tag))
    return UC_HTTP_CHECK_RETURN_FAKED_URL;

  /* we don't want to download this link */
  if (!prop->is_downloadable)
    return UC_HTTP_CHECK_RETURN_OK;

  /* does this item is authorized? */
  if (!uc_check_apply_filters (prop))
    {
      uc_utils_debug ("        FILTERED: %s\n", (prop->url)?prop->url:"");
      return UC_HTTP_CHECK_RETURN_BAD;
    }

  /* If we have not retrieved the source, either something wrong
   * occured or the user has cancelled the check */
  if (strcmp (prop->protocol, UC_PROTOCOL_FTP)
      && (str =
	  uc_check_url_get_content (prop, current_path, prop->port,
				    prop->args, current_host)) == NULL)
    return UC_HTTP_CHECK_RETURN_TIMEOUT;

  if (prop->link_type != LINK_TYPE_IMAGE)
    {
      prop->tags =
	(prop->link_type == LINK_TYPE_CSS ||
	 prop->link_type == LINK_TYPE_CSS_SSL) ?
	uc_css_parser_get_tags (str) : uc_html_parser_get_tags (str);

      if (uc_check_ignore_item_get_value ())
	{
	  g_free (str), str = NULL;
	  return UC_HTTP_CHECK_RETURN_IGNORE;
	}

      item = g_list_first (prop->tags);
      while (item != NULL)
	{
	  UCHTMLTag *tag = (UCHTMLTag *) item->data;
          gchar *normalized_url = NULL;
          UCLinkProperties *prop_tmp = NULL;

	  UC_UPDATE_UI;

	  item = g_list_next (item);

          normalized_url = uc_url_normalize (current_host,
                                             current_path, tag->value);

          // Exclude already checked links
          if ((prop_tmp = uc_check_link_already_checked (normalized_url)))
          {
            if (!uc_lists_similar_links_exist (
              prop_tmp->similar_links_parents, normalized_url))
            {
              uc_lists_similar_links_append ((gpointer) prop_tmp,
                                             normalized_url);
            }

            uc_lists_html_tags_node_free (&prop->tags, &tag);
          }
          else
          {            
            if (tag->type == LINK_TYPE_META)
              prop->metas = g_list_append (prop->metas, (gpointer) tag);
            else if (tag->type == LINK_TYPE_EMAIL)
              prop->emails = g_list_append (prop->emails, (gpointer) tag);
            else if (tag->type != LINK_TYPE_NONE)
              prop->all_links = g_list_append (prop->all_links, (gpointer) tag);
          }

          g_free (normalized_url), normalized_url = NULL;
	}
    }

  g_free (str), str = NULL;

  return UC_HTTP_CHECK_RETURN_OK;
}


/*
 * fill the UCLinkProperties struct to associate with
 * the url
 */
UCLinkProperties *
uc_check_link_get_properties (const guint depth, gchar * current_host,
			      gchar * current_path,
			      UCHTMLTag * tag, UCLinkProperties * old_prop,
			      gboolean * accept, guint retry)
{
  UCLinkProperties *prop = NULL;
  UCStatusCode *sc = NULL;
  gchar *protocol = NULL;
  gchar *hostname = NULL;
  protocol = NULL;
  gboolean cancel = FALSE;
  gboolean blocked_domain = FALSE;
  gboolean already_checked = FALSE;
  gboolean no_check = FALSE;
  UCHTTPCheckReturn ret = UC_HTTP_CHECK_RETURN_OK;


  protocol = uc_url_get_protocol (tag->value);
  if (uc_check_protocol_accepted (protocol))
    {
      // Already checked link?
      already_checked =
	uc_check_link_already_checked_with_insert (currentItem.parent,
						   currentItem.url);

      // Must we block this link?
      hostname =
	uc_url_get_hostname (currentItem.current_host, currentItem.url);
      blocked_domain = (!already_checked
			&& uc_timeout_domains_is_blocked (hostname));
      g_free (hostname), hostname = NULL;

      // If blocked or already checked, do nothing
      no_check = (already_checked || blocked_domain);
    }
  else
    no_check = TRUE;

  if (no_check)
    {
      g_free (protocol), protocol = NULL;
      return NULL;
    }

  uc_application_display_state_message (UC_CHECK_MESSAGE_LABEL_SECOND,
					tag->value);
  uc_application_display_state_message (UC_CHECK_MESSAGE_LABEL_THIRD,
					_("Searching..."));
  UC_UPDATE_UI;

  prop = uc_check_link_properties_node_new ();
  prop->depth_level = depth;
  prop->link_value = g_strdup (tag->value);
  prop->protocol = protocol;
  prop->current_path = g_strdup (current_path);

  (gboolean) uc_url_parse (current_host, current_path, tag->value,
			   &prop->h_name, &prop->port, &prop->path,
			   &prop->args);

  // If we chroot the check, initialize the directory to chroot in
  if (currentItem.is_first)
  {
    gchar *dir = NULL;

 
    // Set project main hostname
    uc_project_set_current_host (prop->h_name);

    // Set main project chroot path
    if (uc_project_get_check_chroot ())
    {
      dir = g_path_get_dirname (prop->path);
      uc_project_set_chroot_path (dir);
      g_free (dir), dir = NULL;
    }
    else
      uc_project_set_chroot_path ("");
  }

  // If we are in chroot mode (just this directory and its subdirectories,
  // or just local links)
  if (!uc_check_chroot_authorized (prop->h_name, prop->protocol, prop->path))
    goto error_handler;

  // MAILTO protocol
  if (tag->type == LINK_TYPE_EMAIL)
    cancel = uc_check_link_get_properties_proto_mailto (prop);
  // FILE protocol
  else if (tag->type == LINK_TYPE_FILE_HREF ||
	   tag->type == LINK_TYPE_FILE_IMAGE)
    cancel =
      uc_check_link_get_properties_proto_file (prop, tag->value,
                                               current_path, current_host,
                                               tag->type);
  // HTTP protocol
  else if (tag->type != LINK_TYPE_NONE)
    {
      gboolean old_no_args = FALSE;


      // Always take URL args in account for images and CSS links
      if (tag->type != LINK_TYPE_HREF)
      {
        old_no_args = uc_project_get_no_urls_args ();
        uc_project_set_no_urls_args (FALSE);
      }

      ret =
	uc_check_link_get_properties_proto_http (prop, tag->value,
						 current_path, current_host,
						 tag, old_prop);
      cancel = (ret == UC_HTTP_CHECK_RETURN_SAME);

      // Restore value
      if (tag->type != LINK_TYPE_HREF)
        uc_project_set_no_urls_args (old_no_args);

      // If the link is a redirection (HTTP), or if there is a META
      // refresh in the page content (HTML), retry with the new location
      // -> Retry 3 times max, to avoid infinite loop
      if (ret == UC_HTTP_CHECK_RETURN_FAKED_URL || 
         (prop->metas != NULL &&
          uc_utils_get_meta_refresh_location (prop, tag)))
      {
        cancel = TRUE;

        if (retry <= 3)
        {
          do
          {
            gchar *normalized_url = NULL;
            gboolean old_no_args = FALSE;


            normalized_url = uc_url_normalize (current_host,
                                               current_path, tag->value);
            uc_check_currentitem_init (currentItem.parent, current_host,
                                       tag, normalized_url,
                                       currentItem.is_first);

            uc_lists_checked_links_node_free (NULL, &prop);

            // Take URL args in account when following HTTP new location or
            // META refresh value
            old_no_args = uc_project_get_no_urls_args ();
            uc_project_set_no_urls_args (FALSE);

            prop = uc_check_link_get_properties (depth, current_host,
                                                 current_path, tag, old_prop,
                                                 accept, retry + 1);

            // Restore old value
            uc_project_set_no_urls_args (old_no_args);

            g_free (normalized_url), normalized_url = NULL;
          }
          while (uc_utils_get_meta_refresh_location (prop, tag));

          cancel = FALSE;
        }
      }
    }
  else
    goto error_handler;

  // Either a error occured or the user has cancelled the check
  if (prop == NULL || cancel)
    goto error_handler;

  // If user choose to ignore this links
  if (uc_check_ignore_item_get_value ())
    {
      if (prop->header != NULL)
	g_hash_table_destroy (prop->header), prop->header = NULL;
      prop->header = uc_check_url_build_header (UC_STATUS_CODE_IGNORED_LINK);
      uc_report_set_ignoredlinks (-1);
      uc_check_ignore_item_set_value (FALSE);
    }
  // If malformed tag
  else if (ret == UC_HTTP_CHECK_RETURN_MALFORMED)
    {
      if (prop->header != NULL)
	g_hash_table_destroy (prop->header), prop->header = NULL;
      prop->header =
	uc_check_url_build_header (UC_STATUS_CODE_MALFORMED_LINK);
      uc_report_set_malformedlinks (-1);
    }
  // FILE protocol detected and user choice is: warn me!
  else
    if ((prop->header
	 && !strcmp ((gchar *)
		     g_hash_table_lookup (prop->header, UC_HEADER_STATUS),
		     UC_STATUS_CODE_FILE_PROTO_ERROR)))
    {
      uc_report_set_malformedlinks (-1);
    }
  // If connection timeout
  else if (ret == UC_HTTP_CHECK_RETURN_TIMEOUT)
    {
      if (prop->header != NULL)
	g_hash_table_destroy (prop->header), prop->header = NULL;
      prop->header = uc_check_url_build_header (UC_STATUS_CODE_TIMEOUT);
      uc_timeout_domains_register (prop->h_name);
    }

  *accept = uc_check_apply_filters (prop);

  prop->link_icon = uc_check_get_link_type_icon (prop->link_type,
						 prop->protocol);

  sc =
    uc_application_get_status_code_properties (g_hash_table_lookup
					       (prop->header,
						UC_HEADER_STATUS));
  prop->status_icon = sc->icon_file;
  prop->label = g_strdup (tag->label);

  // If authentication needed (works only for main URL)
  if (uc_project_get_prompt_auth () &&
      strcmp (g_hash_table_lookup (prop->header, UC_HEADER_STATUS),
	      UC_STATUS_CODE_RESTRICTED) == 0 &&
      strcmp (uc_project_get_current_host (), prop->h_name) == 0)
    {
      if (uc_application_auth_dialog_show (
            g_hash_table_lookup (prop->header, UC_WWW_AUTHENTICATE),
            prop->h_name))
	{
	  uc_lists_checked_links_node_free (NULL, &prop);
	  prop = uc_check_link_get_properties (depth, current_host,
					       current_path, tag,
					       old_prop, accept, retry);
	}
      else
	uc_check_ignore_item_set_value (TRUE);
    }

  return prop;

// Clean at function exit
error_handler:

  if (prop != NULL)
    uc_lists_checked_links_node_free (NULL, &prop);

  return NULL;
}


guint32
uc_check_get_main_page_id (void)
{
  GList *list = NULL;
  guint32 ret = 1;

  list = uc_lists_checked_links_get ();
  if (list != NULL)
    ret = ((UCLinkProperties *) list->data)->uid;

  return ret;
}

gboolean
uc_check_is_main_page (const UCLinkProperties * prop)
{
  g_return_val_if_fail (prop != NULL, FALSE);

  return (prop->uid == uc_check_get_main_page_id ());
}

UCLinkProperties *
uc_check_copy_node (UCLinkProperties * src, UCLinkProperties * dst)
{
  if (dst->header != NULL)
    g_hash_table_destroy (dst->header), dst->header = NULL;
  dst->header = src->header;
  dst->header_size = src->header_size;
  dst->link_type = src->link_type;
  dst->link_icon = src->link_icon;
  dst->status_icon = src->status_icon;

  g_free (dst->link_value), dst->link_value = src->link_value;
  g_free (dst->url), dst->url = src->url;
  g_free (dst->normalized_url), dst->normalized_url = src->normalized_url;
  g_free (dst->label), dst->label = src->label;
  g_free (dst->protocol), dst->protocol = src->protocol;
  g_free (dst->h_name), dst->h_name = src->h_name;
  g_free (dst->port), dst->port = src->port;
  g_free (dst->path), dst->path = src->path;
  g_free (dst->args), dst->args = src->args;
  g_free (dst->domain), dst->domain = src->domain;

  dst->is_parsable = src->is_parsable;
  dst->is_downloadable = src->is_downloadable;
  dst->virii = src->virii;
  dst->w3c_valid = src->w3c_valid;

  /* FIXME dst->similar_links_parents = src->similar_links_parents; */
  dst->bad_extensions = src->bad_extensions;
  dst->tags = src->tags;
  dst->all_links = src->all_links;
  dst->emails = src->emails;
  dst->metas = src->metas;

  if (src->is_downloadable)
    uc_cache_change_id (dst->uid, src->uid);

  return dst;
}

void
uc_check_set_depth (GList * list, const guint depth)
{
  GList *tmp = NULL;

  tmp = g_list_first (list);
  while (tmp != NULL)
    {
      UCLinkProperties *prop = (UCLinkProperties *) tmp->data;

      prop->depth_level = depth;

      if (prop->childs != NULL)
	uc_check_set_depth (prop->childs, depth + 1);

      tmp = g_list_next (tmp);
    }
}

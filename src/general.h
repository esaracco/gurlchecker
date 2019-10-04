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

#ifndef H_UC_GENERAL
#define H_UC_GENERAL

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <libgen.h>
#include <gnet.h>
#include "getopt.h"
#include <ctype.h>
#include <gnome.h>
#include <math.h>
#include <glade/glade.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <resolv.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xmlwriter.h>
#include <libxml/HTMLparser.h>
#include <gconf/gconf-client.h>
#ifdef ENABLE_GNUTLS
#include <gnutls/gnutls.h>
#endif
#ifdef ENABLE_CLAMAV
#include <clamav.h>
#endif
#ifdef ENABLE_CROCO
#include <libcroco/libcroco.h>
#endif
#ifdef ENABLE_TIDY
#ifdef HAVE_TIDY_H_IN_SUBDIR
#include <tidy/tidy.h>
#include <tidy/buffio.h>
#else
#include <tidy.h>
#include <buffio.h>
#endif
#endif
#ifdef ENABLE_SQLITE3
#include <sqlite3.h>
#endif
#ifdef ENABLE_JSON
#include <json-glib/json-glib.h>
#endif


/**
 * UC_SYSTEM_NAME:
 *
 * Macro to determine the system we are running on.
 */
#if defined(__linux__) || defined(__GLIBC__)
#define UC_SYSTEM_NAME "Linux"
#include <sys/vfs.h>
#elif defined(__NetBSD__) || defined(__OpenBSD__) || defined(__FreeBSD__)
#ifdef __NetBSD__
#define UC_SYSTEM_NAME "NetBSD"
#elif __OpenBSD__
#define UC_SYSTEM_NAME "OpenBSD"
#elif __FreeBSD__
#define UC_SYSTEM_NAME "FreeBSD"
#endif
#include <sys/param.h>
#include <sys/mount.h>
#include <arpa/nameser.h>
#include <arpa/nameser_compat.h>
#elif defined(__sun__)
#include <sys/statvfs.h>
#define statfs statvfs
#define UC_SYSTEM_NAME "Solaris"
#endif

#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#define UC_VERSION VERSION

#define UC_UPDATE_UI \
	while (g_main_context_pending (NULL)) \
	{ \
		GDK_THREADS_LEAVE (); \
		g_main_context_iteration (NULL, FALSE); \
	}

/**
 * UC_GLADE_XML_FILENAME:
 *
 * Macro for Glade XML file location.
 */
#define UC_GLADE_XML_FILENAME \
	PACKAGE_DATA_DIR "/" PACKAGE "/ui/gurlchecker.glade"

/**
 * UC_STYLESHEETS_DIR:
 *
 * Macro for export's stylesheets files location.
 */
#define UC_STYLESHEETS_DIR \
	PACKAGE_DATA_DIR "/" PACKAGE "/stylesheets"

/**
 * UC_PIXMAPS_DIR
 *
 * Macro for images path location.
 */
#define UC_PIXMAPS_DIR \
	PACKAGE_DATA_DIR "/" PACKAGE "/ui/"

/**
 * UC_DIALOG_SETTINGS_DOCUMENT_TYPES_MAX_COLS:
 *
 * Macro for the maximum number of columns to display for document types in
 * the "Documents" and "Images" notbooks of the settings.
 */
#define UC_DIALOG_SETTINGS_DOCUMENT_TYPES_MAX_COLS \
	6

/**
 * UC_CHECK_BUTTON_MAX_DISPLAY:
 *
 * Macro for the maximum number of check boxes to display for document types in
 * the "Documents" and "Images" notbooks of the settings.
 */
#define UC_CHECK_BUTTON_MAX_DISPLAY \
	100

/**
 * UC_CHECK_MESSAGE_LABEL_FIRST:
 *
 * Macro to indicate on which location on the check dialog we must dislay 
 * a message.
 */
#define UC_CHECK_MESSAGE_LABEL_FIRST \
	0
/**
 * UC_CHECK_MESSAGE_LABEL_SECOND:
 *
 * Macro to indicate on which location on the check dialog we must dislay 
 * a message.
 */
#define UC_CHECK_MESSAGE_LABEL_SECOND \
	1
/**
 * UC_CHECK_MESSAGE_LABEL_THIRD:
 *
 * Macro to indicate on which location on the check dialog we must dislay 
 * a message.
 */
#define UC_CHECK_MESSAGE_LABEL_THIRD \
	2

/**
 * UC_STATUS_CODE_INTERNALS_LIMIT:
 *
 * Macro to indicate that all status codes upper are gURLChecker
 * internal status codes.
 */
#define UC_STATUS_CODE_INTERNALS_LIMIT \
	700
/**
 * UC_STATUS_CODE_PAGE_OK:
 *
 * Macro for HTTP status code 200.
 */
#define UC_STATUS_CODE_PAGE_OK \
	"200"
/**
 * UC_STATUS_CODE_NO_DATA:
 *
 * Macro for HTTP status code 204.
 */
#define UC_STATUS_CODE_NO_DATA \
	"204"
/**
 * UC_STATUS_CODE_MOVED_PERMANENTLY:
 *
 * Macro for HTTP status code 301.
 */
#define UC_STATUS_CODE_MOVED_PERMANENTLY \
	"301"
/** 
 * UC_STATUS_CODE_REDIRECTED:
 *
 * Macro for HTTP status code 302.
 */
#define UC_STATUS_CODE_REDIRECTED \
	"302"
/** 
 * UC_STATUS_CODE_REDIRECTED_SEE_OTHER:
 *
 * Macro for HTTP status code 303.
 */
#define UC_STATUS_CODE_REDIRECTED_SEE_OTHER \
	"303"
/**
 * UC_STATUS_CODE_RESTRICTED:
 *
 * Macro for HTTP status code 401.
 */
#define UC_STATUS_CODE_RESTRICTED \
	"401"
/**
 * UC_STATUS_CODE_PAGE_NOT_FOUND:
 *
 * Macro for HTTP status code 404.
 */
#define UC_STATUS_CODE_PAGE_NOT_FOUND \
	"404"
/**
 * UC_STATUS_CODE_BAD_METHOD:
 *
 * Macro for HTTP status code 405.
 */
#define UC_STATUS_CODE_BAD_METHOD \
	"405"
/**
 * UC_STATUS_CODE_TIMEOUT:
 *
 * Macro for HTTP status code 408.
 */
#define UC_STATUS_CODE_TIMEOUT \
	"408"

/**
 * UC_STATUS_CODE_FTP_MAX_CLIENTS:
 * 
 * Macro for FTP response we are using for "Max clients".
 */
#define UC_STATUS_CODE_FTP_MAX_CLIENTS \
	"530"
/**
 * UC_STATUS_CODE_HOST_IS_UNREACHABLE:
 *
 * Macro for gURLChecker internal status code.
 */
#define UC_STATUS_CODE_HOST_IS_UNREACHABLE \
	"999"
/**
 * UC_STATUS_CODE_UNKNOWN:
 *
 * Macro for gURLChecker internal status code.
 */
#define UC_STATUS_CODE_UNKNOWN \
	"998"
/**
 * UC_STATUS_CODE_IGNORED_LINK:
 *
 * Macro for gURLChecker internal status code.
 */
#define UC_STATUS_CODE_IGNORED_LINK \
	"997"
/**
 * UC_STATUS_CODE_MALFORMED_LINK:
 *
 * Macro for gURLChecker internal status code.
 */
#define UC_STATUS_CODE_MALFORMED_LINK \
	"899"
/**
 * UC_STATUS_CODE_FILE_PROTO_ERROR:
 *
 * Macro for gURLChecker internal status code.
 */
#define UC_STATUS_CODE_FILE_PROTO_ERROR \
	"898"
/**
 * UC_STATUS_CODE_BAD_EMAIL_LINK:
 *
 * Macro for gURLChecker internal status code.
 */
#define UC_STATUS_CODE_BAD_EMAIL_LINK \
	"799"
/**
 * UC_STATUS_CODE_BAD_EMAIL_LINK_MX:
 *
 * Macro for gURLChecker internal status code.
 */
#define UC_STATUS_CODE_BAD_EMAIL_LINK_MX \
	"798"
/**
 * UC_STATUS_CODE_GOOD_EMAIL_LINK:
 *
 * Macro for gURLChecker internal status code.
 */
#define UC_STATUS_CODE_GOOD_EMAIL_LINK \
	"797"
/**
 * UC_STATUS_CODE_FTP_OK:
 * 
 * Macro for gURLChecker internal status code.
 */
#define UC_STATUS_CODE_FTP_OK \
	"702"
/**
 * UC_STATUS_CODE_FTP_BAD:
 * 
 * Macro for gURLChecker internal status code.
 */
#define UC_STATUS_CODE_FTP_BAD \
	"701"
/**
 * UC_IGNORED_LINK_BGCOLOR:
 *
 * Macro for ignored links color.
 */
#define UC_IGNORED_LINK_BGCOLOR \
	"pink"
/**
 * UC_BAD_LINK_BGCOLOR:
 *
 * Macro for bad links color.
 */
#define UC_BAD_LINK_BGCOLOR \
	"red"
/**
 * UC_GOOD_LINK_BGCOLOR:
 *
 * Macro for good links color.
 */
#define UC_GOOD_LINK_BGCOLOR \
	"green"
/**
 * UC_TIMEOUT_LINK_BGCOLOR:
 *
 * Macro for timeouts links color.
 */
#define UC_TIMEOUT_LINK_BGCOLOR \
	"purple"
/**
 * MALFORMED_LINK_BGCOLOR:
 *
 * Macro for malformed links color.
 */
#define UC_MALFORMED_LINK_BGCOLOR \
	"brown"
/**
 * UC_HOST_UNREACHABLE_LINK_BGCOLOR:
 *
 * Macro for "host unreachable" links color.
 */
#define UC_HOST_UNREACHABLE_LINK_BGCOLOR \
	"yellow"
/**
 * UC_NO_DATA_LINK_BGCOLOR:
 *
 * Macro for color of good links with no content.
 */
#define UC_NO_DATA_LINK_BGCOLOR \
	"orange"
/**
 * UC_RESTRICTED_LINK_BGCOLOR:
 *
 * Macro for restricted links color.
 */
#define UC_RESTRICTED_LINK_BGCOLOR \
	"blue"
/**
 * UC_FILE_ERROR_LINK_BGCOLOR:
 *
 * Macro for local files links color when user choose to consider them
 * like an error.
 */
#define UC_FILE_ERROR_LINK_BGCOLOR \
	"silver"
/**
 * UC_BAD_EMAIL_LINK_BGCOLOR:
 *
 * Macro for bad email links.
 */
#define UC_BAD_EMAIL_LINK_BGCOLOR \
	"red"
/**
 * UC_GOOD_EMAIL_LINK_BGCOLOR:
 *
 * Macro for good email links color.
 */
#define UC_GOOD_EMAIL_LINK_BGCOLOR \
	"green"

#ifdef ENABLE_GNUTLS
	/**
	 * UC_CHECK_PROTOCOLS_ACCEPTED:
	 *
	 * Macro for indicate which protocols we can check.
	 * For the moment gURLChecker can only check HTTP protocol.
	 */
#define UC_CHECK_PROTOCOLS_ACCEPTED \
		"http,https,ftp"
	/**
	 * UC_CHECK_PROTOCOLS_ACCEPTED_LEN:
	 *
	 * Macro for indicate the number of protocols we can check.
	 */
#define UC_CHECK_PROTOCOLS_ACCEPTED_LEN \
	3
#else
	/**
	 * UC_CHECK_PROTOCOLS_ACCEPTED:
	 *
	 * Macro for indicate which protocols we can check.
	 * For the moment gURLChecker can only check HTTP protocol.
	 */
#define UC_CHECK_PROTOCOLS_ACCEPTED \
		"http,ftp"
	/**
	 * UC_CHECK_PROTOCOLS_ACCEPTED_LEN:
	 *
	 * Macro for indicate the number of protocols we can check.
	 */
#define UC_CHECK_PROTOCOLS_ACCEPTED_LEN \
	2
#endif

/**
 * UC_CHECK_PROTOCOLS_GET_ACCEPTED:
 *
 * Macro to get protocols we can check.
 */
#define UC_CHECK_PROTOCOLS_GET_ACCEPTED \
        g_strsplit( \
                        UC_CHECK_PROTOCOLS_ACCEPTED, \
                        UC_SPLIT_DELIMITER, \
                        UC_CHECK_PROTOCOLS_ACCEPTED_LEN \
                  )

#define UC_SPLIT_DELIMITER \
	","

/**
 * UC_DEFAULT_SECURITY_BAD_EXTENSIONS:
 *
 * Macro to indicate for which bad extensions we must search.
 */
#define UC_DEFAULT_SECURITY_BAD_EXTENSIONS \
	"bak,bad,sav,~,ok,-,old,orig,false"

/**
 * UC_DEFAULT_SECURITY_VIRII_EXTENSIONS:
 *
 * Macro to indicate for which extensions we must search for virii.
 */
#define UC_DEFAULT_SECURITY_VIRII_EXTENSIONS \
	"application,audio,image,text,video," \
	"chemical,model,multipart,x-conference,x-epoc,x-world"

/**
 * UC_DEFAULT_W3C_HTML_EXTENSIONS:
 *
 * Macro to indicate for which extensions we must search for HTML validation.
 */
#define UC_DEFAULT_W3C_HTML_EXTENSIONS \
        "cfm,html,htm,pl,phtml,shtml,asp,axp,aspx,php,php3,php4,php5"

/**
 * UC_DEFAULT_W3C_CSS_EXTENSIONS:
 *
 * Macro to indicate for which extensions we must search for CSS validation.
 */
#define UC_DEFAULT_W3C_CSS_EXTENSIONS \
        "css"

/**
 * UC_LABEL_DISPLAY_MAX_LEN:
 *
 * Macro to indicate the maximum display size of a string.
 *
 * @see: uc_utils_string_format4display()
 */
#define UC_LABEL_DISPLAY_MAX_LEN \
	45

/**
 * UC_DEFAULT_WEB_BROWSER_PATH:
 *
 * Macro to specify default browser path location.
 */
#define UC_DEFAULT_WEB_BROWSER_PATH \
	"/usr/bin/mozilla"
/**
 * WSENS:
 * @name: name of the widget in libglade XML file.
 * @value: TRUE or FALSE.
 *
 * Macro to set the "sensitive" property of a widget.
 */
#define WSENS(name, value) \
	gtk_widget_set_sensitive (glade_xml_get_widget(glade, name), value)
/**
 * UC_GET_WIDGET:
 * @name: name of the widget.
 * @window: #GtkWindow widget to work with.
 * @widget: #GtkWidget to work with.
 *
 * Macro to get a #GtkWidget in the third argument.
 *
 * @see: UC_SET_WIDGET()
 */
#define UC_GET_WIDGET(name, window, widget) \
	 widget = GTK_WIDGET (g_object_get_data (G_OBJECT (window), name))
/**
 * UC_SET_WIDGET:
 * @name: name of the widget.
 * @window: #GtkWindow widget to work with.
 * @widget: #GtkWidget to work with.
 *
 * Macro to save a #GtkWidget to retrieve it faster later.
 *
 * @see: UC_GET_WIDGET()
 */
#define UC_SET_WIDGET(name, window, widget) \
	g_object_set_data_full (G_OBJECT (window), name, \
	g_object_ref (widget), (GDestroyNotify) g_object_unref)
/**
 * WGET:
 * @name: name of the widget to retreive.
 * 
 * Macro to get a GtkWidget from a GladeXML object.
 * 
 */
#define WGET(name) \
	glade_xml_get_widget (glade, name)

/**
 * UC_BUFFER_LEN:
 *
 * Macro to indicate maximum length of buffer which contains URLs.
 */
#define UC_BUFFER_LEN \
	1024
/**
 * UC_BUFFER_DATETIME_LEN:
 *
 * Macro to indicate maximum length of date and time buffers.
 */
#define UC_BUFFER_DATETIME_LEN \
	20

/**
 * UC_BUFFER_DATE_LEN:
 *
 * Macro to indicate maximum length of date buffers.
 */
#define UC_BUFFER_DATE_LEN \
	11

/**
 * UC_HEADER_STATUS_CODE_LEN:
 *
 * Macro indicate the maximum length of a HTTP status code.
 */
#define UC_HEADER_STATUS_CODE_LEN \
	3
/**
 *  UC_HEADER_STATUS_CODE_FIRST_POS:
 *
 *  Macro to indicate the first position of the HTTP status code in
 *  the buffer.
 */
#define UC_HEADER_STATUS_CODE_FIRST_POS \
	9
/**
 * UC_PROTOCOL_FTP:
 *
 * Macro for the FTP protocol string identifier.
 */
#define UC_PROTOCOL_FTP \
	"ftp"
/**
 * UC_PROTOCOL_HTTP:
 *
 * Macro for the HTTP protocol string identifier.
 */
#define UC_PROTOCOL_HTTP \
	"http"
/**
 * UC_PROTOCOL_HTTPS:
 *
 * Macro for the HTTPS protocol string identifier.
 */
#define UC_PROTOCOL_HTTPS \
	"https"
/**
 * UC_PROTOCOL_MAILTO:
 *
 * Macro for the MAILTO protocol string identifier.
 */
#define UC_PROTOCOL_MAILTO \
	"mailto"
/**
 * UC_PROTOCOL_FILE:
 *
 * Macro for the FILE protocol string identifier.
 */
#define UC_PROTOCOL_FILE \
	"file"
/**
 *  UC_CONTENT_TYPE:
 *
 *  Macro for the HTTP header content type field name.
 */
#define UC_CONTENT_TYPE \
	"content-type"
/* FIXME */
/**
 * UC_CONTENT_TYPE_FTP:
 *
 * gURLChecker internal mime type.
 */
#define UC_CONTENT_TYPE_FTP \
	"ftp/file"
/**
 * UC_CONTENT_LENGTH:
 *
 * Macro for the HTTP header content length field.
 */
#define UC_CONTENT_LENGTH \
	"content-length"
/**
 * UC_LAST_MODIFIED:
 *
 * Macro for the HTTP header last modified field.
 */
#define UC_LAST_MODIFIED \
	"last-modified"
/**
 * UC_SET_COOKIE:
 *
 * Macro for the HTTP header set-cookie field.
 */
#define UC_SET_COOKIE \
	"set-cookie"
/**
 * UC_CONTENT_TYPE_HTML:
 *
 * Macro for the HTTP content type string identifier.
 */
#define UC_CONTENT_TYPE_HTML \
	"text/html"
/**
 * UC_CONTENT_TYPE_CSS:
 *
 * Macro for the stylesheet content type.
 */
#define UC_CONTENT_TYPE_CSS \
	"text/css"
/**
 * UC_HEADER_STATUS:
 *
 * Macro for gURLChecker internal link status string.
 */
#define UC_HEADER_STATUS \
	"status"
/**
 * UC_CONTENT_LOCATION:
 *
 * Macro for the HTTP header content location field.
 */
#define UC_CONTENT_LOCATION \
	"content-location"
/**
 * UC_LOCATION:
 *
 * Macro for the HTTP header location field.
 */
#define UC_LOCATION \
	"location"

/**
 * UC_WWW_AUTHENTICATE:
 *
 * Macro for the WWW-Authenticate HTTP header field
 */
#define UC_WWW_AUTHENTICATE \
	"www-authenticate"

/**
 *  UC_SERVER:
 *
 *  Macro for the HTTP header server field name.
 */
#define UC_SERVER \
	"server"
/**
 * UC_URL_DEFAULT_PORT:
 *
 * Macro to indicate the default URL port when it is not specified.
 */
#define UC_URL_DEFAULT_PORT \
	"80"
/**
 * UC_URL_DEFAULT_SSL_PORT:
 *
 * Macro to indicate the default URL port when it is not specified.
 */
#define UC_URL_DEFAULT_SSL_PORT \
	"443"

#define UC_URL_DEFAULT_FTP_PORT \
	"21"

#define UC_SOCKET_OPEN_FORCE_ARGS_USE \
	TRUE
#define UC_SOCKET_OPEN_DEFAULT \
	FALSE
/**
 * UC_TOOLTIPS_DELAY_DEFAULT:
 *
 * Macro to indicate main tree view tooltip delay.
 */
#define UC_TOOLTIPS_DELAY_DEFAULT \
	500
/**
 * UC_CHECK_WAIT_DEFAULT:
 *
 * Macro for default time to wait between links check.
 */
#define UC_CHECK_WAIT_DEFAULT \
	0
/**
 * UC_CHECK_TIMEOUT_DEFAULT:
 *
 * Macro for default timeout value.
 */
#define UC_CHECK_TIMEOUT_DEFAULT \
	30
/**
 * UC_MAX_DEPTH_LEVEL:
 *
 * Macro for maximum recursion depth level on pages.
 */
#define UC_MAX_DEPTH_LEVEL \
	1000
/**
 * UC_TIMEOUTS_BLOCKED_DEFAULT:
 *
 * Macro for maximum number of attempts after a timedouted
 * host should not be checked anymore
 */
#define UC_TIMEOUTS_BLOCKED_DEFAULT \
	2
/**
 * UC_USER_AGENT:
 *
 * Macro for user agent HTTP header field.
 */
#define UC_USER_AGENT \
	"Mozilla/5.0 gURLChecker/" UC_VERSION " (" UC_SYSTEM_NAME ")"

/* global pointer for glade xml file */
GladeXML *glade;

/* treeview pointers */
GtkTreeView *treeview;		/* url list */
GtkTreeView *treeview_filter_directories;	/* rejected directories */
GtkTreeView *treeview_projects;	/* existant projects */
GtkTreeView *treeview_filter_domains;	/* rejected domains */
GtkTreeView *treeview_search;	/* result of the search */

/* treestore for the url treeview list */
GtkTreeStore *treestore;	/* url list */
GtkListStore *treestore_filter_directories;	/* rejected directories */
GtkListStore *treestore_filter_domains;	/* rejected domains */

GtkListStore *treestore_search;	/* result of the search */

gboolean uc_refresh_page;
#ifdef ENABLE_CLAMAV
gboolean uc_init_clamav_done;
#endif

GdkPixbuf *security_alert_icon;
GdkPixbuf *w3c_alert_icon;
GdkPixbuf *empty_icon;

/**
 * UCSearchState:
 * @string: String to search.
 * @case_sensitive: %TRUE if the search should be case sensitive.
 * @pages_content: %TRUE if the search must be done in page content.
 * @links_name: %TRUE if the search must be done in links name.
 * @meta_content: %TRUE if the search must be done in META tags.
 * @emails: %TRUE if the search must be done in E-Mails links.
 * 
 * Structure to store search properties.
 */
struct UCSearchState
{
  gchar *string;
  gboolean case_sensitive;
  gboolean status_code;
  gboolean pages_content;
  gboolean links_name;
  gboolean meta_content;
  gboolean emails;
}
search_state;

/**
 * UCURLsUserActions:
 * @id: Index of the action.
 * @label: Action label.
 * 
 * Structure to store bad URLs actions.
 */
typedef struct _UCURLsUserActions UCURLsUserActions;
struct _UCURLsUserActions
{
  gchar *id;
  gchar *label;
};

/**
 * UCStatusCode:
 * @color: Color to associate with the status code.
 * @message: Short message to display.
 * @description: Description of the meaning of the status code.
 * @icon_file: Image to assiciate with the status code.
 * 
 * Structure to store status code properties.
 */
typedef struct _UCStatusCode UCStatusCode;
struct _UCStatusCode
{
  gchar *color;
  gchar *message;
  gchar *description;
  GdkPixbuf *icon_file;
};

#define UC_DISPLAY_PROTO_SET_NONE \
  current_display_proto = UC_DISPLAY_PROTO_NONE

#define UC_DISPLAY_STATUS_SET_NONE \
  current_display_status = UC_DISPLAY_STATUS_NONE

#define UC_DISPLAY_TYPE_SET_NONE \
  current_display_type = UC_DISPLAY_TYPE_NONE

#define UC_DISPLAY_SET_ALL \
  UC_DISPLAY_TYPE_SET_NONE;\
  UC_DISPLAY_STATUS_SET_NONE; \
  UC_DISPLAY_PROTO_SET_NONE

#define UC_DISPLAY_PROTO_SET(flag) current_display_proto |= (flag)
#define UC_DISPLAY_PROTO_UNSET(flag) current_display_proto &= ~(flag)
#define UC_DISPLAY_PROTO_IS(flag) (current_display_proto & flag)
#define UC_DISPLAY_PROTO_IS_NONE (current_display_proto == 0)

typedef enum _UCDisplayProto UCDisplayProto;
enum _UCDisplayProto
{
  UC_DISPLAY_PROTO_NONE = 0,
  UC_DISPLAY_PROTO_HTTP = 1,
  UC_DISPLAY_PROTO_HTTPS  = 1 << 1,
  UC_DISPLAY_PROTO_FTP = 1 << 2,
  UC_DISPLAY_PROTO_FILE = 1 << 3
};
UCDisplayProto current_display_proto;

#define UC_DISPLAY_STATUS_SET(flag) current_display_status |= (flag)
#define UC_DISPLAY_STATUS_UNSET(flag) current_display_status &= ~(flag)
#define UC_DISPLAY_STATUS_IS(flag) (current_display_status & flag)
#define UC_DISPLAY_STATUS_IS_NONE (current_display_status == 0)

typedef enum _UCDisplayStatus UCDisplayStatus;
enum _UCDisplayStatus
{
  UC_DISPLAY_STATUS_NONE = 0,
  UC_DISPLAY_STATUS_GOODLINKS = 1,
  UC_DISPLAY_STATUS_BADLINKS  = 1 << 1,
  UC_DISPLAY_STATUS_TIMEOUTS = 1 << 2,
  UC_DISPLAY_STATUS_MALFORMEDLINKS = 1 << 3,
  UC_DISPLAY_STATUS_SECURITY_ALERTS = 1 << 4,
  UC_DISPLAY_STATUS_W3C_ALERTS = 1 << 5
};
UCDisplayStatus current_display_status;

#define UC_DISPLAY_TYPE_SET(flag) current_display_type |= (flag)
#define UC_DISPLAY_TYPE_UNSET(flag) current_display_type &= ~(flag)
#define UC_DISPLAY_TYPE_IS(flag) (current_display_type & flag)
#define UC_DISPLAY_TYPE_IS_NONE (current_display_type == 0)

typedef enum _UCDisplayType UCDisplayType;
enum _UCDisplayType
{
  UC_DISPLAY_TYPE_NONE = 0,
  UC_DISPLAY_TYPE_HREF = 1,
  UC_DISPLAY_TYPE_IMAGE = 1 << 1,
  UC_DISPLAY_TYPE_FRAME = 1 << 2,
  UC_DISPLAY_TYPE_EMAIL = 1 << 3,
  UC_DISPLAY_TYPE_CSS = 1 << 4
};
UCDisplayType current_display_type;

/**
 * UCLinkType:
 * @LINK_TYPE_NONE: No specific type.
 * @LINK_TYPE_HREF: HREF tag (HTTP) with a page.
 * @LINK_TYPE_HREF_SSL: HREF tag (HTTPS) with a page.
 * @LINK_TYPE_HREF_FTP: HREF tag (FTP) with a page.
 * @LINK_TYPE_BASE_HREF: BASE HREF tag.
 * @LINK_TYPE_FILE_HREF: HREF tag (FILE) with a local page.
 * @LINK_TYPE_FILE_IMAGE: IMG tag with a local image.
 * @LINK_TYPE_IMAGE: IMG tag with a image.
 * @LINK_TYPE_IMAGE_SSL: IMG tag (HTTPS) with a image.
 * @LINK_TYPE_IMAGE_FTP: IMG tag (FTP) with a image.
 * @LINK_TYPE_EMAIL: HREF tag (MAILTO) with a email.
 * @LINK_TYPE_META: META tag.
 * @LINK_TYPE_CSS: CSS tag (HTTP).
 * @LINK_TYPE_CSS_SSL: CSS tag (HTTPS).
 * @LINK_TYPE_FRAME: FRAME or IFRAME tag (HTTP).
 * @LINK_TYPE_FRAME_SSL: FRAME or IFRAME tag (HTTPS).
 * @LINK_TYPE_BOOKMARK_FOLDER: A bookmark folder.
 * @LINK_TYPES_SECURITY_TEST_BAD_EXTENSIONS: A fake link to test existence of 
 *                                           bad files on the server.
 * 
 * Enumeration to identify links types.
 */
typedef enum _UCLinkType UCLinkType;
enum _UCLinkType
{
  LINK_TYPE_NONE,
  LINK_TYPE_HREF,
  LINK_TYPE_HREF_SSL,
  LINK_TYPE_HREF_FTP,
  LINK_TYPE_BASE_HREF,
  LINK_TYPE_FILE_HREF,
  LINK_TYPE_FILE_IMAGE,
  LINK_TYPE_IMAGE,
  LINK_TYPE_IMAGE_SSL,
  LINK_TYPE_IMAGE_FTP,
  LINK_TYPE_EMAIL,
  LINK_TYPE_META,
  LINK_TYPE_CSS,
  LINK_TYPE_CSS_SSL,
  LINK_TYPE_FRAME,
  LINK_TYPE_FRAME_SSL,
  LINK_TYPE_BOOKMARK_FOLDER,
  LINK_TYPES_SECURITY_TEST_BAD_EXTENSIONS
};

/**
 * UCBookmarksType:
 * @UC_BOOKMARKS_TYPE_XBEL: xbel file.
 * @UX_BOOKMARKS_TYPE_FIREFOX: Firefox sqlite3 file.
 * @UX_BOOKMARKS_TYPE_GOOGLECHROME: Google Chrome json file.
 * @UX_BOOKMARKS_TYPE_OPERA: Opera file.
 *
 * Type of current bookmark file.
 */
typedef enum _UCBookmarksType UCBookmarksType;
enum _UCBookmarksType
{
  UC_BOOKMARKS_TYPE_NONE,
  UC_BOOKMARKS_TYPE_XBEL,
  UC_BOOKMARKS_TYPE_FIREFOX,
  UC_BOOKMARKS_TYPE_GOOGLECHROME,
  UC_BOOKMARKS_TYPE_OPERA
};

/**
 * UCEmailStatus:
 * @UC_EMAIL_SYNTAX_BAD: Syntax of email is bad.
 * @UC_EMAIL_SYNTAX_MX_BAD: MX of email's domain is bad.
 * @UC_EMAIL_OK: Email is ok.
 * 
 * Status for email links.
 */
typedef enum _UCEmailStatus UCEmailStatus;
enum _UCEmailStatus
{
  UC_EMAIL_SYNTAX_BAD,
  UC_EMAIL_SYNTAX_MX_BAD,
  UC_EMAIL_OK
};

/**
 * UCExportFormat:
 * @UC_EXPORT_HTML: HTML export
 *
 * Export format.
 */
typedef enum _UCExportFormat UCExportFormat;
enum _UCExportFormat
{
  UC_EXPORT_HTML,
  UC_EXPORT_CSV,
  UC_EXPORT_LIST_NORMAL,
  UC_EXPORT_LIST_SIMPLE
};

typedef enum _UCCookiesAction UCCookiesAction;
enum _UCCookiesAction
{
  UC_COOKIES_ACTION_ADD,
  UC_COOKIES_ACTION_UPDATE,
  UC_COOKIES_ACTION_DELETE
};

/**
 * UCHTMLTag:
 * @depth: Depth of this node.
 * @label: Label of the link.
 * @value: Value of the link.
 * @type: Type of the link.
 * 
 * Structure for HTML tags nodes.
 */
typedef struct _UCHTMLTag UCHTMLTag;
struct _UCHTMLTag
{
  guint depth;
  gchar *label;
  gchar *value;
  UCLinkType type;
};

/**
 * UCLinkProperties:
 * @uid: Unique identifier for this node.
 * @treeview_path: tree view path of this node (it is filled on tree view
 *                 display).
 * @parent: Parent of this node.
 * @current_path: Current path of the page containing this link.
 * @iter: #GtkTreeIter iterator used for main URL treeview management.
 * @tag: #UCHTMLTag node for this link.
 * @link_type: Type of this link.
 * @link_icon: Icon associated with the type of this link.
 * @status_icon: Icon associated with the status of this link.
 * @link_value: Brut value of this link.
 * @url: Well formed URL of this link.
 * @normalized_url: Absolute URL.
 * @label: Label of this link.
 * @protocol: Protocol of the URL.
 * @h_name: Hostname of the URL.
 * @port: Port of the URL.
 * @path: Path of the URL.
 * @args: Arguments of the URL.
 * @domain: Domain name of the URL hostname.
 * @header: #GHashTable containing HTTP header fields and values.
 * @header_size: Size of the HTTP header.
 * @depth_level: Depth level of this node.
 * @is_parsable: TRUE if the content pointed by this link can be parse to 
 *              retreive others links.
 * @is_downloadable: TRUE if the content of this link can be downloaded.
 * @checked: TRUE is this link has already been proccessed.
 * @xml_node: Reference on the XmlNodePtr of the XmlDocPtr for this node. This
 *            field is used only for bookmarks.
 * @bookmark_folder_id: Reference on the parent id. This field is used only for FF
 *                      sqlite3 bookmarks.
 * @bookmark_id: id of the bookmark in FF database
 * @to_delete: TRUE if this node has been deleted. This field is used only for
 *             bookmarks.
 * @metas: #GList of page HTML meta tags (pointers on this node "tags"
 *         propertie).
 * @emails: #GList of E-Mails in this page (pointers on this node "tags"
 *          propertie).
 * @all_links: #GList of all links in this page (pointers on this node "tags"
 *             propertie).
 * @childs: Childs nodes of this node.
 * @similar_links_parents: Pointer on other nodes for links in this page
 *                         that has been already processed in another page.
 * @bad_extensions: #GList of bad extensions that exists on the server for 
 *                  this node.
 * @tags: #GList of #UCHTMLTag for all links in this page.
 * @user_action: A user defined action label
 * @virii: if the file is a virus or not 
 * @virname: Name of the virus is @virii == %TRUE
 * @w3c_valid: %TRUE if the document is in a correct format
 * 
 * Structure to store links properties.
 */
typedef struct _UCLinkProperties UCLinkProperties;
struct _UCLinkProperties
{
  guint32 uid;
  gchar *treeview_path;
  struct _UCLinkProperties *parent;
  gchar *current_path;
  GtkTreeIter iter;
  gpointer tag;
  UCLinkType link_type;
  GdkPixbuf *link_icon;
  GdkPixbuf *status_icon;
  gchar *user_action;
  gchar *link_value;
  gchar *url;
  gchar *normalized_url;
  gchar *label;
  gchar *ip_addr;
  gchar *protocol;
  gchar *h_name;
  gchar *port;
  gchar *path;
  gchar *args;
  gchar *domain;
  GHashTable *header;
  guint header_size;
  guint depth_level;
  gboolean is_parsable;
  gboolean is_downloadable;
  gboolean checked;
  xmlNodePtr xml_node;
#ifdef ENABLE_JSON
  JsonArray *json_array;
#endif
  guint32 bookmark_folder_id;
  guint32 bookmark_id;
  gboolean to_delete;
  GList *metas;
  GList *emails;
  GList *all_links;
  GList *childs;
  GList *similar_links_parents;
  GList *bad_extensions;
  gboolean virii;
  gchar *virname;
  gboolean w3c_valid;
  GList *tags;
};
#endif

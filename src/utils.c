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

#include "general.h"
#include "check.h"
#include "connection.h"
#include "project.h"

#include "utils.h"


void
uc_utils_clear_container (GtkContainer * container)
{
  GList *list = NULL;

  list = gtk_container_get_children (container);
  while (list != NULL)
    {
      GtkWidget *w = GTK_WIDGET (list->data);
      if (!GTK_IS_LABEL (w))
	gtk_widget_destroy (w);
      list = g_list_next (list);
    }
  g_list_free (list), list = NULL;
}

void
uc_utils_swap_file_proto_option (const gboolean begin)
{
  static gint file_error_sav = 0;
  static gint file_check_sav = 0;

  if (begin)
    {
      file_error_sav = uc_project_get_proto_file_is_error ();
      file_check_sav = uc_project_get_proto_file_check ();

      uc_project_set_proto_file_is_error (0);
      uc_project_set_proto_file_check (1);
    }
  else
    {
      uc_project_set_proto_file_is_error (file_error_sav);
      uc_project_set_proto_file_check (file_check_sav);
    }
}

/* /////////////////////////////////////////////////////////////////////////
 * >>>>>>>>>> [BEGIN] Stolen from wget source code
 * */

#define ISSPACE(x) isspace(x)
#define ISDIGIT(x) isdigit(x)
#define ISXDIGIT(x) isxdigit (x)
#define TOUPPER(x) toupper (x)

enum
{
  /* rfc1738 reserved chars, preserved from encoding.  */
  urlchr_reserved = 1,

  /* rfc1738 unsafe chars, plus some more.  */
  urlchr_unsafe = 2
};

#define urlchr_test(c, mask) (urlchr_table[(guchar)(c)] & (mask))
#define URL_RESERVED_CHAR(c) urlchr_test(c, urlchr_reserved)
#define URL_UNSAFE_CHAR(c) urlchr_test(c, urlchr_unsafe)

/* Convert an ASCII hex digit to the corresponding number between 0
   and 15.  X should be a hexadecimal digit that satisfies isxdigit;
   otherwise, the result is undefined.  */
#define XDIGIT_TO_NUM(x) ((x) < 'A' ? (x) - '0' : TOUPPER (x) - 'A' + 10)

/* Convert a sequence of ASCII hex digits X and Y to a number betewen
   0 and 255.  Uses XDIGIT_TO_NUM for conversion of individual
   digits.  */
#define X2DIGITS_TO_NUM(h1, h2) ((XDIGIT_TO_NUM (h1) << 4) + XDIGIT_TO_NUM (h2))

#define urlchr_test(c, mask) (urlchr_table[(guchar)(c)] & (mask))
#define URL_RESERVED_CHAR(c) urlchr_test(c, urlchr_reserved)
#define URL_UNSAFE_CHAR(c) urlchr_test(c, urlchr_unsafe)

/* Shorthands for the table: */
#define R  urlchr_reserved
#define U  urlchr_unsafe
#define RU R|U

const static guchar urlchr_table[256] = {
  U, U, U, U, U, U, U, U,	/* NUL SOH STX ETX  EOT ENQ ACK BEL */
  U, U, U, U, U, U, U, U,	/* BS  HT  LF  VT   FF  CR  SO  SI  */
  U, U, U, U, U, U, U, U,	/* DLE DC1 DC2 DC3  DC4 NAK SYN ETB */
  U, U, U, U, U, U, U, U,	/* CAN EM  SUB ESC  FS  GS  RS  US  */
  U, 0, U, RU, 0, U, R, 0,	/* SP  !   "   #    $   %   &   '   */
  0, 0, 0, R, 0, 0, 0, R,	/* (   )   *   +    ,   -   .   /   */
  0, 0, 0, 0, 0, 0, 0, 0,	/* 0   1   2   3    4   5   6   7   */
  0, 0, RU, R, U, R, U, R,	/* 8   9   :   ;    <   =   >   ?   */
  RU, 0, 0, 0, 0, 0, 0, 0,	/* @   A   B   C    D   E   F   G   */
  0, 0, 0, 0, 0, 0, 0, 0,	/* H   I   J   K    L   M   N   O   */
  0, 0, 0, 0, 0, 0, 0, 0,	/* P   Q   R   S    T   U   V   W   */
  0, 0, 0, RU, U, RU, U, 0,	/* X   Y   Z   [    \   ]   ^   _   */
  U, 0, 0, 0, 0, 0, 0, 0,	/* `   a   b   c    d   e   f   g   */
  0, 0, 0, 0, 0, 0, 0, 0,	/* h   i   j   k    l   m   n   o   */
  0, 0, 0, 0, 0, 0, 0, 0,	/* p   q   r   s    t   u   v   w   */
  0, 0, 0, U, U, U, U, U,	/* x   y   z   {    |   }   ~   DEL */

  U, U, U, U, U, U, U, U, U, U, U, U, U, U, U, U,
  U, U, U, U, U, U, U, U, U, U, U, U, U, U, U, U,
  U, U, U, U, U, U, U, U, U, U, U, U, U, U, U, U,
  U, U, U, U, U, U, U, U, U, U, U, U, U, U, U, U,

  U, U, U, U, U, U, U, U, U, U, U, U, U, U, U, U,
  U, U, U, U, U, U, U, U, U, U, U, U, U, U, U, U,
  U, U, U, U, U, U, U, U, U, U, U, U, U, U, U, U,
  U, U, U, U, U, U, U, U, U, U, U, U, U, U, U, U,
};

#undef R
#undef U
#undef RU

/* The reverse of the above: convert a number in the [0, 16) range to
   its ASCII representation in hex.  The A-F characters are in upper
   case.  */
#define XNUM_TO_DIGIT(x) ("0123456789ABCDEF"[x])

/* Like XNUM_TO_DIGIT, but generates lower-case characters. */
#define XNUM_TO_digit(x) ("0123456789abcdef"[x])

enum copy_method
{ CM_DECODE, CM_ENCODE, CM_PASSTHROUGH };

extern char *strptime (const char *s, const char *format, struct tm *tm);

static inline enum copy_method uc_utils_decide_copy_method (const gchar * p);
static time_t uc_utils_mktime_from_utc (struct tm *t);
static gint uc_utils_check_date_end (const gchar * p);

static inline enum copy_method
uc_utils_decide_copy_method (const gchar * p)
{
  if (*p == '%')
    {
      if (ISXDIGIT (*(p + 1)) && ISXDIGIT (*(p + 2)))
	{
	  /* %xx sequence: decode it, unless it would decode to an
	     unsafe or a reserved char; in that case, leave it as
	     is. */
	  gchar preempt = X2DIGITS_TO_NUM (*(p + 1), *(p + 2));
	  if (URL_UNSAFE_CHAR (preempt) || URL_RESERVED_CHAR (preempt))
	    return CM_PASSTHROUGH;
	  else
	    return CM_DECODE;
	}
      else
	/* Garbled %.. sequence: encode `%'. */
	return CM_ENCODE;
    }
  else if (URL_UNSAFE_CHAR (*p) && !URL_RESERVED_CHAR (*p))
    return CM_ENCODE;
  else
    return CM_PASSTHROUGH;
}

gchar *
uc_utils_url_reencode_escapes (const gchar * s)
{
  const gchar *p1;
  gchar *newstr, *p2;
  gint oldlen, newlen;

  gint encode_count = 0;
  gint decode_count = 0;

  /* First, pass through the string to see if there's anything to do,
     and to calculate the new length.  */
  for (p1 = s; *p1; p1++)
    {
      switch (uc_utils_decide_copy_method (p1))
	{
	case CM_ENCODE:
	  ++encode_count;
	  break;
	case CM_DECODE:
	  ++decode_count;
	  break;
	case CM_PASSTHROUGH:
	  break;
	}
    }

  if (!encode_count && !decode_count)
    /* The string is good as it is. */
    return (gchar *) s;		/* C const model sucks. */

  oldlen = p1 - s;
  /* Each encoding adds two characters (hex digits), while each
     decoding removes two characters.  */
  newlen = oldlen + 2 * (encode_count - decode_count);
  newstr = g_malloc0 (newlen + 1);

  p1 = s;
  p2 = newstr;

  while (*p1)
    {
      switch (uc_utils_decide_copy_method (p1))
	{
	case CM_ENCODE:
	  {
	    guchar c = *p1++;
	    *p2++ = '%';
	    *p2++ = XNUM_TO_DIGIT (c >> 4);
	    *p2++ = XNUM_TO_DIGIT (c & 0xf);
	  }
	  break;
	case CM_DECODE:
	  *p2++ = X2DIGITS_TO_NUM (p1[1], p1[2]);
	  p1 += 3;		/* skip %xx */
	  break;
	case CM_PASSTHROUGH:
	  *p2++ = *p1++;
	}
    }
  *p2 = '\0';
  g_assert (p2 - newstr == newlen);
  return newstr;
}

/* Converts struct tm to time_t, assuming the data in tm is UTC rather
   than local timezone.

   mktime is similar but assumes struct tm, also known as the
   "broken-down" form of time, is in local time zone.  mktime_from_utc
   uses mktime to make the conversion understanding that an offset
   will be introduced by the local time assumption.

   mktime_from_utc then measures the introduced offset by applying
   gmtime to the initial result and applying mktime to the resulting
   "broken-down" form.  The difference between the two mktime results
   is the measured offset which is then subtracted from the initial
   mktime result to yield a calendar time which is the value returned.

   tm_isdst in struct tm is set to 0 to force mktime to introduce a
   consistent offset (the non DST offset) since tm and tm+o might be
   on opposite sides of a DST change.

   Some implementations of mktime return -1 for the nonexistent
   localtime hour at the beginning of DST.  In this event, use
   mktime(tm - 1hr) + 3600.

   Schematically
     mktime(tm)   --> t+o
     gmtime(t+o)  --> tm+o
     mktime(tm+o) --> t+2o
     t+o - (t+2o - t+o) = t

   Note that glibc contains a function of the same purpose named
   `timegm' (reverse of gmtime).  But obviously, it is not universally
   available, and unfortunately it is not straightforwardly
   extractable for use here.  Perhaps configure should detect timegm
   and use it where available.

   Contributed by Roger Beeman <beeman@cisco.com>, with the help of
   Mark Baushke <mdb@cisco.com> and the rest of the Gurus at CISCO.
   Further improved by Roger with assistance from Edward J. Sabol
   based on input by Jamie Zawinski.  */
static time_t
uc_utils_mktime_from_utc (struct tm *t)
{
  time_t tl, tb;
  struct tm *tg;

  tl = mktime (t);
  if (tl == -1)
    {
      t->tm_hour--;
      tl = mktime (t);
      if (tl == -1)
	return -1;		/* can't deal with output from strptime */
      tl += 3600;
    }
  tg = gmtime (&tl);
  tg->tm_isdst = 0;
  tb = mktime (tg);
  if (tb == -1)
    {
      tg->tm_hour--;
      tb = mktime (tg);
      if (tb == -1)
	return -1;		/* can't deal with output from gmtime */
      tb += 3600;
    }
  return (tl - (tb - tl));
}

/* Check whether the result of strptime() indicates success.
   strptime() returns the pointer to how far it got to in the string.
   The processing has been successful if the string is at `GMT' or
   `+X', or at the end of the string.

   In extended regexp parlance, the function returns 1 if P matches
   "^ *(GMT|[+-][0-9]|$)", 0 otherwise.  P being NULL (which strptime
   can return) is considered a failure and 0 is returned.  */
static gint
uc_utils_check_date_end (const gchar * p)
{
  if (!p)
    return 0;
  while (ISSPACE (*p))
    ++p;
  if (!*p
      || (p[0] == 'G' && p[1] == 'M' && p[2] == 'T')
      || ((p[0] == '+' || p[0] == '-') && ISDIGIT (p[1])))
    return 1;
  else
    return 0;
}

time_t
uc_utils_http_atotm (const gchar * time_string)
{
  /* NOTE: Solaris strptime man page claims that %n and %t match white
     space, but that's not universally available.  Instead, we simply
     use ` ' to mean "skip all WS", which works under all strptime
     implementations I've tested.  */

  static const gchar *time_formats[] = {
    "%a, %d %b %Y %T",		/* RFC1123: Thu, 29 Jan 1998 22:12:57 */
    "%A, %d-%b-%y %T",		/* RFC850:  Thursday, 29-Jan-98 22:12:57 */
    "%a, %d-%b-%Y %T",		/* pseudo-RFC850:  Thu, 29-Jan-1998 22:12:57
				   (google.com uses this for their cookies.) */
    "%a %b %d %T %Y"		/* asctime: Thu Jan 29 22:12:57 1998 */
  };

  gint i;
  struct tm t;

  /* According to Roger Beeman, we need to initialize tm_isdst, since
     strptime won't do it.  */
  t.tm_isdst = 0;

  /* Note that under foreign locales Solaris strptime() fails to
     recognize English dates, which renders this function useless.  We
     solve this by being careful not to affect LC_TIME when
     initializing locale.

     Another solution would be to temporarily set locale to C, invoke
     strptime(), and restore it back.  This is slow and dirty,
     however, and locale support other than LC_MESSAGES can mess other
     things, so I rather chose to stick with just setting LC_MESSAGES.

     GNU strptime does not have this problem because it recognizes
     both international and local dates.  */

  if (!time_string)
    return -1;

  for (i = 0; i < G_N_ELEMENTS (time_formats); i++)
    if (uc_utils_check_date_end (strptime (time_string, time_formats[i], &t)))
      return uc_utils_mktime_from_utc (&t);

  /* All formats have failed.  */
  return -1;
}

/* <<<<<<<<<< [END] wget source code 
 * ////////////////////////////////// */

/**
 * uc_utils_get_auth_fields:
 * @g: a #GladeXML object.
 * @w_auth_user: GtkWidget containing auth user string 
 * @w_auth_password: GtkWidget containing auth password string 
 * @auth_user: user will be store here.
 * @auth_password: password will be store here.
 * @auth_line: authentication line will be store here.
 *
 * Get user/password from a authentication dialog and build the line
 * for basic authentication to pass to HTTP headers.
 * 
 * See: uc_utils_build_auth_line ()
 * 
 * Returns: %TRUE if user and password have been entered.
 */
gboolean
uc_utils_get_auth_fields (GladeXML * g,
			  GtkWidget * w_auth_user,
			  GtkWidget * w_auth_password,
			  gchar ** auth_user, gchar ** auth_password,
			  gchar ** auth_line)
{
  gchar *text = NULL;
  gboolean ret = FALSE;

  /* get user */
  if (strlen (
        text = (gchar *) gtk_entry_get_text (GTK_ENTRY (w_auth_user))))
    {
      g_strstrip (text);
      if (strlen (text))
	*auth_user = text;
    }
  /* get password */
  if (strlen (
        text = (gchar *) gtk_entry_get_text (GTK_ENTRY (w_auth_password))))
    {
      g_strstrip (text);
      if (strlen (text))
	*auth_password = text;
    }
  /* build auth line */
  if (*auth_user && *auth_password)
    {
      *auth_line = uc_utils_build_auth_line (*auth_user, *auth_password);
      ret = TRUE;
    }

  return ret;
}

/**
 * uc_utils_build_auth_line:
 * @auth_user: user
 * @auth_password: password
 *
 * Build the line for basic authentication to pass to HTTP headers.
 * 
 * See: uc_utils_get_auth_fields ()
 * 
 * Returns: a new memoty allocated string containing basic authentication
 *          line.
 */
gchar *
uc_utils_build_auth_line (const gchar * auth_user,
			  const gchar * auth_password)
{
  gchar *orig = NULL;
  gchar *base64 = NULL;
  gchar *ret = NULL;

  orig = g_strconcat (auth_user, ":", auth_password, NULL);
  base64 = g_base64_encode ((const guchar *) orig, strlen (orig));
  g_free (orig), orig = NULL;

  ret = g_strconcat ("Authorization: Basic ", base64, "\r\n", NULL);
  g_free (base64), base64 = NULL;

  return ret;

}


/**
 * uc_utils_get_meta_refresh_location:
 * @prop: A #UCLinkProperties node.
 * @tag: A #UCHTMLTag tag.
 *
 * If the page contain a refresh meta tag with a value of 0, replace the 
 * current page URL with it.
 * 
 * Returns: TRUE if there was a REFRESH META tag.
 */
gboolean
uc_utils_get_meta_refresh_location (UCLinkProperties * prop, UCHTMLTag * tag)
{
  GList *item = NULL;
  gchar **tmp = NULL;
  const gchar *p = NULL;
  gboolean ok = TRUE;
  gboolean ret = FALSE;


  if (prop == NULL || prop->metas == NULL)
    return FALSE;

  item = g_list_first (prop->metas);
  while (item != NULL && ok)
  {
    UCHTMLTag *meta = (UCHTMLTag *) item->data;


    if (!g_ascii_strcasecmp (meta->label, "refresh") &&
        meta->value[0] == '0')
    {
      tmp = g_strsplit (meta->value, ";", -1);
      if (tmp[1] != NULL)
      {
        if ((p = strchr (tmp[1], '=')) && ++p)
        {
          g_free (tag->value), tag->value = NULL;
          tag->value = g_strdup (p);
          ret = TRUE;
        }
        ok = FALSE;
      }
      g_strfreev (tmp), tmp = NULL;
    }

    item = g_list_next (item);
  }

  return ret;
}


/**
 * uc_utils_to_utf8:
 * @data: String to proceed.
 *
 * UTF-8 encode a given string.
 * Adapted from screem "screem_support_charset_convert ()"
 * function.
 * 
 * Returns: A new allocated UTF-8 encoded string.
  */
gchar *
uc_utils_to_utf8 (const gchar * data)
{
  const gchar *end;
  gchar *ret = NULL;
  gsize len = 0;

  if (data != NULL)
    {
      if (!g_utf8_validate (data, strlen (data), &end))
	{
	  const gchar *charset;
	  const char *encodings_to_try[2] = { 0, 0 };
	  gboolean utf8 = FALSE;

	  utf8 = g_get_charset (&charset);

	  if (!utf8)
	    encodings_to_try[0] = charset;

	  if (g_ascii_strcasecmp (charset, "ISO-8859-1"))
	    encodings_to_try[1] = "ISO-8859-1";

	  if (encodings_to_try[0])
	    ret = g_convert (data, strlen (data), "UTF-8",
			     encodings_to_try[0], NULL, &len, NULL);

	  if (ret == NULL && encodings_to_try[1])
	    ret = g_convert (data, strlen (data), "UTF-8",
			     encodings_to_try[1], NULL, &len, NULL);
	}
      else
	ret = g_strdup (data);
    }

  if (ret == NULL)
    ret = g_strdup ("");

  return ret;
}

/**
 * uc_utils_get_yesno:
 * @yesno: A string containing "yes" or "no".
 *
 * Proceed a given string to see if it contains "yes" or "no".
 * 
 * Returns: TRUE if the given string is "yes".
 */
gboolean
uc_utils_get_yesno (const gchar * yesno)
{
  return (gboolean) (!g_ascii_strcasecmp (yesno, "yes"));
}

/**
 * uc_utils_debug:
 * @format: The message format.
 * @...: The parameters to insert into the format string.
 * 
 * Display debug informations.
 */
void
uc_utils_debug (const gchar * format, ...)
{
  va_list ap;
  gchar *s = NULL;

  if (!uc_project_get_debug_mode ())
    return;

  s = (gchar *) g_malloc0 (2048 + 1);

  va_start (ap, format);
  vsnprintf (s, 2048, format, ap);
  va_end (ap);

  g_print ("DEBUG: %s", s);

  g_free (s), s = NULL;
}

/**
 * uc_utils_debug_dump_link_properties:
 * @prop: A #UCLinkProperties node.
 * 
 * Dump the link properties structure to the standard
 * output.
 */
void
uc_utils_debug_dump_link_properties (const UCLinkProperties * prop)
{
  if (!uc_project_get_dump_properties ())
    return;

  g_print ("\n\n"
	   "	uid: %u\n"
           "	treeview_path: %s\n"
	   "	current_path: %s\n"
	   "	link_type: %u\n"
	   "	user_action: %s\n"
	   "	link_value: %s\n"
	   "	url: %s\n"
	   "	normalized_url: %s\n"
	   "	label: %s\n"
	   "	protocol: %s\n"
	   "	h_name: %s\n"
	   "	port: %s\n"
	   "	path: %s\n"
	   "	args: %s\n"
	   "	domain: %s\n"
	   "	header_size: %u\n"
	   "	depth_level: %u\n"
	   "	is_parsable: %u\n"
	   "	is_downloadable: %u\n"
	   "	checked: %u\n"
	   "	bookmark_folder_id: %i\n"
	   "	bookmark_id: %i\n"
	   "	to_delete: %u\n"
	   "	metas: %u\n"
	   "	emails: %u\n"
	   "	childs: %u\n"
	   "	similar_links_parents: %u\n"
	   "	bad_extensions: %u\n"
	   "	virii: %u\n"
	   "	w3c_valid: %u\n",
	   prop->uid,
	   (prop->treeview_path)?prop->treeview_path:"",
	   (prop->current_path)?prop->current_path:"",
	   prop->link_type,
	   (prop->user_action)?prop->user_action:"",
	   (prop->link_value)?prop->link_value:"",
	   (prop->url)?prop->url:"",
	   (prop->normalized_url)?prop->normalized_url:"",
	   (prop->label)?prop->label:"",
	   (prop->protocol)?prop->protocol:"",
	   (prop->h_name)?prop->h_name:"",
	   (prop->port)?prop->port:"",
	   (prop->path)?prop-> path:"",
	   (prop->args)?prop->args:"",
	   (prop->domain)?prop->domain:"",
	   prop->header_size,
	   prop->depth_level,
	   prop->is_parsable,
	   prop->is_downloadable,
	   prop->checked,
           prop->bookmark_folder_id,
           prop->bookmark_id,
	   prop->to_delete,
	   (prop->metas != NULL),
	   (prop->emails != NULL),
	   (prop->childs != NULL),
	   (prop->similar_links_parents != NULL),
	   (prop->bad_extensions != NULL),
           prop->virii,
           prop->w3c_valid
        );
}

/**
 * uc_utils_set_userfriendly_treeview_column:
 * @tv: Tree view to work with.
 * @position: Position of the column to work with.
 * 
 * Set a treeview column sortable and resizable.
 */
void
uc_utils_set_userfriendly_treeview_column (const GtkTreeView * tv,
					   const gint32 position)
{
  GtkTreeViewColumn *column = NULL;

  column = gtk_tree_view_get_column (GTK_TREE_VIEW (tv), position);
  if (column == NULL)
    return;

  gtk_tree_view_column_set_sort_column_id (column, position);
  gtk_tree_view_column_set_resizable (column, TRUE);
}

/**
 * uc_utils_get_file_content:
 * @path: Path of the file.
 * @length: Pointer to fill with file length.
 * 
 * Open and read a file to return its content.
 * 
 * Returns: A new allocated string with the content of a the file. 
 *          The @length argument contain the file length.
 */
gchar *
uc_utils_get_file_content (const gchar * path, gsize * length)
{
  gchar *buffer = NULL;
  GError *error = NULL;

  g_file_get_contents (path, &buffer, &(*length), &error);

  return buffer;
}

/* FIXME */
/**
 * uc_utils_email_is_valid:
 * @email: E-Mail to check.
 * @check_mx: TRUE if we also must check the MX.
 * 
 * Check E-Mail syntax and MX if required.
 *
 * Returns: The #UCEmailStatus status of the E-Mail.
 *          Return codes are:
 *          - 0: bad syntax (UC_EMAIL_SYNTAX_BAD)
 *          - 1: syntax ok but MX not respond (UC_EMAIL_SYNTAX_MX_BAD)
 *          - 2: syntax ok and MX ok (UC_EMAIL_OK)
 */
UCEmailStatus
uc_utils_email_is_valid (const gchar * email, const gboolean check_mx)
{
  gchar *tmp = NULL;
  gchar *user = NULL;
  gchar *domain = NULL;
  gchar *email_tmp = NULL;
  gchar *mx = NULL;
  guint ret = UC_EMAIL_SYNTAX_BAD;

  uc_utils_split_email (email, &user, &domain);
  if (strlen (user) == 0 || strlen (domain) == 0)
    {
      g_free (user), user = NULL;
      g_free (domain), domain = NULL;

      return UC_EMAIL_SYNTAX_BAD;
    }

  email_tmp = g_strconcat (user, "@", domain, NULL);

  /* dirty syntax control :-( */
  if ((email_tmp == NULL) ||
      (strlen (email_tmp) == 0) ||
      (strchr ("@.", email_tmp[0])) ||
      (strchr ("@.", email_tmp[strlen (email_tmp) - 1])) ||
      (strchr (email_tmp, ' ')) || ((tmp = strchr (email_tmp, '@')) == NULL)
      || ((tmp = strchr (email_tmp, '.'))) == NULL)
    {
      g_free (email_tmp), email_tmp = NULL;

      return UC_EMAIL_SYNTAX_BAD;
    }

  /* check if MX is alive and listening on
   * port 25 */
  if (check_mx)
    {
      mx = uc_utils_get_mx (domain);

      ret =
	(uc_utils_mx_is_valid (mx)) ? UC_EMAIL_OK : UC_EMAIL_SYNTAX_MX_BAD;

      g_free (mx), mx = NULL;
    }
  else
    ret = UC_EMAIL_OK;

  g_free (user), user = NULL;
  g_free (domain), domain = NULL;

  return ret;
}

/**
 * uc_utils_split_email:
 * @email: E-Mail to work with.
 * @user: Pointer for the "user" part of the E-Mail.
 * @domain: Pointer for the "domain" part of the E-Mail.
 * 
 * Fill the two parts of a E-Mail address. @user and @domain argument are
 * filled with new allocated strings.
 */
void
uc_utils_split_email (const gchar * email, gchar ** user, gchar ** domain)
{
  const gchar *sep = NULL;

  if ((sep = strchr (email, '@')))
    {
      *user = uc_utils_strdup_delim (email, sep);
      *domain = uc_utils_strdup_delim (sep + 1, &email[strlen (email)]);
    }
  else
    {
      *user = g_strdup ("");
      *domain = g_strdup ("");
    }
}

/**
 * uc_utils_get_mx:
 * @domain: Domain from which returning the corresponding MX.
 * 
 * Return the prefered MX for a given domain.
 * Function adapted from 
 * http://www.sslug.dk/emailarkiv/cprog/1999_10/msg00056.html
 *
 * Returns: A new allocated string with the corresponding MX.
 */
gchar *
uc_utils_get_mx (const gchar * domain)
{
  struct rrecord
  {
    gint16 r_type;
    gint16 r_class;
    guint32 r_ttl;
    gint16 r_length;
  };

  static gchar out[1000];
  guchar query[1000];
  HEADER *h;
  guchar *p;
  gint32 qcount, acount;
  gchar buf[1000];
  gint32 best;
  struct rrecord *r;

  /* ask libc to do the query:
   * look up 'domain'
   * class C_IN (internet)
   * type  T_MX (mail exchanger record)
   * record results in query, max len 1000
   */
  if (res_search (domain, C_IN, T_MX, query, (int) 1000) < 0)
    return g_strdup (domain);

  h = (HEADER *) query;

  /* skip over queries in packet */
  p = query + sizeof (HEADER);
  qcount = ntohs (h->qdcount);	/* # of queries present */
  acount = ntohs (h->ancount);	/* # of answers present */
  /* get past queries */
  while (qcount--)
    {
      p += dn_expand (query, query + 1000, p, buf, 1000);
      p += 4;			/* qclass, qtype */
    }

  /* now we are in the answers section. Just get the best exchanger,
   * we don't want to spend too much time messing around
   */
  best = 99999;
  out[0] = 0;
  while (acount--)
    {
      /* name that this answer refers to */
      p += dn_expand (query, query + 1000, p, buf, 1000);

      r = (struct rrecord *) p;
      if (htons (r->r_type) == T_MX)
	{			/* MX record */
	  gint32 pref;
	  guchar *p1 = p + 10;

	  pref = ntohs (*(gint16 *) p1);	/* preference */
	  p1 += 2;
	  if (pref < best)
	    {
	      /* get exchanger name */
	      dn_expand (query, query + 1000, p1, out, 1000);
	      best = pref;
	    }
	}
      p = p + 10 + ntohs (r->r_length);	/* skip RR header and data */
    }

  return (out[0]) ? g_strdup (out) : g_strdup (domain);
}

/**
 * uc_utils_mx_is_valid:
 * @mx: The MX to check.
 * 
 * Check if a given MX is valid or not.
 * 
 * Returns: TRUE if the given MX is alive and listen on port 25.
 */
gboolean
uc_utils_mx_is_valid (const gchar * mx)
{
  GTcpSocket *socket = NULL;
  gboolean ret = FALSE;

  socket = gnet_tcp_socket_connect (mx, 25);

  if (socket != NULL)
    {
      ret = TRUE;
      gnet_tcp_socket_delete (socket), socket = NULL;
    }

  return ret;
}


/**
 * uc_utils_get_ip:
 * @host: Host to work with.
 * 
 * Return the IP of the given host. if arg is already an IP,
 * return it as is.
 *
 * Returns: A new allocated string with the IP.
 */
gchar *
uc_utils_get_ip (const gchar * host)
{
  struct hostent *serverHostEnt = NULL;
  gchar *buffer = NULL;


  if ((serverHostEnt = gethostbyname (host)) != NULL)
    buffer = g_strdup (
      inet_ntoa (*((struct in_addr *) serverHostEnt->h_addr_list[0])));
  else if (inet_addr (host) != -1)
    buffer = g_strdup (host);

  return buffer;
}


/**
 * uc_utils_string_format4display:
 * @label: Label to format.
 * @size: Size limit.
 *
 * format a given string to be displayed in UI.
 *
 * See: uc_utils_string_cut()
 * 
 * Returns: A new allocated string with the new formated label.
 */
gchar *
uc_utils_string_format4display (const gchar * label, const gsize size)
{
  gchar *ret = NULL;
  gchar *tmp = NULL;

  tmp = g_strdup (label);

  if (tmp != NULL)
    uc_utils_replacelr (g_strstrip (tmp), ' ');

  ret = uc_utils_string_cut (tmp, size);

  g_free (tmp), tmp = NULL;

  return ret;
}

/**
 * uc_utils_string_cut:
 * @label: Label to cut.
 * @size: Size limit.
 *
 * Cut a given string to fit with the given length.
 * 
 * See: uc_utils_string_format4display()
 * 
 * Returns: A new allocated string with the cutted label.
 */
gchar *
uc_utils_string_cut (const gchar * label, const gsize size)
{
  gchar *ret = NULL;
  gchar *tmp = NULL;

  tmp = g_strdup (label);

  /*
     if (tmp != NULL && size > 0 && strlen (tmp) > size - 3)
     {
     gchar *tmp1 = NULL;

     tmp1 = g_malloc0 (size + 3 + 1);
     memcpy (tmp1, tmp, size);
     strncat (tmp1, "...", 3);

     g_free (tmp), tmp = NULL;
     tmp = tmp1;
     }
     Logic should be changed :
     - max label size is size + 3 instead of size
     - if strlen(tmp)==size the trailing /0 won't be copied and strncat
     will search for the next /0 in the memory before writing... */
  if (size >= 3)
    {
      if (tmp != NULL && strlen (tmp) > size)
	{
	  gchar *tmp1 = NULL;

	  tmp1 = g_malloc0 (size + 1);
	  memcpy (tmp1, tmp, size - 3);
	  tmp1[size - 3] = '\0';
	  strncat (tmp1, "...", 3);

	  g_free (tmp), tmp = NULL;
	  tmp = tmp1;
	}
    }
  else
    goto cleanout;

  ret = uc_utils_to_utf8 (tmp);

cleanout:
  g_free (tmp), tmp = NULL;

  return ret;
}

/**
 * uc_utils_replace:
 * @str: String to work with.
 * @old: String to replace.
 * @new: New string to replace old with.
 * 
 * Replace one or more characters in a given string.
 *
 * See: uc_utils_replace1()
 * 
 * Returns: A new allocated string.
 */
gchar *
uc_utils_replace (const gchar * str, const gchar * old, const gchar * new)
{
  guint32 i = 0;
  guint32 str_len = 0;
  guint32 old_len = 0;
  GString *tmp = NULL;

  if (!(str && old && new))
    {
      g_warning ("Input data is NULL!");
      return NULL;
    }

  str_len = strlen (str);
  old_len = strlen (old);

  tmp = g_string_new ("");

  for (i = 0; i < str_len; i++)
    if (!memcmp (&str[i], old, old_len))
      tmp = g_string_append (tmp, new), i += old_len - 1;
    else
      tmp = g_string_append_c (tmp, str[i]);

  return g_string_free (tmp, FALSE);
}


/**
 * uc_utils_replace1:
 * @string: String to work with.
 * @c1: Character to replace.
 * @c2: Replacement.
 * 
 * Replace a caracter by another (but do not allocate new memory for the 
 * result string).
 *
 * See: uc_utils_replace()
 * 
 * Returns: The modified string.
 */
gchar *
uc_utils_replace1 (gchar * string, const gchar c1, const char c2)
{
  gchar *p = string;


  do
  {
    if (*p == c1)
      *p = c2;
  }
  while (*p++);

  return string;
}


/**
 * uc_utils_replacelr:
 * @string: String to work with.
 * @c: Character for LR replacement.
 * 
 * Remove all line return in a given string.
 *
 * Returns: The modified string.
 */
gchar *
uc_utils_replacelr (gchar * string, const gchar c)
{
  gchar *p = string;

  do
    {
      if (*p == '\n' || *p == '\r')
	*p = c;
    }
  while (*p++);

  return string;
}

/**
 * uc_utils_convert_uid2file:
 * @uid: The uid of a #UCLinkProperties node.
 * 
 * Convert a cache uid to filename.
 *
 * Returns: The corresponding path. A new allocated string.
 */
gchar *
uc_utils_convert_uid2file (const guint32 uid)
{
  return g_strdup_printf ("%s/%s/%d",
			  uc_project_get_working_path (),
			  uc_project_get_cache_name (), uid);;
}


/**
 * uc_utils_get_gnome_browser_conf:
 * 
 * Get the current default gnome web browser or "mozilla" if it
 * does not find it.
 * 
 * Returns: The current gnome default web browser
 */
gchar *
uc_utils_get_gnome_browser_conf (void)
{
  GConfClient *client = NULL;
  gchar *ret = NULL;


  client = gconf_client_get_default ();
  ret = gconf_client_get_string (
          client, "/desktop/gnome/url-handlers/http/command", NULL);
  g_object_unref (client), client = NULL;

  return (ret == NULL) ? g_strdup ("mozilla") : ret;
}


/**
 * uc_utils_get_gnome_proxy_conf:
 * @host: A string to fill for the host.
 * @port: A integer to fill for the port.
 * 
 * Fill given args with the gconf values for the gnome network proxy 
 * settings.
 */
void
uc_utils_get_gnome_proxy_conf (gchar ** host, guint * port)
{
  GConfClient *client = NULL;

  client = gconf_client_get_default ();

  if (gconf_client_get_bool
      (client, "/system/http_proxy/use_http_proxy", NULL))
    {
      *host =
	gconf_client_get_string (client, "/system/http_proxy/host", NULL);
      *port = gconf_client_get_int (client, "/system/http_proxy/port", NULL);

      if (*port == 0)
	*port = 8080;
    }

  if (*host == NULL)
    *host = g_strdup ("");

  g_object_unref (client), client = NULL;
}


/**
 * uc_utils_clean_tag_link_value:
 * @value: String to clean.
 * 
 * Clean a link value string.
 *
 * Returns: A cleaned value. A new allocated string.
 */
gchar *
uc_utils_clean_tag_link_value (gchar * value)
{
  GString *buffer = NULL;
  const gchar *p = value;

  g_return_val_if_fail (value != NULL, g_strdup (""));

  buffer = g_string_new ("");
  while (*p)
    {
      if (*p != '\'' && *p != '"' && *p != '>')
	g_string_append_c (buffer, *p);
      p++;
    }

  return g_string_free (buffer, FALSE);
}


/**
 * uc_utils_search_string_next:
 * @buf: A #gpointer to the source buffer.
 * @str: The string to search.
 * @limit_char: The char to stop if found.
 * 
 * Search for the next char after the searched string.
 *
 * Returns: If found, retrun a pointer on the next char,
 *          else return the original pointer.
 */
gpointer
uc_utils_search_string_next (gpointer buf, const gchar * str,
			     const gchar limit_char)
{
  gpointer sav = buf;
  gchar *buffer = (gchar *) buf;


  if (str == NULL || strlen (str) == 0 || buffer == NULL
      || strlen (buffer) < strlen (str))
    return (gpointer) buf;

  while (*buffer != '\0' &&
	 !uc_utils_memcasecmp (buffer, str) && *buffer != limit_char)
    buffer++;

  if (*buffer != limit_char)
    sav = (buffer += strlen (str));

  return sav;
}


/**
 * uc_utils_memcasecmp:
 * @str1: First string to compare.
 * @str2: Second string to compare.
 * 
 * Comparison between two strings, based on the length of
 * the second string.
 *
 * Returns: %TRUE if the beginning of %str1 match with %str2.
 */
gboolean
uc_utils_memcasecmp (const gchar * str1, const gchar * str2)
{
  gchar *tmp = NULL;
  guint32 len = 0;
  gboolean ret = FALSE;


  if (str1 == NULL || str2 == NULL)
    return FALSE;

  len = strlen (str2);

  if (strlen (str1) < len)
    return FALSE;

  if (memcmp (str1, str2, len) == 0)
    return TRUE;

  tmp = g_malloc0 (len + 1);
  memcpy (tmp, str1, len);
  ret = (g_ascii_strcasecmp (tmp, str2) == 0);
  g_free (tmp), tmp = NULL;

  return ret;
}


/**
 * uc_utils_mkdirs:
 * @path: Path to create.
 * @create_all: %TRUE if we must create intermediar paths (mkdir -p).
 * 
 * Make given path -- create intermediate paths if they don't exist -- 
 * and test if we are authorized to write in.
 *
 * Returns: %TRUE if all is ok.
 */
gboolean
uc_utils_mkdirs (const gchar * path, const gboolean create_all)
{
  gchar test_path[UC_BUFFER_LEN + 1] = { 0 };
  gchar **paths = NULL;
  gboolean ret = FALSE;
  gint32 fd;
  gint32 i = 0;

  if (create_all)
    {
      paths = g_strsplit (path, "/", 255);
      while (paths[i] != NULL)
	{
	  if (strlen (paths[i]) > 0)
	    {
	      strncat (test_path, "/", UC_BUFFER_LEN - strlen (test_path));
	      if (strncat
		  (test_path, paths[i],
		   UC_BUFFER_LEN - strlen (test_path)) == NULL)
		{
		  g_strfreev (paths), paths = NULL;
		  return FALSE;
		}

	      mkdir (test_path, S_IRWXU);
	    }
	  i++;
	}
    }
  else
    {
      mkdir (path, S_IRWXU);
      strncat (test_path, path, UC_BUFFER_LEN - strlen (test_path));
    }

  strncat (test_path, "/filetest", UC_BUFFER_LEN - strlen (test_path));
  fd = open (test_path, O_CREAT, S_IRUSR | S_IWUSR);
  if (fd >= 0)
    {
      close (fd);
      unlink (test_path);
      ret = TRUE;
    }

  g_strfreev (paths), paths = NULL;
  return ret;
}

/**
 * uc_utils_rmfiles:
 * @path: The path in witch deleting files.
 * 
 * Remove all files in a given directory.
 */
void
uc_utils_rmfiles (const gchar * path)
{
  DIR *dir = NULL;
  struct dirent *file = NULL;

  dir = opendir (path);
  if (dir == NULL)
    return;

  while ((file = readdir (dir)) != NULL)
    {
      gchar *file_path = g_strconcat (path, "/", file->d_name,
				      NULL);
      unlink (file_path);
      g_free (file_path), file_path = NULL;
    }
  closedir (dir);
}

/**
 * uc_utils_copy:
 * @src: Source file.
 * @dest: Destination file.
 *
 * Copy a file from %src to %dest.
 */
void
uc_utils_copy (const gchar * src, const gchar * dest)
{
  gchar *buffer = NULL;
  gsize length = 0;
  FILE *fd = NULL;

  g_assert (g_file_test (src, G_FILE_TEST_EXISTS));

  g_file_get_contents (src, &buffer, &length, NULL);
  fd = fopen (dest, "w");
  fwrite (buffer, 1, length, fd);
  fclose (fd);

  g_free (buffer), buffer = NULL;
}

/**
 * uc_utils_copy_files:
 * @src: Source path.
 * @dest: Destination path.
 * 
 * Copy all the file from a given directorie in another.
 */
void
uc_utils_copy_files (const gchar * src, const gchar * dest)
{
  DIR *dir = NULL;
  struct dirent *file = NULL;
  FILE *fd = NULL;
  gchar *file_src = NULL;
  gchar *file_dest = NULL;

  dir = opendir (src);
  if (dir == NULL)
    return;

  while ((file = readdir (dir)))
    {
      gchar *buffer = NULL;
      gsize length = 0;

      if (!strcmp (file->d_name, ".") || !strcmp (file->d_name, ".."))
	continue;

      file_src = g_strconcat (src, "/", file->d_name, NULL);
      file_dest = g_strconcat (dest, "/", file->d_name, NULL);

      g_file_get_contents (file_src, &buffer, &length, NULL);
      g_assert (buffer != NULL);
      fd = fopen (file_dest, "w");
      g_assert (fd != NULL);
      fwrite (buffer, 1, length, fd);
      fclose (fd);

      g_free (file_src), file_src = NULL;
      g_free (file_dest), file_dest = NULL;
      g_free (buffer), buffer = NULL;
    }

  closedir (dir);
}

/**
 * uc_utils_rmdirs:
 * @path: The path to remove.
 * @delete_all: If %TRUE, remove all intermediar paths.
 * 
 * Remove a given path -- remove intermediate paths too.
 */
void
uc_utils_rmdirs (const gchar * path, const gboolean delete_all)
{
  gchar **paths = NULL;
  gint32 i = 0;

  if (delete_all)
    {
      paths = g_strsplit (path, "/", 255);
      i = uc_utils_vector_length (paths);
      while (i >= 0)
	{
	  gchar dir[UC_BUFFER_LEN + 1] = { 0 };
	  guint j = 0;

	  for (j = 0; j < i; j++)
	    {
	      if (strlen (paths[j]) > 0)
		{
		  if (strncat (dir, "/",
			       UC_BUFFER_LEN - strlen (dir)) == NULL ||
		      strncat (dir, paths[j],
			       UC_BUFFER_LEN - strlen (dir)) == NULL)
		    {
		      g_strfreev (paths), paths = NULL;
		      return;
		    }
		}
	    }

	  uc_utils_rmfiles (dir);
	  rmdir (dir);

	  i--;
	}

      g_strfreev (paths), paths = NULL;
    }
  else
    {
      uc_utils_rmfiles (path);
      rmdir (path);
    }
}

/**
 * uc_utils_vector_length:
 * @data: A vector.
 * 
 * Calculate the length of the given vector.
 * 
 * Returns: The length of a given vector.
 */
guint
uc_utils_vector_length (gpointer data)
{
  guint32 i = 0;
  gchar **vector = (gchar **) data;

  while (vector[i++]);

  return --i;
}

/**
 * uc_utils_test_socket_open:
 * @sock: The socket to test.
 * 
 * Check if a connection if alive or not.
 * -> taken from wget source code
 *
 *  Returns: %TRUE if it is alive.
 */
gboolean
uc_utils_test_socket_open (const guint sock)
{
  fd_set check_set;
  struct timeval to;
  gboolean ret = FALSE;

  /* Check if we still have a valid (non-EOF) connection.  From Andrew
   * Maholski's code in the Unix Socket FAQ.  */

  FD_ZERO (&check_set);
  FD_SET (sock, &check_set);

  /* Wait one microsecond */
  to.tv_sec = 0;
  to.tv_usec = 1;

  /* If we get a timeout, then that means still connected */
  if (select (sock + 1, &check_set, NULL, NULL, &to) == 0)
    {
      /* Connection is valid (not EOF), so continue */
      ret = TRUE;
    }

  return ret;
}

/**
 * uc_utils_strdup_delim:
 * @begin: The source position.
 * @end: The end position.
 * 
 * Make a string with 2 given pointers.
 *
 * Returns: A new allocated string.
 */
gchar *
uc_utils_strdup_delim (const gchar * begin, const gchar * end)
{
  gchar *res = NULL;

  res = (gchar *) g_malloc0 (end - begin + 1);
  memcpy (res, begin, end - begin);

  return res;
}

/**
 * uc_utils_strpbrk_or_eos:
 * @str: A string.
 * @accept: A string with characters to search for.
 * 
 * Return a pointer on a given char address or on the end of 
 * the passed string.
 *
 * Returns: A pointer on the new offset.
 */
gchar *
uc_utils_strpbrk_or_eos (const gchar * str, const gchar * accept)
{
  gchar *p = NULL;

  p = strpbrk (str, accept);
  if (p == NULL)
    p = (gchar *) str + strlen (str);

  return p;
}

/**
 * uc_utils_get_server_from_header_field:
 * @field: A string from the "Server" HTTP header field.
 *
 * Extract server name/type from a HTTP header "Server" field.
 *
 * Returns: A string with the HTTP server. A new allocated string.
 */
gchar *
uc_utils_get_server_from_header_field (gchar * field)
{
  const gchar *b = NULL;
  const gchar *e = NULL;

  b = field;
  e = strchr (b, ' ');

  if (e == NULL)
    return g_strdup (b);

  if (*(e + 1) == '(')
    e++, e = strchr (e, ')');

  return uc_utils_strdup_delim (b, e + 1);
}


/**
 * uc_utils_get_string_from_size:
 * @size: size to convert in string
 *
 * Convert a size in string format (with unit indication (KB, MB...)).
 * 
 * Returns: a string representation of the given size @size.
 */
gchar *
uc_utils_get_string_from_size (const gsize size)
{
  gchar *ret = NULL;


  if (size < 1024)
    ret = g_strdup_printf (_("%lu bytes"), (unsigned long) size);
  else if (size < (1024 * 1024))
    ret = g_strdup_printf (_("%.2f KB (%lu bytes)"),
			   (gfloat) (size / 1024.0), (unsigned long) size);
  else
    ret = g_strdup_printf (_("%.2f MB (%lu bytes)"),
			   (gfloat) (size / (1024.0 * 1024.0)),
			   (unsigned long) size);

  return ret;
}


gboolean
uc_utils_str_is_not_alphanum (const gchar *str)
{
  while (*str)
  {
    if (g_ascii_isalnum (*str) || g_ascii_isalpha (*str))
      return FALSE;
    ++str;
  }

  return TRUE;
}


/**
 * uc_utils_ftp_code_search:
 * @buffer: a buffer containing some FTP output stuff.
 * @code: the FTP code to search for (string format).
 * @len: length of the given buffer @buffer.
 *
 * Search for a given FTP code in a buffer.
 * 
 * Returns: %TRUE if the given FTP code @code has been found
 * in @buffer.
 */
gboolean
uc_utils_ftp_code_search (const gchar * buffer, const gchar * code,
			  const gint len)
{
  if (len < strlen (code))
    return FALSE;

  return (!memcmp (buffer, code, strlen (code)));
}

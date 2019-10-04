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
#include "check.h"
#include "application.h"
#include "lists.h"
#include "project.h"

#include "tooltips.h"

/**
 * UCTTMain:
 * @x: X mouse coord.
 * @y: Y mouse coord.
 * @active: %TRUE if tooltip is being displayed.
 * @out: %TRUE is the mouse is out of the tooltip window.
 * @last_id: #UCLinkProperties Id of the last row for which we displayed 
 *           tooltip in the main tree view.
 * @current_id: #UCLinkProperties Id of the current row for which we are
 *              displaying tooltip.
 *
 * Structure for the tooltips object.
 */
typedef struct _UCTTMain UCTTMain;
struct _UCTTMain
{
  gint32 x;
  gint32 y;
  gboolean active;
  gboolean out;
  guint32 last_id;
  guint32 current_id;
};

static UCTTMain uc_tooltips_main;
static GtkWidget *treeview_tooltip = NULL;
static guint timer_id = 0;
static gboolean uc_tooltips_display_tooltips_value = TRUE;
static gboolean uc_tooltips_main_cb (gpointer data);

/**
 * uc_tooltips_main_set_display:
 * @value: %TRUE if we must display tooltips.
 *
 * Set it user want that tooltips be displayed or not.
 */
void
uc_tooltips_main_set_display (const gboolean value)
{
  uc_tooltips_display_tooltips_value = value;
}

/**
 * uc_tooltips_main_get_display:
 *
 * Check if we can display tooltips.
 * 
 * Returns: %TRUE if we must display tooltips.
 */
gboolean
uc_tooltips_main_get_display (void)
{
  return uc_tooltips_display_tooltips_value;
}

/**
 * uc_tooltips_init:
 *
 * Initialize tooltips object.
 */
void
uc_tooltips_init (void)
{
  if (timer_id != 0)
    return;

  timer_id = g_timeout_add (UC_TOOLTIPS_DELAY_DEFAULT,
			    &uc_tooltips_main_cb, NULL);

  uc_tooltips_main_set_mouse_coord (0, 0);
  uc_tooltips_main_set_active (FALSE);
  uc_tooltips_main_set_frozen (FALSE);
  uc_tooltips_main_set_last_id (0);
  uc_tooltips_main_set_current_id (0);
  uc_tooltips_main_set_display (FALSE);
}

/**
 * uc_tooltips_main_cb:
 * @data: Always %NULL.
 *
 * Callback to manage tooltip show/hide.
 *
 * Returns: Always %TRUE.
 */
static gboolean
uc_tooltips_main_cb (gpointer data)
{
  GtkWidget *widget = NULL;
  GtkTreePath *treepath = NULL;
  GValue *value = NULL;
  gchar *str = NULL;
  UCLinkProperties *prop = NULL;
  gint32 x = 0;
  gint32 y = 0;
  GtkTreeIter iter;

  if (treeview == NULL ||
      !uc_tooltips_main_get_display () ||
      uc_tooltips_main_get_frozen () ||
      uc_lists_checked_links_is_empty () ||
      (uc_project_get_url () == NULL &&
       !uc_project_get_check_is_bookmarks ()))
    return TRUE;

  uc_tooltips_main_get_mouse_coord (&x, &y);
  gtk_tree_view_get_path_at_pos (treeview, x, y, &treepath, NULL, NULL, NULL);

  if (treepath == NULL)
    {
      uc_tooltips_main_destroy ();

      return TRUE;
    }

  gtk_tree_model_get_iter (GTK_TREE_MODEL (treestore), &iter, treepath);
  gtk_tree_path_free (treepath), treepath = NULL;

  value = g_new0 (GValue, 1);
  gtk_tree_model_get_value (GTK_TREE_MODEL (treestore), &iter, 0, value);
  uc_tooltips_main_set_current_id (g_value_get_int (value));
  g_value_unset (value);

  prop =
    uc_lists_checked_links_lookup_by_uid (uc_tooltips_main_get_current_id ());

  if (prop == NULL || prop->link_type == LINK_TYPE_BOOKMARK_FOLDER)
    {
      uc_tooltips_main_destroy ();

      return TRUE;
    }

  if (!uc_tooltips_main_get_active ())
    {

      uc_tooltips_main_set_active (TRUE);
      uc_tooltips_main_set_last_id (uc_tooltips_main_get_current_id ());

      treeview_tooltip = WGET ("treeview_tooltip");

      if (!uc_tooltips_main_get_frozen ())
	{
	  gint32 wx = 0;
	  gint32 wy = 0;
	  gint32 ph = 0;
	  gint32 pw = 0;
	  gchar *lm_date = NULL;
	  gchar *parent_url = NULL;
	  gchar *url = NULL;
	  GtkWidget *w = treeview_tooltip;
	  UCStatusCode *sc = NULL;
	  GdkColor color;

	  parent_url =
	    (prop->parent != NULL &&
	     prop->parent->link_type != LINK_TYPE_BOOKMARK_FOLDER) ?
	    uc_utils_replace (prop->parent->url, "&",
			      "&amp;") : g_strdup ("-");
	  url = uc_utils_replace (prop->url, "&", "&amp;");
	  lm_date = g_hash_table_lookup (prop->header, UC_LAST_MODIFIED);
	  if (lm_date == NULL)
	    lm_date = "-";

	  sc =
	    uc_application_get_status_code_properties (g_hash_table_lookup
						       (prop->header,
							UC_HEADER_STATUS));

	  gdk_color_parse (sc->color, &color);
	  gtk_widget_modify_bg (w, GTK_STATE_NORMAL, &color);

	  widget = WGET ("tt_image");
	  gtk_image_set_from_file (GTK_IMAGE (widget),
				   uc_check_get_link_type_icon_path
				   (prop->link_type, prop->protocol));

	  widget = WGET ("tt_type");
	  str = g_strdup_printf ("<b>%s</b>",
				 uc_check_get_link_type_label
				 (prop->link_type));
	  gtk_label_set_markup (GTK_LABEL (widget), str);
	  g_free (str), str = NULL;

	  widget = WGET ("tt_label");
	  str = g_strdup_printf (_("<b>Status</b>: %s\n"
				   "<b>Location</b>: %s\n"
				   "<b>Parent location</b>: %s\n"
				   "<b>Last modified</b>: %s"),
				 sc->message, url, parent_url, lm_date);
	  gtk_label_set_markup (GTK_LABEL (widget), str);

	  g_free (str), str = NULL;
	  g_free (parent_url), parent_url = NULL;
	  g_free (url), url = NULL;

	  gtk_window_get_size (GTK_WINDOW (w), &pw, &ph);
	  gtk_window_get_position (GTK_WINDOW (w), &wx, &wy);
	  gtk_window_move (GTK_WINDOW (w), wx, wy + (ph / 2) + 10);

	  gtk_widget_show_all (w);
	}
    }
  else if (uc_tooltips_main_get_last_id () !=
	   uc_tooltips_main_get_current_id ())
    {
      uc_tooltips_main_set_active (FALSE);
      uc_tooltips_main_destroy ();
    }

  return TRUE;
}

/**
 * uc_tooltips_main_get_active:
 *
 * Check if a tooltip is being displayed.
 *
 * Returns: %TRUE if a tooltip is being displayed.
 */
gboolean
uc_tooltips_main_get_active (void)
{
  return uc_tooltips_main.active;
}

/**
 * uc_tooltips_main_set_active:
 * @value: %TRUE if a tooltip is active.
 *
 * Tell if a tooltip is active or not.
 */
void
uc_tooltips_main_set_active (const gboolean value)
{
  uc_tooltips_main.active = value;
}

/**
 * uc_tooltips_main_get_current_id:
 *
 * Get the current id of a #UCLinkProperties object currently displayed
 * in a tooltip.
 * 
 * Returns: Id of the #UCLinkProperties object currently displayed
 *          in the tooltip.
 */
guint32
uc_tooltips_main_get_current_id (void)
{
  return uc_tooltips_main.current_id;
}

/**
 * uc_tooltips_main_set_current_id:
 * @id: Id of the #UCLinkProperties object currently displayed in the tooltip.
 *
 * Tell the id of the #UCLinkProperties object currently displayed in 
 * the tooltip.
 */
void
uc_tooltips_main_set_current_id (const guint32 id)
{
  uc_tooltips_main.current_id = id;
}

/**
 * uc_tooltips_main_get_last_id:
 *
 * Get the id of the #UCLinkProperties object previously displayed in
 * the tooltip.
 * 
 * Returns: The last #UCLinkProperties id.
 */
guint32
uc_tooltips_main_get_last_id (void)
{
  return uc_tooltips_main.last_id;
}

/**
 * uc_tooltips_main_set_last_id:
 * @id: #UCLinkProperties id to set.
 *
 * Set the id of the #UCLinkProperties object previously displayed in
 * the tooltip.
 */
void
uc_tooltips_main_set_last_id (const guint32 id)
{
  uc_tooltips_main.last_id = id;
}

/**
 * uc_tooltips_main_set_mouse_coord:
 * @x: X mouse coord.
 * @y: Y mouse coord.
 *
 * Set the current mouse coord.
 */
void
uc_tooltips_main_set_mouse_coord (const gint32 x, const gint32 y)
{
  uc_tooltips_main.x = x;
  uc_tooltips_main.y = y;
}

/**
 * uc_tooltips_main_destroy:
 *
 * Destroy a tooltip window.
 */
void
uc_tooltips_main_destroy (void)
{
  if (treeview_tooltip != NULL && GTK_IS_WIDGET (treeview_tooltip))
    gtk_widget_hide (treeview_tooltip);
}

/**
 * uc_tooltips_main_get_mouse_coord:
 * @x: X mouse coord. to be set.
 * @y: Y mouse coord. to be set.
 *
 * Get the mouse coord. @X and @Y parameters are modified.
 */
void
uc_tooltips_main_get_mouse_coord (gint32 * x, gint32 * y)
{
  if (x != NULL)
    *x = uc_tooltips_main.x;

  if (y != NULL)
    *y = uc_tooltips_main.y;
}

/**
 * uc_tooltips_main_get_frozen:
 *
 * The tooltips display is "frozen" when the mouse is out of the main
 * tree view.
 * 
 * Returns: %TRUE if tooltips display must be "frozen".
 */
gboolean
uc_tooltips_main_get_frozen (void)
{
  return uc_tooltips_main.out;
}

/**
 * uc_tooltips_main_set_frozen:
 * @value: %TRUE is the mouse is out of the main tree view.
 *
 * The tooltips display is "frozen" when the mouse is out of the main
* tree view.
 */
void
uc_tooltips_main_set_frozen (const gboolean value)
{
  uc_tooltips_main.out = value;
}

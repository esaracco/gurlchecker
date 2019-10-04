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

#include "project.h"
#include "application.h"
#include "lists.h"

#include "report.h"


/* structure to store informations during the
   check for managing a project report */
static struct UCProjectReport
{
  guint32 allLinks;
  guint32 checkedLinks;
  guint32 badLinks;
  guint32 goodLinks;
  guint32 timedOutLinks;
  guint32 ignoredLinks;
  guint32 malformedLinks;
  guint32 elapsedTime;
}
projectReport;

static void report_set_progress_bar (const guint32 report_item,
				     const gchar * glade_label,
				     const gchar * report_label,
				     const gchar * color);
static gboolean uc_report_export_dialog_show (const UCExportFormat format,
                                              const UCExportFormat ftype);


/**
 * uc_report_export_dialog_show:
 *
 * Show the report export dialog box.
 */
static gboolean
uc_report_export_dialog_show (const UCExportFormat format,
                              const UCExportFormat ftype)
{
  GtkWidget *widget = NULL;
  gint res = 0;
  gboolean ret = FALSE;
  GtkWidget *dialog = NULL;


  dialog = WGET ("report_export_dialog");

  widget = WGET ("report_export_path");
  if (uc_project_get_report_export_path () != NULL)
  {
    gnome_file_entry_set_default_path (GNOME_FILE_ENTRY (widget),
                                       uc_project_get_report_export_path ());
    gtk_entry_set_text (
      GTK_ENTRY (gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (widget))),
      uc_project_get_report_export_path ());
  }

  widget = WGET ("red_export_labels");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
                                uc_project_get_export_labels ());
  
  widget = WGET ("red_export_numbering");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
                                uc_project_get_export_numbering ());

  widget = WGET ("red_export_external");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
                                uc_project_get_export_external ());

  widget = WGET ("red_export_ip");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
                                uc_project_get_export_ip ());

  gtk_widget_show_all (dialog);

  if (format != UC_EXPORT_CSV)
  {
    widget = WGET ("red_export_external");
    gtk_widget_hide (widget);

    widget = WGET ("red_export_ip");
    gtk_widget_hide (widget);
  }

  if (ftype == UC_EXPORT_LIST_SIMPLE)
  {
    widget = WGET ("red_export_numbering");
    gtk_widget_hide (widget);

    widget = WGET ("red_export_external");
    gtk_widget_hide (widget);

    widget = WGET ("red_export_ip");
    gtk_widget_hide (widget);
  }

again:

  res = gtk_dialog_run (GTK_DIALOG (dialog));
  if (res == GTK_RESPONSE_OK)
  {
    const gchar *path = NULL;


    widget = WGET ("red_export_labels");
    uc_project_set_export_labels (
      (gboolean) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));

    widget = WGET ("red_export_numbering");
    uc_project_set_export_numbering (
      (gboolean) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));

    widget = WGET ("red_export_external");
    uc_project_set_export_external (
      (gboolean) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));

    widget = WGET ("red_export_ip");
    uc_project_set_export_ip (
      (gboolean) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));

    widget = WGET ("report_export_path");
    path = gtk_entry_get_text (
      GTK_ENTRY (gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (widget))));

    if (!g_file_test (path, G_FILE_TEST_IS_DIR))
      goto again;

    uc_project_set_report_export_path (path);

    // Add input to export path history
    gnome_entry_append_history (GNOME_ENTRY (gnome_file_entry_gnome_entry (
                                               GNOME_FILE_ENTRY (widget))),
                                TRUE, path);

    ret = TRUE;
  }

  gtk_widget_hide (dialog);

  return ret;
}


/**
 * uc_report_export:
 * @format: export format.
 * @list: list of URL to export.
 * @ftype: format type (%UC_EXPORT_NORMAL or %UC_EXPORT_SIMPLE).
 *
 * Export check report in the given format.
 */
void
uc_report_export (const UCExportFormat format, GList *list,
                  const UCExportFormat ftype)
{
  gchar *message = NULL;
  GError *err = NULL;
  gchar *path = NULL;
  gint32 ret = 0;
  time_t t;
  struct tm *td;


  if (!uc_report_export_dialog_show (format, ftype))
    return;

  time (&t);
  td = localtime (&t);

  // HTML
  if (format == UC_EXPORT_HTML)
  {
    gchar creation_date[UC_BUFFER_DATETIME_LEN + 1] = { 0 };
    gchar *command_line = NULL;


    strftime (creation_date, UC_BUFFER_DATETIME_LEN, "%Y-%m-%d %H:%M:%S", td);
    path = g_strdup_printf ("%s/projects/%u/project.xml",
                            uc_project_get_working_path (),
                            uc_project_get_id ());

    command_line = g_strdup_printf (" xsltproc "
				    " --stringparam date \"%s\" "
				    " --param withlabel %u "
				    " --param withnum %u "
				    " -o %s/uc_report_%u_%02d%02d%02d.html "
				    " " UC_STYLESHEETS_DIR "/uc_html.xsl %s",
				    creation_date,
				    (gint) uc_project_get_export_labels (),
				    (gint) uc_project_get_export_numbering (),
				    uc_project_get_report_export_path (),
				    uc_project_get_id (),
				    td->tm_hour, td->tm_min, td->tm_sec,
                                    path);

    ret = g_spawn_command_line_async (command_line, &err);
    g_free (command_line), command_line = NULL;
  }
  // CSV
  else if (format == UC_EXPORT_CSV)
  {
    FILE *fd = NULL;


    path = g_strdup_printf (
      "%s/uc_report_%u_%02d%02d%02d.csv",
      uc_project_get_report_export_path (),
      uc_project_get_id (),
      td->tm_hour, td->tm_min, td->tm_sec);

    fd = fopen (path, "w");
    uc_lists_checked_links_dump (list, &fd, ftype);
    fclose (fd);

    ret = 1;
  }

  g_free (path), path = NULL;

  if (ret == 0)
  {
    message = g_strdup_printf (
      "%s\n\n%s",
      _("Cannot execute xsltproc; the following error occured:"), err->message);

    uc_application_dialog_show (message, GTK_MESSAGE_ERROR);

    g_clear_error (&err);
    g_free (message), message = NULL;
  }
}


guint32
uc_report_get_malformedlinks (void)
{
  return projectReport.malformedLinks;
}


guint32
uc_report_get_badlinks (void)
{
  return projectReport.badLinks;
}


guint32
uc_report_get_ignoredlinks (void)
{
  return projectReport.ignoredLinks;
}


guint32
uc_report_get_goodlinks (void)
{
  return projectReport.goodLinks;
}


guint32
uc_report_get_timedoutlinks (void)
{
  return projectReport.timedOutLinks;
}


guint32
uc_report_get_elapsedtime (void)
{
  return projectReport.elapsedTime;
}


/*
 * return the total of links
 */
guint32
uc_report_get_alllinks (void)
{
  return projectReport.allLinks;
}


/*
 * return the number of checked
 * links
 */
guint32
uc_report_get_checkedlinks (void)
{
  return projectReport.checkedLinks;
}


/*
 * set "ignored links" value
 */
void
uc_report_set_ignoredlinks (const gint32 val)
{
  if (val < 0)
    projectReport.ignoredLinks++;
  else
    projectReport.ignoredLinks = val;
}


/*
 * set "checked links" value
 */
void
uc_report_set_alllinks (const gint32 val)
{
  if (val < 0)
    projectReport.allLinks++;
  else
    projectReport.allLinks = val;
}


/*
 * set "good links" value
 */
void
uc_report_set_goodlinks (const gint32 val)
{
  if (val < 0)
    projectReport.goodLinks++;
  else
    projectReport.goodLinks = val;
}


/*
 * set "malformed links" value
 */
void
uc_report_set_malformedlinks (const gint32 val)
{
  if (val < 0)
    projectReport.malformedLinks++;
  else
    projectReport.malformedLinks = val;
}


/*
 * set "bad links" value
 */
void
uc_report_set_badlinks (const gint32 val)
{
  if (val < 0)
    projectReport.badLinks++;
  else
    projectReport.badLinks = val;
}


/*
 * set "timeouts" value
 */
void
uc_report_set_timedoutlinks (const gint32 val)
{
  if (val < 0)
    projectReport.timedOutLinks++;
  else
    projectReport.timedOutLinks = val;
}


void
uc_report_set_elapsedtime (const guint32 val)
{
  projectReport.elapsedTime = val;
}


/*
 * set "single links" value
 */
void
uc_report_set_checkedlinks (void)
{
  projectReport.checkedLinks =
    projectReport.timedOutLinks +
    projectReport.badLinks +
    projectReport.malformedLinks +
    projectReport.goodLinks + projectReport.ignoredLinks;
}


/*
 * reset the check report structure
 */
void
uc_check_report_reset (const guint val)
{
  if (val != 0)
    projectReport.allLinks = 0;

  projectReport.checkedLinks = 0;
  projectReport.badLinks = 0;
  projectReport.malformedLinks = 0;
  projectReport.goodLinks = 0;
  projectReport.ignoredLinks = 0;
  projectReport.timedOutLinks = 0;
  projectReport.elapsedTime = 0;

  uc_report_display_update ();
}


/*
 * initialize the check report structure
 */
void
uc_check_report_force_values (const gint32 all, const gint32 checked,
			      const gint32 bad, const gint32 malformed,
			      const gint32 good, const gint32 ignored,
			      const gint32 timeout, const guint32 elapsedtime)
{
  projectReport.allLinks = all;
  projectReport.checkedLinks = checked;
  projectReport.badLinks = bad;
  projectReport.malformedLinks = malformed;
  projectReport.goodLinks = good;
  projectReport.ignoredLinks = ignored;
  projectReport.timedOutLinks = timeout;
  projectReport.elapsedTime = elapsedtime;

  uc_report_display_update ();
}


/*
 * update the display check report
 */
void
uc_report_display_update (void)
{
  gchar *tmp = NULL;


  uc_report_set_checkedlinks ();

  /* checked links */
  if (uc_project_get_check_is_bookmarks ())
    tmp = g_strdup_printf (_("<i><b>Checked links</b></i>: %d"),
                           projectReport.checkedLinks);
  else
    tmp = g_strdup_printf (_("<i><b>Checked links</b></i>: %d / %d"),
                           projectReport.checkedLinks,
                           projectReport.allLinks);
  gtk_label_set_markup (GTK_LABEL (WGET ("mw_label_checked_links")), tmp);
  g_free (tmp), tmp = NULL;

  /* good links */
  report_set_progress_bar (projectReport.goodLinks,
			   "mw_pb_good_links",
			   _("Good links"), UC_GOOD_LINK_BGCOLOR);

  /* bad links */
  report_set_progress_bar (projectReport.badLinks,
			   "mw_pb_bad_links", _("Bad links"),
			   UC_BAD_LINK_BGCOLOR);

  /* malformed links */
  report_set_progress_bar (projectReport.malformedLinks,
			   "mw_pb_malformed_links", _("Malformed links"),
			   UC_MALFORMED_LINK_BGCOLOR);

  /* ignored links */
  report_set_progress_bar (projectReport.ignoredLinks,
			   "mw_pb_ignored_links",
			   _("Ignored links"), UC_IGNORED_LINK_BGCOLOR);

  /* timeouts */
  report_set_progress_bar (projectReport.timedOutLinks,
			   "mw_pb_timeouts",
			   _("Timeouts"), UC_TIMEOUT_LINK_BGCOLOR);

  UC_UPDATE_UI;
}


/*
 * set label and progression on a report
 * progress bar
 */
static void
report_set_progress_bar (const guint32 report_item, const gchar * glade_label,
			 const gchar * report_label, const gchar * bg_color)
{
  gint32 percent = 0;
  GtkWidget *widget = NULL;
  gchar *tmp = NULL;
  GdkColor color;


  percent = (projectReport.checkedLinks) ?
    (report_item * 100) / projectReport.checkedLinks : 0;

  widget = WGET (glade_label);

  /* set progress bar bg color */
  gdk_color_parse (bg_color, &color);
  gtk_widget_modify_bg (widget, GTK_STATE_PRELIGHT, &color);

  /* set progress bar fg color */
  gdk_color_parse ("black", &color);
  gtk_widget_modify_fg (widget, GTK_STATE_PRELIGHT, &color);

  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (widget),
				 (gfloat) (percent * 0.01));

  tmp = g_strdup_printf ("%s %d (%d%%)", report_label, report_item, percent);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (widget), tmp);
  g_free (tmp), tmp = NULL;
}


/*
 * callback called to display the elapsed
 * time of a check
 */
gboolean
uc_report_timer_callback (gpointer data)
{
  gchar *tmp = NULL;
  guint seconds = 0;
  guint minutes = 0;
  guint hours = 0;

  seconds = projectReport.elapsedTime++;
  minutes = seconds / 60;
  hours = minutes / 60;

  tmp = g_strdup_printf (_("<i><b>Elapsed time</b></i>: %02d:%02d:%02d"),
			 hours % 24, minutes % 60, seconds % 60);
  gtk_label_set_markup (GTK_LABEL (WGET ("mw_label_ellapsed_time")), tmp);
  g_free (tmp), tmp = NULL;

  return TRUE;
}

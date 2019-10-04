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


#ifndef H_UC_REPORT
#define H_UC_REPORT


#include "general.h"

void uc_report_export (const UCExportFormat format, GList *list,
                       const UCExportFormat ftype);
void
uc_check_report_force_values (const gint32 all, const gint32 checked,
			      const gint32 bad, const gint32 malformed,
			      const gint32 good, const gint32 ignored,
			      const gint32 timeout,
			      const guint32 elapsedtime);
guint32 uc_report_get_ignoredlinks (void);
guint32 uc_report_get_checkedlinks (void);
guint32 uc_report_get_badlinks (void);
guint32 uc_report_get_malformedlinks (void);
guint32 uc_report_get_goodlinks (void);
guint32 uc_report_get_timedoutlinks (void);
guint32 uc_report_get_alllinks (void);
guint32 uc_report_get_elapsedtime (void);
gboolean uc_report_timer_callback (gpointer data);
void uc_report_set_elapsedtime (const guint32 val);
void uc_report_set_timedoutlinks (const gint32 val);
void uc_report_set_alllinks (const gint32 val);
void uc_report_set_checkedlinks (void);
void uc_report_display_update (void);
void uc_report_set_badlinks (const gint32 val);
void uc_report_set_malformedlinks (const gint32 val);
void uc_check_report_reset (const guint val);
void uc_report_set_goodlinks (const gint32 val);
void uc_report_set_ignoredlinks (const gint32 val);

#endif

/*
 * Copyright (C) 2002-2005 
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

#ifndef H_UC_TOOLTIPS
#define H_UC_TOOLTIPS


#include "general.h"

void uc_tooltips_main_set_display (const gboolean value);
gboolean uc_tooltips_main_get_display (void);
void uc_tooltips_init (void);
void uc_tooltips_main_set_mouse_coord (const gint32 x, const gint32 y);
guint32 uc_tooltips_main_get_current_id (void);
void uc_tooltips_main_set_current_id (const guint32 id);
guint32 uc_tooltips_main_get_last_id (void);
void uc_tooltips_main_set_last_id (const guint32 id);
void uc_tooltips_main_get_mouse_coord (gint32 * x, gint32 * y);
void uc_tooltips_main_set_mouse_coord (const gint32 x, const gint32 y);
void uc_tooltips_main_set_frozen (const gboolean value);
gboolean uc_tooltips_main_get_frozen (void);
void uc_tooltips_main_destroy (void);
gboolean uc_tooltips_main_get_active (void);
void uc_tooltips_main_set_active (const gboolean value);
gboolean uc_tooltips_get_frozen (void);

#endif

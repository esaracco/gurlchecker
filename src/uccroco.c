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

#ifdef ENABLE_CROCO

#include "general.h"
#include "uccroco.h"

static void uc_uccroco_css_error (CRDocHandler * a_handler);
static void uc_uccroco_init_handler (CRDocHandler * a_handler,
				     UCCroco * croco);
static void uc_uccroco_css_start_selector (CRDocHandler * a_handler,
					   CRSelector * a_selector_list);
static void uc_uccroco_css_end_selector (CRDocHandler * a_handler,
					 CRSelector * a_selector_list);
static void uc_uccroco_css_property (CRDocHandler * a_handler,
				     CRString * a_name, CRTerm * a_expr,
				     gboolean a_important);

static void
uc_uccroco_css_error (CRDocHandler * a_handler)
{
  UCCroco *croco = (UCCroco *) (a_handler->app_data);
  gchar *tmp = NULL;

  if (!croco->log_error)
    return;

  croco->error_count++;

  if (croco->def && croco->def->len)
    tmp = g_strdup_printf (_("%u - Error after: %s\n"),
			   croco->error_count, croco->def->str);
  else
    tmp = g_strdup_printf (_("%u - Unknown error...\n"), croco->error_count);

  g_string_append (croco->error_buffer, tmp);

  g_free (tmp), tmp = NULL;

  croco->is_valid = FALSE;

  croco->log_error = FALSE;
}

static void
uc_uccroco_css_start_selector (CRDocHandler * a_handler,
			       CRSelector * a_selector_list)
{
  UCCroco *croco = (UCCroco *) (a_handler->app_data);

  if (croco->def)
    g_string_free (croco->def, TRUE), croco->def = NULL;

  croco->def = g_string_new (NULL);

  croco->log_error = TRUE;
}

static void
uc_uccroco_css_end_selector (CRDocHandler * a_handler,
			     CRSelector * a_selector_list)
{
  UCCroco *croco = (UCCroco *) (a_handler->app_data);

  if (croco->def)
    g_string_free (croco->def, TRUE), croco->def = NULL;
}

static void
uc_uccroco_css_property (CRDocHandler * a_handler, CRString * a_name,
			 CRTerm * a_expr, gboolean a_important)
{
  UCCroco *croco = (UCCroco *) (a_handler->app_data);
  gchar *expr = NULL;
  gchar *name = NULL;
  gsize len = 0;

  if (a_name && a_expr && croco->def)
    {
      name = (gchar *) cr_string_peek_raw_str (a_name);
      len = cr_string_peek_raw_str_len (a_name);

      g_string_append_len (croco->def, (gchar *) name, len);
      g_string_append (croco->def, ": ");
      expr = (gchar *) cr_term_to_string (a_expr);
      g_string_append_len (croco->def, expr, strlen (expr));
      g_free (expr), expr = NULL;
      g_string_append (croco->def, "; ");
    }
}

static void
uc_uccroco_init_handler (CRDocHandler * a_handler, UCCroco * croco)
{
  a_handler->app_data = croco;
  a_handler->start_document = NULL;
  a_handler->end_document = NULL;
  a_handler->import_style = NULL;
  a_handler->namespace_declaration = NULL;
  a_handler->comment = NULL;
  a_handler->start_selector = uc_uccroco_css_start_selector;
  a_handler->end_selector = uc_uccroco_css_end_selector;
  a_handler->property = uc_uccroco_css_property;
  a_handler->start_font_face = NULL;
  a_handler->end_font_face = NULL;
  a_handler->start_media = NULL;
  a_handler->end_media = NULL;
  a_handler->start_page = NULL;
  a_handler->end_page = NULL;
  a_handler->ignorable_at_rule = NULL;
  a_handler->error = uc_uccroco_css_error;
  a_handler->unrecoverable_error = uc_uccroco_css_error;
}

UCCroco *
uc_uccroco_new (void)
{
  UCCroco *croco = NULL;

  croco = g_new0 (UCCroco, 1);
  croco->def = NULL;
  croco->error_buffer = g_string_new (NULL);
  croco->error_count = 0;
  croco->is_valid = TRUE;
  croco->log_error = TRUE;
  croco->css_handler = cr_doc_handler_new ();
  uc_uccroco_init_handler (croco->css_handler, croco);
  croco->parser = NULL;

  return croco;
}

gboolean
uc_uccroco_validate (UCCroco * croco, const gchar * buffer)
{
  croco->parser = cr_parser_new_from_buf ((guchar *) buffer,
					  (gulong) strlen (buffer), CR_UTF_8,
					  FALSE);

  if (cr_parser_set_sac_handler (croco->parser, croco->css_handler) != CR_OK
      || cr_parser_set_use_core_grammar (croco->parser, FALSE) != CR_OK
      || cr_parser_parse (croco->parser) != CR_OK)
    return FALSE;

  return croco->is_valid;
}

void
uc_uccroco_free (UCCroco ** croco)
{
  if (!croco)
    return;

  if ((*croco)->parser)
    cr_parser_destroy ((*croco)->parser), (*croco)->parser = NULL;
  if ((*croco)->def)
    g_string_free ((*croco)->def, TRUE), (*croco)->def = NULL;
  if ((*croco)->error_buffer)
    g_string_free ((*croco)->error_buffer, TRUE), (*croco)->error_buffer = NULL;

  g_free (*croco), *croco = NULL;
}

#endif

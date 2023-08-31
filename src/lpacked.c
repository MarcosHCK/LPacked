/* Copyright 2023 MarcosHCK
 * This file is part of LPacked.
 *
 * LPacked is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LPacked is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LPacked. If not, see <http://www.gnu.org/licenses/>.
 */
#include <config.h>
#include <builder.h>
#include <descriptor.h>

#define _g_array_unref0(var) ((var == NULL) ? NULL : (var = (g_array_unref (var), NULL)))
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))

struct _LPackedApplication
{
  GApplication parent;

  /*<private>*/
  GOptionEntry* execute_entries;
  GOptionEntry* main_entries;
  GOptionEntry* pack_entries;

  const gchar* execute;
  const gchar* pack;
  const gchar* pack_output;
};

struct _LPackedApplicationClass
{
  GApplicationClass parent;
};

G_DECLARE_FINAL_TYPE (LPackedApplication, lpacked_application, LPACKED, APPLICATION, GApplication);
G_DEFINE_FINAL_TYPE (LPackedApplication, lpacked_application, G_TYPE_APPLICATION);

int main (int argc, char* argv[])
{
  gpointer app = g_object_new (lpacked_application_get_type (), NULL);
  gint result = g_application_run (app, argc, argv);
return (g_object_unref (app), result);
}

static void open_vectors (LPackedDescriptor* desc, GHashTable* table, GError** error)
{
  GError* tmperr = NULL;
  GList* list = NULL;

  list = lpacked_descriptor_get_aliases (desc);
  list = g_list_sort (list, (GCompareFunc) g_strcmp0);

  for (; list; list = list->next)
    {
      const gchar* alias = list->data;
      const gchar* file = lpacked_descriptor_get_file_by_alias (desc, alias);

      if ((lpacked_builder_add_source_from_file (table, alias, file, &tmperr)), G_UNLIKELY (tmperr != NULL))
        {
          g_propagate_error (error, tmperr);
          break;
        }
    }
}

static void do_active (LPackedApplication* self, GError** error)
{
  GFile* file = NULL;
  GError* tmperr = NULL;
  GOutputStream* stream = NULL;
  LPackedDescriptor* desc = NULL;

  file = g_file_new_for_commandline_arg (self->pack);

  if ((desc = lpacked_descriptor_new_from_gfile (file, &tmperr), g_object_unref (file)), G_UNLIKELY (tmperr != NULL))
    g_propagate_error (error, tmperr);
  else
    {
      GHashTable* table = lpacked_builder_new ();

      if ((open_vectors (desc, table, &tmperr)), G_UNLIKELY (tmperr != NULL))
        g_propagate_error (error, tmperr);
      else
        {
          gboolean byteswap = G_BYTE_ORDER != G_LITTLE_ENDIAN;
          gchar* output = NULL;

          if (self->pack_output != NULL)
            output = g_strdup (self->pack_output);
          else
            {
              GPathBuf buf = G_PATH_BUF_INIT;
              gchar* path = NULL;

              g_path_buf_push (&buf, lpacked_descriptor_get_name (desc));
              g_path_buf_set_extension (&buf, "lpack");
              output = g_path_buf_clear_to_path (&buf);
            }

          if ((lpacked_builder_write (table, output, &tmperr), g_free (output)), G_UNLIKELY (tmperr != NULL))
            g_propagate_error (error, tmperr);
        }

      g_hash_table_remove_all (table);
      g_hash_table_unref (table);
      g_object_unref (desc);
    }
}

static void lpacked_application_class_activate (GApplication* pself)
{
  GError* tmperr = NULL;
  LPackedApplication* self = (gpointer) pself;

  if (self->pack != NULL)
    {
      if ((do_active (self, &tmperr)), G_UNLIKELY (tmperr != NULL))
        {
          const guint code = tmperr->code;
          const gchar* domain = g_quark_to_string (tmperr->domain);
          const gchar* message = tmperr->message;

          g_printerr ("(" G_STRLOC "): %s: %i: %s\n", domain, code, message);
          g_error_free (tmperr);
        }
    }
}

static void lpacked_application_class_finalize (GObject* pself)
{
  g_free (G_STRUCT_MEMBER (gpointer, pself, G_STRUCT_OFFSET (LPackedApplication, execute_entries)));
  g_free (G_STRUCT_MEMBER (gpointer, pself, G_STRUCT_OFFSET (LPackedApplication, main_entries)));
  g_free (G_STRUCT_MEMBER (gpointer, pself, G_STRUCT_OFFSET (LPackedApplication, pack_entries)));
  G_OBJECT_CLASS (lpacked_application_parent_class)->finalize (pself);
}

static void lpacked_application_class_constructed (GObject* pself)
{
  LPackedApplication* self = (gpointer) pself;
  GOptionGroup* execute_group = NULL;
  GOptionGroup* pack_group = NULL;

  G_OBJECT_CLASS (lpacked_application_parent_class)->constructed (pself);

  const gchar* parameter_string = NULL;
  const gchar* description = "";
  const gchar* summary = "";
  const gchar* translation_domain = "en_US";

  const GOptionEntry execute_entries [] =
    {
      G_OPTION_ENTRY_NULL,
    };

  const GOptionEntry main_entries [] = 
    {
      { "execute", 'e', 0, G_OPTION_ARG_FILENAME, & self->execute, "Execute packed application FILE", "FILE", },
      { "pack", 'p', 0, G_OPTION_ARG_FILENAME, & self->pack, "Create a packed application from application description in FILE", "FILE", },
      G_OPTION_ENTRY_NULL,
    };

  const GOptionEntry pack_entries [] =
    {
      { "output", 'o', 0, G_OPTION_ARG_FILENAME, & self->pack_output, "Output packed project into FILE", "FILE", },
      G_OPTION_ENTRY_NULL,
    };

  self->execute_entries = g_memdup2 (&execute_entries, sizeof (execute_entries));
  self->main_entries = g_memdup2 (&main_entries, sizeof (main_entries));
  self->pack_entries = g_memdup2 (&pack_entries, sizeof (pack_entries));

  execute_group = g_option_group_new ("execute", "Execute mode specific options", "Show execute mode specific options", NULL, NULL);
  pack_group = g_option_group_new ("pack", "Pack mode specific options", "Show pack mode specific options", NULL, NULL);

  g_application_add_main_option_entries (G_APPLICATION (pself), self->main_entries);
  g_application_set_option_context_description (G_APPLICATION (pself), description);
  g_application_set_option_context_parameter_string (G_APPLICATION (pself), parameter_string);
  g_application_set_option_context_summary (G_APPLICATION (pself), summary);

  g_option_group_add_entries (execute_group, self->execute_entries);
  g_option_group_set_translation_domain (execute_group, translation_domain);
  g_application_add_option_group (G_APPLICATION (pself), execute_group);
  g_option_group_add_entries (pack_group, self->pack_entries);
  g_option_group_set_translation_domain (pack_group, translation_domain);
  g_application_add_option_group (G_APPLICATION (pself), pack_group);
}

static void lpacked_application_class_init (LPackedApplicationClass* klass)
{
  G_APPLICATION_CLASS (klass)->activate = lpacked_application_class_activate;
  G_OBJECT_CLASS (klass)->finalize = lpacked_application_class_finalize;
  G_OBJECT_CLASS (klass)->constructed = lpacked_application_class_constructed;
}

static void lpacked_application_init (LPackedApplication* self)
{
}

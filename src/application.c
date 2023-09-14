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
#include <application.h>

struct _LpApplication
{
  GApplication parent;

  /* <private> */
  gchar* exec;
  gchar* pack;
  gchar* pack_output;

  /* <private> */
  GOptionEntry* exec_entries;
  GOptionEntry* main_entries;
  GOptionEntry* pack_entries;
};

G_DEFINE_FINAL_TYPE (LpApplication, lp_application, G_TYPE_APPLICATION);

static void lp_application_init (LpApplication* self)
{
  const GOptionEntry exec_entries [] =
    {
      G_OPTION_ENTRY_NULL,
    };

  const GOptionEntry main_entries [] =
    {
      { "exec", 'e', G_OPTION_FLAG_NONE, G_OPTION_ARG_FILENAME, &self->exec, "Executes packed application FILE", "FILE", },
      { "pack", 'p', G_OPTION_FLAG_NONE, G_OPTION_ARG_FILENAME, &self->pack, "Packs application described by FILE", "FILE", },
      G_OPTION_ENTRY_NULL,
    };

  const GOptionEntry pack_entries [] =
    {
      { "output", 'o', G_OPTION_FLAG_NONE, G_OPTION_ARG_FILENAME, &self->pack_output, "Write packed application in FILE", "FILE", },
      G_OPTION_ENTRY_NULL,
    };

  GApplication* pself = (gpointer) self;
  GOptionGroup *exec_group, *pack_group;

  self->exec_entries = g_memdup2 (exec_entries, sizeof (exec_entries));
  self->main_entries = g_memdup2 (main_entries, sizeof (main_entries));
  self->pack_entries = g_memdup2 (pack_entries, sizeof (pack_entries));

  exec_group = g_option_group_new ("exec", "exec mode specific flags", "Show exec mode specific flags", NULL, NULL);
  pack_group = g_option_group_new ("pack", "pack mode specific flags", "Show pack mode specific flags", NULL, NULL);

  g_option_group_add_entries (exec_group, self->exec_entries);
  g_option_group_add_entries (pack_group, self->pack_entries);

  g_application_add_main_option_entries (pself, self->main_entries);
  g_application_add_option_group (pself, exec_group);
  g_application_add_option_group (pself, pack_group);
}

static void lp_application_class_activate (GApplication* pself)
{
  LpApplication* self = (gpointer) pself;
}

static void lp_application_class_open (GApplication* pself, GFile** files, gint n_files, const gchar* hint)
{
  LpApplication* self = (gpointer) pself;

  if (self->pack != NULL)
    g_warning ("error: pack mode does not takes any additional files");
  else
    g_assert_not_reached ();
}

static void lp_application_class_finalize (GObject* pself)
{
  g_free (G_STRUCT_MEMBER (gpointer, pself, G_STRUCT_OFFSET (LpApplication, exec_entries)));
  g_free (G_STRUCT_MEMBER (gpointer, pself, G_STRUCT_OFFSET (LpApplication, main_entries)));
  g_free (G_STRUCT_MEMBER (gpointer, pself, G_STRUCT_OFFSET (LpApplication, pack_entries)));
  g_clear_pointer (&G_STRUCT_MEMBER (gpointer, pself, G_STRUCT_OFFSET (LpApplication, exec)), g_free);
  g_clear_pointer (&G_STRUCT_MEMBER (gpointer, pself, G_STRUCT_OFFSET (LpApplication, pack)), g_free);
  g_clear_pointer (&G_STRUCT_MEMBER (gpointer, pself, G_STRUCT_OFFSET (LpApplication, pack_output)), g_free);
  G_OBJECT_CLASS (lp_application_parent_class)->finalize (pself);
}

static void lp_application_class_init (LpApplicationClass* klass)
{
  G_APPLICATION_CLASS (klass)->activate = lp_application_class_activate;
  G_APPLICATION_CLASS (klass)->open = lp_application_class_open;
  G_OBJECT_CLASS (klass)->finalize = lp_application_class_finalize;
}

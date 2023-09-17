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

enum
{
  prop_0,
  prop_exec,
  prop_pack,
  prop_pack_output,
  prop_number,
};

G_DEFINE_FINAL_TYPE (LpApplication, lp_application, G_TYPE_APPLICATION);

#define _g_free0(var) ((var == NULL) ? NULL : (var = (g_free (var), NULL)))
static GParamSpec* properties [prop_number] = {0};

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

static void lp_application_class_finalize (GObject* pself)
{
  _g_free0 (G_STRUCT_MEMBER (gpointer, pself, G_STRUCT_OFFSET (LpApplication, exec)));
  _g_free0 (G_STRUCT_MEMBER (gpointer, pself, G_STRUCT_OFFSET (LpApplication, exec_entries)));
  _g_free0 (G_STRUCT_MEMBER (gpointer, pself, G_STRUCT_OFFSET (LpApplication, main_entries)));
  _g_free0 (G_STRUCT_MEMBER (gpointer, pself, G_STRUCT_OFFSET (LpApplication, pack)));
  _g_free0 (G_STRUCT_MEMBER (gpointer, pself, G_STRUCT_OFFSET (LpApplication, pack_entries)));
  _g_free0 (G_STRUCT_MEMBER (gpointer, pself, G_STRUCT_OFFSET (LpApplication, pack_output)));
  G_OBJECT_CLASS (lp_application_parent_class)->finalize (pself);
}

static void lp_application_class_get_property (GObject* pself, guint property_id, GValue* value, GParamSpec* pspec)
{
  LpApplication* self = (gpointer) pself;

  switch (property_id)
    {
      default: G_OBJECT_WARN_INVALID_PROPERTY_ID (pself, property_id, pspec); break;
      case prop_exec: g_value_set_string (value, self->exec); break;
      case prop_pack: g_value_set_string (value, self->pack); break;
      case prop_pack_output: g_value_set_string (value, self->pack_output); break;
    }
}

static void lp_application_class_set_property (GObject* pself, guint property_id, const GValue* value, GParamSpec* pspec)
{
  LpApplication* self = (gpointer) pself;

  switch (property_id)
    {
      default: G_OBJECT_WARN_INVALID_PROPERTY_ID (pself, property_id, pspec); break;
      case prop_exec: _g_free0 (self->exec); self->exec = g_value_dup_string (value); break;
      case prop_pack: _g_free0 (self->pack); self->pack = g_value_dup_string (value); break;
      case prop_pack_output: _g_free0 (self->pack_output); self->pack_output = g_value_dup_string (value); break;
    }
}

static void lp_application_class_init (LpApplicationClass* klass)
{
  G_OBJECT_CLASS (klass)->finalize = lp_application_class_finalize;
  G_OBJECT_CLASS (klass)->get_property = lp_application_class_get_property;
  G_OBJECT_CLASS (klass)->set_property = lp_application_class_set_property;

  /**
   * LpApplication:exec:
   * 
   * Command line argument --exec value.
  */

  /**
   * LpApplication:pack:
   * 
   * Command line argument --pack value.
  */

  /**
   * LpApplication:pack-output:
   * 
   * Command line argument --output value.
  */

  properties [prop_exec] = g_param_spec_string ("exec", "exec", "exec", NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  properties [prop_pack] = g_param_spec_string ("pack", "pack", "pack", NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  properties [prop_pack_output] = g_param_spec_string ("pack-output", "pack-output", "pack-output", NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties (G_OBJECT_CLASS (klass), prop_number, properties);
}

/**
 * lp_application_new:
 *
 * Creates an instance of #LpApplication.
 * 
 * Returns: a new #LpApplication instance.
*/
LpApplication* lp_application_new ()
{
  return g_object_new (LP_TYPE_APPLICATION,
                       "application-id", "org.hck.lpacked",
                       "flags", G_APPLICATION_HANDLES_OPEN | G_APPLICATION_NON_UNIQUE,
                        NULL);
}

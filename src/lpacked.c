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
#include <execute.h>
#include <glib.h>
#include <pack.h>

#define report(error) G_STMT_START { \
    const guint __code = (error)->code; \
    const gchar* __domain = g_quark_to_string ((error)->domain); \
    const gchar* __message = (error)->message; \
    g_critical ("(" G_STRLOC "): %s: %i: %s", __domain, __code, __message); \
  } G_STMT_END

int main (int argc, char* argv[])
{
  gchar** arguments = NULL;
  GError* tmperr = NULL;
  GOptionContext* context = NULL;

  const gchar* parameter_string = NULL;
  const gchar* description = "";
  const gchar* summary = "";
  const gchar* translation_domain = "en_US";

  context = g_option_context_new (parameter_string);
  g_option_context_set_description (context, description);
  g_option_context_set_help_enabled (context, TRUE);
  g_option_context_set_ignore_unknown_options (context, FALSE);
  g_option_context_set_strict_posix (context, FALSE);
  g_option_context_set_summary (context, summary);
  g_option_context_set_translation_domain (context, translation_domain);

#ifndef G_OS_WIN32
  if (TRUE)
    {
      gint i;

      arguments = g_new (gchar*, argc + 1);
      arguments [argc] = NULL;

      for (i = 0; i < argc; ++i)
        {
          arguments [i] = g_strdup (argv [i]);
        }
    }
#else // G_OS_WIN32
  if (TRUE)
    {
      gchar** new_argv;
      guint i, new_argc;

      new_argv = g_win32_get_command_line ();
      new_argc = g_strv_length (new_argv);
      arguments = new_argv;

        if (new_argc > argc)
          {
            for (i = 0; i < (new_argc - argc); ++i)
              g_free (new_argv [i]);
              memmove (&new_argv [0],
                       &new_argv [new_argc - argc],
                       sizeof (new_argv [0]) * (argc + 1));
          }
    }
#endif // G_OS_WIN32

  const gchar* execute = NULL;
  const gchar* pack = NULL;

  const GOptionEntry main_entries [] = 
    {
      { "execute", 'e', 0, G_OPTION_ARG_FILENAME, &execute, "Execute packed application FILE", "FILE", },
      { "pack", 'p', 0, G_OPTION_ARG_FILENAME, &pack, "Create a packed application from application description in FILE", "FILE", },
      G_OPTION_ENTRY_NULL,
    };

  g_option_context_add_main_entries (context, main_entries, translation_domain);
  g_option_context_parse_strv (context, &arguments, &tmperr);
  g_option_context_free (context);

  if (G_UNLIKELY (tmperr != NULL))
    {
      report (tmperr);
      g_error_free (tmperr);
      g_strfreev (arguments);
      return 1;
    }
  else
    {
      if (execute != NULL)
        {
          if ((do_execute (execute, &tmperr)), G_UNLIKELY (tmperr != NULL))
            {
              report (tmperr);
              g_error_free (tmperr);
              g_strfreev (arguments);
              return 1;
            }
        }

      if (pack != NULL)
        {
          if ((do_pack (pack, &tmperr)), G_UNLIKELY (tmperr != NULL))
            {
              report (tmperr);
              g_strfreev (arguments);
              return 1;
            }
        }
    }
return (g_strfreev (arguments), 0);
}

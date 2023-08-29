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
#include <glib-object.h>
#include <lauxlib.h>
#include <lua.h>
#include <luaerror.h>
#include <lualib.h>
#include <pack.h>
#include <compat.h>
#include <zip.h>

#define PACK_ERROR (pack_error_quark ())
G_DEFINE_QUARK (lpacked-pack-error-quark, pack_error);

#ifndef ZIP_LENGTH_TO_END
# define ZIP_LENGTH_TO_END -1
#endif // ZIP_LENGTH_TO_END

static inline GError* error_from_ziperr (zip_error_t* ziperr)
{
  const gint code = zip_error_code_zip (ziperr);
  const char* message = zip_error_strerror (ziperr);
  GError* error = g_error_new (PACK_ERROR, 1, "%i: %s", code, message);
return (error);
}

static inline GError* error_from_zip (zip_t* zip)
{
  zip_error_t* ziperr = zip_get_error (zip);
  GError* error = error_from_ziperr (ziperr);
return (error);
}

static inline GError* error_from_ziperr_code (int error_code)
{
  GError* error = NULL;
  zip_error_t ziperr = {0};

  zip_error_init_with_code (&ziperr, error_code);
  g_propagate_error (&error, error_from_ziperr (&ziperr));
return (zip_error_fini (&ziperr), error);
}

static void do_packfile (lua_State* L, zip_t* zip, const gchar* src_path, const gchar* dst_path, GError** error)
{
  zip_source_t* source;
  zip_int64_t result;

  if ((source = zip_source_file (zip, src_path, 0, ZIP_LENGTH_TO_END)), G_UNLIKELY (source == NULL))
    g_propagate_prefixed_error (error, error_from_zip (zip), "libzip error: ");
  else
    {
      if ((result = zip_file_add (zip, dst_path, source, ZIP_FL_ENC_UTF_8)), G_UNLIKELY (result < 0))
        {
          zip_source_free (source);
          g_propagate_prefixed_error (error, error_from_zip (zip), "libzip error: ");
        }
    }
}

static void do_packtable (lua_State* L, zip_t* zip, const gchar* name, GError** error)
{
  GError* tmperr = NULL;
  GPathBuf buf = G_PATH_BUF_INIT;
  const gchar* src = NULL;
  const gchar* dst = NULL;
  int type;

  lua_pushnil (L);

  while (lua_next (L, -2))
    {
      if ((type = lua_type (L, -1)), G_UNLIKELY (type != LUA_TSTRING))
        {
          lua_pop (L, 2);
          g_set_error (error, PACK_ERROR, 0, "descriptor field '%s' table must contain string-string or number-string pairs", name);
          break;
        }

      if ((type = lua_type (L, -2)), G_UNLIKELY (type != LUA_TNUMBER && type != LUA_TSTRING))
        {
          lua_pop (L, 2);
          g_set_error (error, PACK_ERROR, 0, "descriptor field '%s' table must contain string-string or number-string pairs", name);
          break;
        }

      g_path_buf_push (&buf, name);
      g_path_buf_push (&buf, lua_tostring (L, (type == LUA_TNUMBER) ? -1 : -2));

      src = lua_tostring (L, -1);
      dst = g_path_buf_clear_to_path (&buf);
  
      if ((do_packfile (L, zip, src, dst, &tmperr), g_free ((gchar*) dst)), G_UNLIKELY (tmperr != NULL))
        {
          lua_pop (L, 2);
          g_propagate_error (error, tmperr);
          break;
        }

      lua_pop (L, 1);
    }
}

static void do_packdescriptor (lua_State* L, zip_t* zip, GError** error)
{
  GError* tmperr = NULL;
  const char* tables [] = { "sources", "resources", NULL, };
  int i, j, nils = 0;

  for (i = 0; i < G_N_ELEMENTS (tables); ++i)
    {
      if (tables [i] == NULL)
        {
          if (G_UNLIKELY (nils == G_N_ELEMENTS (tables)))
            {
              luaL_Buffer B;
              luaL_buffinit (L, &B);
              luaL_addstring (&B, "at least one of ");

              for (j = 0; j < G_N_ELEMENTS (tables); ++i)
                {
                  if (j > 0)
                  luaL_addstring (&B, ((j + 1) == G_N_ELEMENTS (tables)) ? " o " : ", ");
                  luaL_addstring (&B, tables [j]);
                }

              luaL_addstring (&B, " must be a non-nil value");
              luaL_pushresult (&B);

              g_set_error_literal (error, PACK_ERROR, 0, lua_tostring (L, -1));
              lua_pop (L, 1);
            }
        }
      else
        {
          lua_getfield (L, -1, tables [i]);

          switch (lua_type (L, -1))
            {
              default:
                i = G_N_ELEMENTS (tables) + 1;
                g_set_error (error, PACK_ERROR, 0, "descriptor field '%s' must contain an string or nil value", tables [i]);
                break;

              case LUA_TNIL: ++nils; break;

              case LUA_TTABLE:
                if ((do_packtable (L, zip, tables [i], &tmperr)), G_UNLIKELY (tmperr != NULL))
                  {
                    i = G_N_ELEMENTS (tables) + 1;
                    g_propagate_error (error, tmperr);
                  }
                break;
            }
          lua_pop (L, 1);
        }
    }
}

void do_pack (const gchar* file, const gchar* output, GError** error)
{
  gchar* contents = NULL;
  gsize length = 0;
  GError* tmperr = NULL;

  if ((g_file_get_contents (file, &contents, &length, &tmperr)), G_UNLIKELY (tmperr != NULL))
    g_propagate_error (error, tmperr);
  else
    {
      lua_State* L = luaL_newstate ();
      zip_t* zip = NULL;
      int type, result;

      lua_gc (L, LUA_GCSTOP, -1);
      luaopen_base (L);
      luaopen_math (L);
      luaopen_string (L);
      luaopen_table (L);

      lua_gc (L, LUA_GCRESTART, 0);
      lua_settop (L, 0);

      if ((result = luaL_loadbuffer (L, contents, length, "=descriptor"), g_free (contents)), G_UNLIKELY (result != LUA_OK))
        {
          switch (result)
            {
              case LUA_ERRMEM: g_error ("(" G_STRLOC "): out of memory");
              case LUA_ERRSYNTAX: g_set_error_literal (error, LUA_ERROR, result, lua_tostring (L, -1));
            }
        }
      else
        {
          if ((result = lua_pcall (L, 0, 1, 0)), G_UNLIKELY (result != LUA_OK))
            {
              switch (result)
                {
                  case LUA_ERRMEM: g_error ("(" G_STRLOC "): out of memory");
                  case LUA_ERRRUN: g_set_error_literal (error, LUA_ERROR, result, lua_tostring (L, -1));
                }
            }
          else
            {
              if (lua_getfield (L, -1, "name"), type = lua_type (L, -1), G_UNLIKELY (type != LUA_TSTRING))
                g_set_error_literal (error, PACK_ERROR, 0, "descriptor field 'name' must contain an string value");
              else
                {
                  if (output != NULL)
                    {
                      lua_pop (L, 1);
                      output = g_strdup (output);
                    }
                  else
                    {
                      GPathBuf buf = G_PATH_BUF_INIT;

                      g_path_buf_push (&buf, lua_tostring (L, -1));
                      lua_pop (L, 1);
                      g_path_buf_set_extension (&buf, "lpack");
                      output = g_path_buf_clear_to_path (&buf);
                    }

                  if ((zip = zip_open (output, ZIP_CREATE | ZIP_TRUNCATE, &result), g_free ((gchar*) output)), G_UNLIKELY (zip == NULL))
                    g_propagate_prefixed_error (error, error_from_ziperr_code (result), "libzip error: ");
                  else if ((do_packdescriptor (L, zip, &tmperr)), G_UNLIKELY (tmperr != NULL))
                    {
                      zip_discard (zip);
                      g_propagate_error (error, tmperr);
                    }
                  else if ((result = zip_close (zip)), G_UNLIKELY (result != 0))
                    {
                      zip_discard (zip);
                      g_propagate_prefixed_error (error, error_from_zip (zip), "libzip error: ");
                    }
                }
            }
        }

      lua_close (L);
    }
}

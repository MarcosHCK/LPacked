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
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <compat.h>
#include <package.h>

static int __gc (lua_State* L)
{
  g_queue_clear_full
    ((GQueue*) luaL_checkudata (L, 1, LP_RESOURCES_QUEUE),
     (GDestroyNotify) g_resource_unref);
return 0;
}

void lp_package_create_resources (lua_State* L)
{
  g_queue_init (lua_newuserdata (L, sizeof (GQueue)));

  if (!luaL_newmetatable (L, LP_RESOURCES_QUEUE))
    lua_setmetatable (L, -2);
  else
    {
      lua_pushcfunction (L, __gc);
      lua_setfield (L, -2, "__gc");
      lua_setmetatable (L, -2);
    }
}

void lp_package_delete_resource (lua_State* L, int idx, GResource* resource)
{
  GQueue* queue = luaL_checkudata (L, idx, LP_RESOURCES_QUEUE);
  GList* link = g_queue_find (queue, resource);

  if (link != NULL)
    {
      g_queue_delete_link (queue, link);
      g_list_free_full (link, (GDestroyNotify) g_resource_unref);
    }
}

void lp_package_insert_resource (lua_State* L, int idx, GResource* resource)
{
  g_queue_push_tail
    ((GQueue*) luaL_checkudata (L, idx, LP_RESOURCES_QUEUE),
     (GResource*) g_resource_ref (resource));
}

static const gchar* nexttemplate (lua_State* L, const gchar* path)
{
  const char* l;

  while (*path == *LUA_PATH_SEP)
    path++;
  if (*path == '\0')
    return NULL;
  if ((l = strchr (path, *LUA_PATH_SEP)) == NULL)
    l = path + strlen (path);
return (lua_pushlstring (L, path, l - path), l);
}

static GBytes* open (lua_State* L, const gchar* fullpath, GError** error)
{
  GBytes* bytes = NULL;
  GError* tmperr = NULL;
  GList* list = NULL;
  GQueue* queue = NULL;

  lua_getfield (L, lua_upvalueindex (1), "resources");

  queue = lua_touserdata (L, -1);
  list = g_queue_peek_head_link (queue);
        lua_pop (L, 1);

  for (; list; list = list->next)
    {
      if ((bytes = g_resource_lookup_data (list->data, fullpath, 0, &tmperr)), (tmperr == NULL))
        break;
      else
        {
          if (g_error_matches (tmperr, G_RESOURCE_ERROR, G_RESOURCE_ERROR_NOT_FOUND))
            g_error_free (tmperr);
          else
            {
              g_propagate_error (error, tmperr);
              return NULL;
            }
        }
    }

  if (G_UNLIKELY (bytes == NULL))
    {
      const GQuark domain = G_RESOURCE_ERROR;
      const guint code = G_RESOURCE_ERROR_NOT_FOUND;
      const gchar* message = "not found";

      g_set_error_literal (error, domain, code, message);
    }
return bytes;
}

static GBytes* search (lua_State* L, const gchar* name, const gchar* path)
{
  luaL_Buffer B;
  GBytes* bytes = NULL;

  name = luaL_gsub (L, name, ".", LUA_DIRSEP);

  luaL_buffinit (L, &B);

  while ((path = nexttemplate (L, path)) != NULL)
    {
      GError* tmperr = NULL;
      const gchar* template = template = luaL_checkstring (L, -1);
      const gchar* fullpath = fullpath = luaL_gsub (L, template, LUA_PATH_MARK, name);

      lua_remove (L, -2);

      if ((bytes = open (L, fullpath, &tmperr)), G_LIKELY (tmperr == NULL))
        return bytes;
      else
        {
          if (g_error_matches (tmperr, G_RESOURCE_ERROR, G_RESOURCE_ERROR_NOT_FOUND))
            {
              lua_pushfstring (L, "\n\tno resource '%s'", fullpath);
              g_error_free (tmperr);
              lua_remove (L, -2);
              luaL_addvalue (& B);
            }
          else
            {
              const guint code = tmperr->code;
              const gchar* domain = g_quark_to_string (tmperr->domain);
              const gchar* message = tmperr->message;

              lua_pushfstring (L, "%s: %i: %s", domain, code, message);
              g_error_free (tmperr);
              lua_error (L);
            }
        }
    }
return (luaL_pushresult (&B), NULL);
}

int lp_package_searcher (lua_State* L)
{
  GBytes* bytes = NULL;
  GError* tmperr = NULL;
  const gchar* name = NULL;
  const gchar* path = NULL;
  int type, result;

  if ((lua_getfield (L, lua_upvalueindex (1), "path"), (type = lua_type (L, -1))), G_UNLIKELY (type != LUA_TSTRING))
    luaL_error (L, "package.path must be an string");
  if ((lua_getfield (L, lua_upvalueindex (1), "resources"), (type = lua_type (L, -1))), G_UNLIKELY (type != LUA_TUSERDATA))
    luaL_error (L, "package.path must be an string");
  else
    {
      luaL_checkudata (L, -1, LP_RESOURCES_QUEUE);
      lua_pop (L, 1);
    }

  name = luaL_checkstring (L, 1);
  path = lua_tostring (L, -1);

  if ((bytes = search (L, name, path)) != NULL)
    {
      lua_pushliteral (L, "=");
      lua_insert (L, -2);
      lua_concat (L, 2);

      gsize size;
      const gchar* name = lua_tostring (L, -1);
      const gchar* data = g_bytes_get_data (bytes, &size);

      switch ((result = luaL_loadbuffer (L, data, size, name)))
        {
          case LUA_OK: break;
          case LUA_ERRSYNTAX: lua_error (L); break;
          case LUA_ERRMEM: g_error ("(" G_STRLOC "): out of memory");
          default: g_error ("(" G_STRLOC "): unknown error %i", result);
        }
    }
return 1;
}

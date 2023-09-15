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
#include <compat.h>
#include <glib.h>
#include <package.h>
#include <resources.h>

static int doinit (lua_State* L)
{
  lua_getglobal (L, "require");
  lua_pushliteral (L, "org.hck.lpacked");
  lua_call (L, 1, 1);
return 1;
}

static int msghandler (lua_State* L)
{
  const char* msg = NULL;
  const char* type = NULL;

  if ((msg = lua_tostring (L, 1)) == NULL)
    {
      if (luaL_callmeta (L, 1, "__tostring") && lua_isstring (L, -1))
        return 1;
      else
        {
          type = luaL_typename (L, 1);
          msg = lua_pushfstring (L, "(error object is a %s value)", type);
        }
    }
return (luaL_traceback (L, L, msg, 1), 1);
}

int main (int argc, char* argv[])
{
  lua_State* L = NULL;
  int i, result = 0;

  if ((L = luaL_newstate ()) == NULL)
    {
      g_error ("(" G_STRLOC ") luaL_newstate()!");
      g_assert_not_reached ();
    }

  lua_gc (L, LUA_GCSTOP, -1);
  luaL_openlibs (L);
  lua_gc (L, LUA_GCRESTART, 0);
  lua_settop (L, 0);

  lua_getglobal (L, "package");

  lua_getfield (L, -1, "path");
  lua_pushliteral (L, ";/?.lua;/?/init.lua");
  lua_concat (L, 2);
  lua_setfield (L, -2, "path");

#if LUA_VERSION_NUM >= 502
  lua_getfield (L, -1, "searchers");
#else // LUA_VERSION_NUM < 502
  lua_getfield (L, -1, "loaders");
#endif // LUA_VERSION_NUM
#if LUA_VERSION_NUM >= 502
  lua_Unsigned objlen = lua_rawlen (L, -1);
#else // LUA_VERSION_NUM < 502
  size_t objlen = lua_objlen (L, -1);
#endif // LUA_VERSION_NUM
  lua_insert (L, -2);

  lp_resources_register_resource ();
  lp_package_create_resources (L);
  lp_package_insert_resource (L, -1, lp_resources_get_resource ());
  lua_setfield (L, -2, "resources");
  lua_pushcclosure (L, lp_package_searcher, 1);

  lua_rawseti (L, -2, objlen + 1);
  lua_pop (L, 1);

  lua_pushcfunction (L, doinit);

  lua_createtable (L, argc, 0);

  for (i = 0; i < argc; ++i)
    {
      lua_pushstring (L, argv [i]);
      lua_rawseti (L, -2, i + 1);
    }

  switch ((result = lua_pcall (L, 1, 1, 1)))
    {
      case LUA_OK: result = (int) lua_tonumber (L, -1); break;
      case LUA_ERRRUN: g_critical ("(" G_STRLOC ") lua_pcall()!: %s", lua_tostring (L, -1)); break;
      case LUA_ERRMEM: g_error ("(" G_STRLOC ") lua_pcall()!: out of memory"); break;
      case LUA_ERRERR: g_error ("(" G_STRLOC ") lua_pcall()!: msghandler error"); break;
      default: g_error ("(" G_STRLOC ") lua_pcall()!: unknown error %i", result);
    }
return (lua_close (L), result);
}

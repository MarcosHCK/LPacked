/* Copyright 2023 MarcosHCK
 * This file is part of DigitalAir.
 *
 * DigitalAir is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DigitalAir is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DigitalAir. If not, see <http://www.gnu.org/licenses/>.
 */
#include <config.h>
#include <compat.h>

#if LUA_VERSION_NUM < 502

static int reader_s (lua_State* L)
{
  const int si = lua_upvalueindex (1);
  const int bi = lua_upvalueindex (2);

  if (lua_toboolean (L, bi) == 1)
    lua_pushnil (L);
  else
  {
    lua_pushboolean (L, 1);
    lua_replace (L, bi);
    lua_pushvalue (L, si);
  }
return 1;
}

int repl_load (lua_State* L)
{
  switch (lua_type (L, 1))
  {
    case LUA_TSTRING:
      lua_pushvalue (L, 1);
      lua_pushboolean (L, 0);
      lua_pushcclosure (L, reader_s, 2);
      lua_replace (L, 1);
    case LUA_TFUNCTION:
      lua_pushvalue (L, lua_upvalueindex (1));
      lua_insert (L, 1);
      lua_call (L, lua_gettop (L) - 1, LUA_MULTRET);
      break;

    default:
      {
        const char* name = lua_typename (L, lua_type (L, 1));
        const char* mesg = lua_pushfstring (L, "expected string or function, got %s", name);
          luaL_argerror (L, 1, mesg);
          break;
      }
  }
return lua_gettop (L);
}

# ifndef D_LUAJIT

#define LEVELS1 12
#define LEVELS2 10

 /*
 * luaL_traceback campatibility patch based
 * upon 'db_errorfb' function in file
 * ldblib.c, Lua 5.1 source
 */
void luaL_traceback (lua_State* L, lua_State* L1, const char* message, int level)
{
  lua_Debug ar;
  int firstpart = 1;
  int top = lua_gettop (L);
  lua_pushstring (L, message ? message : "");
  lua_pushliteral (L, "\nstack traceback:");

  while (lua_getstack (L1, level++, &ar))
  {
    if (level > LEVELS1 && firstpart)
    {
      if (!lua_getstack (L1, level + LEVELS2, &ar))
        --level;
      else
      {
        lua_pushliteral (L, "\n\t...");
        while (lua_getstack (L1, level + LEVELS2, &ar))
          ++level;
      }

      firstpart = 0;
      continue;
    }

    lua_pushliteral (L, "\n\t");
    lua_getinfo (L1, "Snl", &ar);
    lua_pushfstring (L, "%s:", ar.short_src);

    if (ar.currentline > 0)
      lua_pushfstring (L, "%d:", ar.currentline);
    if (*ar.namewhat != '\0')
      lua_pushfstring (L, " in function " LUA_QS, ar.name);
    else
    {
      switch (*ar.what)
      {
        case 'm':
          lua_pushliteral (L, " in main chunk");
          break;
        case 'C':
        case 't':
          lua_pushliteral (L, " ?");
          break;
        default:
          lua_pushfstring (L, " in function <%s:%d>", ar.short_src, ar.linedefined);
          break;
      }
    }

    lua_concat (L, lua_gettop (L) - top);
  }

  lua_concat (L, lua_gettop (L) - top);
}

int luaL_testudata (lua_State *L, int idx, const char* tname)
{
  void* ud = lua_touserdata (L, idx);
  int itis = 0;

  if (ud != NULL)
  {
    if (lua_getmetatable (L, idx))
    {
      lua_getfield (L, LUA_REGISTRYINDEX, tname);
      if (lua_rawequal (L, -1, -2))
        itis = 1;

      lua_pop (L, 2);
    }
  }
return itis;
}

# endif // D_LUAJIT
#endif // LUA_VERSION_NUM

int luaopen_compat (lua_State* L)
{
#if LUA_VERSION_NUM < 502
  lua_getglobal (L, "load");
  lua_pushcclosure (L, repl_load, 1);
  lua_setglobal (L, "load");

  lua_getglobal (L, "table");
  lua_getglobal (L, "unpack");
  lua_setfield (L, -2, "unpack");
  lua_pushnil (L);
  lua_setglobal (L, "unpack");
#endif // LUA_VERSION_NUM
return 0;
}

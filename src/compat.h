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
#ifndef __D_LUA_COMPAT__
#define __D_LUA_COMPAT__ 1

#if __cplusplus
extern "C" {
#endif // __cplusplus

#include <lauxlib.h>
#include <lua.h>
#ifndef LUA_OK
# define LUA_OK (0)
#endif // LUA_OK
#ifndef LUA_PATH_SEP
# ifdef LUA_PATHSEP
#   define LUA_PATH_SEP LUA_PATHSEP
# else
#   define LUA_PATH_SEP ";"
# endif // LUA_PATH_SEP
#endif // LUA_PATHSEP
#ifndef LUA_PATH_MARK
# define LUA_PATH_MARK "?"
#endif // LUA_PATHSEP

#if LUA_VERSION_NUM < 502
  int repl_load (lua_State* L);
# ifndef D_LUAJIT
# define luaL_setmetatable(L,tname) (luaL_getmetatable ((L), (tname)), lua_setmetatable ((L), -2))
  void luaL_traceback (lua_State* L, lua_State* L1, const char* message, int level);
  int luaL_testudata (lua_State *L, int idx, const char *tname);
# endif // D_LUAJIT
#endif // LUA_VERSION_NUM

  int luaopen_compat (lua_State* L);

#if __cplusplus
}
#endif // __cplusplus

#endif // __D_LUA_COMPAT__

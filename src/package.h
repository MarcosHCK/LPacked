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
#ifndef __LP_PACKAGE__
#define __LP_PACKAGE__
#include <gio/gio.h>

#define LP_RESOURCES_QUEUE "lpacked.resources.queue"

#if __cplusplus
extern "C" {
#endif // __cplusplus

#include <lua.h>

  void lp_package_create_resources (lua_State* L);
  void lp_package_delete_resource (lua_State* L, int idx, GResource* resource);
  void lp_package_insert_resource (lua_State* L, int idx, GResource* resource);
  int lp_package_searcher (lua_State* L);

#if __cplusplus
}
#endif // __cplusplus

#endif // __LP_PACKAGE__

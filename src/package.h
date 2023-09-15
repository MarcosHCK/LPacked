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

typedef struct _LpPackage LpPackage;
#define LP_RESOURCES_QUEUE "lpacked.resources.queue"

#if __cplusplus
extern "C" {
#endif // __cplusplus

#include <lua.h>

  GType lp_package_get_type (void) G_GNUC_CONST;
  void lp_package_delete (LpPackage* package, GResource* resource);
  void lp_package_insert (LpPackage* package, GResource* resource);
  GBytes* lp_package_lookup (LpPackage* package, const gchar* path, GError** error);
  LpPackage* lp_package_new ();
  LpPackage* lp_package_ref (LpPackage* package);
  void lp_package_unref (LpPackage* package);

#if __cplusplus
}
#endif // __cplusplus

#endif // __LP_PACKAGE__

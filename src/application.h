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
#ifndef __LP_APPLICATION__
#define __LP_APPLICATION__ 1
#include <gio/gio.h>

#define LP_TYPE_APPLICATION (lp_application_get_type ())

#if __cplusplus
extern "C" {
#endif // __cplusplus

  G_DECLARE_FINAL_TYPE (LpApplication, lp_application, LP, APPLICATION, GApplication);

#if __cplusplus
}
#endif // __cplusplus

#endif // __LP_APPLICATION__

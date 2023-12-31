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
#ifndef __LPACKED_BLOB__
#define __LPACKED_BLOB__ 1
#include <gio/gio.h>

#if __cplusplus
extern "C" {
#endif // __cplusplus

  GResource* lpacked_blob_load_from_bytes (GBytes* bytes, GError** error);
  GResource* lpacked_blob_load_from_gfile (GFile* file, GError** error);

#if __cplusplus
}
#endif // __cplusplus

#endif // __LPACKED_BLOB__

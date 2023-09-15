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
#ifndef __LP_PACK_BUILDER__
#define __LP_PACK_BUILDER__ 1
#include <gio/gio.h>

#define LP_TYPE_PACK_BUILDER (lp_pack_builder_get_type ())

#if __cplusplus
extern "C" {
#endif // __cplusplus

  G_DECLARE_FINAL_TYPE (LpPackBuilder, lp_pack_builder, LP, PACK_BUILDER, GObject);

  gchar* lp_canonicalize_alias (const gchar* path, const gchar* alias);
  void lp_pack_builder_add_from_bytes (LpPackBuilder* builder, const gchar* path, GBytes* bytes);
  gboolean lp_pack_builder_add_from_file (LpPackBuilder* builder, const gchar* path, GFile* file, GError** error);
  gboolean lp_pack_builder_add_from_filename (LpPackBuilder* builder, const gchar* path, const gchar* filename, GError** error);
  void lp_pack_builder_add_from_stream (LpPackBuilder* builder, const gchar* path, GInputStream* stream);
  LpPackBuilder* lp_pack_builder_new ();

#if __cplusplus
}
#endif // __cplusplus

#endif // __LP_PACK_BUILDER__

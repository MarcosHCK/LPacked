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
#ifndef __LPACKED_BUILDER__
#define __LPACKED_BUILDER__ 1
#include <gio/gio.h>
#include <gvdb/gvdb-builder.h>

#if __cplusplus
extern "C" {
#endif // __cplusplus

  GType lpacked_builder_get_type (void) G_GNUC_CONST;
  void lpacked_builder_add_source_from_bytes (GHashTable* builder, const gchar* name, GBytes* bytes);
  void lpacked_builder_add_source_from_file (GHashTable* builder, const gchar* name, const gchar* filename, GError** error);
  void lpacked_builder_add_source_from_gfile (GHashTable* builder, const gchar* name, GFile* file, GError** error);
  void lpacked_builder_add_source_from_stream (GHashTable* builder, const gchar* name, GInputStream* stream, GError** error);
  GHashTable* lpacked_builder_new ();
  gboolean lpacked_builder_write (GHashTable* builder, const gchar* filename, GError** error);

#if __cplusplus
}
#endif // __cplusplus

#endif // __LPACKED_BUILDER__

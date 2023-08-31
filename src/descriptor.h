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
#ifndef __LPACKED_DESCRIPTOR__
#define __LPACKED_DESCRIPTOR__ 1
#include <gio/gio.h>

typedef enum
{
  LPACKED_DESCRIPTOR_ERROR_FAILED,
  LPACKED_DESCRIPTOR_ERROR_LOAD,
  LPACKED_DESCRIPTOR_ERROR_MISSING_FIELD,
  LPACKED_DESCRIPTOR_ERROR_INVALID_FIELD,
} LPackedDescriptorError;

#define LPACKED_TYPE_DESCRIPTOR (lpacked_descriptor_get_type ())
#define LPACKED_DESCRIPTOR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), LPACKED_TYPE_DESCRIPTOR, LPackedDescriptor))
#define LPACKED_IS_DESCRIPTOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LPACKED_TYPE_DESCRIPTOR))
typedef struct _LPackedDescriptor LPackedDescriptor;

#define LPACKED_DESCRIPTOR_ERROR (lpacked_descriptor_error_quark ())

#if __cplusplus
extern "C" {
#endif // __cplusplus

  GQuark lpacked_descriptor_error_quark (void) G_GNUC_CONST;
  GType lpacked_descriptor_get_type (void) G_GNUC_CONST;
  GList* lpacked_descriptor_get_aliases (LPackedDescriptor* descriptor);
  GList* lpacked_descriptor_get_files (LPackedDescriptor* descriptor);
  const gchar* lpacked_descriptor_get_file_by_alias (LPackedDescriptor* descriptor, const gchar* alias);
  const gchar* lpacked_descriptor_get_name (LPackedDescriptor* descriptor);
  LPackedDescriptor* lpacked_descriptor_new_from_bytes (GBytes* bytes, GError** error);
  void lpacked_descriptor_new_from_bytes_async (GBytes* bytes, GAsyncReadyCallback callback, gpointer user_data);
  LPackedDescriptor* lpacked_descriptor_new_from_gfile (GFile* file, GError** error);
  void lpacked_descriptor_new_from_gfile_async (GFile* file, GAsyncReadyCallback callback, gpointer user_data);
  LPackedDescriptor* lpacked_descriptor_new_finish (GAsyncResult* result, GError** error);

#if __cplusplus
}
#endif // __cplusplus

#endif // __LPACKED_DESCRIPTOR__

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
#ifndef __LP_PACK_READER__
#define __LP_PACK_READER__ 1
#include <gio/gio.h>

#define LP_TYPE_PACK_READER (lp_pack_reader_get_type ())
#define LP_PACK_READER_ERROR (lp_pack_reader_error_quark ())

typedef enum
{
  LP_PACK_READER_ERROR_FAILED,
  LP_PACK_READER_ERROR_OPEN,
  LP_PACK_READER_ERROR_WRITE,
  LP_PACK_READER_ERROR_CLOSE,
} LpPackReaderError;

#if __cplusplus
extern "C" {
#endif // __cplusplus

  G_DECLARE_FINAL_TYPE (LpPackReader, lp_pack_reader, LP, PACK_READER, GObject);

  GQuark lp_pack_reader_error_quark (void) G_GNUC_CONST;

  void lp_pack_reader_add_from_bytes (LpPackReader* reader, GBytes* bytes);
  gboolean lp_pack_reader_add_from_file (LpPackReader* reader, GFile* file, GError** error);
  gboolean lp_pack_reader_add_from_filename (LpPackReader* reader, const gchar* filename, GError** error);
  void lp_pack_reader_add_from_stream (LpPackReader* reader, GInputStream* stream);
  LpPackReader* lp_pack_reader_new ();

#if __cplusplus
}
#endif // __cplusplus

#endif // __LP_PACK_READER__

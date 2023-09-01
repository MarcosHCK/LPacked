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
#include <config.h>
#include <blob.h>

GResource* lpacked_blob_load_from_bytes (GBytes* source_bytes, GError** error)
{
  GBytes* bytes = NULL;
  GError* tmperr = NULL;
  GInputStream* converter_stream = NULL;
  GInputStream* source_stream = NULL;
  GMemoryOutputStream* memory_stream = NULL;
  GOutputStream* target_stream = NULL;
  GOutputStreamSpliceFlags splice_flags = 0;
  GResource* resource = NULL;
  GZlibDecompressor* zlib_decompressor = NULL;

  zlib_decompressor = g_zlib_decompressor_new (G_ZLIB_COMPRESSOR_FORMAT_GZIP);
  target_stream = g_memory_output_stream_new_resizable ();
  source_stream = g_memory_input_stream_new_from_bytes (source_bytes);
  splice_flags = G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET;
  converter_stream = g_converter_input_stream_new (source_stream, G_CONVERTER (zlib_decompressor));

  g_object_unref (zlib_decompressor);
  g_object_unref (source_stream);

  if ((g_output_stream_splice (target_stream, converter_stream, splice_flags, NULL, &tmperr)), G_UNLIKELY (tmperr != NULL))
      g_propagate_error (error, tmperr);
    else
      {
        memory_stream = G_MEMORY_OUTPUT_STREAM (target_stream);
        bytes = g_memory_output_stream_steal_as_bytes (memory_stream);

        if ((resource = g_resource_new_from_data (bytes, &tmperr)), G_UNLIKELY (tmperr != NULL))
          g_propagate_error (error, tmperr);
          g_bytes_unref (bytes);
      }

  g_object_unref (target_stream);
  g_object_unref (converter_stream);
return resource;
}

GResource* lpacked_blob_load_from_gfile (GFile* file, GError** error)
{
  GBytes* bytes = NULL;
  GError* tmperr = NULL;
  GFileInputStream* file_stream = NULL;
  GInputStream* converter_stream = NULL;
  GMemoryOutputStream* memory_stream = NULL;
  GOutputStream* target_stream = NULL;
  GOutputStreamSpliceFlags splice_flags = 0;
  GResource* resource = NULL;
  GZlibDecompressor* zlib_decompressor = NULL;

  if ((file_stream = g_file_read (file, NULL, &tmperr)), G_UNLIKELY (tmperr != NULL))
    g_propagate_error (error, tmperr);
  else
    {
      zlib_decompressor = g_zlib_decompressor_new (G_ZLIB_COMPRESSOR_FORMAT_GZIP);
      target_stream = g_memory_output_stream_new_resizable ();
      splice_flags = G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET;
      converter_stream = g_converter_input_stream_new (G_INPUT_STREAM (file_stream), G_CONVERTER (zlib_decompressor));

      g_object_unref (zlib_decompressor);
      g_object_unref (file_stream);

      if ((g_output_stream_splice (target_stream, converter_stream, splice_flags, NULL, &tmperr)), G_UNLIKELY (tmperr != NULL))
        g_propagate_error (error, tmperr);
      else
        {
          memory_stream = G_MEMORY_OUTPUT_STREAM (target_stream);
          bytes = g_memory_output_stream_steal_as_bytes (memory_stream);

          if ((resource = g_resource_new_from_data (bytes, &tmperr)), G_UNLIKELY (tmperr != NULL))
            g_propagate_error (error, tmperr);
            g_bytes_unref (bytes);
        }

      g_object_unref (target_stream);
      g_object_unref (converter_stream);
    }
return resource;
}

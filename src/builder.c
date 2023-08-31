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
#include <builder.h>

static GvdbItem* get_parent (GHashTable* table, const gchar* const_key, gsize length)
{
  GvdbItem* grandparent = NULL;
  GvdbItem* parent = NULL;
  gchar* key = NULL;

  if (length == 1)
    return NULL;

  key = g_strndup (const_key, length);

  while (key[--length - 1] != '/');
  key [length] = '\0';

  if ((parent = g_hash_table_lookup (table, key)) == NULL)
    {
      parent = gvdb_hash_table_insert (table, key);

      if ((grandparent = get_parent (table, key, length)) != NULL)
	      gvdb_item_set_parent (parent, grandparent);
    }
return (g_free (key), parent);
}

static void construct_from_bytes (GHashTable* table, const gchar* name, GBytes* bytes, gboolean fixed)
{
  if (fixed == FALSE)
    {
      gsize size = g_bytes_get_size (bytes);
      gchar* buffer = g_malloc (size + 1);

      memcpy (buffer, g_bytes_get_data (bytes, NULL), size);

      buffer [size] = 0;
      bytes = g_bytes_new_take (buffer, size + 1);

      construct_from_bytes (table, name, bytes, TRUE);
      g_bytes_unref (bytes);
    }
  else
    {
      GvdbItem* item = gvdb_hash_table_insert (table, name);
      GVariant* value = g_variant_new_from_bytes (G_VARIANT_TYPE ("ay"), bytes, TRUE);
      GVariantBuilder vb;

      g_variant_builder_init (&vb, G_VARIANT_TYPE ("(uuay)"));
      g_variant_builder_add (&vb, "u", g_bytes_get_size (bytes));
      g_variant_builder_add (&vb, "u", G_RESOURCE_FLAGS_NONE);
      g_variant_builder_add_value (&vb, value);
      gvdb_item_set_value (item, g_variant_builder_end (&vb));
      gvdb_item_set_parent (item, get_parent (table, name, strlen (name)));
    }
}

void lpacked_builder_add_source_from_bytes (GHashTable* table, const gchar* name, GBytes* bytes)
{
  construct_from_bytes (table, name, bytes, FALSE);
}

void lpacked_builder_add_source_from_file (GHashTable* table, const gchar* name, const gchar* filename, GError** error)
{
  GError* tmperr = NULL;
  GMappedFile* mapped = NULL;

  if ((mapped = g_mapped_file_new (filename, FALSE, &tmperr)), G_UNLIKELY (tmperr != NULL))
    g_propagate_error (error, tmperr);
  else
    {
      construct_from_bytes (table, name, g_mapped_file_get_bytes (mapped), FALSE);
      g_mapped_file_unref (mapped);
    }
}

void lpacked_builder_add_source_from_gfile (GHashTable* table, const gchar* name, GFile* file, GError** error)
{
  GError* tmperr = NULL;
  GFileInputStream* stream = NULL;
  const gchar* path = NULL;

  if ((path = g_file_peek_path (file)) != NULL)
    lpacked_builder_add_source_from_file (table, name, path, error);
  else
    {
      if ((stream = g_file_read (file, NULL, &tmperr)), G_UNLIKELY (tmperr != NULL))
        g_propagate_error (error, tmperr);
      else
        {
          lpacked_builder_add_source_from_stream (table, name, G_INPUT_STREAM (stream), error);
          g_object_unref (stream);
        }
    }
}

void lpacked_builder_add_source_from_stream (GHashTable* table, const gchar* name, GInputStream* stream, GError** error)
{
  GBytes* bytes = NULL;
  GError* tmperr = NULL;
  GMemoryOutputStream* memory = NULL;
  GOutputStream* ostream = NULL;
  gchar null_terminator [] = {0};

  ostream = g_memory_output_stream_new_resizable ();

  if ((g_output_stream_splice (ostream, stream, G_OUTPUT_STREAM_SPLICE_NONE, NULL, &tmperr)), G_UNLIKELY (tmperr != NULL))
    {
      g_propagate_error (error, tmperr);
      g_object_unref (ostream);
    }
  else if ((g_output_stream_write_all (ostream, null_terminator, sizeof (null_terminator), NULL, NULL, &tmperr)), G_UNLIKELY (tmperr != NULL))
    {
      g_propagate_error (error, tmperr);
      g_object_unref (ostream);
    }
  else if ((g_output_stream_close (ostream, NULL, &tmperr)), G_UNLIKELY (tmperr != NULL))
    {
      memory = G_MEMORY_OUTPUT_STREAM (ostream);
      bytes = g_memory_output_stream_steal_as_bytes (memory);
      g_object_unref (ostream);

      construct_from_bytes (table, name, bytes, TRUE);
      g_bytes_unref (bytes);
    }
}

GHashTable* lpacked_builder_new ()
{
  return gvdb_hash_table_new (NULL, NULL);
}

static void dump_to_file (GHashTable* builder, GFile* file, GError** error)
{
  GError* tmperr = NULL;
  GFileOutputStream* file_stream = NULL;
  GOutputStream* converter_stream = NULL;
  GString* string = NULL;
  GZlibCompressor* zlib_compressor = NULL;

  if ((file_stream = g_file_replace (file, NULL, FALSE, 0, NULL, &tmperr)), G_UNLIKELY (tmperr != NULL))
    g_propagate_error (error, tmperr);
  else
    {
      zlib_compressor = g_zlib_compressor_new (G_ZLIB_COMPRESSOR_FORMAT_GZIP, 9);
      string = gvdb_table_serialize (builder, G_LITTLE_ENDIAN != G_BYTE_ORDER);
      converter_stream = g_converter_output_stream_new (G_OUTPUT_STREAM (file_stream), G_CONVERTER (zlib_compressor));

      g_object_unref (zlib_compressor);
      g_object_unref (file_stream);

      if ((g_output_stream_write_all (converter_stream, string->str, string->len, NULL, NULL, error)), G_UNLIKELY (tmperr != NULL))
        g_propagate_error (error, tmperr);
      else if ((g_output_stream_close (converter_stream, NULL, &tmperr)), G_UNLIKELY (tmperr != NULL))
        g_propagate_error (error, tmperr);
        g_object_unref (converter_stream);
        g_string_free (string, TRUE);
    }
}

gboolean lpacked_builder_write (GHashTable* builder, const gchar* filename, GError** error)
{
  GError* tmperr = NULL;
  GFile* file = g_file_new_for_path (filename);

  if ((dump_to_file (builder, file, &tmperr), g_object_unref (file)), G_UNLIKELY (tmperr != NULL))
    {
      g_propagate_error (error, tmperr);
      return FALSE;
    }
return TRUE;
}

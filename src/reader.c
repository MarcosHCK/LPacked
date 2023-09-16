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
#include <readaux.h>

#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))

struct _LpPackReader
{
  GObject parent;

  /* <private> */
  GTree* vfs;
};

struct _LpPackReaderStream
{
  GInputStream parent;

  /* <private> */
  Archive* ar;
  Reader reader;
  Source* source;
};

G_DEFINE_QUARK (lp-pack-reader-error-quark, lp_pack_reader_error);
G_DEFINE_FINAL_TYPE (LpPackReader, lp_pack_reader, G_TYPE_OBJECT);
G_DECLARE_FINAL_TYPE (LpPackReaderStream, lp_pack_reader_stream, LP, PACK_READER_STREAM, GInputStream);
G_DEFINE_FINAL_TYPE (LpPackReaderStream, lp_pack_reader_stream, G_TYPE_INPUT_STREAM);

static void lp_pack_reader_init (LpPackReader* self)
{
  const GCompareDataFunc func1 = (GCompareDataFunc) file_cmp;
  const GDestroyNotify func2 = (GDestroyNotify) file_free;
  const GDestroyNotify func3 = (GDestroyNotify) source_unref;

  self->vfs = g_tree_new_full (func1, NULL, func2, func3);
}

static void lp_pack_reader_class_dispose (GObject* pself)
{
  LpPackReader* self = (gpointer) pself;
  g_tree_remove_all (self->vfs);
  G_OBJECT_CLASS (lp_pack_reader_parent_class)->dispose (pself);
}

static void lp_pack_reader_class_finalize (GObject* pself)
{
  LpPackReader* self = (gpointer) pself;
  g_tree_unref (self->vfs);
  G_OBJECT_CLASS (lp_pack_reader_parent_class)->finalize (pself);
}

static void lp_pack_reader_class_init (LpPackReaderClass* klass)
{
  G_OBJECT_CLASS (klass)->dispose = lp_pack_reader_class_dispose;
  G_OBJECT_CLASS (klass)->finalize = lp_pack_reader_class_finalize;
}

static void lp_pack_reader_stream_init (LpPackReaderStream* self)
{
}

static gboolean lp_pack_reader_stream_class_close_fn (GInputStream* pself, GCancellable* cancellable, GError** error)
{
  LpPackReaderStream* self = (gpointer) pself;
return (closepack (self->ar, self->source, &self->reader, error) == ARCHIVE_OK);
}

static gssize lp_pack_reader_stream_class_read_fn (GInputStream* pself, void* buffer, gsize count, GCancellable* cancellable, GError** error)
{
  LpPackReaderStream* self = (gpointer) pself;
  la_ssize_t result;

  if ((result = archive_read_data (self->ar, buffer, count)), G_UNLIKELY (result < 0))
    {
      report (error, archive_read_data, self->ar, &self->reader);
      return -1;
    }
return (gssize) result;
}

static void lp_pack_reader_stream_class_dispose (GObject* pself)
{
  LpPackReaderStream* self = (gpointer) pself;
  g_clear_pointer (&self->ar, (GDestroyNotify) archive_read_free);
  g_clear_pointer (&self->source, (GDestroyNotify) source_unref);
  G_OBJECT_CLASS (lp_pack_reader_parent_class)->dispose (pself);
}

static void lp_pack_reader_stream_class_init (LpPackReaderStreamClass* klass)
{
  G_INPUT_STREAM_CLASS (klass)->close_fn = lp_pack_reader_stream_class_close_fn;
  G_INPUT_STREAM_CLASS (klass)->read_fn = lp_pack_reader_stream_class_read_fn;
  G_OBJECT_CLASS (klass)->dispose = lp_pack_reader_stream_class_dispose;
}

static int walkpack (LpPackReader* self, Archive* ar, Source* source, Reader* reader, GError** error)
{
  ArchiveEntry* ent = NULL;
  int result;

  while (TRUE)
    {
      if ((result = archive_read_next_header (ar, &ent)), G_UNLIKELY (result == ARCHIVE_FATAL))
        {
          report (error, archive_read_next_header, ar, reader);
          break;
        }
      else if (result == ARCHIVE_EOF)
        {
          result = ARCHIVE_OK;
          break;
        }

      const gchar* path = archive_entry_pathname_utf8 (ent);

      File template =
        {
          .path = g_strdup (path),
          .hash = g_str_hash (path),
        };

      if (g_tree_lookup_extended (self->vfs, &template, NULL, NULL) == FALSE)
        g_tree_insert (self->vfs, g_slice_dup (File, &template), source_ref (source));
      else
        {
          result = ARCHIVE_FATAL;
          g_set_error (error, LP_PACK_READER_ERROR, LP_PACK_READER_ERROR_SCAN, "duplicated entry '%s'", path);
          g_free (template.path);
          break;
        }
    }
return result;
}

static gboolean scanpack (LpPackReader* self, Source* source, GError** error)
{
  Archive* ar = NULL;
  Reader reader = {0};
  int result;

  ar = archive_read_new ();

  if ((result = openpack (ar, source, &reader, error)), G_LIKELY (result == ARCHIVE_OK))
    {
      if ((result = walkpack (self, ar, source, &reader, error)), G_UNLIKELY (result != ARCHIVE_OK))
        closepack (ar, source, &reader, NULL);
      else
        {
          result = closepack (ar, source, &reader, error);
        }
    }
return (archive_read_free (ar), result == ARCHIVE_OK);
}

/**
 * lp_pack_reader_add_from_bytes:
 * @reader: #LpPackReader instance.
 * @bytes: data to be added into @reader output.
 * @error: return location for a #GError, or %NULL.
 * 
 * Adds data contained in @bytes into @reader under @path.
 * 
 * Returns: if operation was successful.
*/
gboolean lp_pack_reader_add_from_bytes (LpPackReader* reader, GBytes* bytes, GError** error)
{
  g_return_val_if_fail (LP_IS_PACK_READER (reader), FALSE);
  g_return_val_if_fail (bytes != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  LpPackReader* self = (reader);
  Source template = { .refs = 1, .type = source_bytes, .bytes = g_bytes_ref (bytes), };
  Source* source = g_slice_dup (Source, &template);
  gboolean good = scanpack (self, source, error);
return (source_unref (source), good);
}

/**
 * lp_pack_reader_add_from_file:
 * @reader: #LpPackReader instance.
 * @file: #GFile instance.
 * @error: return location for a #GError, or %NULL.
 *
 * Adds data from file pointed by @file into @reader under @path.
 * 
 * Returns: if operation was successful.
*/
gboolean lp_pack_reader_add_from_file (LpPackReader* reader, GFile* file, GError** error)
{
  g_return_val_if_fail (LP_IS_PACK_READER (reader), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  LpPackReader* self = (reader);
  Source template = { .refs = 1, .type = source_file, .file = g_object_ref (file), };
  Source* source = g_slice_dup (Source, &template);
  gboolean good = scanpack (self, source, error);
return (source_unref (source), good);
}

/**
 * lp_pack_reader_add_from_filename:
 * @reader: #LpPackReader instance.
 * @filename: file to extract data from.
 * @error: return location for a #GError, or %NULL.
 *
 * Similar to #lp_pack_reader_add_from_file but takes a filename
 * instead of a #GFile instance. Uses #g_file_new_for_path so
 * make sure @filename is compatible with it.
 *
 * Returns: if operation was successful.
*/
gboolean lp_pack_reader_add_from_filename (LpPackReader* reader, const gchar* filename, GError** error)
{
  g_return_val_if_fail (LP_IS_PACK_READER (reader), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  GFile* file = g_file_new_for_path (filename);
  gboolean good = lp_pack_reader_add_from_file (reader, file, error);
return (g_object_unref (file), good);
}

/**
 * lp_pack_reader_add_from_stream:
 * @reader: #LpPackReader instance.
 * @stream: stream to extract data from.
 * @error: return location for a #GError, or %NULL.
 *
 * Similar to #lp_pack_reader_add_from_bytes, but takes an
 * #GInputStream instead of #GBytes. Upon output data from
 * @stream will be spliced.
 * 
 * Returns: if operation was successful.
*/
gboolean lp_pack_reader_add_from_stream (LpPackReader* reader, GInputStream* stream, GError** error)
{
  g_return_val_if_fail (LP_IS_PACK_READER (reader), FALSE);
  g_return_val_if_fail (G_IS_INPUT_STREAM (stream), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  LpPackReader* self = (reader);

  if (G_IS_SEEKABLE (stream) && g_seekable_can_seek (G_SEEKABLE (stream)))
    {
      /* Resetable stream */
      Source template = { .refs = 1, .blocked = FALSE, .type = source_stream, .stream = g_object_ref (stream), };
      Source* source = g_slice_dup (Source, &template);
      gboolean good = scanpack (self, source, error);
      return (source_unref (source), good);
    }
  else
    {
      /* No resetable stream, dump it to bytes */
      GBytes* bytes;
      GOutputStream* target = g_memory_output_stream_new_resizable ();
      GOutputStreamSpliceFlags flags = G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET;
      gboolean good = g_output_stream_splice (target, stream, flags, NULL, error);

      if (good)
        {
          bytes = g_memory_output_stream_steal_as_bytes (G_MEMORY_OUTPUT_STREAM (target));
          good = lp_pack_reader_add_from_bytes (self, bytes, error);
                  g_bytes_unref (bytes);
        }

      return (g_object_unref (target), good);
    }
}

/**
 * lp_pack_reader_contains:
 * @reader: #LpPackReader instance.
 * @path: path to look up in @reader.
 * 
 * Returns: whether @reader contains @path.
*/
gboolean lp_pack_reader_contains (LpPackReader* reader, const gchar* path)
{
  g_return_val_if_fail (LP_IS_PACK_READER (reader), FALSE);
  g_return_val_if_fail (path != NULL, FALSE);
  LpPackReader* self = (reader);
  gchar* canon = (gchar*) g_canonicalize_filename (path, "/");
  gchar* value = (gchar*) g_path_skip_root (canon);
  File file = { .path = value, .hash = g_str_hash (value), };
  gboolean has = g_tree_lookup_extended (self->vfs, &file, NULL, NULL);
return (g_free (canon), has);
}

/**
 * lp_pack_reader_new: (constructor)
 * 
 * Creates and initializes a #LpPackReader instance.
 * 
 * Returns: (transfer full): a new #LpPackReader instance.
*/
LpPackReader* lp_pack_reader_new ()
{
  return g_object_new (LP_TYPE_PACK_READER, NULL);
}

/**
 * lp_pack_reader_open:
 * @reader: #LpPackReader instance.
 * @path: path to look up in @reader.
 * @error: return location for a #GError, or %NULL.
 * 
 * Opens packed file @path
 * 
 * Returns: (transfer full): a #GInputStream where to read @path.
*/
GInputStream* lp_pack_reader_open (LpPackReader* reader, const gchar* path, GError** error)
{
  g_return_val_if_fail (LP_IS_PACK_READER (reader), NULL);
  g_return_val_if_fail (path != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  LpPackReader* self = (reader);
  gchar* canon = (gchar*) g_canonicalize_filename (path, "/");
  gchar* value = (gchar*) g_path_skip_root (canon);
  File file = { .path = value, .hash = g_str_hash (value), };
  Source* source = g_tree_lookup (self->vfs, &file);
  LpPackReaderStream* stream = NULL;
  int result;

  if (G_UNLIKELY (source == NULL))
    g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "path '%s' not found", path);
  else
    {
      stream = g_object_new (lp_pack_reader_stream_get_type (), NULL);
      stream->ar = archive_read_new ();
      stream->source = source_ref (source);

      if ((result = openpack (stream->ar, stream->source, &stream->reader, error)), G_UNLIKELY (result != ARCHIVE_OK))
        {
          g_input_stream_close ((GInputStream*) stream, NULL, NULL);
          g_clear_object (&stream);
        }
      else
        {
          ArchiveEntry* ent = NULL;

          while (TRUE)
            {
              if ((result = archive_read_next_header (stream->ar, &ent)), G_UNLIKELY (result != ARCHIVE_OK))
                {
                  report (error, archive_read_next_header, stream->ar, &stream->reader);
                  break;
                }
              else if (G_UNLIKELY (result == ARCHIVE_EOF))
                g_assert_not_reached ();
              else
                {
                  const gchar* pathname = archive_entry_pathname_utf8 (ent);
                  File file2 = { .path = (gchar*) pathname, .hash = g_str_hash (pathname), };

                  if (file_cmp (&file, &file2) == 0)
                    break;
                }
            }

          if (G_UNLIKELY (result != ARCHIVE_OK))
            {
              g_input_stream_close ((GInputStream*) stream, NULL, NULL);
              g_clear_object (&stream);
            }
        }
    }
return (GInputStream*) stream;
}

/**
 * lp_pack_reader_query_info:
 * @reader: #LpPackReader instance.
 * @path: path to look up in @reader.
 * @attributes: an attribute query string.
 * @error: return location for a #GError, or %NULL.
 *
 * Queries info about @path.
 *
 * Returns: (transfer full): a #GFileInfo containig info about @path.
 */
GFileInfo* lp_pack_reader_query_info (LpPackReader* reader, const gchar* path, const gchar* attributes, GError** error)
{
  g_return_val_if_fail (LP_IS_PACK_READER (reader), NULL);
  g_return_val_if_fail (path != NULL, NULL);
  g_return_val_if_fail (attributes != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  LpPackReader* self = (reader);
  gchar* canon = (gchar*) g_canonicalize_filename (path, "/");
  gchar* value = (gchar*) g_path_skip_root (canon);
  File file = { .path = value, .hash = g_str_hash (value), };
  Source* source = g_tree_lookup (self->vfs, &file);
  GFileInfo* info = NULL;
  int result;

  if (G_UNLIKELY (source == NULL))
    g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "path '%s' not found", path);
  else
    {
      Archive* ar = NULL;
      ArchiveEntry* ent = NULL;
      Reader reader = {0};

      if ((result = openpack (ar = archive_read_new (), source, &reader, error)), G_LIKELY (result == ARCHIVE_OK))
        {
          while (TRUE)
            {
              if ((result = archive_read_next_header (ar, &ent)), G_UNLIKELY (result != ARCHIVE_OK))
                {
                  report (error, archive_read_next_header, ar, &reader);
                  break;
                }
              else if (G_UNLIKELY (result == ARCHIVE_EOF))
                g_assert_not_reached ();
              else
                {
                  const gchar* pathname = archive_entry_pathname_utf8 (ent);
                  File file2 = { .path = (gchar*) pathname, .hash = g_str_hash (pathname), };

                  if (file_cmp (&file, &file2) == 0)
                    break;
                }
            }

          if (G_UNLIKELY (result != ARCHIVE_OK))
            closepack (ar, source, &reader, NULL);
          else
            {
              GFileAttributeMatcher* matcher = NULL;

              matcher = g_file_attribute_matcher_new (attributes);
              info = g_file_info_new ();

              if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_TYPE))
                g_file_info_set_file_type (info, G_FILE_TYPE_REGULAR);
              if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN))
                g_file_info_set_is_hidden (info, FALSE);
              if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_NAME))
                {
                  gchar* pathname = (gchar*) archive_entry_pathname (ent);
                  gchar* basename = (gchar*) g_path_get_basename (pathname);

                  g_file_info_set_name (info, basename);
                  g_free (basename);
                }
              if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME))
                {
                  gchar* pathname = (gchar*) archive_entry_pathname_utf8 (ent);
                  gchar* basename = (gchar*) g_path_get_basename (pathname);

                  g_file_info_set_display_name (info, basename);
                  g_free (basename);
                }
              if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_EDIT_NAME))
                {
                  gchar* pathname = (gchar*) archive_entry_pathname_utf8 (ent);
                  gchar* basename = (gchar*) g_path_get_basename (pathname);

                  g_file_info_set_edit_name (info, basename);
                  g_free (basename);
                }
              if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_COPY_NAME))
                {
                  gchar* pathname = (gchar*) archive_entry_pathname_utf8 (ent);
                  gchar* basename = (gchar*) g_path_get_basename (pathname);

                  g_file_info_set_attribute_string (info, G_FILE_ATTRIBUTE_STANDARD_COPY_NAME, basename);
                  g_free (basename);
                }

              if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_SIZE))
                g_file_info_set_size (info, (goffset) archive_entry_size (ent));
              if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_ALLOCATED_SIZE))
                g_file_info_set_attribute_uint64 (info, G_FILE_ATTRIBUTE_STANDARD_ALLOCATED_SIZE, archive_entry_size (ent));
              if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET))
                g_file_info_set_symlink_target (info, archive_entry_symlink (ent));
              if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_ACCESS_CAN_READ))
                g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ, TRUE);

              if (archive_entry_atime_is_set (ent))
                {
                  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_TIME_ACCESS))
                    g_file_info_set_attribute_uint64 (info, G_FILE_ATTRIBUTE_TIME_ACCESS, archive_entry_atime (ent));
                  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_TIME_ACCESS_NSEC))
                    g_file_info_set_attribute_uint64 (info, G_FILE_ATTRIBUTE_TIME_ACCESS_NSEC, archive_entry_atime_nsec (ent));
                }

              if (archive_entry_birthtime_is_set (ent))
                {
                  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_TIME_CREATED))
                    g_file_info_set_attribute_uint64 (info, G_FILE_ATTRIBUTE_TIME_CREATED, archive_entry_birthtime (ent));
                  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_TIME_CREATED_NSEC))
                    g_file_info_set_attribute_uint64 (info, G_FILE_ATTRIBUTE_TIME_CREATED_NSEC, archive_entry_birthtime_nsec (ent));
                }

              if (archive_entry_ctime_is_set (ent))
                {
                  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_TIME_CHANGED))
                    g_file_info_set_attribute_uint64 (info, G_FILE_ATTRIBUTE_TIME_CHANGED, archive_entry_ctime (ent));
                  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_TIME_CHANGED_NSEC))
                    g_file_info_set_attribute_uint64 (info, G_FILE_ATTRIBUTE_TIME_CHANGED_NSEC, archive_entry_ctime_nsec (ent));
                }

              g_file_attribute_matcher_unref (matcher);

              if ((result = closepack (ar, source, &reader, error)), G_UNLIKELY (result != ARCHIVE_OK))
                g_clear_object (&info);
            }
        }
    }
return info;
}

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
#include <archive.h>
#include <archive_entry.h>
#include <format.h>
#include <reader.h>

#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))

typedef struct archive Archive;
typedef struct archive_entry ArchiveEntry;
typedef struct _File File;
typedef struct _Reader Reader;
typedef struct _Source Source;

struct _LpPackReader
{
  GObject parent;

  /* <private> */
  GTree* vfs;
};

struct _File
{
  gchar* path;
  guint hash;
};

struct _Source
{
  guint refs : (sizeof (guint) << 3) - 2;
  guint type : 2;

  union
  {
    GBytes* bytes;
    GFile* file;
    GInputStream* stream;
  };
} *source;

enum
{
  prop_0,
  prop_base_file,
  prop_number,
};

enum
{
  source_bytes,
  source_file,
  source_stream,
};

G_DEFINE_FINAL_TYPE (LpPackReader, lp_pack_reader, G_TYPE_OBJECT);
G_DEFINE_QUARK (lp-pack-reader-error-quark, lp_pack_reader_error);

static GParamSpec* properties [prop_number] = {0};

static int file_cmp (File* file_a, File* file_b)
{
  if (file_a->hash != file_b->hash)
    return file_a->hash - file_b->hash;
  else
    return g_strcmp0 (file_a->path, file_b->path);
}

static void file_free (File* file)
{
  g_free (file->path);
  g_slice_free (File, file);
}

static Source* source_ref (Source* source)
{
  return (++source->refs, source);
}

static void source_unref (Source* source)
{
  if (--source->refs == 0)
    {
      switch (source->type)
        {
          case source_bytes: g_bytes_unref (source->bytes); break;
          case source_file: g_object_unref (source->file); break;
          case source_stream: g_object_unref (source->stream); break;
        }

      g_slice_free (Source, source);
    }
}

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

struct _Reader
{
  gchar buffer [512];
  GError* error;

  union
  {
    GFile* file;
    GInputStream* stream;
  };
};

static int on_close (struct archive* ar, void* user_data)
{
  GError** error = & G_STRUCT_MEMBER (GError*, user_data, G_STRUCT_OFFSET (Reader, error));
  GInputStream* stream = G_STRUCT_MEMBER (gpointer, user_data, G_STRUCT_OFFSET (Reader, stream));
  int result = ARCHIVE_OK;

  if ((result = g_input_stream_close (stream, NULL, error)), G_UNLIKELY (result == FALSE))
    result = ARCHIVE_FATAL;
return (g_object_unref (stream), result);
}

static int on_open (struct archive* ar, void* user_data)
{
  GError** error = & G_STRUCT_MEMBER (GError*, user_data, G_STRUCT_OFFSET (Reader, error));
  GFile* file = G_STRUCT_MEMBER (gpointer, user_data, G_STRUCT_OFFSET (Reader, file));
  GFileInputStream* stream = NULL;
  int result = ARCHIVE_OK;

  if ((stream = g_file_read (file, NULL, error)), G_UNLIKELY (stream == NULL))
    result = ARCHIVE_FATAL;
  G_STRUCT_MEMBER (gpointer, user_data, G_STRUCT_OFFSET (Reader, stream)) = stream;
return (result);
}

G_STATIC_ASSERT (sizeof (la_ssize_t) == sizeof (gssize));
G_STATIC_ASSERT (sizeof (la_int64_t) == sizeof (gssize));

static la_ssize_t on_read (struct archive* ar, void* user_data, const void** out_buffer)
{
  gchar* buffer = & G_STRUCT_MEMBER (gchar, user_data, G_STRUCT_OFFSET (Reader, buffer [0]));
  GError** error = & G_STRUCT_MEMBER (GError*, user_data, G_STRUCT_OFFSET (Reader, error));
  GInputStream* stream = G_STRUCT_MEMBER (gpointer, user_data, G_STRUCT_OFFSET (Reader, stream));
  const gsize count = G_SIZEOF_MEMBER (Reader, buffer);
  gssize result = ARCHIVE_OK;

  if ((result = g_input_stream_read (stream, buffer, count, NULL, error)), G_UNLIKELY (result < 0))
    result = (gssize) ARCHIVE_FATAL;
return (*out_buffer = buffer, result);
}

static la_int64_t on_skip (struct archive* ar, void* user_data, la_int64_t request)
{
  GError** error = & G_STRUCT_MEMBER (GError*, user_data, G_STRUCT_OFFSET (Reader, error));
  GInputStream* stream = G_STRUCT_MEMBER (gpointer, user_data, G_STRUCT_OFFSET (Reader, stream));
  gssize result = ARCHIVE_OK;

  if ((result = g_input_stream_skip (stream, request, NULL, error)), G_UNLIKELY (result < 0))
    result = ARCHIVE_FATAL;
return (result);
}

static int openpack (LpPackReader* self, Archive* ar, Source* source, Reader* reader, GError** error)
{
  switch (source->type)
    {
      case source_bytes:
        {
          gsize size;
          gconstpointer data = g_bytes_get_data (source->bytes, &size);
          return archive_read_open_memory (ar, data, size);
        }

      case source_file:
        reader->file = source->file;
        return archive_read_open2 (ar, reader, on_open, on_read, on_skip, on_close);

      case source_stream:
        reader->stream = source->stream;
        return archive_read_open2 (ar, reader, NULL, on_read, on_skip, NULL);
    }
return ARCHIVE_FAILED;
}

#define report(error, funcname, ar, reader) \
  G_STMT_START { \
    Archive* __archive = (ar); \
    GError** __error = (error); \
    Reader* __reader = (reader); \
 ; \
    if (G_LIKELY (__reader->error != NULL)) \
      g_propagate_error (error, __reader->error); \
    else \
      { \
        const GQuark __domain = LP_PACK_READER_ERROR; \
        const guint __code = LP_PACK_READER_ERROR_WRITE; \
        const gchar* __strerror = archive_error_string (__archive); \
 ; \
        g_set_error (__error, __domain, __code, #funcname "()!: %s", __strerror); \
      } \
  } G_STMT_END

static int walkpack (LpPackReader* self, Archive* ar, Source* source, Reader* reader, GError** error)
{
  ArchiveEntry* ent = NULL;
  int result;

  while (TRUE)
    {
      if ((result = archive_read_next_header (ar, &ent)), G_UNLIKELY (result == ARCHIVE_FATAL))
        report (error, archive_read_next_header, ar, reader);
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

      g_tree_insert (self->vfs, g_slice_dup (File, &template), source_ref (source));
    }
return result;
}

static gboolean scanpack (LpPackReader* self, Source* source, GError** error)
{
  Archive* ar = NULL;
  Reader reader = {0};
  int result;

  ar = archive_read_new ();

  if ((result = archive_read_append_filter (ar, LP_PACK_COMPRESSION)), G_UNLIKELY (result != ARCHIVE_OK))
    g_set_error (error, LP_PACK_READER_ERROR, LP_PACK_READER_ERROR_OPEN, "archive_read_append_filter()!: %s", archive_error_string (ar));
  else if ((result = archive_read_set_format (ar, LP_PACK_FORMAT)), G_UNLIKELY (result != ARCHIVE_OK))
    g_set_error (error, LP_PACK_READER_ERROR, LP_PACK_READER_ERROR_OPEN, "archive_read_set_format()!: %s", archive_error_string (ar));
  else if ((result = openpack (self, ar, source, &reader, error)), G_LIKELY (result == ARCHIVE_OK))
    {
      if ((result = walkpack (self, ar, source, &reader, error)), G_LIKELY (result == ARCHIVE_OK))
      if ((result = archive_read_close (ar)), G_UNLIKELY (result != ARCHIVE_OK))
        {
          if (G_LIKELY (reader.error != NULL))
            g_propagate_error (error, reader.error);
          else
            {
              const GQuark domain = LP_PACK_READER_ERROR;
              const guint code = LP_PACK_READER_ERROR_CLOSE;
              const gchar* message = archive_error_string (ar);

              g_set_error (error, domain, code, "archive_read_close()!: %s", message);
            }
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
      Source template = { .refs = 1, .type = source_stream, .stream = g_object_ref (stream), };
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

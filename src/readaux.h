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
#pragma once
#include <archive.h>
#include <archive_entry.h>
#include <format.h>
#include <gio/gio.h>
#include <reader.h>

typedef struct _File
{
  gchar* path;
  guint hash;
} File;

typedef struct _Reader
{
  gchar buffer [512];
  GError* error;

  union
  {
    GFile* file;
    GInputStream* stream;
  };
} Reader;

typedef struct _Source
{
  guint refs : (sizeof (guint) << 3) - 3;
  guint blocked : 1;
  guint type : 2;

  union
  {
    GBytes* bytes;
    GFile* file;
    GInputStream* stream;
  };
} Source;

enum
{
  source_bytes,
  source_file,
  source_stream,
};

typedef struct archive Archive;
typedef struct archive_entry ArchiveEntry;

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

static int on_close (struct archive* ar, void* user_data)
{
  GError** error = & G_STRUCT_MEMBER (GError*, user_data, G_STRUCT_OFFSET (Reader, error));
  GInputStream* stream = G_STRUCT_MEMBER (GInputStream*, user_data, G_STRUCT_OFFSET (Reader, stream));
  int result = ARCHIVE_OK;

  if ((result = g_input_stream_close (stream, NULL, error)), G_UNLIKELY (result == FALSE))
    result = ARCHIVE_FATAL;
return (g_object_unref (stream), result);
}

static int on_open (struct archive* ar, void* user_data)
{
  GError** error = & G_STRUCT_MEMBER (GError*, user_data, G_STRUCT_OFFSET (Reader, error));
  GFile* file = G_STRUCT_MEMBER (GFile*, user_data, G_STRUCT_OFFSET (Reader, file));
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
  GInputStream* stream = G_STRUCT_MEMBER (GInputStream*, user_data, G_STRUCT_OFFSET (Reader, stream));
  const gsize count = G_SIZEOF_MEMBER (Reader, buffer);
  gssize result = ARCHIVE_OK;

  if ((result = g_input_stream_read (stream, buffer, count, NULL, error)), G_UNLIKELY (result < 0))
    result = (gssize) ARCHIVE_FATAL;
return (*out_buffer = buffer, result);
}

static la_int64_t on_skip (struct archive* ar, void* user_data, la_int64_t request)
{
  GError** error = & G_STRUCT_MEMBER (GError*, user_data, G_STRUCT_OFFSET (Reader, error));
  GInputStream* stream = G_STRUCT_MEMBER (GInputStream*, user_data, G_STRUCT_OFFSET (Reader, stream));
  gssize result = ARCHIVE_OK;

  if ((result = g_input_stream_skip (stream, request, NULL, error)), G_UNLIKELY (result < 0))
    result = ARCHIVE_FATAL;
return (result);
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
        const guint __code = LP_PACK_READER_ERROR_SCAN; \
        const gchar* __strerror = archive_error_string (__archive); \
 ; \
        g_set_error (__error, __domain, __code, #funcname "()!: %s", __strerror); \
      } \
  } G_STMT_END

static int closepack (Archive* ar, Source* source, Reader* reader, GError** error)
{
  int result;
  if ((result = archive_read_close (ar)), G_UNLIKELY (result != ARCHIVE_OK))
    report (error, archive_read_close, ar, reader);
return (source->blocked = FALSE, result);
}

static int openpack (Archive* ar, Source* source, Reader* reader, GError** error)
{
  int result = ARCHIVE_OK;

  if ((result = archive_read_append_filter (ar, LP_PACK_COMPRESSION)), G_UNLIKELY (result != ARCHIVE_OK))
    g_set_error (error, LP_PACK_READER_ERROR, LP_PACK_READER_ERROR_OPEN, "archive_read_append_filter()!: %s", archive_error_string (ar));
  else if ((result = archive_read_set_format (ar, LP_PACK_FORMAT)), G_UNLIKELY (result != ARCHIVE_OK))
    g_set_error (error, LP_PACK_READER_ERROR, LP_PACK_READER_ERROR_OPEN, "archive_read_set_format()!: %s", archive_error_string (ar));
  else
    {
      switch (source->type)
        {
          case source_bytes:
            {
              gsize size;
              gconstpointer data;

              data = g_bytes_get_data (source->bytes, &size);
              result = archive_read_open_memory (ar, data, size);
              break;
            }

          case source_file:
            reader->file = source->file;
            result = archive_read_open2 (ar, reader, on_open, on_read, on_skip, on_close);
            break;

          case source_stream:
            {
              if (source->blocked == FALSE)
                source->blocked = TRUE;
              else
                {
                  const GQuark domain = LP_PACK_READER_ERROR;
                  const guint code = LP_PACK_READER_ERROR_OPEN;
                  const gchar* message = "pack source is blocked";

                  g_set_error_literal (error, domain, code, message);
                  return ARCHIVE_FATAL;
                }

              if ((result = g_seekable_seek (G_SEEKABLE (source->stream), 0, G_SEEK_SET, NULL, error)), G_UNLIKELY (result == FALSE))
                return ARCHIVE_FATAL;

              reader->stream = source->stream;
              result = archive_read_open2 (ar, reader, NULL, on_read, on_skip, NULL);
              break;
            }
        }

      if (G_UNLIKELY (result != ARCHIVE_OK))
        {
          result = ARCHIVE_FATAL;
          source->blocked = FALSE;

          report (error, archive_read_open*, ar, reader);
        }
    }
return result;
}

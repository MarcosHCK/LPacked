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
#include <builder.h>
#include <format.h>

typedef struct _Source Source;

struct _LpPackBuilder
{
  GObject parent;

  /* <private> */
  GKeyFile* manifest;
  GHashTable* sources;
};

struct _Source
{
  GInputStream* stream;
  gsize size;
};

enum
{
  prop_0,
  prop_name,
  prop_description,
  prop_number,
};

G_DEFINE_FINAL_TYPE (LpPackBuilder, lp_pack_builder, G_TYPE_OBJECT);
G_DEFINE_QUARK (lp-pack-builder-error-quark, lp_pack_builder_error);

static GParamSpec* properties [prop_number] = {0};

static void source_free (gpointer ptr)
{
  Source* source = ptr;

  g_object_unref (source->stream);
  g_slice_free (Source, source);
}

static void lp_pack_builder_init (LpPackBuilder* self)
{
  const GHashFunc func1 = g_str_hash;
  const GEqualFunc func2 = g_str_equal;
  const GDestroyNotify func3 = g_free;
  const GDestroyNotify func4 = source_free;

  self->manifest = g_key_file_new ();
  self->sources = g_hash_table_new_full (func1, func2, func3, func4);
}

static void lp_pack_builder_class_dispose (GObject* pself)
{
  LpPackBuilder* self = (gpointer) pself;
  g_hash_table_remove_all (self->sources);
G_OBJECT_CLASS (lp_pack_builder_parent_class)->dispose (pself);
}

static void lp_pack_builder_class_finalize (GObject* pself)
{
  LpPackBuilder* self = (gpointer) pself;
  g_key_file_free (self->manifest);
  g_hash_table_unref (self->sources);
G_OBJECT_CLASS (lp_pack_builder_parent_class)->finalize (pself);
}

static void lp_pack_builder_class_get_property (GObject* pself, guint property_id, GValue* value, GParamSpec* pspec)
{
  LpPackBuilder* self = (gpointer) pself;

  switch (property_id)
    {
      default: G_OBJECT_WARN_INVALID_PROPERTY_ID (pself, property_id, pspec); break;
      case prop_name: g_value_take_string (value, g_key_file_get_string (self->manifest, LP_PACK_MANIFEST_GROUP, LP_PACK_MANIFEST_KEY_NAME, NULL)); break;
      case prop_description: g_value_take_string (value, g_key_file_get_string (self->manifest, LP_PACK_MANIFEST_GROUP, LP_PACK_MANIFEST_KEY_DESCRIPTION, NULL)); break;
    }
}

static void lp_pack_builder_class_set_property (GObject* pself, guint property_id, const GValue* value, GParamSpec* pspec)
{
  LpPackBuilder* self = (gpointer) pself;

  switch (property_id)
    {
      default: G_OBJECT_WARN_INVALID_PROPERTY_ID (pself, property_id, pspec); break;
      case prop_name: g_key_file_set_string (self->manifest, LP_PACK_MANIFEST_GROUP, LP_PACK_MANIFEST_KEY_NAME, g_value_get_string (value)); break;
      case prop_description: g_key_file_set_string (self->manifest, LP_PACK_MANIFEST_GROUP, LP_PACK_MANIFEST_KEY_DESCRIPTION, g_value_get_string (value)); break;
    }
}

static void lp_pack_builder_class_init (LpPackBuilderClass* klass)
{
  G_OBJECT_CLASS (klass)->dispose = lp_pack_builder_class_dispose;
  G_OBJECT_CLASS (klass)->finalize = lp_pack_builder_class_finalize;
  G_OBJECT_CLASS (klass)->get_property = lp_pack_builder_class_get_property;
  G_OBJECT_CLASS (klass)->set_property = lp_pack_builder_class_set_property;

  properties [prop_name] = g_param_spec_string ("name", "name", "name", NULL, G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);
  properties [prop_description] = g_param_spec_string ("description", "description", "description", NULL, G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);
  g_object_class_install_properties (G_OBJECT_CLASS (klass), prop_number, properties);
}

/**
 * lp_canonicalize_alias:
 * @basepath: base path to use as prefix.
 * @alias: alias to canonicalize.
 * 
 * Canonicalize @alias, appending @basepath to it if
 * @alias is relative, or returns a copy of @alias
 * otherwise. Also canonicalize both @basepath and @alias.
 * 
 * Returns: (transfer full): canonical form of @alias.
*/
gchar* lp_canonicalize_alias (const gchar* basepath, const gchar* alias)
{
  g_return_val_if_fail (basepath != NULL, NULL);
  g_return_val_if_fail (alias != NULL, NULL);
  GPathBuf pathbuf = G_PATH_BUF_INIT;

  g_path_buf_push (&pathbuf, "/");
  g_path_buf_push (&pathbuf, basepath);
  g_path_buf_push (&pathbuf, alias);
return g_path_buf_clear_to_path (&pathbuf);
}

/**
 * lp_canonicalize_pack_name:
 * @pack_name: package name to canonicalize.
 * 
 * Canonicalize @pack_name so its suitable for output
 * naming.
 * 
 * Returns: (transfer full): canonical form of @alias.
*/
gchar* lp_canonicalize_pack_name (const gchar* pack_name)
{
  g_return_val_if_fail (pack_name != NULL, NULL);
  GPathBuf pathbuf = G_PATH_BUF_INIT;

  g_path_buf_push (&pathbuf, pack_name);
  g_path_buf_set_extension (&pathbuf, "lpack");
return g_path_buf_clear_to_path (&pathbuf);
}

/**
 * lp_pack_builder_add_from_bytes:
 * @builder: #LpPackBuilder instance.
 * @path: path under which to store data contained in @bytes.
 * @bytes: data to be added into @builder output.
 * 
 * Adds data contained in @bytes into @builder under @path.
*/
void lp_pack_builder_add_from_bytes (LpPackBuilder* builder, const gchar* path, GBytes* bytes)
{
  g_return_if_fail (LP_IS_PACK_BUILDER (builder));
  g_return_if_fail (path != NULL);
  g_return_if_fail (bytes != NULL);
  GInputStream* stream;

  stream = g_memory_input_stream_new_from_bytes (bytes);
          lp_pack_builder_add_from_stream (builder, path, stream, g_bytes_get_size (bytes));
          g_object_unref (stream);
}

/**
 * lp_pack_builder_add_from_file:
 * @builder: #LpPackBuilder instance.
 * @path: path under which to store data contained in @bytes.
 * @file: #GFile instance.
 * @error: return location for a #GError, or %NULL.
 *
 * Adds data from file pointed by @file into @builder under @path.
 * 
 * Returns: if operation was successful.
*/
gboolean lp_pack_builder_add_from_file (LpPackBuilder* builder, const gchar* path, GFile* file, GError** error)
{
  g_return_val_if_fail (LP_IS_PACK_BUILDER (builder), FALSE);
  g_return_val_if_fail (path != NULL, FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  GFileInfo* info;
  GFileInputStream* stream;

  if ((info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_SIZE, 0, NULL, error)) == NULL)
    return FALSE;
  else if ((stream = g_file_read (file, NULL, error)) == NULL)
    {
      g_object_unref (info);
      return FALSE;
    }
  else
    {
      lp_pack_builder_add_from_stream (builder, path, G_INPUT_STREAM (stream), g_file_info_get_size (info));

      g_object_unref (info);
      g_object_unref (stream);
    }
return TRUE;
}

/**
 * lp_pack_builder_add_from_filename:
 * @builder: #LpPackBuilder instance.
 * @path: path under which to store data contained in @bytes.
 * @filename: file to extract data from.
 * @error: return location for a #GError, or %NULL.
 *
 * Similar to #lp_pack_builder_add_from_file but takes a filename
 * instead of a #GFile instance. Uses #g_file_new_for_path so
 * make sure @filename is compatible with it.
 *
 * Returns: if operation was successful.
*/
gboolean lp_pack_builder_add_from_filename (LpPackBuilder* builder, const gchar* path, const gchar* filename, GError** error)
{
  g_return_val_if_fail (LP_IS_PACK_BUILDER (builder), FALSE);
  g_return_val_if_fail (path != NULL, FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  GFile* file = g_file_new_for_path (filename);
  gboolean good = lp_pack_builder_add_from_file (builder, path, file, error);
return (g_object_unref (file), good);
}

/**
 * lp_pack_builder_add_from_stream:
 * @builder: #LpPackBuilder instance.
 * @path: path under which to store data contained in @bytes.
 * @stream: stream to extract data from.
 * @size: number of bytes expected.
 *
 * Similar to #lp_pack_builder_add_from_bytes, but takes an
 * #GInputStream instead of #GBytes. Upon output data from
 * @stream will be spliced.
*/
void lp_pack_builder_add_from_stream (LpPackBuilder* builder, const gchar* path, GInputStream* stream, gsize size)
{
  g_return_if_fail (LP_IS_PACK_BUILDER (builder));
  g_return_if_fail (path != NULL);
  g_return_if_fail (G_IS_INPUT_STREAM (stream));
  gchar* name = g_canonicalize_filename (path, G_DIR_SEPARATOR_S);
  Source source = { g_object_ref (stream), size, };

  g_hash_table_insert (builder->sources, name, g_slice_dup (Source, &source));
}

/**
 * lp_pack_builder_new: (constructor)
 * 
 * Returns: a new #LpPackBuilder instance
*/
LpPackBuilder* lp_pack_builder_new ()
{
  return g_object_new (LP_TYPE_PACK_BUILDER, NULL);
}

struct _Writer
{
  GOutputStream* stream;
  GError* error;
};

typedef struct archive Archive;
typedef struct archive_entry ArchiveEntry;
typedef struct _Writer Writer;

static la_ssize_t on_write (struct archive* ar, void* user_data, const void* buffer, size_t count)
{
  GOutputStream* stream = G_STRUCT_MEMBER (gpointer, user_data, G_STRUCT_OFFSET (Writer, stream));
  GError** error = & G_STRUCT_MEMBER (GError*, user_data, G_STRUCT_OFFSET (Writer, error));
  gssize wrote = 0;

  if ((wrote = g_output_stream_write (stream, buffer, (gsize) count, NULL, error)) < 0)
        wrote = ARCHIVE_FATAL;
return (la_ssize_t) wrote;
}

#define report(error, funcname, ar, writer) \
  G_STMT_START { \
    Archive* __archive = (ar); \
    GError** __error = (error); \
    Writer* __writer = (writer); \
 ; \
    if (G_LIKELY (__writer->error != NULL)) \
      g_propagate_error (error, __writer->error); \
    else \
      { \
        const GQuark __domain = LP_PACK_BUILDER_ERROR; \
        const guint __code = LP_PACK_BUILDER_ERROR_WRITE; \
        const gchar* __strerror = archive_error_string (__archive); \
 ; \
        g_set_error (__error, __domain, __code, #funcname "()!: %s", __strerror); \
      } \
  } G_STMT_END

static int begin_file (Archive* ar, const gchar* name, gsize size, Writer* writer, GError** error)
{
  ArchiveEntry* ent = NULL;
  const gchar* path = NULL;
  int result;

  ent = archive_entry_new2 (ar);
  path = g_path_skip_root (name);

  archive_entry_set_pathname (ent, path);
  archive_entry_set_size (ent, size);
  archive_entry_set_filetype (ent, S_IFREG);
  archive_entry_set_perm (ent, 0644);

  if ((result = archive_write_header (ar, ent)), G_LIKELY (result == ARCHIVE_OK))
    archive_entry_free (ent);
  else
    {
      archive_entry_free (ent);
      report (error, archive_write_header, ar, writer);
    }
return result;
}

static int write_manifest (LpPackBuilder* self, Archive* ar, Writer* writer, GError** error)
{
  gchar* data = NULL;
  gsize wrote, size = 0;
  const gchar* path;
  int result;

  data = g_key_file_to_data (self->manifest, &size, NULL);
  path = G_DIR_SEPARATOR_S LP_PACK_MANIFEST_PATH;

  if ((result = begin_file (ar, path, size, writer, error)), G_UNLIKELY (result != ARCHIVE_OK))
    {
      g_free (data);
      return result;
    }

  if ((wrote = archive_write_data (ar, data, size), g_free (data)), G_UNLIKELY (wrote < 0))
    {
      report (error, archive_write_data, ar, writer);
      return result;
    }

  if ((result = archive_write_finish_entry (ar)), G_UNLIKELY (result != ARCHIVE_OK))
    {
      report (error, archive_write_finish_entry, ar, writer);
      return result;
    }
return result;
}

static int write_archive (LpPackBuilder* self, Archive* ar, Writer* writer, GError** error)
{
  GError* tmperr = NULL;
  GHashTableIter iter = {0};
  const gchar* name = NULL;
  const Source* source = NULL;
  gchar buffer [512];
  gint result;

  if ((result = write_manifest (self, ar, writer, error)), G_UNLIKELY (result != ARCHIVE_OK))
    return result;

  g_hash_table_iter_init (&iter, self->sources);

  while (g_hash_table_iter_next (&iter, (gpointer*) &name, (gpointer*) &source))
    {
      if ((result = begin_file (ar, name, source->size, writer, error)), G_UNLIKELY (result != ARCHIVE_OK))
        break;

      while (TRUE)
        {
          la_ssize_t done;
          gsize read;

          if ((read = g_input_stream_read (source->stream, buffer, sizeof (buffer), NULL, error)) < 0)
            return ARCHIVE_FATAL;
          else if (read == 0) break;
          else
            {
              if ((done = archive_write_data (ar, buffer, read)), G_UNLIKELY (done < 0))
                {
                  report (error, archive_write_data, ar, writer);
                  return ARCHIVE_FATAL;
                }
              else if (done < read)
                {
                  archive_set_error (ar, ARCHIVE_FATAL, "partial write");
                  report (error, archive_write_data, ar, writer);
                  return ARCHIVE_FATAL;
                }
            }
        }

      if ((result = archive_write_finish_entry (ar)), G_UNLIKELY (result != ARCHIVE_OK))
        {
          report (error, archive_write_finish_entry, ar, writer);
          break;
        }
    }
return result;
}

/**
 * lp_pack_builder_write_to_stream:
 * @builder: #LpPackBuilder instance.
 * @stream: stream in which to write data.
 * @error: return location for a #GError, or %NULL.
 * 
 * Writes accumulated data into @stream. After this function concludes,
 * whether it was successful or not, any further call leads to
 * undefined behavior
*/
gboolean lp_pack_builder_write_to_stream (LpPackBuilder* builder, GOutputStream* stream, GError** error)
{
  g_return_val_if_fail (LP_IS_PACK_BUILDER (builder), FALSE);
  g_return_val_if_fail (G_IS_OUTPUT_STREAM (stream), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  Archive* ar = archive_write_new ();
  Writer writer = { stream, NULL, };
  int result = ARCHIVE_OK;

  G_STATIC_ASSERT (sizeof (la_ssize_t) == sizeof (gssize));
  G_STATIC_ASSERT (sizeof (size_t) == sizeof (gsize));

  if ((result = archive_write_add_filter (ar, LP_PACK_COMPRESSION)), G_UNLIKELY (result != ARCHIVE_OK))
    g_set_error (error, LP_PACK_BUILDER_ERROR, LP_PACK_BUILDER_ERROR_OPEN, "archive_write_add_filter()!: %s", archive_error_string (ar));
  else if ((result = archive_write_set_format (ar, LP_PACK_FORMAT)), G_UNLIKELY (result != ARCHIVE_OK))
    g_set_error (error, LP_PACK_BUILDER_ERROR, LP_PACK_BUILDER_ERROR_OPEN, "archive_write_set_format()!: %s", archive_error_string (ar));
  else if ((result = archive_write_open2 (ar, &writer, NULL, on_write, NULL, NULL)), G_UNLIKELY (result != ARCHIVE_OK))
    g_set_error (error, LP_PACK_BUILDER_ERROR, LP_PACK_BUILDER_ERROR_OPEN, "archive_write_open2()!: %s", archive_error_string (ar));
  else
    {
      if ((result = write_archive (builder, ar, &writer, error)), G_LIKELY (result == ARCHIVE_OK))
      if ((result = archive_write_close (ar)) != ARCHIVE_OK)
        {
          if (G_LIKELY (writer.error != NULL))
            g_propagate_error (error, writer.error);
          else
            {
              const GQuark domain = LP_PACK_BUILDER_ERROR;
              const guint code = LP_PACK_BUILDER_ERROR_CLOSE;
              const gchar* message = archive_error_string (ar);

              g_set_error (error, domain, code, "archive_write_close()!: %s", message);
            }
        }
    }
return (archive_write_free (ar), result == ARCHIVE_OK);
}

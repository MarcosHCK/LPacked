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

struct _LpPackBuilder
{
  GObject parent;

  /* <private> */
  GHashTable* sources;
};

G_DEFINE_FINAL_TYPE (LpPackBuilder, lp_pack_builder, G_TYPE_OBJECT);

static void lp_pack_builder_init (LpPackBuilder* self)
{
  const GHashFunc func1 = g_str_hash;
  const GEqualFunc func2 = g_str_equal;
  const GDestroyNotify func3 = g_free;
  const GDestroyNotify func4 = g_object_unref;

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
  g_hash_table_unref (self->sources);
G_OBJECT_CLASS (lp_pack_builder_parent_class)->finalize (pself);
}

static void lp_pack_builder_class_init (LpPackBuilderClass* klass)
{
  G_OBJECT_CLASS (klass)->dispose = lp_pack_builder_class_dispose;
  G_OBJECT_CLASS (klass)->finalize = lp_pack_builder_class_finalize;
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
          lp_pack_builder_add_from_stream (builder, path, stream);
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
  GFileInputStream* stream;

  if ((stream = g_file_read (file, NULL, error)) == NULL)
    return FALSE;
  else
    {
      lp_pack_builder_add_from_stream (builder, path, G_INPUT_STREAM (stream));
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
 *
 * Similar to #lp_pack_builder_add_from_bytes, but takes an
 * #GInputStream instead of #GBytes. Upon output data from
 * @stream will be spliced.
*/
void lp_pack_builder_add_from_stream (LpPackBuilder* builder, const gchar* path, GInputStream* stream)
{
  g_return_if_fail (LP_IS_PACK_BUILDER (builder));
  g_return_if_fail (path != NULL);
  g_return_if_fail (G_IS_INPUT_STREAM (stream));
  gchar* name = g_canonicalize_filename (path, G_DIR_SEPARATOR_S);
  g_hash_table_insert (builder->sources, name, g_object_ref (stream));
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

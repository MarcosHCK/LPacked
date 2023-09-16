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
#include <reader.h>

struct _LpPackReader
{
  GObject parent;

  /* <private> */
  GFile* base_file;
  GTree* index;
};

enum
{
  prop_0,
  prop_base_file,
  prop_number,
};

#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))

G_DEFINE_FINAL_TYPE (LpPackReader, lp_pack_reader, G_TYPE_OBJECT);
G_DEFINE_QUARK (lp-pack-reader-error-quark, lp_pack_reader_error);

static GParamSpec* properties [prop_number] = {0};

static void lp_pack_reader_init (LpPackReader* self)
{
  const GCompareDataFunc func1 = (GCompareDataFunc) g_strcmp0;
  const GDestroyNotify func2 = (GDestroyNotify) g_free;
  const GDestroyNotify func3 = (GDestroyNotify) NULL;

  self->index = g_tree_new_full (func1, NULL, func2, func3);
}

static void lp_pack_reader_class_dispose (GObject* pself)
{
  LpPackReader* self = (gpointer) pself;
  _g_object_unref0 (self->base_file);
  g_tree_remove_all (self->index);
  G_OBJECT_CLASS (lp_pack_reader_parent_class)->dispose (pself);
}

static void lp_pack_reader_class_finalize (GObject* pself)
{
  LpPackReader* self = (gpointer) pself;
  g_tree_unref (self->index);
  G_OBJECT_CLASS (lp_pack_reader_parent_class)->finalize (pself);
}

static void lp_pack_reader_class_get_property (GObject* pself, guint property_id, GValue* value, GParamSpec* pspec)
{
  LpPackReader* self = (gpointer) pself;

  switch (property_id)
    {
      default: G_OBJECT_WARN_INVALID_PROPERTY_ID (pself, property_id, pspec); break;
      case prop_base_file: g_value_set_object (value, self->base_file);
    }
}

static void lp_pack_reader_class_set_property (GObject* pself, guint property_id, const GValue* value, GParamSpec* pspec)
{
  LpPackReader* self = (gpointer) pself;

  switch (property_id)
    {
      default: G_OBJECT_WARN_INVALID_PROPERTY_ID (pself, property_id, pspec); break;
      case prop_base_file: g_set_object (&self->base_file, g_value_get_object (value)); break;
    }
}

static void lp_pack_reader_class_init (LpPackReaderClass* klass)
{
  G_OBJECT_CLASS (klass)->dispose = lp_pack_reader_class_dispose;
  G_OBJECT_CLASS (klass)->finalize = lp_pack_reader_class_finalize;
  G_OBJECT_CLASS (klass)->get_property = lp_pack_reader_class_get_property;
  G_OBJECT_CLASS (klass)->set_property = lp_pack_reader_class_set_property;

  GParamFlags flags1 = G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY;

  properties [prop_base_file] = g_param_spec_object ("base-file", "base-file", "base-file", G_TYPE_FILE, flags1);
  g_object_class_install_properties (G_OBJECT_CLASS (klass), prop_number, properties);
}

/**
 * lp_pack_reader_add_from_bytes:
 * @reader: #LpPackReader instance.
 * @bytes: data to be added into @reader output.
 * 
 * Adds data contained in @bytes into @reader under @path.
*/
void lp_pack_reader_add_from_bytes (LpPackReader* reader, GBytes* bytes)
{
  g_return_if_fail (LP_IS_PACK_READER (reader));
  g_return_if_fail (bytes != NULL);
  GInputStream* stream;

  stream = g_memory_input_stream_new_from_bytes (bytes);
          lp_pack_reader_add_from_stream (reader, stream);
          g_object_unref (stream);
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
  GFileInputStream* stream;

  if ((stream = g_file_read (file, NULL, error)) == NULL)
    return FALSE;
  else
    {
      lp_pack_reader_add_from_stream (reader, G_INPUT_STREAM (stream));
      g_object_unref (stream);
    }
return TRUE;
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
 *
 * Similar to #lp_pack_reader_add_from_bytes, but takes an
 * #GInputStream instead of #GBytes. Upon output data from
 * @stream will be spliced.
*/
void lp_pack_reader_add_from_stream (LpPackReader* reader, GInputStream* stream)
{
  g_return_if_fail (LP_IS_PACK_READER (reader));
  g_return_if_fail (G_IS_INPUT_STREAM (stream));
  g_assert_not_reached ();
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

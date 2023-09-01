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
#include <descriptor.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#define LPACKED_DESCRIPTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), LPACKED_TYPE_DESCRIPTOR, LPackedDescriptorClass))
#define LPACKED_IS_DESCRIPTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LPACKED_TYPE_DESCRIPTOR))
#define LPACKED_DESCRIPTOR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), LPACKED_TYPE_DESCRIPTOR, LPackedDescriptorClass))
typedef struct _LPackedDescriptorClass LPackedDescriptorClass;
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))
static void lpacked_descriptor_g_initable_iface (GInitableIface *iface);
static void lpacked_descriptor_g_async_initable_iface (GAsyncInitableIface *iface);

G_DEFINE_QUARK (lpacked-descriptor-error-quark, lpacked_descriptor_error);

struct _LPackedDescriptor
{
  GObject parent;

  /*<private>*/
  GBytes* base_bytes;
  GFile* base_file;
  gchar* entry;
  GHashTable* files;
  gchar* name;
};

struct _LPackedDescriptorClass
{
  GObjectClass parent;
};

enum
{
  prop_0,
  prop_base_bytes,
  prop_base_file,
  prop_entry,
  prop_name,
  prop_number,
};

G_DEFINE_FINAL_TYPE_WITH_CODE (LPackedDescriptor, lpacked_descriptor, G_TYPE_OBJECT,
  G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, lpacked_descriptor_g_initable_iface)
  G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE, lpacked_descriptor_g_async_initable_iface));
  static GParamSpec* properties [prop_number] = {0};

static void explore_table (lua_State* L, LPackedDescriptor* self, const gchar* tname, GError** error)
{
  GError* tmperr = NULL;
  GPathBuf buf = G_PATH_BUF_INIT;
  const gchar* src = NULL;
  const gchar* dst = NULL;
  int type;

  lua_pushnil (L);

  while (lua_next (L, -2))
    {
      if ((type = lua_type (L, -1)), G_UNLIKELY (type != LUA_TSTRING))
        {
          lua_pop (L, 2);
          g_set_error (error, LPACKED_DESCRIPTOR_ERROR, LPACKED_DESCRIPTOR_ERROR_INVALID_FIELD,
            "descriptor field '%s' table must contain string-string or number-string pairs", tname);
          break;
        }

      if ((type = lua_type (L, -2)), G_UNLIKELY (type != LUA_TNUMBER && type != LUA_TSTRING))
        {
          lua_pop (L, 2);
          g_set_error (error, LPACKED_DESCRIPTOR_ERROR, LPACKED_DESCRIPTOR_ERROR_INVALID_FIELD,
            "descriptor field '%s' table must contain string-string or number-string pairs", tname);
          break;
        }

      g_path_buf_push (&buf, "/");
      g_path_buf_push (&buf, tname);
      g_path_buf_push (&buf, lua_tostring (L, (type == LUA_TNUMBER) ? -1 : -2));

      src = lua_tostring (L, -1);
      dst = g_path_buf_clear_to_path (&buf);

      g_hash_table_insert (self->files, (gchar*) dst, g_strdup (src));
      lua_pop (L, 1);
    }
}

static void explore_descriptor (lua_State* L, LPackedDescriptor* self, GError** error)
{
  GError* tmperr = NULL;
  const gchar* tname = NULL;
  int type;

  lua_pushnil (L);

  while (lua_next (L, -2))
    {
      if ((type = lua_type (L, -1)), G_UNLIKELY (type != LUA_TTABLE))
        {
          lua_pop (L, 2);
          g_set_error_literal (error, LPACKED_DESCRIPTOR_ERROR, LPACKED_DESCRIPTOR_ERROR_INVALID_FIELD,
                                  "descriptor field 'table' table must contain string-table pairs");
          break;
        }

      if ((type = lua_type (L, -2)), G_UNLIKELY (type != LUA_TSTRING))
        {
          lua_pop (L, 2);
          g_set_error_literal (error, LPACKED_DESCRIPTOR_ERROR, LPACKED_DESCRIPTOR_ERROR_INVALID_FIELD,
                                  "descriptor field 'table' table must contain string-table pairs");
          break;
        }

      tname = lua_tostring (L, -2);

      if ((explore_table (L, self, tname, &tmperr)), G_UNLIKELY (tmperr != NULL))
        {
          lua_pop (L, 2);
          g_propagate_error (error, tmperr);
          break;
        }

      lua_pop (L, 1);
    }
}

static gboolean lpacked_descriptor_g_initable_iface_init (GInitable* pself, GCancellable* cancellable, GError** error)
{
  GBytes* bytes = NULL;
  GError* tmperr = NULL;
  LPackedDescriptor* self = (gpointer) pself;

  if (self->base_bytes != NULL)
    bytes = g_bytes_ref (self->base_bytes);
  else
    {
      if (self->base_file == NULL)
        return FALSE;
      else
        {
          GInputStream* istream = NULL;
          GOutputStream* ostream = NULL;
          guint flags = 0;

          if ((istream = (gpointer) g_file_read (self->base_file, cancellable, &tmperr)), G_UNLIKELY (tmperr != NULL))
            g_propagate_error (error, tmperr);
          else
            {
              ostream = g_memory_output_stream_new_resizable ();
              flags = G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET;

              if (g_output_stream_splice (ostream, istream, flags, cancellable, &tmperr), G_UNLIKELY (tmperr != NULL))
                g_propagate_error (error, tmperr);
              else
                {
                  GMemoryOutputStream* memory;

                  memory = G_MEMORY_OUTPUT_STREAM (ostream);
                  bytes = g_memory_output_stream_steal_as_bytes (memory);
                }
            }

          _g_object_unref0 (istream);
          _g_object_unref0 (ostream);
        }
    }

  if (G_UNLIKELY (bytes == NULL))
    return FALSE;
  else
    {
      lua_State* L = luaL_newstate ();
      gsize size = 0;
      gconstpointer data = g_bytes_get_data (bytes, &size);
      int type, result = 0;

      lua_gc (L, LUA_GCSTOP, -1);
      luaopen_base (L);
      luaopen_math (L);
      luaopen_string (L);
      luaopen_table (L);

      lua_gc (L, LUA_GCRESTART, 0);
      lua_settop (L, 0);

      if ((result = luaL_loadbuffer (L, data, size, "=descriptor")), G_UNLIKELY (result != LUA_OK))
        {
          switch (result)
            {
              case LUA_ERRMEM: g_set_error_literal (&tmperr, LPACKED_DESCRIPTOR_ERROR, LPACKED_DESCRIPTOR_ERROR_FAILED, "out of memory");
                                g_propagate_error (error, tmperr); break;
              case LUA_ERRSYNTAX: g_set_error_literal (&tmperr, LPACKED_DESCRIPTOR_ERROR, LPACKED_DESCRIPTOR_ERROR_LOAD, lua_tostring (L, -1));
                                g_propagate_error (error, tmperr); break;
            }
        }
      else
        {
          if ((result = lua_pcall (L, 0, 1, 0)), G_UNLIKELY (result != LUA_OK))
            {
              switch (result)
                {
                  case LUA_ERRMEM: g_set_error_literal (&tmperr, LPACKED_DESCRIPTOR_ERROR, LPACKED_DESCRIPTOR_ERROR_FAILED, "out of memory");
                                    g_propagate_error (error, tmperr); break;
                  case LUA_ERRRUN: g_set_error_literal (&tmperr, LPACKED_DESCRIPTOR_ERROR, LPACKED_DESCRIPTOR_ERROR_LOAD, lua_tostring (L, -1));
                                    g_propagate_error (error, tmperr); break;
                }
            }
          else
            {
              if ((lua_getfield (L, -1, "name"), type = lua_type (L, -1)), G_UNLIKELY (type != LUA_TSTRING))
                {
                  g_set_error (&tmperr, LPACKED_DESCRIPTOR_ERROR,
                                      (type == LUA_TNIL) ? LPACKED_DESCRIPTOR_ERROR_MISSING_FIELD
                                                         : LPACKED_DESCRIPTOR_ERROR_INVALID_FIELD,
                              "descriptor field 'name' %s", (type == LUA_TNIL) ? "is missing"
                                                                               : "must contain an string value");
                  g_propagate_error (error, tmperr);
                }
              else
                {
                  self->name = g_strdup (lua_tostring (L, -1));

                  lua_pop (L, 1);

                  if ((lua_getfield (L, -1, "pack"), type = lua_type (L, -1)), G_UNLIKELY (type != LUA_TTABLE))
                    {
                      g_set_error (&tmperr, LPACKED_DESCRIPTOR_ERROR,
                                          (type == LUA_TNIL) ? LPACKED_DESCRIPTOR_ERROR_MISSING_FIELD
                                                             : LPACKED_DESCRIPTOR_ERROR_INVALID_FIELD,
                                  "descriptor field 'pack' %s", (type == LUA_TNIL) ? "is missing"
                                                                                   : "must contain a table value");
                      g_propagate_error (error, tmperr);
                    }
                  else
                    {
                      if ((explore_descriptor (L, self, &tmperr)), G_UNLIKELY (tmperr != NULL))
                        g_propagate_error (error, tmperr);
                      else lua_pop (L, 1);
                    }
                }

              lua_pop (L, 1);
            }
        }

      lua_close (L);
      g_bytes_unref (bytes);
    }
return tmperr == NULL;
}

static void lpacked_descriptor_g_initable_iface (GInitableIface* iface)
{
  iface->init = lpacked_descriptor_g_initable_iface_init;
}

static void lpacked_descriptor_g_async_initable_iface (GAsyncInitableIface* iface)
{
}

static void lpacked_descriptor_class_dispose (GObject* pself)
{
  g_clear_pointer (& G_STRUCT_MEMBER (gpointer, pself, G_STRUCT_OFFSET (LPackedDescriptor, base_bytes)), g_bytes_unref);
  g_clear_pointer (& G_STRUCT_MEMBER (gpointer, pself, G_STRUCT_OFFSET (LPackedDescriptor, base_file)), g_object_unref);
  g_hash_table_remove_all (G_STRUCT_MEMBER (GHashTable*, pself, G_STRUCT_OFFSET (LPackedDescriptor, files)));
G_OBJECT_CLASS (lpacked_descriptor_parent_class)->dispose (pself);
}

static void lpacked_descriptor_class_finalize (GObject* pself)
{
  g_free (G_STRUCT_MEMBER (gpointer, pself, G_STRUCT_OFFSET (LPackedDescriptor, name)));
  g_hash_table_unref (G_STRUCT_MEMBER (GHashTable*, pself, G_STRUCT_OFFSET (LPackedDescriptor, files)));
G_OBJECT_CLASS (lpacked_descriptor_parent_class)->finalize (pself);
}

static void lpacked_descriptor_class_get_property (GObject* pself, guint property_id, GValue* value, GParamSpec* pspec)
{
  switch (property_id)
    {
      default: G_OBJECT_WARN_INVALID_PROPERTY_ID (pself, property_id, pspec); break;
      case prop_entry: g_value_set_string (value, lpacked_descriptor_get_entry ((LPackedDescriptor*) pself)); break;
      case prop_name: g_value_set_string (value, lpacked_descriptor_get_name ((LPackedDescriptor*) pself)); break;
    }
}

static void lpacked_descriptor_class_set_property (GObject* pself, guint property_id, const GValue* value, GParamSpec* pspec)
{
  switch (property_id)
    {
      default: G_OBJECT_WARN_INVALID_PROPERTY_ID (pself, property_id, pspec); break;

      case prop_base_bytes:
        g_clear_pointer (& G_STRUCT_MEMBER (gpointer, pself, G_STRUCT_OFFSET (LPackedDescriptor, base_bytes)), g_bytes_unref);
        G_STRUCT_MEMBER (gpointer, pself, G_STRUCT_OFFSET (LPackedDescriptor, base_bytes)) = g_value_dup_boxed (value);
        break;

      case prop_base_file:
        g_clear_pointer (& G_STRUCT_MEMBER (gpointer, pself, G_STRUCT_OFFSET (LPackedDescriptor, base_file)), g_object_unref);
        G_STRUCT_MEMBER (gpointer, pself, G_STRUCT_OFFSET (LPackedDescriptor, base_file)) = g_value_dup_object (value);
        break;
    }
}

static void lpacked_descriptor_class_init (LPackedDescriptorClass* klass)
{
  const GParamFlags flags1 = G_PARAM_STATIC_STRINGS | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY;
  const GParamFlags flags2 = G_PARAM_STATIC_STRINGS | G_PARAM_READABLE;

  G_OBJECT_CLASS (klass)->dispose = lpacked_descriptor_class_dispose;
  G_OBJECT_CLASS (klass)->finalize = lpacked_descriptor_class_finalize;
  G_OBJECT_CLASS (klass)->get_property = lpacked_descriptor_class_get_property;
  G_OBJECT_CLASS (klass)->set_property = lpacked_descriptor_class_set_property;

  properties [prop_base_bytes] = g_param_spec_boxed ("base-bytes", "base-bytes", "base-bytes", G_TYPE_BYTES, flags1);
  properties [prop_base_file] = g_param_spec_object ("base-file", "base-file", "base-file", G_TYPE_FILE, flags1);
  properties [prop_entry] = g_param_spec_string ("entry", "entry", "entry", NULL, flags2);
  properties [prop_name] = g_param_spec_string ("name", "name", "name", NULL, flags2);
  g_object_class_install_properties (G_OBJECT_CLASS (klass), prop_number, properties);
}

static void lpacked_descriptor_init (LPackedDescriptor* self)
{
  const GHashFunc func1 = g_str_hash;
  const GEqualFunc func2 = g_str_equal;
  const GDestroyNotify func3 = g_free;
  const GDestroyNotify func4 = g_free;

  self->files = g_hash_table_new_full (func1, func2, func3, func4);
}

GList* lpacked_descriptor_get_aliases (LPackedDescriptor* descriptor)
{
  g_return_val_if_fail (LPACKED_IS_DESCRIPTOR (descriptor), NULL);
return g_hash_table_get_keys (descriptor->files);
}

const gchar* lpacked_descriptor_get_entry (LPackedDescriptor* descriptor)
{
  g_return_val_if_fail (LPACKED_IS_DESCRIPTOR (descriptor), NULL);
return descriptor->entry;
}

GList* lpacked_descriptor_get_files (LPackedDescriptor* descriptor)
{
  g_return_val_if_fail (LPACKED_IS_DESCRIPTOR (descriptor), NULL);
return g_hash_table_get_values (descriptor->files);
}

const gchar* lpacked_descriptor_get_file_by_alias (LPackedDescriptor* descriptor, const gchar* alias)
{
  g_return_val_if_fail (LPACKED_IS_DESCRIPTOR (descriptor), NULL);
  g_return_val_if_fail (alias != NULL, NULL);
return g_hash_table_lookup (descriptor->files, alias);
}

const gchar* lpacked_descriptor_get_name (LPackedDescriptor* descriptor)
{
  g_return_val_if_fail (LPACKED_IS_DESCRIPTOR (descriptor), NULL);
return descriptor->name;
}

LPackedDescriptor* lpacked_descriptor_new_from_bytes (GBytes* bytes, GError** error)
{
  g_return_val_if_fail (bytes != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
return g_initable_new (LPACKED_TYPE_DESCRIPTOR, NULL, error, "base-bytes", bytes, NULL);
}

void lpacked_descriptor_new_from_bytes_async (GBytes* bytes, GAsyncReadyCallback callback, gpointer user_data)
{
  g_return_if_fail (bytes != NULL);
  g_return_if_fail (callback == NULL);

  g_async_initable_new_async (LPACKED_TYPE_DESCRIPTOR, G_PRIORITY_DEFAULT, NULL, callback, user_data, "base-bytes", bytes, NULL);
}

LPackedDescriptor* lpacked_descriptor_new_from_gfile (GFile* file, GError** error)
{
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
return g_initable_new (LPACKED_TYPE_DESCRIPTOR, NULL, error, "base-file", file, NULL);
}

void lpacked_descriptor_new_from_gfile_async (GFile* file, GAsyncReadyCallback callback, gpointer user_data)
{
  g_return_if_fail (G_IS_FILE (file));
  g_return_if_fail (callback == NULL);

  g_async_initable_new_async (LPACKED_TYPE_DESCRIPTOR, G_PRIORITY_DEFAULT, NULL, callback, user_data, "base-file", file, NULL);
}

LPackedDescriptor* lpacked_descriptor_new_finish (GAsyncResult* result, GError** error)
{
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  GObject* source_object = NULL;
  GObject* result_object = NULL;

  source_object = g_async_result_get_source_object (result);
  result_object = g_async_initable_new_finish (G_ASYNC_INITABLE (source_object), result, error);
                  g_object_unref (source_object);
return (LPackedDescriptor*) result_object;
}

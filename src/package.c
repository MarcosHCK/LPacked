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
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <compat.h>
#include <package.h>

struct _LpPackage
{
  guint ref_count;
  GQueue queue;
};

/**
 * LpPackage:
 * 
 * A GResource-backed package.
*/

G_DEFINE_BOXED_TYPE (LpPackage, lp_package, lp_package_ref, lp_package_unref);

/**
 * lp_package_delete:
 * @package: #LpPackage instance.
 * @resource: #GResource instance.
 *
 * Removes @resource from @package internal list. As a result
 * further look ups will not be propagated to @resource.
 */
void lp_package_delete (LpPackage* package, GResource* resource)
{
  g_return_if_fail (package != NULL);
  g_return_if_fail (resource != NULL);
  GList* link = g_queue_find (&package->queue, resource);

  g_queue_delete_link (&package->queue, link);
  g_list_free_full (link, (GDestroyNotify) g_resource_unref);
}

/**
 * lp_package_insert:
 * @package: #LpPackage instance.
 * @resource: #GResource instance.
 *
 * Inserts @resource into @package internal list. As a result
 * further look ups into @package will be delegated to @resources.
 */
void lp_package_insert (LpPackage* package, GResource* resource)
{
  g_return_if_fail (package != NULL);
  g_return_if_fail (resource != NULL);

  g_queue_push_tail (&package->queue, g_resource_ref (resource));
}

/**
 * lp_package_lookup:
 * @package: #LpPackage instance.
 * @path: path name pointing to desired data.
 * @error: return location for a #GError, or %NULL.
 *
 * Looks up for data under @path path. Returns a #GBytes instance
 * containing such data.
 *
 * Returns: (transfer full): (nullable): found file data, or NULL.
 */
GBytes* lp_package_lookup (LpPackage* package, const gchar* path, GError** error)
{
  g_return_val_if_fail (package != NULL, NULL);
  g_return_val_if_fail (path != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  GBytes* bytes = NULL;
  GError* tmperr = NULL;
  GList* list;

  for (list = g_queue_peek_head_link (&package->queue); list; list = list->next)
    {
      if ((bytes = g_resource_lookup_data (list->data, path, 0, &tmperr)) == NULL)
        {
          if (g_error_matches (tmperr, G_RESOURCE_ERROR, G_RESOURCE_ERROR_NOT_FOUND))
            g_clear_error (&tmperr);
          else
            {
              g_propagate_error (error, tmperr);
              return NULL;
            }
        }
    }

  if (bytes == NULL)
    {
      g_set_error_literal (error, G_RESOURCE_ERROR, G_RESOURCE_ERROR_NOT_FOUND, "not found");
    }
return (bytes);
}

/**
 * lp_package_new: (constructor)
 *
 * Creates a #LpPackage instance.
 *
 * Returns: (transfer full): a new #LpPackage instance.
 */
LpPackage* lp_package_new ()
{
  LpPackage* self;

  self = g_slice_new (LpPackage);
  self->ref_count = 1;
return (g_queue_init (&self->queue), self);
}

/**
 * lp_package_ref:
 * @package: #LpPackage instance.
 *
 * Increments @package reference count.
 * 
 * Returns: (transfer full): a new reference to @package.
 */
LpPackage* lp_package_ref (LpPackage* package)
{
  g_return_val_if_fail (package != NULL, NULL);
  g_atomic_int_inc (&package->ref_count);
return package;
}

/**
 * lp_package_unref:
 * @package: #LpPackage instance.
 *
 * Decrements @package reference count.
 */
void lp_package_unref (LpPackage* package)
{
  g_return_if_fail (package != NULL);

  if (g_atomic_int_dec_and_test (&package->ref_count))
    {
      g_queue_clear_full (&package->queue, (GDestroyNotify) g_resource_unref);
      g_slice_free (LpPackage, package);
    }
}

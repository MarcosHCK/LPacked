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
#include <application.h>

int main (int argc, char* argv[])
{
  GApplicationFlags flags = G_APPLICATION_HANDLES_OPEN;
  gpointer application = g_object_new (LP_TYPE_APPLICATION, "flags", flags, NULL);
  gint result = g_application_run (G_APPLICATION (application), argc, argv);
return (g_object_unref (application), result);
}

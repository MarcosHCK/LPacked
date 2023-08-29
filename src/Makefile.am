# Copyright 2023 MarcosHCK
# This file is part of LPacked.
#
# LPacked is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# LPacked is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with LPacked. If not, see <http://www.gnu.org/licenses/>.
#

bin_PROGRAMS=lpacked 
noinst_HEADERS=compat.h execute.h luaerror.h pack.h 

lpacked_CFLAGS=$(GLIB_CFLAGS) $(GOBJECT_CFLAGS) $(LUA_CFLAGS) $(ZIP_CFLAGS) -flto 
lpacked_LDADD=$(GLIB_LIBS) $(GOBJECT_LIBS) $(LUA_LIBS) $(ZIP_LIBS) 
lpacked_LDFLAGS=-flto 
lpacked_SOURCES=compat.c execute.c lpacked.c pack.c  

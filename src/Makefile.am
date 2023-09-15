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

resources_FILES=$(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies resources.xml) 

bin_PROGRAMS=lpacked
pkglib_LTLIBRARIES=liblpacked.la
noinst_DATA=$(resources_FILES) 
noinst_HEADERS=application.h compat.h package.h 

liblpacked_la_CFLAGS=$(GIO_CFLAGS) $(LGI_CFLAGS) $(LUA_CFLAGS) -flto 
liblpacked_la_LDFLAGS=-flto 
liblpacked_la_LIBADD=$(GIO_LIBS) $(LGI_LIBS) $(LUA_LIBS) 
liblpacked_la_SOURCES=application.c compat.c package.c 

lpacked_CFLAGS=$(GIO_CFLAGS) $(LGI_CFLAGS) $(LUA_CFLAGS) -flto 
lpacked_LDADD=$(GIO_LIBS) $(LGI_LIBS) $(LUA_LIBS) liblpacked.la 
lpacked_LDFLAGS=-flto 
lpacked_SOURCES=lpacked.c resources.c 

lpacked.c: resources.h

resources.c: resources.xml $(resources_FILES)
	$(GLIB_COMPILE_RESOURCES) --target=$@ --manual-register --internal --generate-source --c-name=lp_resources $<

resources.h: resources.xml $(resources_FILES)
	$(GLIB_COMPILE_RESOURCES) --target=$@ --manual-register --internal --generate-header --c-name=lp_resources $<

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
noinst_HEADERS=application.h builder.h compat.h format.h package.h readaux.h reader.h 
SUFFIXES=.gir .typelib 

liblpacked_la_CFLAGS=$(ARCHIVE_CFLAGS) $(GIO_CFLAGS) $(LGI_CFLAGS) $(LUA_CFLAGS) -flto 
liblpacked_la_LDFLAGS=-flto 
liblpacked_la_LIBADD=$(ARCHIVE_LIBS) $(GIO_LIBS) $(LGI_LIBS) $(LUA_LIBS) 
liblpacked_la_SOURCES=application.c builder.c compat.c package.c reader.c 

lpacked_CFLAGS=$(ARCHIVE_CFLAGS) $(GIO_CFLAGS) $(LGI_CFLAGS) $(LUA_CFLAGS) -flto 
lpacked_LDADD=$(ARCHIVE_LIBS) $(GIO_LIBS) $(LGI_LIBS) $(LUA_LIBS) liblpacked.la 
lpacked_LDFLAGS=-flto 
lpacked_SOURCES=lpacked.c resources.c 

.gir.typelib:
	$(G_IR_COMPILER) --output=$@ $<

lpacked.c: resources.h

resources.c: resources.xml $(resources_FILES)
	$(GLIB_COMPILE_RESOURCES) --target=$@ --manual-register --internal --generate-source --c-name=lp_resources $<

resources.h: resources.xml $(resources_FILES)
	$(GLIB_COMPILE_RESOURCES) --target=$@ --manual-register --internal --generate-header --c-name=lp_resources $<

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS=LPacked.gir
INTROSPECTION_SCANNER_ARGS=--add-include-path=$(srcdir) --libtool="$(LIBTOOL)" --warn-all \
													 --identifier-prefix=Lp --symbol-prefix=lp 
INTROSPECTION_COMPILER_ARGS=--includedir=$(srcdir)

LPacked.gir: liblpacked.la
LPacked_gir_CFLAGS=$(ARCHIVE_CFLAGS) $(GIO_CFLAGS) $(LGI_CFLAGS) $(LUA_CFLAGS) 
LPacked_gir_FILES=application.c application.h builder.c builder.h package.c package.h reader.c reader.h 
LPacked_gir_INCLUDES=Gio-2.0 
LPacked_gir_LIBS=liblpacked.la  
LPacked_gir_NAMESPACE=LPacked
LPacked_gir_VERSION=$(PACKAGE_VERSION_MAJOR).$(PACKAGE_VERSION_MINOR)

CLEANFILES=LPacked.gir LPacked.typelib resources.c resources.h 

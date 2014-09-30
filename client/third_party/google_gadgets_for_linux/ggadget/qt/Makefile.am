#
# Copyright 2008 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

EXTRA_DIST = CMakeLists.txt

if GGL_BUILD_LIBGGADGET_QT

# Putting . into SBUDIRS list to force building current directory first.
SUBDIRS = . tests

MOC_FILES		= qt_main_loop.moc \
			  qt_menu_internal.moc \
			  utilities_internal.moc \
			  qt_view_host_internal.moc \
			  qt_view_widget.moc

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

lib_LTLIBRARIES		= libggadget-qt@GGL_EPOCH@.la

noinst_HEADERS		= qt_canvas.h \
			  qt_font.h \
			  qt_image.h \
			  utilities_internal.h \
			  qt_menu_internal.h \
			  qt_view_host_internal.h

qtincludedir		= $(GGL_INCLUDE_DIR)/ggadget/qt
qtinclude_HEADERS	= qt_view_widget.h \
			  qt_graphics.h \
			  qt_main_loop.h \
			  qt_menu.h \
			  qt_view_host.h \
			  utilities.h

libggadget_qt@GGL_EPOCH@_la_SOURCES = \
			  qt_canvas.cc \
			  qt_font.cc \
			  qt_graphics.cc \
			  qt_image.cc \
			  qt_main_loop.cc \
			  qt_menu.cc \
			  qt_view_host.cc \
			  qt_view_widget.cc \
			  utilities.cc

nodist_libggadget_qt@GGL_EPOCH@_la_SOURCES = \
			  $(MOC_FILES)

libggadget_qt@GGL_EPOCH@_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

libggadget_qt@GGL_EPOCH@_la_CPPFLAGS = \
			  $(QT4_CFLAGS) \
			  $(PREDEFINED_MACROS)

libggadget_qt@GGL_EPOCH@_la_LIBADD = \
			  $(QT4_LIBS) \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/xdg/libggadget-xdg@GGL_EPOCH@.la

libggadget_qt@GGL_EPOCH@_la_LDFLAGS = \
			  -version-info $(LIBGGADGET_VERSION) \
			  -no-undefined \
			  -export-dynamic

pkgconfigdir= $(libdir)/pkgconfig
pkgconfig_DATA= libggadget-qt@GGL_EPOCH@.pc

BUILT_SOURCES		= $(MOC_FILES)
CLEANFILES		= $(MOC_FILES)

qt_main_loop.moc:qt_main_loop.h
	$(MOC) $< -o $@
qt_menu_internal.moc:qt_menu_internal.h
	$(MOC) $< -o $@
qt_view_host_internal.moc:qt_view_host_internal.h
	$(MOC) $< -o $@
utilities_internal.moc:utilities_internal.h
	$(MOC) $< -o $@
qt_view_widget.moc:qt_view_widget.h
	$(MOC) $< -o $@
endif

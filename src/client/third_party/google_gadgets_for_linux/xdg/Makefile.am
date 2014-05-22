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

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

noinst_PROGRAMS		= intl-desktop-file

intl_desktop_file_SOURCES= intl_desktop_file.cc
intl_desktop_file_CPPFLAGS= $(PREDEFINED_MACROS)
intl_desktop_file_CXXFLAGS= $(DEFAULT_COMPILE_FLAGS)
intl_desktop_file_LDADD= $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la

if GGL_DISABLE_SHARED
intl_desktop_file_LDADD += \
	-dlpreopen $(top_builddir)/extensions/libxml2_xml_parser/libxml2-xml-parser.la
endif

noinst_SCRIPTS=intl-desktop-file.sh

.PHONY: intl-desktop-file.sh

if GGL_CROSS_COMPILING
intl-desktop-file.sh: intl-desktop-file$(EXEEXT)
	@echo '#!/bin/sh' > $@; \
	 echo 'echo intl-desktop-file can not work in cross-compiling mode.' >> $@; \
	 echo 'cp $$@' >> $@; \
	 chmod 0755 $@
else
intl-desktop-file.sh: intl-desktop-file$(EXEEXT)
	@echo '#!/bin/sh' > $@; \
	 echo 'GGL_MODULE_PATH=`pwd`' >> $@; \
	 echo 'for i in $(abs_top_builddir)/extensions/*; do' >> $@; \
	 echo '  if test -d $$i; then' >> $@; \
	 echo '    GGL_MODULE_PATH=$$GGL_MODULE_PATH:$$i' >> $@; \
	 echo '  fi' >> $@; \
	 echo 'done' >> $@; \
	 echo 'export GGL_MODULE_PATH' >> $@; \
	 echo '$(abs_builddir)/intl-desktop-file $$@' >> $@; \
	 chmod 0755 $@
endif

# To make sure our mime type has higher priority.
xml_files = 00-google-gadgets.xml

mimedir = $(datadir)/mime/packages
mime_DATA = $(xml_files)

ICONS = 32x32.png \
	48x48.png \
	64x64.png \
	128x128.png \
	192x192.png

EXTRA_DIST = $(xml_files) $(ICONS) CMakeLists.txt

if GGL_ENABLE_XDG_ICON_RESOURCE
update-icon-cache:
	@if test -z "$(DESTDIR)"; then \
		echo "Updating icon cache."; \
		$(XDG_ICON_RESOURCE) forceupdate --theme hicolor; \
	fi
else
update-icon-cache:
	@echo "Updating icon cache is disabled."
endif

ICON_SIZES = 32x32 48x48 64x64 128x128 192x192

install-icons:
	@for icon in $(ICON_SIZES); do \
	  mkdir -p $(DESTDIR)$(datadir)/icons/hicolor/$${icon}/apps; \
	  mkdir -p $(DESTDIR)$(datadir)/icons/hicolor/$${icon}/mimetypes; \
	  $(INSTALL_DATA) $(srcdir)/$${icon}.png $(DESTDIR)$(datadir)/icons/hicolor/$${icon}/apps/google-gadgets.png; \
	  rm -f $(DESTDIR)$(datadir)/icons/hicolor/$${icon}/mimetypes/application-x-google-gadget.png; \
	  $(LN_S) $(datadir)/icons/hicolor/$${icon}/apps/google-gadgets.png \
	          $(DESTDIR)$(datadir)/icons/hicolor/$${icon}/mimetypes/application-x-google-gadget.png; \
	done; \
	mkdir -p $(DESTDIR)$(datadir)/pixmaps; \
	rm -f $(DESTDIR)$(datadir)/pixmaps/google-gadgets.png; \
	$(LN_S) $(datadir)/icons/hicolor/128x128/apps/google-gadgets.png $(DESTDIR)$(datadir)/pixmaps/

uninstall-icons:
	@for icon in $(ICON_SIZES); do \
	  rm -f $(DESTDIR)$(datadir)/icons/hicolor/$${icon}/mimetypes/application-x-google-gadget.png; \
	  rm -f $(DESTDIR)$(datadir)/icons/hicolor/$${icon}/apps/google-gadgets.png; \
	done; \
	rm -f $(DESTDIR)$(datadir)/pixmaps/google-gadgets.png

install-data-local: install-icons update-icon-cache

uninstall-local: uninstall-icons update-icon-cache

if GGL_ENABLE_UPDATE_MIME_DATABASE
install-data-hook:
	@if test -z "$(DESTDIR)"; then \
	  $(UPDATE_MIME_DATABASE) "$(DESTDIR)$(datadir)/mime"; \
	fi

uninstall-hook:
	@if test -z "$(DESTDIR)"; then \
	  $(UPDATE_MIME_DATABASE) "$(DESTDIR)$(datadir)/mime"; \
	fi
endif

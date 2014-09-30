#
# spec file for package google-gadgets (Version 0.10.1)
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

# norootforbuild

#############################################################################
# Commom part
#############################################################################
Name:           google-gadgets
Version:        0.11.0
Release:        1
License:        Apache License Version 2.0
Group:          Productivity/Networking/Web/Utilities
Summary:        Google Gadgets for Linux
Url:            http://code.google.com/p/google-gadgets-for-linux/
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
Source0:        http://google-gadgets-for-linux.googlecode.com/files/google-gadgets-for-linux-%{version}.tar.bz2
#Autoreqprov:   on

#############################################################################
# openSUSE
#############################################################################
%if 0%{?suse_version}
BuildRequires:  gcc-c++ zip autoconf update-desktop-files flex libtool
BuildRequires:  gtk2-devel >= 2.10.0
BuildRequires:  cairo-devel >= 1.2.0
BuildRequires:  libcurl-devel >= 7.16.0
BuildRequires:  libxml2-devel >= 2.6.0
BuildRequires:  zlib-devel >= 1.2.0
BuildRequires:  librsvg-devel >= 2.18.0
%if %suse_version >= 1100
BuildRequires:  mozilla-xulrunner190-devel
%else
BuildRequires:  mozilla-xulrunner181-devel
%endif
%if %suse_version > 1030
BuildRequires:  gstreamer-0_10-plugins-base-devel
BuildRequires:  libQtWebKit-devel >= 4.4.0
%else
BuildRequires:  gstreamer010-plugins-base-devel
%endif
BuildRequires:  dbus-1-devel >= 1.0.2
BuildRequires:  libqt4-devel >= 4.3
BuildRequires:  startup-notification-devel
BuildRequires:  NetworkManager-devel >= 0.6.5
%endif

#############################################################################
# Fedora, RHEL and CentOS
#############################################################################
%if 0%{?fedora_version}
BuildRequires:  gcc-c++ zip autoconf flex libtool
BuildRequires:  gtk2-devel >= 2.10.0
BuildRequires:  cairo-devel >= 1.2.0
BuildRequires:  libxml2-devel >= 2.6.0
BuildRequires:  zlib-devel >= 1.2.0
BuildRequires:  librsvg2-devel >= 2.18.0
BuildRequires:  gstreamer-plugins-base-devel >= 0.10.6
BuildRequires:  gstreamer-devel >= 0.10.6
BuildRequires:  dbus-devel >= 1.0.2
BuildRequires:  libtool-ltdl-devel
BuildRequires:  startup-notification-devel
BuildRequires:  NetworkManager-devel >= 0.6.5

%if %{fedora_version} >= 9
BuildRequires:  xulrunner-devel >= 1.9
BuildRequires:  xulrunner-devel-unstable >= 1.9
BuildRequires:  qt-devel >= 4.3
BuildRequires:  libcurl-devel >= 7.16.0
%else
BuildRequires:  firefox-devel >= 2.0
BuildRequires:  qt4-devel >= 4.3
BuildRequires:  curl-devel >= 7.16.0
%endif
%endif


%description
Google Gadgets for Linux provides a platform for running desktop gadgets under
Linux, catering to the unique needs of Linux users. It's compatible with the
gadgets written for Google Desktop for Windows as well as the Universal
Gadgets on iGoogle. Following Linux norms, this project is open-sourced
under the Apache License.

Authors:
--------
    Google Gadgets for Linux team<google-gadgets-for-linux-dev@googlegroups.com>


%package -n libggadget-1_0-0
License:        Apache License Version 2.0
Group:          System/Libraries
Summary:        Google Gadgets main libraries

%if 0%{?suse_version}
%if %suse_version >= 1110
Requires:       libltdl7
%else
Requires:       libltdl
%endif
Requires:       dbus-1 >= 1.0.2
%endif

%if 0%{?fedora_version}
Requires:       libtool-ltdl
Requires:       dbus >= 1.0.2
%endif

%description -n libggadget-1_0-0
This package contains the main Google Gadgets libraries, it is required by both
the GTK+ and QT versions of Google Gadgets.

Authors:
--------
    Google Gadgets for Linux team<google-gadgets-for-linux-dev@googlegroups.com>


%package -n libggadget-devel
License:        Apache License Version 2.0
Group:          Development/Libraries/C and C++
Summary:        Google Gadgets main development files
Requires:       libggadget-1_0-0 = %{version}

%if 0%{?suse_version}
Requires:       dbus-1-devel >= 1.0.2
%endif

%if 0%{?fedora_version}
Requires:       dbus-devel >= 1.0.2
%endif


%description -n libggadget-devel
This package contains the development files assoicated with libggadget, it is
needed to write programs that utilise libggadget.

Authors:
--------
    Google Gadgets for Linux team<google-gadgets-for-linux-dev@googlegroups.com>


%package -n libggadget-gtk-1_0-0
License:        Apache License Version 2.0
Group:          System/Libraries
Summary:        Google Gadgets GTK+ library
Requires:       libggadget-1_0-0 = %{version}

%if 0%{?suse_version}
Requires:       gtk2 >= 2.10.0
Requires:       cairo >= 1.2.0
Requires:       librsvg >= 2.18.0
%endif

%if 0%{?fedora_version}
Requires:       gtk2 >= 2.10.0
Requires:       cairo >= 1.2.0
Requires:       librsvg2 >= 2.18.0
%endif


%description -n libggadget-gtk-1_0-0
This package contains the GTK+ Google Gadgets library, it is required to run
the GTK+ version of Google Gadgets.

Authors:
--------
    Google Gadgets for Linux team<google-gadgets-for-linux-dev@googlegroups.com>


%package -n libggadget-gtk-devel
License:        Apache License Version 2.0
Group:          Development/Libraries/C and C++
Summary:        Google Gadgets GTK+ development files
Requires:       libggadget-devel = %{version}
Requires:       libggadget-gtk-1_0-0 = %{version}

%if 0%{?suse_version}
Requires:       gtk2-devel >= 2.10.0
Requires:       cairo-devel >= 1.2.0
Requires:       librsvg-devel >= 2.18.0
%endif

%if 0%{?fedora_version}
Requires:       gtk2-devel >= 2.10.0
Requires:       cairo-devel >= 1.2.0
Requires:       librsvg2-devel >= 2.18.0
%endif

%description -n libggadget-gtk-devel
This package contains the development files assoicated with libggadget-gtk,
it is needed to write GTK+ programs that utilise libggadget.

Authors:
--------
    Google Gadgets for Linux team<google-gadgets-for-linux-dev@googlegroups.com>


%package -n libggadget-qt-1_0-0
License:        Apache License Version 2.0
Group:          System/Libraries
Summary:        Google Gadgets QT library
Requires:       libggadget-1_0-0 = %{version}

%if 0%{?suse_version}
Requires:       libqt4 >= 4.3
%if %suse_version > 1030
Requires:       libQtWebKit4 >= 4.4.0
%endif
%endif

%if 0%{?fedora_version}
Requires:       qt-x11 >= 4.3
%endif

%description -n libggadget-qt-1_0-0
This package contains the QT Google Gadgets library, it is required to run
the QT version of Google Gadgets.

Authors:
--------
    Google Gadgets for Linux team<google-gadgets-for-linux-dev@googlegroups.com>


%package -n libggadget-qt-devel
License:        Apache License Version 2.0
Group:          Development/Libraries/C and C++
Summary:        Google Gadgets QT development files
Requires:       libggadget-devel = %{version}
Requires:       libggadget-qt-1_0-0 = %{version}

%if 0%{?suse_version}
Requires:       libqt4-devel >= 4.3
%if %suse_version > 1030
Requires:       libQtWebKit-devel >= 4.4.0
%endif
%endif

%if 0%{?fedora_version}
Requires:       qt-devel >= 4.3
%endif

%description -n libggadget-qt-devel
This package contains the development files assoicated with libggadget-qt,
it is needed to write QT programs that utilise libggadget.

Authors:
--------
    Google Gadgets for Linux team<google-gadgets-for-linux-dev@googlegroups.com>

%package common
License:        Apache License Version 2.0
Group:          Productivity/Networking/Web/Utilities
Summary:        Common files for QT and GTK+ versions of google-gadgets
Requires:       libggadget-1_0-0 = %{version}

%if 0%{?suse_version}
Requires:       libcurl4 >= 7.16.0
Requires:       libxml2 >= 2.6.0
%endif

%if 0%{?fedora_version}
%if %{fedora_version} >= 9
Requires:       libcurl >= 7.16.0
%else
Requires:       curl >= 7.16.0
%endif
Requires:       libxml2 >= 2.6.0
%endif

%description common
Google Gadgets for Linux provides a platform for running desktop gadgets under
Linux, catering to the unique needs of Linux users. It's compatible with the
gadgets written for Google Desktop for Windows as well as the Universal
Gadgets on iGoogle. Following Linux norms, this project is open-sourced
under the Apache License.

This package includes files common to both GTK+ and QT versions.

Authors:
--------
    Google Gadgets for Linux team<google-gadgets-for-linux-dev@googlegroups.com>


%package gtk
License:        Apache License Version 2.0
Group:          Productivity/Networking/Web/Utilities
Summary:        GTK+ Version of Google Gadgets
Requires:       libggadget-gtk-1_0-0 = %{version}
Requires:       google-gadgets-common = %{version}
Requires:       google-gadgets-gst = %{version}
Requires:       google-gadgets-xul = %{version}

%description gtk
Google Gadgets for Linux provides a platform for running desktop gadgets under
Linux, catering to the unique needs of Linux users. It's compatible with the
gadgets written for Google Desktop for Windows as well as the Universal
Gadgets on iGoogle. Following Linux norms, this project is open-sourced
under the Apache License.

This package includes the GTK+ version.

Authors:
--------
    Google Gadgets for Linux team<google-gadgets-for-linux-dev@googlegroups.com>


%package qt
License:        Apache License Version 2.0
Group:          Productivity/Networking/Web/Utilities
Summary:        QT Version of Google Gadgets
Requires:       libggadget-qt-1_0-0 = %{version}
Requires:       google-gadgets-common = %{version}
Requires:       google-gadgets-gst = %{version}
Requires:       google-gadgets-xul = %{version}

%description qt
Google Gadgets for Linux provides a platform for running desktop gadgets under
Linux, catering to the unique needs of Linux users. It's compatible with the
gadgets written for Google Desktop for Windows as well as the Universal
Gadgets on iGoogle. Following Linux norms, this project is open-sourced
under the Apache License.

This package includes the QT version.

Authors:
--------
    Google Gadgets for Linux team<google-gadgets-for-linux-dev@googlegroups.com>


%package gst
License:        Apache License Version 2.0
Group:          Productivity/Networking/Web/Utilities
Summary:        GStreamer modules for Google Gadgets
Requires:       libggadget-1_0-0 = %{version}

%if 0%{?suse_version}
%if %suse_version > 1030
Requires:       gstreamer-0_10-plugins-base
%else
Requires:       gstreamer010-plugins-base
%endif
%endif

%if 0%{?fedora_version}
Requires:       gstreamer-plugins-base >= 0.10.6
%endif

%description gst
Google Gadgets for Linux provides a platform for running desktop gadgets under
Linux, catering to the unique needs of Linux users. It's compatible with the
gadgets written for Google Desktop for Windows as well as the Universal
Gadgets on iGoogle. Following Linux norms, this project is open-sourced
under the Apache License.

This package includes the GStreamer modules.

Authors:
--------
    Google Gadgets for Linux team<google-gadgets-for-linux-dev@googlegroups.com>


%package xul
License:        Apache License Version 2.0
Group:          Productivity/Networking/Web/Utilities
Summary:        XULRunner modules for Google Gadgets
Requires:       libggadget-1_0-0 = %{version}

%if 0%{?suse_version}
%if %suse_version >= 1100
Requires:       mozilla-xulrunner190
%else
Requires:       mozilla-xulrunner181
%endif
%endif

%if 0%{?fedora_version}
%if %{fedora_version} >= 9
Requires:       xulrunner >= 1.9
%else
Requires:       firefox >= 2.0
%endif
%endif

%description xul
Google Gadgets for Linux provides a platform for running desktop gadgets under
Linux, catering to the unique needs of Linux users. It's compatible with the
gadgets written for Google Desktop for Windows as well as the Universal
Gadgets on iGoogle. Following Linux norms, this project is open-sourced
under the Apache License.

This package includes the XULRunner modules.

Authors:
--------
    Google Gadgets for Linux team<google-gadgets-for-linux-dev@googlegroups.com>

%prep
%setup -q -n google-gadgets-for-linux-%{version}

%build
%if 0%{?suse_version}
%{suse_update_config -f}
# autoreconf doesn't work on opensuse 11.1
# autoreconf
%configure \
  --disable-werror \
  --with-browser-plugins-dir=%{_libdir}/browser-plugins \
  --disable-soup-xml-http-request \
  --disable-webkit-script-runtime \
  --disable-gtkwebkit-browser-element
make %{?jobs:-j%jobs}
%endif

%if 0%{?fedora_version}
autoreconf
%configure \
  --disable-werror \
  --with-browser-plugins-dir=%{_libdir}/mozilla/plugins \
  --disable-soup-xml-http-request \
  --disable-webkit-script-runtime \
  --disable-gtkwebkit-browser-element
make %{?_smp_mflags}
%endif

%install
rm -fr $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install-strip
# These are dynamic modules... we shouldn't be installing them
rm -f $RPM_BUILD_ROOT/%{_libdir}/google-gadgets/modules/*.la
rm -f $RPM_BUILD_ROOT/%{_libdir}/google-gadgets/modules/*.a
# Remove all static libraries.
rm -f $RPM_BUILD_ROOT/%{_libdir}/*.a

%if 0%{?suse_version}
%suse_update_desktop_file -n ggl-gtk
%suse_update_desktop_file -n ggl-qt
%suse_update_desktop_file -n ggl-designer

MD5SUM=$(md5sum COPYING | sed 's/ .*//')
if test -f /usr/share/doc/licenses/md5/$MD5SUM ; then
  ln -sf /usr/share/doc/licenses/md5/$MD5SUM COPYING
fi
%endif


%post -n google-gadgets-common
if [ -x /usr/bin/update-mime-database ]; then
  /usr/bin/update-mime-database %{_datadir}/mime &> /dev/null || :
fi
if [ -x /usr/bin/xdg-icon-resource ]; then
  /usr/bin/xdg-icon-resource forceupdate --theme hicolor &> /dev/null || :
fi

%postun -n google-gadgets-common
if [ -x /usr/bin/update-mime-database ]; then
  /usr/bin/update-mime-database %{_datadir}/mime &> /dev/null || :
fi
if [ -x /usr/bin/xdg-icon-resource ]; then
  /usr/bin/xdg-icon-resource forceupdate --theme hicolor &> /dev/null || :
fi

%post -n libggadget-1_0-0 -p /sbin/ldconfig
%postun -n libggadget-1_0-0 -p /sbin/ldconfig

%post -n libggadget-gtk-1_0-0 -p /sbin/ldconfig
%postun -n libggadget-gtk-1_0-0 -p /sbin/ldconfig

%post -n libggadget-qt-1_0-0 -p /sbin/ldconfig
%postun -n libggadget-qt-1_0-0 -p /sbin/ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files -n google-gadgets-common
%defattr(-, root, root)
%doc COPYING AUTHORS README NEWS
%dir %{_libdir}/google-gadgets/
%dir %{_libdir}/google-gadgets/modules/
%{_libdir}/google-gadgets/modules/analytics*.so
%{_libdir}/google-gadgets/modules/default*.so
%{_libdir}/google-gadgets/modules/linux*.so
%{_libdir}/google-gadgets/modules/google-gadget-manager.so
%{_libdir}/google-gadgets/modules/libxml2*.so
%{_libdir}/google-gadgets/modules/curl*.so
%{_libdir}/google-gadgets/modules/dbus*.so
%{_libdir}/google-gadgets/modules/html*.so
%dir %{_datadir}/google-gadgets/
%{_datadir}/google-gadgets/*.gg
%{_datadir}/mime/packages/*.xml
%{_datadir}/pixmaps/*.png
%{_datadir}/icons/hicolor/*/apps/*.png
%{_datadir}/icons/hicolor/*/mimetypes/*.png

%files -n libggadget-1_0-0
%defattr(-, root, root)
%{_libdir}/libggadget-1.0*.so.*
%{_libdir}/libggadget-dbus-1.0*.so.*
%{_libdir}/libggadget-js-1.0*.so.*
%{_libdir}/libggadget-xdg-1.0*.so.*
%{_libdir}/libggadget-npapi-1.0*.so.*

%files -n libggadget-devel
%defattr(-, root, root)
%dir %{_includedir}/google-gadgets/
%dir %{_includedir}/google-gadgets/ggadget
%dir %{_includedir}/google-gadgets/ggadget/dbus
%dir %{_includedir}/google-gadgets/ggadget/js
%dir %{_includedir}/google-gadgets/ggadget/xdg
%dir %{_includedir}/google-gadgets/ggadget/npapi
%{_includedir}/google-gadgets/ggadget/*.h
%{_includedir}/google-gadgets/ggadget/dbus/*.h
%{_includedir}/google-gadgets/ggadget/js/*.h
%{_includedir}/google-gadgets/ggadget/xdg/*.h
%{_includedir}/google-gadgets/ggadget/npapi/*.h
%dir %{_libdir}/google-gadgets/include/
%dir %{_libdir}/google-gadgets/include/ggadget/
%{_libdir}/google-gadgets/include/ggadget/sysdeps.h
%{_libdir}/libggadget-1.0*.so
%{_libdir}/libggadget-dbus-1.0*.so
%{_libdir}/libggadget-js-1.0*.so
%{_libdir}/libggadget-xdg-1.0*.so
%{_libdir}/libggadget-npapi-1.0*.so
%{_libdir}/libggadget-1.0*.la
%{_libdir}/libggadget-dbus-1.0*.la
%{_libdir}/libggadget-js-1.0*.la
%{_libdir}/libggadget-xdg-1.0*.la
%{_libdir}/libggadget-npapi-1.0*.la
%{_libdir}/pkgconfig/libggadget-1.0.pc
%{_libdir}/pkgconfig/libggadget-dbus-1.0.pc
%{_libdir}/pkgconfig/libggadget-js-1.0.pc
%{_libdir}/pkgconfig/libggadget-xdg-1.0.pc
%{_libdir}/pkgconfig/libggadget-npapi-1.0.pc

%files -n libggadget-gtk-1_0-0
%defattr(-, root, root)
%{_libdir}/libggadget-gtk-1.0*.so.*

%files -n libggadget-gtk-devel
%defattr(-, root, root)
%dir %{_includedir}/google-gadgets/ggadget/gtk/
%{_includedir}/google-gadgets/ggadget/gtk/*.h
%{_libdir}/libggadget-gtk-1.0*.so
%{_libdir}/libggadget-gtk-1.0*.la
%{_libdir}/pkgconfig/libggadget-gtk-1.0.pc

%files -n libggadget-qt-1_0-0
%defattr(-, root, root)
%{_libdir}/libggadget-qt-1.0*.so.*

%files -n libggadget-qt-devel
%defattr(-, root, root)
%dir %{_includedir}/google-gadgets/ggadget/qt/
%{_includedir}/google-gadgets/ggadget/qt/*.h
%{_libdir}/libggadget-qt-1.0*.so
%{_libdir}/libggadget-qt-1.0*.la
%{_libdir}/pkgconfig/libggadget-qt-1.0.pc

%files -n google-gadgets-gtk
%defattr(-, root, root)
%{_bindir}/ggl-gtk
%{_datadir}/applications/ggl-gtk.desktop
%{_datadir}/applications/ggl-designer.desktop
%{_libdir}/google-gadgets/modules/gtk-*.so

%files -n google-gadgets-qt
%defattr(-, root, root)
%{_bindir}/ggl-qt
%{_datadir}/applications/ggl-qt.desktop
%{_libdir}/google-gadgets/modules/qt*.so

%files -n google-gadgets-gst
%defattr(-, root, root)
%{_libdir}/google-gadgets/modules/gst*.so

%files -n google-gadgets-xul
%defattr(-, root, root)
%{_libdir}/google-gadgets/modules/smjs*.so
%{_libdir}/google-gadgets/modules/gtkmoz*.so
%{_libdir}/google-gadgets/gtkmoz-browser-child

%changelog
* Sun May 31 2009 James Su <james.su@gmail.com>
- Updates to version 0.11.0

* Thu Jan 08 2009 James Su <james.su@gmail.com>
- Updates to version 0.10.5

* Tue Dec 16 2008 James Su <james.su@gmail.com>
- Updates to version 0.10.4

* Tue Nov 4 2008 James Su <james.su@gmail.com>
- Updates to support version 0.10.3

* Wed Sep 17 2008 James Su <james.su@gmail.com>
- Fix the name and version of curl library.

* Tue Sep 16 2008 James Su <james.su@gmail.com>
- Updates dependency information.
- Removes static libraries.
- Adds support for Fedora 8 and 9.

* Thu Sep 14 2008 James Su <james.su@gmail.com>
- Initial release.

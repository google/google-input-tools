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

# Process this file with autoconf to produce a configure script

# Specifies:
#  Package name
#  Version
#  Bug report address
AC_INIT([google-gadgets-for-linux],
        [0.11.2],
        [google-gadgets-for-linux-dev@googlegroups.com])

# Requires autoconf version >= 2.50
AC_PREREQ([2.50])

# Specifies a source file in the source directory.
AC_CONFIG_SRCDIR([ggadget/common.h])

# Tells autoconf where to find necessary build scripts and macros.
AC_CONFIG_AUX_DIR([autotools])
AC_CONFIG_MACRO_DIR([autotools])


# Save the user defined CFLAGS and CXXFLAGS, then restore them later.
# So that the value set by AC_PROG_CC and AC_PROG_CXX won't be used,
# even if they are empty.
ggl_save_CFLAGS=$CFLAGS
ggl_save_CPPFLAGS=$CPPFLAGS
ggl_save_CXXFLAGS=$CXXFLAGS
ggl_save_LDFLAGS=$LDFLAGS

###########################################################
##          Some definitions of Version macros.          ##
###########################################################

# Version numbers of Google Gadgets for Linux package.
GGL_MAJOR_VERSION=0
GGL_MINOR_VERSION=11
GGL_MICRO_VERSION=2

GGL_VERSION_UNQUOTED="$GGL_MAJOR_VERSION.$GGL_MINOR_VERSION.$GGL_MICRO_VERSION"
GGL_VERSION="\"$GGL_VERSION_UNQUOTED\""

AC_SUBST(GGL_MAJOR_VERSION)
AC_SUBST(GGL_MINOR_VERSION)
AC_SUBST(GGL_MICRO_VERSION)
AC_SUBST(GGL_VERSION_UNQUOTED)
AC_SUBST(GGL_VERSION)


# libtool versioning for libggadget

# Increment if the interface has additions, changes, removals.
LIBGGADGET_CURRENT=0

# Increment any time the source changes; set to 0 if you increment
# LIBGGADGET_CURRENT
LIBGGADGET_REVISION=0

# Increment if any interfaces have been added; set to 0
# if any interfaces have been removed. removal has
# precedence over adding, so set to 0 if both happened.
LIBGGADGET_AGE=0

LIBGGADGET_VERSION=$LIBGGADGET_CURRENT:$LIBGGADGET_REVISION:$LIBGGADGET_AGE

AC_SUBST(LIBGGADGET_CURRENT)
AC_SUBST(LIBGGADGET_REVISION)
AC_SUBST(LIBGGADGET_AGE)
AC_SUBST(LIBGGADGET_VERSION)

# Define a string for the earliest version that this release has
# binary compatibility with. This is used for module locations.
#
LIBGGADGET_BINARY_VERSION_UNQUOTED="1.0.0"
LIBGGADGET_BINARY_VERSION="\"$LIBGGADGET_BINARY_VERSION_UNQUOTED\""
AC_SUBST(LIBGGADGET_BINARY_VERSION_UNQUOTED)
AC_SUBST(LIBGGADGET_BINARY_VERSION)

# This string is used in auto update request. It should be updated to the
# time of a release build is made. Its format is yymmdd-HHMMSS.
GGL_VERSION_TIMESTAMP="\"100127-000000\""
AC_SUBST(GGL_VERSION_TIMESTAMP)

# Define the version of Google Desktop Gadget API that this release supports.
GGL_API_MAJOR_VERSION=5
GGL_API_MINOR_VERSION=8
GGL_API_VERSION_UNQUOTED="$GGL_API_MAJOR_VERSION.$GGL_API_MINOR_VERSION.0.0"
GGL_API_VERSION="\"$GGL_API_VERSION_UNQUOTED\""
AC_SUBST(GGL_API_MAJOR_VERSION)
AC_SUBST(GGL_API_MINOR_VERSION)
AC_SUBST(GGL_API_VERSION_UNQUOTED)
AC_SUBST(GGL_API_VERSION)

# Define a string for the earliest version that this release cannot
# coexist with. This is used for directory hierarchies.
#
GGL_EPOCH=-1.0
AC_SUBST(GGL_EPOCH)

GGL_BASE_DIR=google-gadgets
AC_SUBST(GGL_BASE_DIR)

# Check build and host system types
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

###########################################################
##                      Config options.                  ##
###########################################################

# Host conditions.
build_gtk_host=yes
build_qt_host=yes

# Library conditions.
build_libggadget_dbus=yes
build_libggadget_gtk=yes
build_libggadget_npapi=yes
build_libggadget_qt=yes

# Extension conditions.
# analytics_usage_collector will always be built.
build_curl_xml_http_request=yes
build_dbus_script_class=yes
# default_framework will always be built.
# default_options will always be built.
# google_gadget_manager will always be built.
build_gst_audio_framework=yes
build_gst_video_element=yes
build_gtk_edit_element=yes
build_gtk_flash_element=yes
build_gtk_system_framework=yes
build_gtkmoz_browser_element=yes
build_libxml2_xml_parser=yes
# linux_system_framework will only be enabled for Linux system.
build_linux_system_framework=no
build_qt_edit_element=yes
build_qt_script_runtime=yes
build_qt_system_framework=yes
build_qt_xml_http_request=yes
build_qtwebkit_browser_element=yes
build_smjs_script_runtime=yes
build_webkit_script_runtime=yes
build_gtkwebkit_browser_element=yes
build_soup_xml_http_request=yes

#================ compile options ==============
AC_ARG_ENABLE([debug],
              AS_HELP_STRING([--enable-debug], [enable debug]),
              [enable_debug=$enableval],
              [enable_debug=no])

AC_ARG_ENABLE([werror],
              AS_HELP_STRING([--disable-werror],
                             [Don't make all compile warnings into errors.]),
              [use_werror=$withval],
              [use_werror=yes])

AC_ARG_ENABLE([stack-protector],
              AS_HELP_STRING([--disable-stack-protector],
                             [Disable stack protector.]),
              [enable_stack_protector=$enableval],
              [enable_stack_protector=yes])

#================ with options =================
AC_ARG_WITH([oem-brand],
            AC_HELP_STRING([--with-oem-brand=code],
                           [Specify the OEM brand code.]),
            [ggl_oem_brand=$withval],
            [ggl_oem_brand=""])

AC_ARG_WITH([memcheck-command],
            AS_HELP_STRING([--with-memcheck-command=CMD],
                [Specify a memory check command for running unit tests.]),
            [memcheck_command=$withval],
            [memcheck_command=""])

AC_ARG_WITH([gst-audio-sink],
            AS_HELP_STRING([--with-gst-audio-sink=audiosink],
                [Use a specified audiosink instead of autoaudiosink.]),
            [default_gst_audio_sink=$withval],
            [default_gst_audio_sink="autoaudiosink"])

AC_ARG_WITH([browser-plugins-dir],
            AS_HELP_STRING([--with-browser-plugins-dir=plugins directory],
                [Specifiy directory of browser plugins. But when the program is
                 running, the environment variable BROWSER_PLUGINS_DIR has
                 higher priority if it's set. ]),
            [default_browser_plugins_dir=$withval],
            [default_browser_plugins_dir="$libdir/firefox/plugins"])

AC_ARG_WITH([gtkmozembed],
            AC_HELP_STRING(
                [--with-gtkmozembed=@<:@libxul|xulrunner|firefox2|firefox|seamonkey@:>@],
                [Selects with gecko engine to build against, it affects the
                 search rule of spidermonkey library (libmozjs) as well.]),
            [with_gtkmozembed=$withval],
            [with_gtkmozembed=auto])

AC_ARG_WITH([mozjs-libname],
            AC_HELP_STRING([--with-mozjs-libname=libmozjs.so],
                [Specify the file name of spidermonkey library to be
                 dynamically loaded. It only takes effect when using a
                 xulrunner library which supports XPCOM_GLUE.]),
            [with_mozjs_libname=$withval],
            [with_mozjs_libname=libmozjs.so])

AC_ARG_WITH([ssl-ca-file],
            AS_HELP_STRING([--with-ssl-ca-file=/path/to/ca-certificates.crt],
                [Specifiy CA certificates file used by soup-xml-http-request
                 extension. If it's set to empty, then SSL certificate won't be
                 checked when accessing https:// sites. CAUTION: it should
                 always be set to a valid CA file, otherwise there will be
                 serious security issue. It's not set by default.]),
            [default_ssl_ca_file=$withval],
            [default_ssl_ca_file=""])

#=================== host options =================
AC_ARG_ENABLE([gtk-host],
              AS_HELP_STRING([--disable-gtk-host],
                             [don't build gtk host]),
              [build_gtk_host=$enableval],
              [build_gtk_host=yes])

AC_ARG_ENABLE([qt-host],
              AS_HELP_STRING([--disable-qt-host],
                             [don't build qt host]),
              [build_qt_host=$enableval],
              [build_qt_host=yes])

#================== library options ===============
AC_ARG_ENABLE([libggadget-dbus],
              AS_HELP_STRING([--disable-libggadget-dbus],
                             [don't build libggadget-dbus]),
              [build_libggadget_dbus=$enableval],
              [build_libggadget_dbus=yes])

AC_ARG_ENABLE([libggadget-gtk],
              AS_HELP_STRING([--disable-libggadget-gtk],
                             [don't build libggadget-gtk]),
              [build_libggadget_gtk=$enableval],
              [build_libggadget_gtk=yes])

AC_ARG_ENABLE([libggadget-npapi],
              AS_HELP_STRING([--disable-libggadget-npapi],
                             [don't build libggadget-npapi]),
              [build_libggadget_npapi=$enableval],
              [build_libggadget_npapi=yes])

AC_ARG_ENABLE([libggadget-qt],
              AS_HELP_STRING([--disable-libggadget-qt],
                             [don't build libggadget-qt]),
              [build_libggadget_qt=$enableval],
              [build_libggadget_qt=yes])

#================== extension options ==============
AC_ARG_ENABLE([curl-xml-http-request],
              AS_HELP_STRING([--disable-curl-xml-http-request],
                             [don't build curl-xml-http-request]),
              [build_curl_xml_http_request=$enableval],
              [build_curl_xml_http_request=yes])

AC_ARG_ENABLE([soup-xml-http-request],
              AS_HELP_STRING([--disable-soup-xml-http-request],
                             [don't build soup-xml-http-request]),
              [build_soup_xml_http_request=$enableval],
              [build_soup_xml_http_request=yes])

AC_ARG_ENABLE([dbus-script-class],
              AS_HELP_STRING([--disable-dbus-script-class],
                             [don't build dbus-script-class]),
              [build_dbus_script_class=$enableval],
              [build_dbus_script_class=yes])

AC_ARG_ENABLE([gst-audio-framework],
              AS_HELP_STRING([--disable-gst-audio-framework],
                             [don't build gst-audio-framework]),
              [build_gst_audio_framework=$enableval],
              [build_gst_audio_framework=yes])

AC_ARG_ENABLE([gst-video-element],
              AS_HELP_STRING([--disable-gst-video-element],
                             [don't build gst-video-element]),
              [build_gst_video_element=$enableval],
              [build_gst_video_element=yes])

AC_ARG_ENABLE([gtk-edit-element],
              AS_HELP_STRING([--disable-gtk-edit-element],
                             [don't build gtk-edit-element]),
              [build_gtk_edit_element=$enableval],
              [build_gtk_edit_element=yes])

AC_ARG_ENABLE([gtk-flash-element],
              AS_HELP_STRING([--disable-gtk-flash-element],
                             [don't build gtk-flash-element]),
              [build_gtk_flash_element=$enableval],
              [build_gtk_flash_element=yes])

AC_ARG_ENABLE([gtk-system-framework],
              AS_HELP_STRING([--disable-gtk-system-framework],
                             [don't build gtk-system-framework]),
              [build_gtk_system_framework=$enableval],
              [build_gtk_system_framework=yes])

AC_ARG_ENABLE([gtkmoz-browser-element],
              AS_HELP_STRING([--disable-gtkmoz-browser-element],
                             [don't build gtkmoz-browser-element]),
              [build_gtkmoz_browser_element=$enableval],
              [build_gtkmoz_browser_element=yes])

AC_ARG_ENABLE([libxml2-xml-parser],
              AS_HELP_STRING([--disable-libxml2-xml-parser],
                             [don't build libxml2-xml-parser]),
              [build_libxml2_xml_parser=$enableval],
              [build_libxml2_xml_parser=yes])

AC_ARG_ENABLE([qt-edit-framework],
              AS_HELP_STRING([--disable-qt-edit-framework],
                             [don't build qt-edit-framework]),
              [build_qt_edit_framework=$enableval],
              [build_qt_edit_framework=yes])

AC_ARG_ENABLE([qt-script-runtime],
              AS_HELP_STRING([--disable-qt-script-runtime],
                             [don't build qt-script-runtime]),
              [build_qt_script_runtime=$enableval],
              [build_qt_script_runtime=yes])

AC_ARG_ENABLE([qt-system-framework],
              AS_HELP_STRING([--disable-qt-system-framework],
                             [don't build qt-system-framework]),
              [build_qt_system_framework=$enableval],
              [build_qt_system_framework=yes])

AC_ARG_ENABLE([qt-xml-http-request],
              AS_HELP_STRING([--disable-qt-xml-http-request],
                             [don't build qt-xml-http-request]),
              [build_qt_xml_http_request=$enableval],
              [build_qt_xml_http_request=yes])

AC_ARG_ENABLE([qtwebkit-browser-element],
              AS_HELP_STRING([--disable-qtwebkit-browser-element],
                             [don't build qtwebkit-browser-element]),
              [build_qtwebkit_browser_element=$enableval],
              [build_qtwebkit_browser_element=yes])

AC_ARG_ENABLE([smjs-script-runtime],
              AS_HELP_STRING([--disable-smjs-script-runtime],
                             [don't build smjs-script-runtime]),
              [build_smjs_script_runtime=$enableval],
              [build_smjs_script_runtime=yes])

AC_ARG_ENABLE([webkit-script-runtime],
              AS_HELP_STRING([--disable-webkit-script-runtime],
                             [don't build webkit-script-runtime]),
              [build_webkit_script_runtime=$enableval],
              [build_webkit_script_runtime=yes])

AC_ARG_ENABLE([gtkwebkit-browser-element],
              AS_HELP_STRING([--disable-gtkwebkit-browser-element],
                             [don't build gtkwebkit-browser-element]),
              [build_gtkwebkit_browser_element=$enableval],
              [build_gtkwebkit_browser_element=yes])

#================ other options =======================
AC_ARG_ENABLE([update-mime-database],
              AC_HELP_STRING([--disable-update-mime-database],
                             [don't run update-mime-database after install]),
              [enable_update_mime_database=$enableval],
              [enable_update_mime_database=yes])

AC_ARG_ENABLE([update-desktop-database],
              AC_HELP_STRING([--disable-update-desktop-database],
                             [don't run update-desktop-database after install]),
              [enable_update_desktop_database=$enableval],
              [enable_update_desktop_database=yes])

AC_ARG_ENABLE([xdg-icon-resource],
              AC_HELP_STRING([--disable-xdg-icon-resource],
                             [don't run xdg-icon-resource after install]),
              [enable_xdg_icon_resource=$enableval],
              [enable_xdg_icon_resource=yes])

AC_ARG_ENABLE([xdgmime],
              AC_HELP_STRING([--disable-xdgmime],
                             [disable built-in xdgmime support]),
              [enable_xdgmime=$enableval],
              [enable_xdgmime=yes])

# correct option dependencies.
if test x$build_libggadget_dbus != xyes; then
  build_dbus_script_class=no
fi

if test x$build_libggadget_gtk != xyes; then
  build_gtk_host=no
  build_gtk_edit_element=no
  build_gtk_flash_element=no
  build_gtk_system_framework=no
  build_gtkmoz_browser_element=no
fi

if test x$build_libggadget_qt != xyes; then
  build_qt_host=no
  build_qt_edit_element=no
  build_qt_script_runtime=no
  build_qt_system_framework=no
  build_qt_xml_http_request=no
  build_qtwebkit_browser_element=no
fi

if test x$build_libggadget_npapi != xyes; then
  build_gtk_flash_element=no
fi

###########################################################
##                      Default dirs.                    ##
###########################################################
GGL_MODULE_DIR=$libdir/$GGL_BASE_DIR/modules
GGL_INCLUDE_DIR=$includedir/$GGL_BASE_DIR
GGL_SYSDEPS_INCLUDE_DIR=$libdir/$GGL_BASE_DIR/include
GGL_LIBEXEC_DIR=$libdir/$GGL_BASE_DIR
GGL_RESOURCE_DIR=$datadir/$GGL_BASE_DIR

AC_SUBST(GGL_MODULE_DIR)
AC_SUBST(GGL_INCLUDE_DIR)
AC_SUBST(GGL_SYSDEPS_INCLUDE_DIR)
AC_SUBST(GGL_LIBEXEC_DIR)
AC_SUBST(GGL_RESOURCE_DIR)

###########################################################
##                 Default compiler options.             ##
###########################################################

# Check -fstack-protector and -fstack-protector-all
has_stack_protector=no
if test x$enable_stack_protector = xyes; then
  GGL_CHECK_STACK_PROTECTOR([has_stack_protector=yes], [has_stack_protector=no])
fi

if test x$enable_debug = xyes; then
  DEFAULT_COMPILE_FLAGS="-g -O0 -Wall"
  DEBUG_MACROS="_DEBUG";
  if test x$has_stack_protector = xyes; then
    DEFAULT_COMPILE_FLAGS="$DEFAULT_COMPILE_FLAGS -fstack-protector-all"
  fi
else
  DEFAULT_COMPILE_FLAGS="-O2 -Wall"
  DEBUG_MACROS="NDEBUG";
  if test x$has_stack_protector = xyes; then
    DEFAULT_COMPILE_FLAGS="$DEFAULT_COMPILE_FLAGS -fstack-protector"
  fi
fi

if test x$use_werror = xyes; then
  DEFAULT_COMPILE_FLAGS="$DEFAULT_COMPILE_FLAGS -Werror"
fi


# Enable stdc const macros by default, like INT64_C etc.
PREDEFINED_MACROS="-D__STDC_CONSTANT_MACROS \
                   -D$DEBUG_MACROS \
                   -DGGL_MODULE_DIR=\\\"$GGL_MODULE_DIR\\\" \
                   -DGGL_INCLUDE_DIR=\\\"$GGL_INCLUDE_DIR\\\" \
                   -DGGL_SYSDEPS_INCLUDE_DIR=\\\"$GGL_SYSDEPS_INCLUDE_DIR\\\" \
                   -DGGL_LIBEXEC_DIR=\\\"$GGL_LIBEXEC_DIR\\\" \
                   -DGGL_RESOURCE_DIR=\\\"$GGL_RESOURCE_DIR\\\""

case $host_os in
  *interix* | *mks* | *winnt* | *cygwin* | *mingw* )
    ggl_host_type=windows
    PREDEFINED_MACROS="$PREDEFINED_MACROS -DGGL_HOST_WINDOWS=1"
    GGL_PLATFORM_SHORT="\"win\""
    GGL_PLATFORM="\"windows\""
    ;;
  *darwin* )
    ggl_host_type=macosx
    PREDEFINED_MACROS="$PREDEFINED_MACROS -DGGL_HOST_MACOSX=1"
    GGL_PLATFORM_SHORT="\"mac\""
    GGL_PLATFORM="\"mac\""
    ;;
  *linux* )
    ggl_host_type=linux
    PREDEFINED_MACROS="$PREDEFINED_MACROS -DGGL_HOST_LINUX=1"
    GGL_PLATFORM_SHORT="\"linux\""
    GGL_PLATFORM="\"linux\""
    build_linux_system_framework=yes
    ;;
  *solaris* )
    ggl_host_type=solaris
    PREDEFINED_MACROS="$PREDEFINED_MACROS -DGGL_HOST_SOLARIS=1"
    GGL_PLATFORM_SHORT="\"solaris\""
    GGL_PLATFORM="\"solaris\""
    ;;
  * )
    ggl_host_type=unknown
    ;;
esac

AC_SUBST(GGL_PLATFORM_SHORT)
AC_SUBST(GGL_PLATFORM)

# Check OEM brand code.
if test x$ggl_oem_brand != x; then
  GGL_OEM_BRAND=$ggl_oem_brand
  AC_SUBST(GGL_OEM_BRAND)
  PREDEFINED_MACROS="$PREDEFINED_MACROS \"-DGGL_OEM_BRAND=\\\"$GGL_OEM_BRAND\\\"\""
fi

AM_CONDITIONAL([GGL_HOST_WINDOWS], [test x$ggl_host_type = xwindows])
AM_CONDITIONAL([GGL_HOST_MACOSX], [test x$ggl_host_type = xmacosx])
AM_CONDITIONAL([GGL_HOST_LINUX], [test x$ggl_host_type = xlinux])
AM_CONDITIONAL([GGL_HOST_SOLARIS], [test x$ggl_host_type = xsolaris])

AC_SUBST(PREDEFINED_MACROS)
AC_SUBST(DEFAULT_COMPILE_FLAGS)

###########################################################
##                  Start Configuration.                 ##
###########################################################

# Init automake stuff
AM_INIT_AUTOMAKE([tar-ustar])

# Initializes libtool
AC_LIBLTDL_INSTALLABLE
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL
AC_SUBST(LIBLTDL)
AC_SUBST(LTDLINCL)
AC_SUBST(LIBTOOL_DEPS)
AC_CONFIG_SUBDIRS([libltdl])
AM_CONDITIONAL(GGL_BUILD_LIBLTDL, [test x$enable_ltdl_install = xyes])

# Initialize pkg-config
PKG_PROG_PKG_CONFIG([0.20])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LN_S
AC_PROG_SED

# Checks for zip command.
AC_PATH_PROG([ZIP], [zip])

# Do compile test for C and C++ source code.
AC_LANG(C)
AC_LANG(C++)

# Checks for standard header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdint.h],[has_stdint=yes],[has_stdint=no])

if test x$has_stdint = xno; then
  AC_MSG_ERROR([Google Gadgets for Linux depends on stdint.h])
fi

# Check necessary data types
AC_TYPE_SIZE_T
AC_CHECK_SIZEOF(char)
AC_CHECK_SIZEOF(short int)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long int)
AC_CHECK_SIZEOF(size_t)
AC_CHECK_SIZEOF(double)
AC_CHECK_SIZEOF(wchar_t)

GGL_SIZEOF_CHAR=$ac_cv_sizeof_char
GGL_SIZEOF_SHORT_INT=$ac_cv_sizeof_short_int
GGL_SIZEOF_INT=$ac_cv_sizeof_int
GGL_SIZEOF_LONG_INT=$ac_cv_sizeof_long_int
GGL_SIZEOF_SIZE_T=$ac_cv_sizeof_size_t
GGL_SIZEOF_DOUBLE=$ac_cv_sizeof_double
GGL_SIZEOF_WCHAR_T=$ac_cv_sizeof_wchar_t

AC_SUBST(GGL_SIZEOF_CHAR)
AC_SUBST(GGL_SIZEOF_SHORT_INT)
AC_SUBST(GGL_SIZEOF_INT)
AC_SUBST(GGL_SIZEOF_LONG_INT)
AC_SUBST(GGL_SIZEOF_SIZE_T)
AC_SUBST(GGL_SIZEOF_DOUBLE)
AC_SUBST(GGL_SIZEOF_WCHAR_T)

if test x$ac_cv_type_size_t != xyes; then
  AC_MSG_ERROR([
No type size_t, but google Gadgets for Linux needs it!
This problem might be caused by missing C++ compiler, please make sure there is
an usable C++ compiler installed, for example the g++ compiler.
])
fi

# check for bytesex stuff
AC_C_BIGENDIAN(
  [PREDEFINED_MACROS="$PREDEFINED_MACROS -DGGL_BIG_ENDIAN"],
  [PREDEFINED_MACROS="$PREDEFINED_MACROS -DGGL_LITTLE_ENDIAN"],
)

# Check necessary libraries

# Check zlib
has_zlib=no
GGL_CHECK_ZLIB([1.2.3], [has_zlib=yes], [has_zlib=no])
if test x$has_zlib != xyes; then
  AC_MSG_ERROR([zlib is not available, but it's mandatory.])
fi

# Check pthread lib
has_pthread=no
AC_CHECK_LIB([pthread], [pthread_create], [has_pthread=yes], [has_pthread=no])
if test x$has_pthread = xyes; then
  PTHREAD_LIBS="-lpthread"
  AC_SUBST(PTHREAD_LIBS)
  PREDEFINED_MACROS="$PREDEFINED_MACROS -DHAVE_PTHREAD=1"
fi

# Check Xlib
has_xlib=no
AC_PATH_XTRA
if test "x$no_x" != "xyes"; then
  PREDEFINED_MACROS="$PREDEFINED_MACROS -DHAVE_X11=1"
  X_LIBS="$X_LIBS $X_PRE_LIBS -lX11"
  AC_SUBST(X_CFLAGS)
  AC_SUBST(X_LIBS)
  AC_SUBST(X_PRE_LIBS)
  has_xlib=yes
else
  build_libggadget_npapi=no
  build_gtk_flash_element=no
  build_gtkmoz_browser_element=no
fi

# Check qt4
has_qt4=no
has_qtnetwork=no
has_qtwebkit=no
has_qtscript=no
if test x$build_libggadget_qt = xyes; then
  if test x$ggl_host_type = xlinux; then
    PKG_CHECK_MODULES(QT4, [QtGui >= 4.3 QtCore >= 4.3 QtNetwork >= 4.3],
                      [has_qt4=yes
                       MOC=`$PKG_CONFIG --variable=moc_location QtCore`],
                      [has_qt4=no])
    PKG_CHECK_MODULES(QT4NETWORK, [QtCore >= 4.3 QtNetwork >= 4.3],
                      [has_qtnetwork=yes],
                      [has_qtnetwork=no])
    PKG_CHECK_MODULES(QT4WEBKIT,
                      [QtGui >= 4.3 QtCore >= 4.3 QtNetwork >= 4.3 QtWebKit],
                      [has_qtwebkit=yes],
                      [has_qtwebkit=no])
    PKG_CHECK_MODULES(QT4SCRIPT, [QtCore >= 4.4 QtScript >= 4.4],
                      [has_qtscript=yes],
                      [has_qtscript=no])
  elif test x$ggl_host_type = xmacosx; then
    # There is no pkgconfig for Qt on MacOSX, just do header check.
    AC_CHECK_HEADERS([QtCore/QtCore QtGui/QtGui QtNetwork/QtNetwork],
                     [has_qt4=yes], [has_qt4=no])
    AC_CHECK_HEADERS([QtNetwork/QtNetwork],
                     [has_qtnetwork=yes], [has_qtnetwork=no])
    AC_CHECK_HEADERS([QtWebKit/QtWebKit],
                     [has_qtwebkit=yes], [has_qtwebkit=no])
    # FIXME: Version should be checked. Only 4.4 and above are supported
    AC_CHECK_HEADERS([QtScript/QtScript],
                     [has_qtscript=yes], [has_qtscript=no])
    if test x$has_qt4 = xyes; then
      QT4_LIBS="-framework QtGui \
                -framework Carbon -framework AppKit -framework QtCore \
                -framework QtNetwork \
                -framework ApplicationServices -lz -lm"
      AC_PATH_PROG([MOC], [moc])
    fi
    if test x$has_qtnetwork = xyes; then
      QT4NETWORK_LIBS="-framework QtCore -framework QtNetwork -lz -lm"
    fi
    if test x$has_qtwebkit = xyes; then
      QT4WEBKIT_LIBS="-framework QtGui -framework Carbon -framework AppKit \
                      -framework QtCore -framework QtNetwork \
                      -framework QtWebKit -lz -lm"
    fi
    if test x$has_qtscript = xyes; then
      QT4SCRIPT_LIBS="-framework QtCore -framework QtScript -lz -lm"
    fi
  else
    AC_MSG_WARN([Qt support is not available for $ggl_host_type])
  fi

  # MOC is mandatory to build qt related code.
  AC_SUBST(MOC)
  if test "x$MOC" = "x"; then
    AC_MSG_WARN([Can not find moc command, which is mandatory for Qt support.])
    has_qt4=no
    has_qtnetwork=no
    has_qtwebkit=no
    has_qtscript=no
  fi

  if test x$has_qt4 != xyes; then
    AC_MSG_WARN([Qt >= 4.3 is required by libggadget-qt.])
    build_libggadget_qt=no
    build_qt_host=no
    build_qt_edit_element=no
    build_qt_script_runtime=no
    build_qt_system_framework=no
    build_qt_xml_http_request=no
    build_qtwebkit_browser_element=no
  fi

  if test x$has_qtnetwork != xyes -a x$build_qt_xml_http_request = xyes; then
    AC_MSG_WARN([QtNetwork >= 4.3 is required by qt-xml-http-request.])
    build_qt_xml_http_request=no
  fi

  if test x$has_qtwebkit != xyes -a x$build_qtwebkit_browser_element = xyes; then
    AC_MSG_WARN([QtWebkit >= 4.4 is required by qtwebkit-browser-element.])
    build_qtwebkit_browser_element=no
  fi

  if test x$has_qtscript != xyes -a x$build_qt_script_runtime = xyes; then
    AC_MSG_WARN([QtScript >= 4.4 is required by qt-script-runtime.])
    build_qt_script_runtime=no
  fi
fi

# Check gtk2 related libraries.
has_gtk2=no
if test x$build_libggadget_gtk = xyes; then
  PKG_CHECK_MODULES(GTK2,
                    [cairo >= 1.0.0
                     pango  >= 1.10.0
                     glib-2.0 >= 2.10.0
                     gtk+-2.0 >= 2.8.0
                     gdk-2.0 >= 2.8.0
                     gthread-2.0 >= 2.8.0
                     fontconfig >= 2.0.0],
                     [has_gtk2=yes], [has_gtk2=no])

  if test x$has_gtk2 != xyes; then
    AC_MSG_WARN([Gtk+ >= 2.8.0 is required by libggadget-gtk.])
    build_libggadget_gtk=no
    build_gtk_host=no
    build_gtk_edit_element=no
    build_gtk_flash_element=no
    build_gtk_system_framework=no
    build_gtkmoz_browser_element=no
  else
    gtk2_target=`$PKG_CONFIG --variable=target gtk+-2.0`
  fi
fi

# Check gstreamer-0.10 related libraries.
has_gstreamer=no
has_gstreamer_video=no
if test x$build_gst_audio_framework = xyes -o \
        x$build_gst_video_element = xyes; then
  PKG_CHECK_MODULES(GSTREAMER,
                    [gstreamer-0.10 >= 0.10.0
                     gstreamer-base-0.10 >= 0.10.0
                     gstreamer-plugins-base-0.10 >= 0.10.0],
                    [has_gstreamer=yes], [has_gstreamer=no])
  if test x$has_gstreamer = xyes; then
    CPPFLAGS="$CPPFLAGS $GSTREAMER_CFLAGS"
    AC_CHECK_HEADERS([gst/video/gstvideosink.h],
                     [has_gstreamer_video=yes], [has_gstreamer_video=no])
    if test x$has_gstreamer_video = xyes; then
      GSTVIDEOSINK_LIBRARIES="-lgstvideo-0.10"
      AC_SUBST(GSTVIDEOSINK_LIBRARIES)
    fi
    CPPFLAGS=$ggl_save_CPPFLAGS
  fi

  if test x$has_gstreamer != xyes; then
    AC_MSG_WARN([gstreamer >= 0.10.0 is required by gst-audio-framework and gst-video-element.])
    build_gst_audio_framework=no
    build_gst_video_element=no
  else
    if test x$has_gstreamer_video != xyes -a \
            x$build_gst_video_element = xyes; then
      AC_MSG_WARN([gstvideo >= 0.10.0 is required by gst-video-element.])
      build_gst_video_element=no
    fi
  fi
fi

# Check libcurl.
has_libcurl=no
if test x$build_curl_xml_http_request = xyes; then
  LIBCURL_CHECK_CONFIG([yes], [7.15.0], [has_libcurl=yes], [has_libcurl=no])

  if test x$libcurl_feature_SSL != xyes -o \
          x$libcurl_protocol_HTTPS != xyes; then
    AC_MSG_WARN([Libcurl without support SSL or HTTPS may not usable.])
  fi

  if test x$has_libcurl != xyes; then
    AC_MSG_WARN([libcurl >= 7.15.0 is required by curl-xml-http-request.])
    build_curl_xml_http_request=no
  fi
fi

# Check libsoup.
has_libsoup=no
has_libsoup_gnome=no
if test x$build_soup_xml_http_request = xyes; then
  PKG_CHECK_MODULES(LIBSOUP, [libsoup-2.4 >= 2.26.0],
                    [has_libsoup=yes], [has_libsoup=no])

  if test x$has_libsoup = xyes; then
    PKG_CHECK_MODULES(LIBSOUP_GNOME, [libsoup-gnome-2.4 >= 2.26.0],
                      [has_libsoup_gnome=yes], [has_libsoup_gnome=no])
    if test x$has_libsoup_gnome = xyes; then
      LIBSOUP_CFLAGS="$LIBSOUP_CFLAGS $LIBSOUP_GNOME_CFLAGS -DHAVE_LIBSOUP_GNOME"
      LIBSOUP_LIBS="$LIBSOUP_LIBS $LIBSOUP_GNOME_LIBS"
    fi
    LDFLAGS="$LIBSOUP_LIBS"
    AC_CHECK_FUNC(soup_message_headers_get_one,
                  [has_soup_message_headers_get_one=yes])
    if test x$has_soup_message_headers_get_one = xyes; then
      LIBSOUP_CFLAGS="$LIBSOUP_CFLAGS -DHAVE_SOUP_MESSAGE_HEADERS_GET_ONE"
    fi
    LDFLAGS=$ggl_save_LDFLAGS
  else
    AC_MSG_WARN([libsoup-2.4 >= 2.26.0 is required by soup-xml-http-request.])
    build_soup_xml_http_request=no
  fi
fi

# Check libxml2.
has_libxml2=no
if test x$build_libxml2_xml_parser = xyes; then
  AM_PATH_XML2([2.4.0], [has_libxml2=yes], [has_libxml2=no])

  if test x$has_libxml2 != xyes; then
    AC_MSG_WARN([libxml2 >= 2.4.0 is required by libxml2-xml-parser.])
    build_libxml2_xml_parser=no
  fi
fi

# Check D-Bus library
has_dbus=no
if test x$build_libggadget_dbus = xyes; then
  PKG_CHECK_MODULES(DBUS, [dbus-1 >= 1.0.2], [has_dbus=yes],
                    [PKG_CHECK_MODULES(DBUS, [dbus-1 >= 0.60],
                                       [has_dbus=old], [has_dbus=no])])

  if test x$has_dbus != xno; then
    PREDEFINED_MACROS="$PREDEFINED_MACROS -DHAVE_DBUS_LIBRARY"
    if test x$has_dbus = xold; then
      PREDEFINED_MACROS="$PREDEFINED_MACROS -DDBUS_API_SUBJECT_TO_CHANGE=1"
    fi
    LDFLAGS="$DBUS_LIBS"
    AC_CHECK_FUNC(dbus_watch_get_unix_fd, [has_dbus_watch_get_unix_fd=yes])
    if test x$has_dbus_watch_get_unix_fd = xyes; then
      PREDEFINED_MACROS="$PREDEFINED_MACROS -DHAVE_DBUS_WATCH_GET_UNIX_FD"
    fi
    LDFLAGS=$ggl_save_LDFLAGS
  else
    AC_MSG_WARN([DBus >= 0.60 is required by libggadget-dbus.])
    build_libggadget_dbus=no
    build_dbus_script_class=no
  fi
fi

# Check librsvg
has_librsvg=no
if test x$build_libggadget_gtk = xyes; then
  PKG_CHECK_MODULES(RSVG, [librsvg-2.0 >= 2.14.0],
                    [has_librsvg=yes], [has_librsvg=no])
  if test x$has_librsvg = xyes; then
    PREDEFINED_MACROS="$PREDEFINED_MACROS -DHAVE_RSVG_LIBRARY"
  fi
fi

# Check startup-notification library
has_startup_notification=no
PKG_CHECK_MODULES(STARTUP_NOTIFICATION, [libstartup-notification-1.0 >= 0.8],
                  [has_startup_notification=yes], [has_startup_notification=no])
if test x$has_startup_notification = xyes; then
  PREDEFINED_MACROS="$PREDEFINED_MACROS -DHAVE_STARTUP_NOTIFICATION"
fi

# Check NetworkManager
has_network_manager=no
PKG_CHECK_MODULES(NETWORK_MANAGER,
                  [NetworkManager >= 0.7], [has_network_manager=yes],
                  [PKG_CHECK_MODULES(NETWORK_MANAGER,
                   [NetworkManager >= 0.6], [has_network_manager=old],
                   [has_network_manager=no])])

if test x$has_network_manager != xno; then
  PREDEFINED_MACROS="$PREDEFINED_MACROS -DHAVE_NETWORK_MANAGER"
  if test x$has_network_manager != xold; then
    PREDEFINED_MACROS="$PREDEFINED_MACROS -DHAVE_NEW_NETWORK_MANAGER_API"
  fi
else
  AC_MSG_WARN([Development package of NetworkManager is not installed,]
              [network/wireless support of framework.system won't be built.])
fi

# Check gtkmozembed
# Check priority:
#  xulrunner 1.9: libxul-embedding-unstable and mozilla-js
#  xulrunner 1.8: xulrunner-gtkmozembed and xulrunner-js
#  firefox 2.0: firefox2-gtkmozembed and firefox2-js
#  firefox 1.5: firefox-gtkmozembed and firefox-js
#  seamonkey 1.1: seamonkey-gtkmozembed seamonkey-js
#  microb(Maemo): microb-engine and microb-engine-js
#
# Add more rules here if your distribution doesn't support above rules.
has_gtkmozembed=no
has_libmozjs=no
if test x$build_smjs_script_runtime = xyes -o \
        x$build_gtkmoz_browser_element = xyes; then
  gtkmozembed_list=""
  if test x$with_gtkmozembed = xauto; then
    gtkmozembed_list="libxul-embedding-unstable \
                      libxul-embedding \
                      xulrunner-gtkmozembed \
                      firefox2-gtkmozembed \
                      firefox-gtkmozembed \
                      seamonkey-gtkmozembed \
                      microb-engine"
  else
    case $with_gtkmozembed in
      libxul)
        gtkmozembed_list="libxul-embedding-unstable libxul-embedding"
        ;;
      xulrunner)
        gtkmozembed_list="xulrunner-gtkmozembed"
        ;;
      firefox2)
        gtkmozembed_list="firefox2-gtkmozembed"
        ;;
      firefox)
        gtkmozembed_list="firefox-gtkmozembed"
        ;;
      seamonkey)
        gtkmozembed_list="seamonkey-gtkmozembed"
        ;;
      microb)
        gtkmozembed_list="microb-engine"
        ;;
      *)
        AC_MSG_ERROR([--with-gtkmozembed, invalid value])
        ;;
    esac
  fi

  for gtkmozembed in $gtkmozembed_list; do
    PKG_CHECK_MODULES(GTKMOZEMBED, [$gtkmozembed],
                      [has_gtkmozembed=$gtkmozembed], [has_gtkmozembed=no])
    if test x$has_gtkmozembed != xno; then
      break
    fi
  done

  # Check spidermonkey
  # If gtkmozembed is available, then use bundled libmozjs.so.
  # If libmozjs is not available, then set has_gtkmozembed to false as well,
  # because gtkmoz-browser-child needs libmozjs.
  libmozjs_list=""
  case $has_gtkmozembed in
    libxul-embedding-unstable | libxul-embedding)
      libmozjs_list="mozilla-js"
      ;;
    xulrunner-gtkmozembed)
      libmozjs_list="xulrunner-js"
      ;;
    firefox2-gtkmozembed)
      libmozjs_list="firefox2-js"
      ;;
    firefox-gtkmozembed)
      libmozjs_list="firefox-js"
      ;;
    seamonkey-gtkmozembed)
      libmozjs_list="seamonkey-js"
      ;;
    microb-engine)
      libmozjs_list="microb-engine-js"
      ;;
    *)
      libmozjs_list="mozilla-js xulrunner-js firefox2-js \
                    firefox-js seamonkey-js microb-engine-js"
      ;;
  esac

  for libmozjs in $libmozjs_list; do
    PKG_CHECK_MODULES(LIBMOZJS, [$libmozjs],
                      [has_libmozjs=$libmozjs], [has_libmozjs=no])
    if test x$has_libmozjs != xno; then
      break
    fi
  done

  if test x$has_gtkmozembed != xno; then
    # Add additional include dirs for mozilla 1.8 and pre 1.9
    EMBED_INCDIR=`$PKG_CONFIG --variable=includedir $has_gtkmozembed`
    GTKMOZEMBED_CFLAGS="$GTKMOZEMBED_CFLAGS -I$EMBED_INCDIR"
    GTKMOZEMBED_CFLAGS="$GTKMOZEMBED_CFLAGS -I$EMBED_INCDIR/content"
    GTKMOZEMBED_CFLAGS="$GTKMOZEMBED_CFLAGS -I$EMBED_INCDIR/dom"
    GTKMOZEMBED_CFLAGS="$GTKMOZEMBED_CFLAGS -I$EMBED_INCDIR/xpconnect"
    GTKMOZEMBED_CFLAGS="$GTKMOZEMBED_CFLAGS -I$EMBED_INCDIR/widget"
    GTKMOZEMBED_CFLAGS="$GTKMOZEMBED_CFLAGS -I$EMBED_INCDIR/gtkembedmoz"
    GTKMOZEMBED_CFLAGS="$GTKMOZEMBED_CFLAGS -I$EMBED_INCDIR/xpcom"
    GTKMOZEMBED_CFLAGS="$GTKMOZEMBED_CFLAGS -I$EMBED_INCDIR/string"
    GTKMOZEMBED_LDFLAGS=""
    MOZILLA_FIVE_HOME=`$PKG_CONFIG --variable=libdir $has_gtkmozembed`
    if test "x$MOZILLA_FIVE_HOME" = "x"; then
    # Some distributions doesn't have libdir, so try to extract libdir from LIBS.
    # It usually occurs on some pre xulrunner 1.9 versions.
    # @<:@ and @:>@ will be replace by [ and ] by M4. Really evil.
    # FIXME: Following method might not work on systems other than Linux.
      MOZILLA_FIVE_HOME=`echo $GTKMOZEMBED_LIBS | $SED -e 's/.*-L\(@<:@^ @:>@*\) .*/\1/'`
      # Try to find out the correct xulrunner path.
      if test -e "$MOZILLA_FIVE_HOME/libxpcom.so"; then
        XPCOM_PATH=`readlink -f $MOZILLA_FIVE_HOME/libxpcom.so`
        MOZILLA_FIVE_HOME=`dirname $XPCOM_PATH`
      fi
    fi
    if test "x$MOZILLA_FIVE_HOME" != "x" -a -d "$MOZILLA_FIVE_HOME"; then
      GTKMOZEMBED_CFLAGS="$GTKMOZEMBED_CFLAGS -DMOZILLA_FIVE_HOME=\\\"$MOZILLA_FIVE_HOME\\\""
      GTKMOZEMBED_LDFLAGS="$GTKMOZEMBED_LDFLAGS -R$MOZILLA_FIVE_HOME"
    fi

    AC_SUBST(GTKMOZEMBED_LDFLAGS)
  fi

  if test x$has_libmozjs != xno; then
    # Add additional include dirs
    LIBMOZJS_INCDIR=`$PKG_CONFIG --variable=includedir $has_libmozjs`
    if (echo $LIBMOZJS_CFLAGS | grep '/stable') > /dev/null 2>&1; then
      if test -f $LIBMOZJS_INCDIR/unstable/jsapi.h; then
        LIBMOZJS_CFLAGS="$LIBMOZJS_CFLAGS -I$LIBMOZJS_INCDIR/unstable"
      fi
    fi

    # Find out the real path of libmozjs.so
    LIBMOZJS_LIBDIR=`$PKG_CONFIG --variable=libdir $has_libmozjs`
    if test "x$LIBMOZJS_LIBDIR" = "x"; then
    # Some distributions doesn't have libdir, so try to extract libdir from LIBS.
    # It usually occurs on some pre xulrunner 1.9 versions.
    # @<:@ and @:>@ will be replace by [ and ] by M4. Really evil.
    # FIXME: Following method might not work on systems other than Linux.
      LIBMOZJS_LIBDIR=`echo $LIBMOZJS_LIBS | $SED -e 's/.*-L\(@<:@^ @:>@*\) .*/\1/'`
      # Try to find out the correct xulrunner path.
      if test -e "$LIBMOZJS_LIBDIR/libmozjs.so"; then
        LIBMOZJS_PATH=`readlink -f $LIBMOZJS_LIBDIR/libmozjs.so`
        LIBMOZJS_LIBDIR=`dirname $LIBMOZJS_PATH`
      fi
    fi

    # GGL_CHECK_LIBMOZJS requires LIBMOZJS_LIBDIR
    GGL_CHECK_LIBMOZJS([170], [], [has_libmozjs=no])
    if test x$has_libmozjs != xno; then
      # If gtkmozembed is available and uses xpcomglue, then use xpcomglue for
      # libmozjs as well.
      if (echo $GTKMOZEMBED_CFLAGS | grep XPCOM_GLUE) > /dev/null 2>&1; then
        AC_MSG_NOTICE([Uses xulrunner's xpcomglue to load libmozjs.])
        LIBMOZJS_CFLAGS="$LIBMOZJS_CFLAGS $GTKMOZEMBED_CFLAGS"
        # Use libs of gtkmozembed instead of libmozjs's own library here to
        # avoid direct linking to libmozjs. As xpcomglue is used, the only
        # library shall be a static xpcomglue lib.
        # No more library dependency will be introduced.
        LIBMOZJS_LIBS="$GTKMOZEMBED_LIBS"
      else
        # If there is no xpcomglue support, then gtkmoz_browser_element needs
        # link to libmozjs explicitly.
        GTKMOZEMBED_LIBS="$GTKMOZEMBED_LIBS $LIBMOZJS_LIBS"
      fi
      if test x$with_mozjs_libname != x; then
        LIBMOZJS_CFLAGS="$LIBMOZJS_CFLAGS -DGGL_MOZJS_LIBNAME=\\\"$with_mozjs_libname\\\""
      fi
      if test "x$LIBMOZJS_LIBDIR" != "x" -a \
              "x$LIBMOZJS_LIBDIR" != "$libdir"; then
        LIBMOZJS_LDFLAGS="-R$LIBMOZJS_LIBDIR"
        AC_SUBST(LIBMOZJS_LDFLAGS)
      fi
    fi
  fi

  if test x$has_libmozjs = xno; then
    has_gtkmozembed=no
    if test x$build_smjs_script_runtime = xyes; then
      AC_MSG_WARN([libmozjs >= 1.7 is required by smjs-script-runtime.])
      build_smjs_script_runtime=no
    fi
  else
    AC_MSG_NOTICE([Uses libmozjs from $has_libmozjs])
  fi

  if test x$has_gtkmozembed = xno; then
    if test x$build_gtkmoz_browser_element = xyes; then
      AC_MSG_WARN([gtkmozembed is required by gtkmoz-browser-element.])
      build_gtkmoz_browser_element=no
    fi
  else
    AC_MSG_NOTICE([Uses gtkmozembed from $has_gtkmozembed])
  fi
fi

# Check WebKit/JavaScriptCore
has_javascriptcore=no
if test x$build_webkit_script_runtime = xyes; then
  PKG_CHECK_MODULES(JAVA_SCRIPT_CORE, [webkit-1.0 >= 1.0.3],
                    [has_javascriptcore=yes],
                    [has_javascriptcore=no])
  if test x$has_javascriptcore = xyes; then
    CPPFLAGS="$CPPFLAGS $JAVA_SCRIPT_CORE_CFLAGS"
    AC_CHECK_HEADERS([JavaScriptCore/JavaScript.h],
                     [has_JavaScript_h=yes],
                     [has_JavaScript_h=no])
    AC_CHECK_HEADERS([JavaScriptCore/JavaScriptCore.h],
                     [has_JavaScriptCore_h=yes],
                     [has_JavaScriptCore_h=no])
    CPPFLAGS="$ggl_save_CPPFLAGS"
    if test x$has_JavaScript_h = xyes; then
      PREDEFINED_MACROS="$PREDEFINED_MACROS -DHAVE_JAVA_SCRIPT_H"
    fi
    if test x$has_JavaScriptCore_h = xyes; then
      PREDEFINED_MACROS="$PREDEFINED_MACROS -DHAVE_JAVA_SCRIPT_CORE_H"
    fi
  fi

  if test x$has_javascriptcore != xyes -a x$ggl_host_type = xmacosx; then
    # Use system built-in JavaScriptCore.framework
    # No gtk webkit support on mac yet.
    AC_CHECK_HEADERS([JavaScriptCore/JavaScriptCore.h],
                     [has_javascriptcore=yes],
                     [has_javascriptcore=no])
    if test x$has_javascriptcore = xyes; then
      JAVA_SCRIPT_CORE_LIBS="-framework JavaScriptCore"
      PREDEFINED_MACROS="$PREDEFINED_MACROS -DHAVE_JAVA_SCRIPT_CORE_H"
    fi
  else
    AC_MSG_WARN([JavaScriptCore support is not available for $ggl_host_type])
  fi

  if test x$has_javascriptcore != xyes; then
    AC_MSG_WARN([WebKit/JavaScriptCore is required by webkit-script-runtime.])
    build_webkit_script_runtime=no;
  fi
fi

# Check gtk webkit for gtkwebkit-browser-element
has_gtkwebkit=no
if test x$build_gtkwebkit_browser_element = xyes; then
  PKG_CHECK_MODULES([GTKWEBKIT], [webkit-1.0 >= 1.0.3],
                    [has_gtkwebkit=yes], [has_gtkwebkit=no])
  if test x$has_gtkwebkit != xyes; then
    AC_MSG_WARN([GtkWebKit is required by gtkwebkit-browser-element])
    build_gtkwebkit_browser_element=no;
  elif test x$build_webkit_script_runtime = xyes; then
    PREDEFINED_MACROS="$PREDEFINED_MACROS -DGGL_GTK_WEBKIT_SUPPORT_JSC"
  fi
fi

# Check necessary functions
AC_CHECK_FUNC(mkdtemp, [PREDEFINED_MACROS="$PREDEFINED_MACROS -DHAVE_MKDTEMP"])
AC_CHECK_FUNC(mmap, [PREDEFINED_MACROS="$PREDEFINED_MACROS -DHAVE_MMAP"])

# Check flex
AC_PROG_LEX
if test "x$LEX" != "xflex"; then
  LEX="$SHELL $abs_top_srcdir/autotools/missing flex"
fi

# Check update-mime-database
AC_PATH_PROG(UPDATE_MIME_DATABASE, update-mime-database, no)
if test x$UPDATE_MIME_DATABASE = xno; then
  enable_update_mime_database=no
fi

# Check update-desktop-database
AC_PATH_PROG(UPDATE_DESKTOP_DATABASE, update-desktop-database, no)
if test x$UPDATE_DESKTOP_DATABASE = xno; then
  enable_update_desktop_database=no
fi

# Check xdg-icon-resource
AC_PATH_PROG(XDG_ICON_RESOURCE, xdg-icon-resource, no)
if test x$XDG_ICON_RESOURCE = xno; then
  enable_xdg_icon_resource=no
fi

# Check distribution information
GGL_CHECK_DIST_INFO
if test "x$GGL_DIST_INFO" != "x"; then
  PREDEFINED_MACROS="$PREDEFINED_MACROS \"-DGGL_DIST_INFO=\\\"$GGL_DIST_INFO\\\"\""
fi

###########################################################
##                  Process Config options.              ##
###########################################################

if test x$enable_shared != xyes; then
  PREDEFINED_MACROS="$PREDEFINED_MACROS -DGGL_DISABLE_SHARED"
fi

if test x$ZIP = x; then
  AC_MSG_WARN([Can't generate resource file bundle without zip command.])
fi

#Check memory check command
if test x$memcheck_command != x; then
  AC_CHECK_PROGS([MEMCHECK_COMMAND], [$memcheck_command])
  AC_SUBST(MEMCHECK_COMMAND)
fi

# Check mandatory dependencies
if test x$build_gtk_host = xyes; then
  has_gtk_script_engine=no
  if test x$build_smjs_script_runtime = xyes -o \
          x$build_webkit_script_runtime = xyes; then
    has_gtk_script_engine=yes
  fi
  has_gtk_xml_http_request=no
  if test x$build_curl_xml_http_request = xyes -o \
          x$build_soup_xml_http_request = xyes; then
    has_gtk_xml_http_request=yes
  fi
  if test x$has_gtk_xml_http_request != xyes -o \
          x$build_libxml2_xml_parser != xyes -o \
          x$has_gtk_script_engine != xyes; then
    AC_MSG_WARN([gtk host won't work because of missing dependencies])
    build_gtk_host=no
  fi
fi

if test x$build_qt_host = xyes; then
  has_qt_script_engine=no
  if test x$build_smjs_script_runtime = xyes -o \
          x$build_qt_script_runtime = xyes; then
    has_qt_script_engine=yes
  fi
  if test x$build_qt_xml_http_request != xyes -o \
          x$build_libxml2_xml_parser != xyes -o \
          x$has_qt_script_engine != xyes; then
    AC_MSG_WARN([qt host won't work because of missing dependencies.])
    build_qt_host=no
  fi
fi

#Gstreamer audiosink
if test "x$default_gst_audio_sink" != "x"; then
  PREDEFINED_MACROS="$PREDEFINED_MACROS \
                     -DGGL_DEFAULT_GST_AUDIO_SINK=\\\"$default_gst_audio_sink\\\""
fi

#Browser plugins directory
if test "x$default_browser_plugins_dir" != "x"; then
  PREDEFINED_MACROS="$PREDEFINED_MACROS \
                     -DGGL_DEFAULT_BROWSER_PLUGINS_DIR=\\\"$default_browser_plugins_dir\\\""
fi

#ssl ca file for soup-xml-http-request
if test "x$default_ssl_ca_file" != "x" -a "x$default_ssl_ca_file" != "xno"; then
  PREDEFINED_MACROS="$PREDEFINED_MACROS \
                     -DGGL_DEFAULT_SSL_CA_FILE=\\\"$default_ssl_ca_file\\\""
fi

#xdgmime
if test x$enable_xdgmime = xyes; then
  PREDEFINED_MACROS="$PREDEFINED_MACROS -DGGL_ENABLE_XDGMIME"
fi

GGL_GTK_HTML_SCRIPT_ENGINE=xulrunner
if test x$build_smjs_script_runtime = xyes -a \
        x$build_gtkmoz_browser_element = xyes; then
  GGL_GTK_HTML_SCRIPT_ENGINE=xulrunner
elif test x$build_webkit_script_runtime = xyes -a \
          x$build_gtkwebkit_browser_element = xyes; then
  GGL_GTK_HTML_SCRIPT_ENGINE=webkit
fi

PREDEFINED_MACROS="$PREDEFINED_MACROS -DGGL_GTK_HTML_SCRIPT_ENGINE=\\\"$GGL_GTK_HTML_SCRIPT_ENGINE\\\""

GGL_GTK_XML_HTTP_REQUEST=soup
if test x$build_soup_xml_http_request = xyes; then
  GGL_GTK_XML_HTTP_REQUEST=soup
elif test x$build_curl_xml_http_request = xyes; then
  GGL_GTK_XML_HTTP_REQUEST=curl
fi

PREDEFINED_MACROS="$PREDEFINED_MACROS -DGGL_GTK_XML_HTTP_REQUEST=\\\"$GGL_GTK_XML_HTTP_REQUEST\\\""

GGL_QT_SCRIPT_ENGINE=smjs
if test x$build_smjs_script_runtime = xyes; then
  GGL_QT_SCRIPT_ENGINE=smjs
elif test x$build_qt_script_runtime = xyes; then
  GGL_QT_SCRIPT_ENGINE=qt
fi

PREDEFINED_MACROS="$PREDEFINED_MACROS -DGGL_QT_SCRIPT_ENGINE=\\\"$GGL_QT_SCRIPT_ENGINE\\\""

AM_CONDITIONAL(
  [GGL_BUILD_GTK_HOST],
  [test x$build_gtk_host = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_QT_HOST],
  [test x$build_qt_host = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_LIBGGADGET_DBUS],
  [test x$build_libggadget_dbus = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_LIBGGADGET_GTK],
  [test x$build_libggadget_gtk = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_LIBGGADGET_NPAPI],
  [test x$build_libggadget_npapi = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_LIBGGADGET_QT],
  [test x$build_libggadget_qt = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_CURL_XML_HTTP_REQUEST],
  [test x$build_curl_xml_http_request = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_SOUP_XML_HTTP_REQUEST],
  [test x$build_soup_xml_http_request = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_DBUS_SCRIPT_CLASS],
  [test x$build_dbus_script_class = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_GST_AUDIO_FRAMEWORK],
  [test x$build_gst_audio_framework = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_GST_VIDEO_ELEMENT],
  [test x$build_gst_video_element = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_GTK_EDIT_ELEMENT],
  [test x$build_gtk_edit_element = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_GTK_FLASH_ELEMENT],
  [test x$build_gtk_flash_element = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_GTK_SYSTEM_FRAMEWORK],
  [test x$build_gtk_system_framework = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_GTKMOZ_BROWSER_ELEMENT],
  [test x$build_gtkmoz_browser_element = xyes])

AM_CONDITIONAL(
  [GTK2_TARGET_X11],
  [test x$gtk2_target = xx11])

AM_CONDITIONAL(
  [GGL_BUILD_LIBXML2_XML_PARSER],
  [test x$build_libxml2_xml_parser = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_QT_EDIT_ELEMENT],
  [test x$build_qt_edit_element = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_QT_SCRIPT_RUNTIME],
  [test x$build_qt_script_runtime = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_QT_SYSTEM_FRAMEWORK],
  [test x$build_qt_system_framework = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_QT_XML_HTTP_REQUEST],
  [test x$build_qt_xml_http_request = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_QTWEBKIT_BROWSER_ELEMENT],
  [test x$build_qtwebkit_browser_element = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_SMJS_SCRIPT_RUNTIME],
  [test x$build_smjs_script_runtime = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_WEBKIT_SCRIPT_RUNTIME],
  [test x$build_webkit_script_runtime = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_GTKWEBKIT_BROWSER_ELEMENT],
  [test x$build_gtkwebkit_browser_element = xyes])

AM_CONDITIONAL(
  [GGL_ENABLE_UPDATE_MIME_DATABASE],
  [test x$enable_update_mime_database = xyes])

AM_CONDITIONAL(
  [GGL_ENABLE_UPDATE_DESKTOP_DATABASE],
  [test x$enable_update_desktop_database = xyes])

AM_CONDITIONAL(
  [GGL_ENABLE_XDG_ICON_RESOURCE],
  [test x$enable_xdg_icon_resource = xyes])

AM_CONDITIONAL(
  [GGL_ENABLE_XDGMIME],
  [test x$enable_xdgmime = xyes])

AM_CONDITIONAL([GGL_HAVE_XLIB], [test x$has_xlib = xyes])
AM_CONDITIONAL([GGL_HAVE_ZIP], [test x$ZIP != x])
AM_CONDITIONAL([GGL_HAVE_GTK2], [test x$has_gtk2 = xyes])
AM_CONDITIONAL([GGL_HAVE_RSVG], [test x$has_librsvg = xyes])
AM_CONDITIONAL([GGL_HAVE_NETWORK_MANAGER], [test x$has_network_manager != xno])
AM_CONDITIONAL([GGL_CROSS_COMPILING], [test x$cross_compiling = xyes])
AM_CONDITIONAL([GGL_DISABLE_SHARED], [test x$enable_shared != xyes])

# Clear the default CFLAGS and CXXFLAGS
CFLAGS=$ggl_save_CFLAGS
CPPFLAGS=$ggl_save_CPPFLAGS
CXXFLAGS=$ggl_save_CXXFLAGS
LDFLAGS=$ggl_save_LDFLAGS

###########################################################
##                      Output files.                    ##
###########################################################
AC_SUBST(ac_aux_dir)

# Workaround a bug in autoconf 2.59, which doesn't export these variables.
AC_SUBST(abs_srcdir)
AC_SUBST(abs_builddir)
AC_SUBST(abs_top_srcdir)
AC_SUBST(abs_top_builddir)

# Specifies configurable headers.
AC_CONFIG_HEADERS([config.h])

# Specifies configurable files.
AC_CONFIG_FILES([Makefile
                 extensions/Makefile
                 extensions/analytics_usage_collector/Makefile
                 extensions/curl_xml_http_request/Makefile
                 extensions/dbus_script_class/Makefile
                 extensions/default_framework/Makefile
                 extensions/default_options/Makefile
                 extensions/default_options/tests/Makefile
                 extensions/google_gadget_manager/Makefile
                 extensions/google_gadget_manager/tests/Makefile
                 extensions/gst_audio_framework/Makefile
                 extensions/gst_video_element/Makefile
                 extensions/gtk_edit_element/Makefile
                 extensions/gtk_system_framework/Makefile
                 extensions/gtkmoz_browser_element/Makefile
                 extensions/libxml2_xml_parser/Makefile
                 extensions/linux_system_framework/Makefile
                 extensions/linux_system_framework/tests/Makefile
                 extensions/gtk_flash_element/Makefile
                 extensions/qt_edit_element/Makefile
                 extensions/qt_script_runtime/Makefile
                 extensions/qt_script_runtime/tests/Makefile
                 extensions/qt_system_framework/Makefile
                 extensions/qt_xml_http_request/Makefile
                 extensions/qtwebkit_browser_element/Makefile
                 extensions/smjs_script_runtime/Makefile
                 extensions/smjs_script_runtime/tests/Makefile
                 extensions/webkit_script_runtime/Makefile
                 extensions/webkit_script_runtime/tests/Makefile
                 extensions/gtkwebkit_browser_element/Makefile
                 extensions/html_flash_element/Makefile
                 extensions/soup_xml_http_request/Makefile
                 extensions/soup_xml_http_request/tests/Makefile
                 gadgets/Makefile
                 gadgets/analog_clock/Makefile
                 gadgets/designer/Makefile
                 gadgets/designer/gadget/Makefile
                 gadgets/designer/blank_gadget/Makefile
                 gadgets/digital_alarm_clock/Makefile
                 gadgets/igoogle/Makefile
                 gadgets/photos/Makefile
                 gadgets/rss/Makefile
                 gadgets/gadget_browser/Makefile
                 ggadget/Makefile
                 ggadget/libggadget-1.0.pc
                 ggadget/sysdeps.h
                 ggadget/tests/Makefile
                 ggadget/tests/test_modules/Makefile
                 ggadget/dbus/Makefile
                 ggadget/dbus/tests/Makefile
                 ggadget/dbus/libggadget-dbus-1.0.pc
                 ggadget/npapi/Makefile
                 ggadget/npapi/tests/Makefile
                 ggadget/npapi/libggadget-npapi-1.0.pc
                 ggadget/gtk/Makefile
                 ggadget/gtk/tests/Makefile
                 ggadget/gtk/libggadget-gtk-1.0.pc
                 ggadget/js/Makefile
                 ggadget/js/tests/Makefile
                 ggadget/js/libggadget-js-1.0.pc
                 ggadget/qt/Makefile
                 ggadget/qt/tests/Makefile
                 ggadget/qt/libggadget-qt-1.0.pc
                 ggadget/xdg/Makefile
                 ggadget/xdg/tests/Makefile
                 ggadget/xdg/libggadget-xdg-1.0.pc
                 hosts/Makefile
                 hosts/gtk/Makefile
                 hosts/qt/Makefile
                 resources/Makefile
                 third_party/Makefile
                 third_party/codesighs/Makefile
                 third_party/editline/Makefile
                 third_party/unzip/Makefile
                 third_party/xdgmime/Makefile
                 unittest/Makefile
                 xdg/Makefile
                 ])
AC_OUTPUT

AC_MSG_RESULT([
Build options:
  Version                         $GGL_VERSION
  Install prefix                  $prefix
  Install included libltdl        $enable_ltdl_install
  Build shared libs               $enable_shared
  Build static libs               $enable_static
  Enable debug                    $enable_debug
  Host type                       $ggl_host_type
  Cross Compiling                 $cross_compiling
  OEM brand                       $ggl_oem_brand

 Libraries:
  Build libggadget-dbus           $build_libggadget_dbus
  Build libggadget-gtk            $build_libggadget_gtk
  Build libggadget-npapi          $build_libggadget_npapi
  Build libggadget-qt             $build_libggadget_qt

 Extensions:
  Build curl-xml-http-request     $build_curl_xml_http_request
  Build soup-xml-http-request     $build_soup_xml_http_request
  Build dbus-script-class         $build_dbus_script_class
  Build gst-audio-framework       $build_gst_audio_framework
  Build gst-video-element         $build_gst_video_element
  Build gtk-edit-element          $build_gtk_edit_element
  Build gtk-flash-element         $build_gtk_flash_element
  Build gtk-system-framework      $build_gtk_system_framework
  Build gtkmoz-browser-element    $build_gtkmoz_browser_element
  Build libxml2-xml-parser        $build_libxml2_xml_parser
  Build linux-system-framework    $build_linux_system_framework
  Build qt-edit-element           $build_qt_edit_element
  Build qt-script-runtime         $build_qt_script_runtime
  Build qt-system-framework       $build_qt_system_framework
  Build qt-xml-http-request       $build_qt_xml_http_request
  Build qtwebkit-browser          $build_qtwebkit_browser_element
  Build smjs-script-runtime       $build_smjs_script_runtime
  Build webkit-script-runtime     $build_webkit_script_runtime
  Build gtkwebkit-browser-element $build_gtkwebkit_browser_element

 Other features:
  GTK SVG Support                 $has_librsvg
  NetworkManager support          $has_network_manager
  Startup notification support    $has_startup_notification
  Built-in xdgmime support        $enable_xdgmime
  Browser plugins directory       $default_browser_plugins_dir
  Default gstreamer audio sink    $default_gst_audio_sink
  SSL CA certificate file         $default_ssl_ca_file
  Memory check command            $memcheck_command

 Hosts:
  Build gtk host                  $build_gtk_host
  Build qt host                   $build_qt_host
])

if test x$build_gtk_host != xyes -a x$build_qt_host != xyes; then
  AC_MSG_WARN([Neither gtk host nor qt host will be built !!!]
              [Nothing can be used to run Google Gadgets for Linux.])
fi

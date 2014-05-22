#
# Copyright 2007 Google Inc.
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
# GGL_CHECK_ZLIB([MINIMUM-VERSION],
#                [ACTION-IF-FOUND],
#                [ACTION-IF-NOT-FOUND])
# Check for zlib library, if zlib library with version greater than
# MINIMUM-VERSION is found, ACTION-IF-FOUND will be executed,
# and ZLIB_CPPFLAGS, ZLIB_LIBS, ZLIB_LDFLAGS
# will be set accordingly,
# otherwise, ACTION-IF-NOT-FOUND will be executed.


AC_DEFUN([GGL_CHECK_ZLIB], [
AC_ARG_WITH([zlib-libdir],
	    AS_HELP_STRING([--with-zlib-libdir=DIR],
		[specify where to find zlib library]),
	    [zlib_libdir=$withval],
	    [zlib_libdir=""])

AC_ARG_WITH([zlib-incdir],
	    AS_HELP_STRING([--with-zlib-incdir=DIR],
		[specify where to find zlib header files]),
	    [zlib_incdir=$withval],
	    [zlib_incdir=""])

min_zlib_version=ifelse([$1], , 1.2.3, [$1])

AC_MSG_CHECKING([for zlib version >= $min_zlib_version])

zlib_save_CPPFLAGS="$CPPFLAGS"
zlib_save_LIBS="$LIBS"
zlib_save_LDFLAGS="$LDFLAGS"

zlib_CPPFLAGS=""
zlib_LDFLAGS=""
zlib_LIBS="-lz"

if test "x$zlib_incdir" != "x" ; then
  zlib_CPPFLAGS="$zlib_CPPFLAGS -I$zlib_incdir"
elif test -f "$prefix/include/zlib.h" ; then
  zlib_CPPFLAGS="$zlib_CPPFLAGS -I$prefix/include"
elif test -f "/usr/include/zlib.h" ; then
  zlib_CPPFLAGS="$zlib_CPPFLAGS -I/usr/include"
elif test -f "/usr/local/include/zlib.h" ; then
  zlib_CPPFLAGS="$zlib_CPPFLAGS -I/usr/local/include"
elif test -f "/opt/local/include/zlib.h" ; then
  zlib_CPPFLAGS="$zlib_CPPFLAGS -I/opt/local/include"
fi

if test "x$zlib_libdir" != "x" ; then
  zlib_LDFLAGS="$zlib_LDFLAGS -L$zlib_libdir"
fi

CPPFLAGS="$zlib_CPPFLAGS $CPPFLAGS"
LDFLAGS="$zlib_LDFLAGS $LDFLAGS"
LIBS="$zlib_LIBS $LIBS"

min_zlib_vernum=`echo $min_zlib_version | sed 's/\.//g'`

AC_LINK_IFELSE([[
#include<zlib.h>

#if ZLIB_VERNUM < 0x${min_zlib_vernum}0
#error "zlib version is too low."
#endif

int main() {
  z_stream strm;
  deflate(&strm, 0);
  return 0;
}
]],
[ac_have_zlib=yes],
[ac_have_zlib=no])

if test "x$ac_have_zlib" = "xyes" ; then
  ZLIB_CPPFLAGS="$zlib_CPPFLAGS"
  ZLIB_LIBS="$zlib_LIBS"
  ZLIB_LDFLAGS="$zlib_LDFLAGS"
  AC_SUBST(ZLIB_CPPFLAGS)
  AC_SUBST(ZLIB_LIBS)
  AC_SUBST(ZLIB_LDFLAGS)
  AC_MSG_RESULT([yes (CPPFLAGS=$zlib_CPPFLAGS  LIBS=$zlib_LIBS  LDFLAGS=$zlib_LDFLAGS)])
  ifelse([$2], , :, [$2])
else
  AC_MSG_RESULT([no])
  ifelse([$3], , :, [$3])
fi

CPPFLAGS=$zlib_save_CPPFLAGS
LIBS=$zlib_save_LIBS
LDFLAGS=$zlib_save_LDFLAGS
])

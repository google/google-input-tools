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
# GGL_CHECK_DIST_INFO
# Check distribution information, including distribution id, version,
# architecture, etc. String macro GGL_DIST_INFO will be defined.

AC_DEFUN([GGL_CHECK_DIST_INFO], [
AC_PATH_PROG(LSB_RELEASE, lsb_release, no)

GGL_DIST_CPU=$host_cpu
GGL_DIST_OS=$ggl_host_type
GGL_DIST_DESC=

if test x$GGL_DIST_OS = xunknown -o x$GGL_DIST_OS = x; then
  GGL_DIST_OS=$host_os
fi

DIST_RELEASE_FILES="/etc/fedora-release \
                    /etc/redhat-release \
                    /etc/SuSE-release \
                    /etc/sles-release \
                    /etc/gentoo-release \
                    /etc/mandriva-release \
                    /etc/debian_version \
                    /etc/arch-release \
                    /etc/arklinux-release \
                    /etc/slaclware-version \
                    /etc/pld-release \
                    /etc/knoppix_version \
                    /etc/lfs-release \
                    /etc/linuxppc-release \
                    /etc/yellowdog-release \
                    /etc/immunix-release \
                    /etc/sun-release \
                    /etc/release"

# lsb_release and /etc/*-release can only be trusted
# when it's not cross compiling.
if test x$cross_compiling = xno; then
  if test x$LSB_RELEASE != xno; then
    GGL_DIST_DESC=`$LSB_RELEASE -d -s | $SED -e 's/"//g'`
  fi
  if test "x$GGL_DIST_DESC" = "x"; then
    for relfile in $DIST_RELEASE_FILES; do
      if test -f $relfile; then
        GGL_DIST_DESC=`cat $relfile | $SED -n -e "1p"`
        if test "x$GGL_DIST_DESC" != "x"; then
          break
        fi
      fi
    done
  fi
fi

if test "x$GGL_DIST_DESC" = "x"; then
  GGL_DIST_DESC=$host_vendor
fi

GGL_DIST_INFO="$GGL_DIST_OS-$GGL_DIST_CPU ($GGL_DIST_DESC)"

if test "x$GGL_OEM_BRAND" != "x"; then
  GGL_DIST_INFO="$GGL_DIST_INFO ($GGL_OEM_BRAND)"
else
  GGL_DIST_INFO="$GGL_DIST_INFO (-)"
fi

AC_SUBST(GGL_DIST_INFO)

]) # GGL_CHECK_DIST_INFO

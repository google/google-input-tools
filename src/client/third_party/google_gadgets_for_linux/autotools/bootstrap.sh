#!/bin/sh
set -e

PKG_NAME=google-gadgets-for-linux

LIBTOOLIZE_FLAGS="--force --copy --automake --ltdl"
ACLOCAL_FLAGS="-I autotools"
AUTOMAKE_FLAGS="--add-missing --copy"

if test ! -f configure.ac; then
  echo "ERROR: No configure.ac found."
  exit 1
fi

autoconf_min_vers=2.59
automake_min_vers=1.9.6
libtoolize_min_vers=1.5.22
aclocal_min_vers=1.9.6

LIBTOOL_SOURCE=\
"http://ftp.gnu.org/pub/gnu/libtool/libtool-${libtool_min_vers}.tar.gz"

AUTOCONF_SOURCE=\
"http://ftp.gnu.org/pub/gnu/autoconf/autoconf-${autoconf_min_vers}.tar.gz"

AUTOMAKE_SOURCE=\
"http://ftp.gnu.org/pub/gnu/automake/automake-${automake_min_vers}.tar.gz"

# Not all echo versions allow -n, so we check what is possible. This test is
# based on the one in autoconf.
ECHO_C=
ECHO_N=
case `echo -n x` in
-n*)
  case `echo 'x\c'` in
  *c*) ;;
  *)   ECHO_C='\c';;
  esac;;
*)
  ECHO_N='-n';;
esac

printerr() {
    echo "$@" >&2
}

# Usage:
#     compare_versions MIN_VERSION ACTUAL_VERSION
# returns true if ACTUAL_VERSION >= MIN_VERSION
compare_versions() {
  ch_min_version=$1
  ch_actual_version=$2
  ch_status=0
  IFS="${IFS=         }"; ch_save_IFS="$IFS"; IFS="."
  set $ch_actual_version
  for ch_min in $ch_min_version; do
    ch_cur=`echo $1 | sed 's/[^0-9].*$//'`; shift # remove letter suffixes
    if [ -z "$ch_min" ]; then break; fi
    if [ -z "$ch_cur" ]; then ch_status=1; break; fi
    if [ $ch_cur -gt $ch_min ]; then break; fi
    if [ $ch_cur -lt $ch_min ]; then ch_status=1; break; fi
  done
  IFS="$ch_save_IFS"
  return $ch_status
}

# Usage:
#     version_check PACKAGE VARIABLE CHECKPROGS MIN_VERSION SOURCE
# checks to see if the package is available
version_check() {
  vc_package=$1
  vc_variable=$2
  vc_checkprogs=$3
  vc_min_version=$4
  vc_source=$5
  vc_status=1

  vc_checkprog=`eval echo "\\$$vc_variable"`
  if [ -n "$vc_checkprog" ]; then
    echo "using $vc_checkprog for $vc_package"
    return 0
  fi

  if test "x$vc_package" = "xautomake" -a "x$vc_min_version" = "x1.4"; then
    vc_comparator="="
  else
    vc_comparator=">="
  fi
  echo "checking for $vc_package $vc_comparator $vc_min_version..."
  for vc_checkprog in $vc_checkprogs; do
    echo $ECHO_N "  testing $vc_checkprog... " $ECHO_C
    if $vc_checkprog --version < /dev/null > /dev/null 2>&1; then
      vc_actual_version=`$vc_checkprog --version | head -n 1 | \
                         sed 's/^.*[         ]\([0-9.]*[a-z]*\).*$/\1/'`
      if compare_versions $vc_min_version $vc_actual_version; then
        echo "found $vc_actual_version"
        # set variables
        eval "$vc_variable=$vc_checkprog; \
              ${vc_variable}_VERSION=$vc_actual_version"
        vc_status=0
        break
      else
        echo "too old (found version $vc_actual_version)"
      fi
    else
      echo "not found."
    fi
  done
  if [ "$vc_status" != 0 ]; then
    printerr "ERROR: You must have $vc_package $vc_comparator $vc_min_version"
    printerr "       installed to build $PKG_NAME."
    printerr "       Download the appropriate package for your distribution or"
    printerr "       get the source tarball at"
    printerr "       $vc_source"
    printerr
    exit $vc_status
  fi
  return $vc_status
}

#
# Check required comopnents.
#
version_check autoconf AUTOCONF \
  autoconf \
  $autoconf_min_vers \
  $AUTOCONF_SOURCE || DIE=1

version_check autoconf AUTOHEADER \
  autoheader \
  $autoconf_min_vers \
  $AUTOCONF_SOURCE || DIE=1

#
# Hunt for an appropriate version of automake and aclocal; we can't
# assume that 'automake' is necessarily the most recent installed version
#
# We check automake first to allow it to be a newer version than we know about.
#
version_check automake AUTOMAKE \
  "automake automake-1.10 automake-1.9 automake-1.8 automake-1.7" \
  $automake_min_vers \
  $AUTOMAKE_SOURCE || DIE=1

ACLOCAL=`echo $AUTOMAKE | sed s/automake/aclocal/`

version_check libtool LIBTOOLIZE \
  "libtoolize glibtoolize" \
  $libtoolize_min_vers || DIE=1

if test "X$DIE" != X; then
  exit 1
fi

acdir=`$ACLOCAL --print-ac-dir`
if test ! -f $acdir/pkg.m4; then
  printerr "ERROR: Could not find pkg-config macros. (Looked in $acdir/pkg.m4)"
  printerr "       If pkg.m4 is available another directory, please copy it"
  printerr "       into ./autotools/, so that it can be found."
  printerr "       Otherwise, please install pkg-config."
  exit 1
fi

if test ! -f $acdir/libxml.m4 -a ! -f $acdir/libxml2.m4 -a \
        ! -f autotools/libxml.m4 -a ! -f autotools/libxml2.m4; then
  printerr "ERROR: Could not find libxml2 macros. (Looked in $acdir/libxml.m4)"
  printerr "       If libxml.m4 is available in another directory, please copy it"
  printerr "       into ./autotools/, so that it can be found."
  printerr "       Otherwise, please install devel package of libxml2."
  exit 1
fi


echo "Generates autoconf/automake files."

do_cmd() {
    echo "Running \`$@'"
    $@
}

do_cmd $LIBTOOLIZE $LIBTOOLIZE_FLAGS
do_cmd $ACLOCAL $ACLOCAL_FLAGS
do_cmd $AUTOHEADER
do_cmd $AUTOMAKE $AUTOMAKE_FLAGS
do_cmd $AUTOCONF

echo "DONE."
echo
echo "Please copy mkinstalldirs script into ./libltdl/ directory."
echo "You can find it in automake's data directory. Usually, it's in:"
echo "/usr/share/automake-$AUTOMAKE_VERSION/"

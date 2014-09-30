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
# GGL_CHECK_LIBMOZJS([minversion], [ACTION-IF-OK], [ACTION-IF-NOT-OK])
# Check if libmozjs (spidermonkey) library is ok.
# LIBMOZJS_CFLAGS, LIBMOZJS_LIBS, LIBMOZJS_LIBDIR must be set properly before
# calling this method.

AC_DEFUN([GGL_CHECK_LIBMOZJS], [
ggl_check_libmozjs_save_CPPFLAGS="$CPPFLAGS"
ggl_check_libmozjs_save_LIBS="$LIBS"

ggl_check_libmozjs_min_version=ifelse([$1], , 170, [$1])

if test "x$LIBMOZJS_CFLAGS" = "x" -o "x$LIBMOZJS_LIBS" = "x"; then
  ifelse([$3], , :, [$3])
else
  CPPFLAGS=$LIBMOZJS_CFLAGS
  LIBS=$LIBMOZJS_LIBS

  AC_MSG_CHECKING([for libmozjs.so version >= $ggl_check_libmozjs_min_version])

  AC_CHECK_HEADER([jsversion.h], [has_jsversion_h=yes], [has_jsversion_h=no])
  if test x$has_jsversion_h = xyes; then
    LIBMOZJS_CFLAGS="$LIBMOZJS_CFLAGS -DHAVE_JSVERSION_H"
    CPPFLAGS=$LIBMOZJS_CFLAGS
  fi

  AC_LINK_IFELSE([[
    #include<jsapi.h>
    #ifdef HAVE_JSVERSION_H
    #include<jsversion.h>
    #else
    #include<jsconfig.h>
    #endif

    #if JS_VERSION < $ggl_check_libmozjs_min_version
    #error "libmozjs.so version is too low."
    #endif

    int main() {
      JSRuntime *runtime;
      runtime = JS_NewRuntime(1048576);
      JS_DestroyRuntime(runtime);
    #ifdef JS_THREADSAFE
      JS_BeginRequest(0);
    #endif
      JS_ShutDown();
      return 0;
    }
  ]],
  [ggl_check_libmozjs_ok=yes],
  [ggl_check_libmozjs_ok=no])

  if (echo $CPPFLAGS | grep -v 'MOZILLA_1_8_BRANCH') > /dev/null 2>&1; then
    CPPFLAGS="$CPPFLAGS -DMOZILLA_1_8_BRANCH"
    if test "x$LIBMOZJS_LIBDIR" != "x" -a "x$LIBMOZJS_LIBDIR" != "$LIBDIR"; then
      ggl_check_libmozjs_save_LD_LIBRARY_PATH="$LD_LIBRARY_PATH"
      export LD_LIBRARY_PATH="$LIBMOZJS_LIBDIR"
    fi

    AC_RUN_IFELSE([[
      // This file is used to test if MOZILLA_1_8_BRANCH should be defined to
      // use the SpiderMonkey library.
      // It will fail to execute (crash or return error code) if
      // MOZILLA_1_8_BRANCH macro is not defined but the library was compiled
      // with the flag, or vise versa.
      #include <jsapi.h>
      #ifdef HAVE_JSVERSION_H
      #include<jsversion.h>
      #else
      #include<jsconfig.h>
      #endif

      static JSBool f(JSContext *c, JSObject *o, uintN ac, jsval *av, jsval *r) {
        return JS_TRUE;
      }

      int main() {
        JSRuntime *rt;
        JSContext *cx;
        JSObject *obj;
        JSFunctionSpec funcs[] = {
          { "a", f, 5, JSFUN_HEAVYWEIGHT, 0 },
          { "b", f, 5, JSFUN_HEAVYWEIGHT, 0 },
          { NULL, NULL, 0, 0, 0 },
        };

        rt = JS_NewRuntime(1048576);
        cx = JS_NewContext(rt, 8192);
        obj = JS_NewObject(cx, 0, 0, 0);
        JS_SetGlobalObject(cx, obj);
        // If MOZILLA_1_8_BRANCH is not properly defined, this JS_DefineFunctions
        // may crash or define only the first function because of the different
        // sizes of the nargs and flags fields.
        JS_DefineFunctions(cx, obj, funcs);
        jsval v;
        if (!JS_GetProperty(cx, obj, "b", &v) || !JSVAL_IS_OBJECT(v) ||
            !JS_ObjectIsFunction(cx, JSVAL_TO_OBJECT(v)) ||
            JS_GetFunctionFlags(JS_ValueToFunction(cx, v)) != JSFUN_HEAVYWEIGHT)
          return 1;

        JS_DestroyContext(cx);
        JS_DestroyRuntime(rt);
        JS_ShutDown();
        return 0;
      }
    ]],
    [ggl_check_libmozjs_18_branch=yes],
    [ggl_check_libmozjs_18_branch=no],
    [
      AC_MSG_WARN([Can't determine the status of MOZILLA_1_8_BRANCH
                   macro in cross compile mode.])
      ggl_check_libmozjs_18_branch=no
    ])
    if test "x$LIBMOZJS_LIBDIR" != x -a "x$LIBMOZJS_LIBDIR" != "$libdir"; then
      export LD_LIBRARY_PATH="$ggl_check_libmozjs_save_LD_LIBRARY_PATH"
    fi
  fi

  if test "x$ggl_check_libmozjs_ok" = "xyes"; then
    if test "x$ggl_check_libmozjs_18_branch" = "xyes"; then
      LIBMOZJS_CFLAGS="$LIBMOZJS_CFLAGS -DMOZILLA_1_8_BRANCH"
    fi
    AC_CHECK_FUNC(JS_SetOperationCallback,
                  [LIBMOZJS_CFLAGS="$LIBMOZJS_CFLAGS -DHAVE_JS_SetOperationCallback"])
    AC_CHECK_FUNC(JS_TriggerAllOperationCallbacks,
                  [LIBMOZJS_CFLAGS="$LIBMOZJS_CFLAGS -DHAVE_JS_TriggerAllOperationCallbacks"])
    ifelse([$2], , :, [$2])
    AC_MSG_RESULT([yes (CFLAGS=$LIBMOZJS_CFLAGS  LIBS=$LIBMOZJS_LIBS)])
  else
    AC_MSG_RESULT([no])
    ifelse([$3], , :, [$3])
  fi
fi

CPPFLAGS="$ggl_check_libmozjs_save_CPPFLAGS"
LIBS="$ggl_check_libmozjs_save_LIBS"
]) # GGL_CHECK_LIBMOZJS

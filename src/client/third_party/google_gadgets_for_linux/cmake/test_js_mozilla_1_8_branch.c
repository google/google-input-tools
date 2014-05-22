// This file is used to test if MOZILLA_1_8_BRANCH should be defined to use
// the SpiderMonkey library.
// It will fail to execute (crash or return error code) if MOZILLA_1_8_BRANCH
// macro is not defined but the library was compiled with the flag.

#include <jsapi.h>

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

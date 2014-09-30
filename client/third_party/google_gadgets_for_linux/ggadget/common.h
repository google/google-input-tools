/*
  Copyright 2011 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef GGADGET_COMMON_H__
#define GGADGET_COMMON_H__
#include <cassert>
#include <cstdio>
#include <ggadget/build_config.h>
#include <ggadget/format_macros.h>

#if defined(OS_WIN)
#include <ggadget/win32/port.h>     // Includes windows-dependent header files.
#include <ggadget/win32/sysdeps.h>  // Includes ggadget constants for windows.
#elif defined(OS_POSIX)
#include <dirent.h>
#include <inttypes.h>
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>
#include <ggadget/sysdeps.h>
#endif

#ifdef _DEBUG
#if defined(OS_WIN)
#include <exception>  // typeinfo depends on this file.
#endif
#include <typeinfo>
#endif
/**
 * @defgroup SharedLibraries Shared libraries
 * Shared libraries that host applications and third party applications can
 * use.
 */

/**
 * @defgroup CoreLibrary libggadget - the core library
 * @ingroup SharedLibraries
 *
 * The core library includes all platform-independent interfaces, classes and
 * utilities to support Google Desktop Gadgets API.
 */

/**
 * @defgroup Utilities Utilities
 * @ingroup CoreLibrary
 * Utility classes and functions.
 */

/**
 * @defgroup Interfaces Interfaces
 * @ingroup CoreLibrary
 * Interface classes.
 */

namespace ggadget {

/**
 * @defgroup CommonUtilities Common utilities
 * @ingroup Utilities
 * @{
 */

#ifdef __GNUC__
/**
 * Tell the compiler to do printf format string checking if the
 * compiler supports it.
 */
#define PRINTF_ATTRIBUTE(arg1,arg2) \
  __attribute__((__format__ (__printf__, arg1, arg2)))

/**
 * Tell the compiler to do scanf format string checking if the
 * compiler supports it.
 */
#define SCANF_ATTRIBUTE(arg1,arg2) \
    __attribute__((__format__ (__scanf__, arg1, arg2)))

#else // __GNUC__

#ifndef PRINTF_ATTRIBUTE  // avoid name conflict
#define PRINTF_ATTRIBUTE(arg1, arg2)
#endif

#ifndef SCANF_ATTRIBUTE  // avoid name conflict
#define SCANF_ATTRIBUTE(arg1, arg2)
#endif

#endif // else __GNUC__

/** A macro to turn a symbol into a string. */
#ifndef AS_STRING  // avoid name conflict
#define AS_STRING(x)   AS_STRING_INTERNAL(x)
#define AS_STRING_INTERNAL(x)   #x
#endif

/**
 * A macro to disallow the evil copy constructor and @c operator= methods.
 * This should be used in the @c private: declarations for a class.
 */
#ifndef DISALLOW_EVIL_CONSTRUCTORS  // avoid name conflict
#define DISALLOW_EVIL_CONSTRUCTORS(TypeName)    \
  TypeName(const TypeName&);                    \
  void operator=(const TypeName&)
#endif

/**
 * A macro to disallow all the implicit constructors, namely the
 * default constructor, copy constructor and @c operator= methods.
 * This should be used in the private: declarations for a class
 * that wants to prevent anyone from instantiating it. This is
 * especially useful for classes containing only static methods.
 */
#ifndef DISALLOW_IMPLICIT_CONSTRUCTORS  // avoid name conflict
#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
  TypeName();                                    \
  DISALLOW_EVIL_CONSTRUCTORS(TypeName)
#endif

#undef ASSERT
#ifdef NDEBUG
#define ASSERT(x)
#else
/**
 * Assert an expression and abort if it is not true.
 * Normally it only works in debug versions.
 */
#define ASSERT(x) assert(x)
#endif

/**
 * The @c COMPILE_ASSERT macro can be used to verify that a compile time
 * expression is @c true.
 *
 * The second argument to the macro is the name of the variable. If
 * the expression is @c false, most compilers will issue a warning/error
 * containing the name of the variable.
 * Sample usage:
 * <code>COMPILE_ASSERT(sizeof(char) == 1, invalid_size_of_char)</code>
 *
 * Simply using <code>typedef char msg[bool(expr) ? 1 : -1]</code> doesn't
 * work for some compilers like gcc, because they support array types of
 * size determined at runtime (a C99 feature).
 */
#ifndef COMPILE_ASSERT  // avoid name conflict
template <bool> struct CompileAssertHelper { };
#define COMPILE_ASSERT(expr, msg) \
  typedef ::ggadget::CompileAssertHelper<bool(expr)> msg[bool(expr) ? 1 : -1]
#endif
/**
 * Use @c implicit_cast as a safe version of @c static_cast or @c const_cast
 * for upcasting in the type hierarchy.
 * It's used to cast a pointer to @c Foo to a pointer to @c SuperclassOfFoo
 * or casting a pointer to @c Foo to a const pointer to @c Foo).
 *
 * When you use @c implicit_cast, the compiler checks that the cast is safe.
 * Such explicit <code>implicit_cast</code>s are necessary in surprisingly
 * many situations where C++ demands an exact type match instead of an
 * argument type convertable to a target type.
 *
 * The From type can be inferred, so the preferred syntax for using
 * implicit_cast is the same as for static_cast etc.:
 * <code>implicit_cast<To>(expr)</code>
 *
 * @c implicit_cast would have been part of the C++ standard library,
 * but the proposal was submitted too late.  It will probably make
 * its way into the language in the future.
 */
template<typename To, typename From>
inline To implicit_cast(From const &f) {
  return f;
}

/**
 * Downcast a pointer.
 * When you upcast (that is, cast a pointer from type @c Foo to type
 * @c SuperclassOfFoo), it's fine to use @c implicit_cast, since upcasts
 * always succeed.  When you downcast (that is, cast a pointer from
 * type @c Foo to type @c SubclassOfFoo), @c static_cast isn't safe, because
 * how do you know the pointer is really of type @c SubclassOfFoo?  It
 * could be a bare @c Foo, or of type @c DifferentSubclassOfFoo.  Thus,
 * when you downcast, you should use this template.  In debug mode, we
 * use @c dynamic_cast to double-check the downcast is legal (we die
 * if it's not).  In normal mode, we do the efficient @c static_cast
 * instead.
 *
 * Use like this: <code>down_cast<To *>(foo)</code>.
 */
template<typename To, typename From>
inline To down_cast(From *f) {
// dynamic_cast doesn't work properly when ggl library is dlopen'ed
// So we comment following code out
#if 0
#ifdef _DEBUG   // RTTI: debug mode only!
  if (false) {
    implicit_cast<const From *, To>(0);
  }
  if (f != NULL && dynamic_cast<To>(f) == NULL) {
    fprintf(stderr, "down_cast from %s to %s failed: \n",
           typeid(*f).name(), typeid(To).name());
    ASSERT(false);
  }
#endif
#endif
  return static_cast<To>(f);
}

/**
 * Detects at compile time whether type @a Derived is derived or the same as
 * type @a Base.
 * This is an incomplete implementation omitting many special cases, but works
 * in almost all cases in this product.
 *
 * Usage: <code>if (IsDerived<Base, Derived>::value) ...</code> or use it in
 * COMPILE_ASSERT macro.
 */
template <typename Base, typename Derived>
struct IsDerived {
  static char Check(const Base *);
  static int Check(const void *);
  static const bool value =
      sizeof(Check(static_cast<const Derived *>(NULL))) == sizeof(char);
};
/**
 * Avoids warning on unused parameters of functions.
 */
#define GGL_UNUSED(x) (void)x;

#ifndef arraysize  // avoid name conflict
/**
 * This template function declaration is used in defining arraysize.
 * Note that the function doesn't need an implementation, as we only
 * use its type.
 */
template <typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];
template <typename T, size_t N>
char (&ArraySizeHelper(const T (&array)[N]))[N];

/**
 * The @c arraysize(arr) macro returns the # of elements in an array arr.
 * The expression is a compile-time constant, and therefore can be
 * used in defining new arrays, for example.  If you use @c arraysize on
 * a pointer by mistake, you will get a compile-time error.
 *
 * One caveat is that @c arraysize() doesn't accept any array of an
 * anonymous type or a type defined inside a function.  This is
 * due to a limitation in C++'s template system.  The limitation might
 * eventually be removed, but it hasn't happened yet.
 */
#define arraysize(array) (sizeof(::ggadget::ArraySizeHelper(array)))
#endif
/**
 * Used to indicate an invalid index when size_t is used for the type of the
 * index.
 */
const size_t kInvalidIndex = static_cast<size_t>(-1L);

/** @} */

} // namespace ggadget

#endif  // GGADGET_COMMON_H__

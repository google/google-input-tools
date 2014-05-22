/*
  Copyright 2014 Google Inc.

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

// ----
//
// This code is compiled directly on many platforms, including client
// platforms like Windows, Mac, and embedded systems.  Before making
// any changes here, make sure that you're not breaking any platforms.
//
// Define a small subset of tr1 type traits. The traits we define are:
//   enable_if
//   is_integral
//   is_floating_point
//   is_pointer
//   is_enum
//   is_reference
//   is_pod
//   has_trivial_constructor
//   has_trivial_copy
//   has_trivial_assign
//   has_trivial_destructor
//   remove_const
//   remove_volatile
//   remove_cv
//   remove_reference
//   add_reference
//   remove_pointer
//   is_same
//   is_convertible
// We can add more type traits as required.

#ifndef BASE_TYPE_TRAITS_H_
#define BASE_TYPE_TRAITS_H_

#include <utility>                  // For pair

#include "base/template_util.h"     // For true_type and false_type

/* MOE:begin_strip */
namespace base {
/* MOE:end_strip_and_replace
_START_GOOGLE_NAMESPACE_
*/

template <bool cond, class T> struct enable_if;
template <class T> struct is_integral;
template <class T> struct is_floating_point;
template <class T> struct is_pointer;
// MSVC can't compile this correctly, and neither can gcc 3.3.5 (at least)
#if !defined(_MSC_VER) && !(defined(__GNUC__) && __GNUC__ <= 3)
// is_enum uses is_convertible, which is not available on MSVC.
template <class T> struct is_enum;
#endif
template <class T> struct is_reference;
template <class T> struct is_pod;
template <class T> struct has_trivial_constructor;
template <class T> struct has_trivial_copy;
template <class T> struct has_trivial_assign;
template <class T> struct has_trivial_destructor;
template <class T> struct remove_const;
template <class T> struct remove_volatile;
template <class T> struct remove_cv;
template <class T> struct remove_reference;
template <class T> struct add_reference;
template <class T> struct remove_pointer;
template <class T, class U> struct is_same;
#if !(defined(_MSC_VER) && _MSC_VER < 1600) && \
    !(defined(__GNUC__) && __GNUC__ <= 3)
template <class From, class To> struct is_convertible;
#endif

// enable_if, equivalent semantics to c++11 std::enable_if, specifically:
//   "If B is true, the member typedef type shall equal T; otherwise, there
//    shall be no member typedef type."
// Specified by 20.9.7.6 [Other transformations]
template<bool cond, class T = void> struct enable_if { typedef T type; };
template<class T> struct enable_if<false, T> {};

// is_integral is false except for the built-in integer types. A
// cv-qualified type is integral if and only if the underlying type is.
template <class T> struct is_integral : false_type { };
template<> struct is_integral<bool> : true_type { };
template<> struct is_integral<char> : true_type { };
template<> struct is_integral<unsigned char> : true_type { };
template<> struct is_integral<signed char> : true_type { };
#if defined(_MSC_VER)
// wchar_t is not by default a distinct type from unsigned short in
// Microsoft C.
// See http://msdn2.microsoft.com/en-us/library/dh8che7s(VS.80).aspx
template<> struct is_integral<__wchar_t> : true_type { };
#else
template<> struct is_integral<wchar_t> : true_type { };
#endif
template<> struct is_integral<short> : true_type { };
template<> struct is_integral<unsigned short> : true_type { };
template<> struct is_integral<int> : true_type { };
template<> struct is_integral<unsigned int> : true_type { };
template<> struct is_integral<long> : true_type { };
template<> struct is_integral<unsigned long> : true_type { };
// MOE:insert #ifdef HAVE_LONG_LONG
template<> struct is_integral<long long> : true_type { };
template<> struct is_integral<unsigned long long> : true_type { };
// MOE:insert #endif
template <class T> struct is_integral<const T> : is_integral<T> { };
template <class T> struct is_integral<volatile T> : is_integral<T> { };
template <class T> struct is_integral<const volatile T> : is_integral<T> { };

// is_floating_point is false except for the built-in floating-point types.
// A cv-qualified type is integral if and only if the underlying type is.
template <class T> struct is_floating_point : false_type { };
template<> struct is_floating_point<float> : true_type { };
template<> struct is_floating_point<double> : true_type { };
template<> struct is_floating_point<long double> : true_type { };
template <class T> struct is_floating_point<const T>
    : is_floating_point<T> { };
template <class T> struct is_floating_point<volatile T>
    : is_floating_point<T> { };
template <class T> struct is_floating_point<const volatile T>
    : is_floating_point<T> { };

// is_pointer is false except for pointer types. A cv-qualified type (e.g.
// "int* const", as opposed to "int const*") is cv-qualified if and only if
// the underlying type is.
template <class T> struct is_pointer : false_type { };
template <class T> struct is_pointer<T*> : true_type { };
template <class T> struct is_pointer<const T> : is_pointer<T> { };
template <class T> struct is_pointer<volatile T> : is_pointer<T> { };
template <class T> struct is_pointer<const volatile T> : is_pointer<T> { };

#if !(defined(_MSC_VER) && _MSC_VER < 1600) && \
    !(defined(__GNUC__) && __GNUC__ <= 3)

namespace internal {

template <class T> struct is_class_or_union {
  template <class U> static small_ tester(void (U::*)());
  template <class U> static big_ tester(...);
  static const bool value = sizeof(tester<T>(0)) == sizeof(small_);
};

// is_convertible chokes if the first argument is an array. That's why
// we use add_reference here.
template <bool NotUnum, class T> struct is_enum_impl
    : is_convertible<typename add_reference<T>::type, int> { };

template <class T> struct is_enum_impl<true, T> : false_type { };

}  // namespace internal

// Specified by TR1 [4.5.1] primary type categories.

// Implementation note:
//
// Each type is either void, integral, floating point, array, pointer,
// reference, member object pointer, member function pointer, enum,
// union or class. Out of these, only integral, floating point, reference,
// class and enum types are potentially convertible to int. Therefore,
// if a type is not a reference, integral, floating point or class and
// is convertible to int, it's a enum. Adding cv-qualification to a type
// does not change whether it's an enum.
//
// Is-convertible-to-int check is done only if all other checks pass,
// because it can't be used with some types (e.g. void or classes with
// inaccessible conversion operators).
template <class T> struct is_enum
    : internal::is_enum_impl<
          is_same<T, void>::value ||
              is_integral<T>::value ||
              is_floating_point<T>::value ||
              is_reference<T>::value ||
              internal::is_class_or_union<T>::value,
          T> { };

template <class T> struct is_enum<const T> : is_enum<T> { };
template <class T> struct is_enum<volatile T> : is_enum<T> { };
template <class T> struct is_enum<const volatile T> : is_enum<T> { };

#endif

// is_reference is false except for reference types.
template<typename T> struct is_reference : false_type {};
template<typename T> struct is_reference<T&> : true_type {};

// We can't get is_pod right without compiler help, so fail conservatively.
// We will assume it's false except for arithmetic types, enumerations,
// pointers and cv-qualified versions thereof. Note that std::pair<T,U>
// is not a POD even if T and U are PODs.
template <class T> struct is_pod
 : integral_constant<bool, (is_integral<T>::value ||
                            is_floating_point<T>::value ||
#if !defined(_MSC_VER) && !(defined(__GNUC__) && __GNUC__ <= 3)
                            // is_enum is not available on MSVC.
                            is_enum<T>::value ||
#endif
                            is_pointer<T>::value)> { };
template <class T> struct is_pod<const T> : is_pod<T> { };
template <class T> struct is_pod<volatile T> : is_pod<T> { };
template <class T> struct is_pod<const volatile T> : is_pod<T> { };

// We can't get has_trivial_constructor right without compiler help, so
// fail conservatively. We will assume it's false except for: (1) types
// for which is_pod is true. (2) std::pair of types with trivial
// constructors. (3) array of a type with a trivial constructor.
// (4) const versions thereof.
template <class T> struct has_trivial_constructor : is_pod<T> { };
template <class T, class U> struct has_trivial_constructor<std::pair<T, U> >
  : integral_constant<bool,
                      (has_trivial_constructor<T>::value &&
                       has_trivial_constructor<U>::value)> { };
template <class A, int N> struct has_trivial_constructor<A[N]>
  : has_trivial_constructor<A> { };
template <class T> struct has_trivial_constructor<const T>
  : has_trivial_constructor<T> { };

// We can't get has_trivial_copy right without compiler help, so fail
// conservatively. We will assume it's false except for: (1) types
// for which is_pod is true. (2) std::pair of types with trivial copy
// constructors. (3) array of a type with a trivial copy constructor.
// (4) const versions thereof.
template <class T> struct has_trivial_copy : is_pod<T> { };
template <class T, class U> struct has_trivial_copy<std::pair<T, U> >
  : integral_constant<bool,
                      (has_trivial_copy<T>::value &&
                       has_trivial_copy<U>::value)> { };
template <class A, int N> struct has_trivial_copy<A[N]>
  : has_trivial_copy<A> { };
template <class T> struct has_trivial_copy<const T> : has_trivial_copy<T> { };

// We can't get has_trivial_assign right without compiler help, so fail
// conservatively. We will assume it's false except for: (1) types
// for which is_pod is true. (2) std::pair of types with trivial copy
// constructors. (3) array of a type with a trivial assign constructor.
template <class T> struct has_trivial_assign : is_pod<T> { };
template <class T, class U> struct has_trivial_assign<std::pair<T, U> >
  : integral_constant<bool,
                      (has_trivial_assign<T>::value &&
                       has_trivial_assign<U>::value)> { };
template <class A, int N> struct has_trivial_assign<A[N]>
  : has_trivial_assign<A> { };

// We can't get has_trivial_destructor right without compiler help, so
// fail conservatively. We will assume it's false except for: (1) types
// for which is_pod is true. (2) std::pair of types with trivial
// destructors. (3) array of a type with a trivial destructor.
// (4) const versions thereof.
template <class T> struct has_trivial_destructor : is_pod<T> { };
template <class T, class U> struct has_trivial_destructor<std::pair<T, U> >
  : integral_constant<bool,
                      (has_trivial_destructor<T>::value &&
                       has_trivial_destructor<U>::value)> { };
template <class A, int N> struct has_trivial_destructor<A[N]>
  : has_trivial_destructor<A> { };
template <class T> struct has_trivial_destructor<const T>
  : has_trivial_destructor<T> { };

// Specified by TR1 [4.7.1]
template<typename T> struct remove_const { typedef T type; };
template<typename T> struct remove_const<T const> { typedef T type; };
template<typename T> struct remove_volatile { typedef T type; };
template<typename T> struct remove_volatile<T volatile> { typedef T type; };
template<typename T> struct remove_cv {
  typedef typename remove_const<typename remove_volatile<T>::type>::type type;
};

// Specified by TR1 [4.7.2] Reference modifications.
template<typename T> struct remove_reference { typedef T type; };
template<typename T> struct remove_reference<T&> { typedef T type; };

template <typename T> struct add_reference { typedef T& type; };
template <typename T> struct add_reference<T&> { typedef T& type; };

// Specified by TR1 [4.7.4] Pointer modifications.
template<typename T> struct remove_pointer { typedef T type; };
template<typename T> struct remove_pointer<T*> { typedef T type; };
template<typename T> struct remove_pointer<T* const> { typedef T type; };
template<typename T> struct remove_pointer<T* volatile> { typedef T type; };
template<typename T> struct remove_pointer<T* const volatile> {
  typedef T type; };

// Specified by TR1 [4.6] Relationships between types
template<typename T, typename U> struct is_same : public false_type { };
template<typename T> struct is_same<T, T> : public true_type { };

// Specified by TR1 [4.6] Relationships between types
#if !defined(_MSC_VER) && !(defined(__GNUC__) && __GNUC__ <= 3)
namespace internal {

// This class is an implementation detail for is_convertible, and you
// don't need to know how it works to use is_convertible. For those
// who care: we declare two different functions, one whose argument is
// of type To and one with a variadic argument list. We give them
// return types of different size, so we can use sizeof to trick the
// compiler into telling us which function it would have chosen if we
// had called it with an argument of type From.  See Alexandrescu's
// _Modern C++ Design_ for more details on this sort of trick.

template <typename From, typename To>
struct ConvertHelper {
  static small_ Test(To);
  static big_ Test(...);
  static From Create();
};
}  // namespace internal

// Inherits from true_type if From is convertible to To, false_type otherwise.
template <typename From, typename To>
struct is_convertible
    : integral_constant<bool,
                        sizeof(internal::ConvertHelper<From, To>::Test(
                                  internal::ConvertHelper<From, To>::Create()))
                        == sizeof(small_)> {
};
#endif

/* MOE:begin_strip */
}  // namespace base
/* MOE:end_strip_and_replace
_END_GOOGLE_NAMESPACE_
*/

/* MOE:begin_strip */
// NOTE(csilvers): the following has trouble compiling on all
// platforms we want to support with opensource, and we don't need it,
// so I've just commented it out of the opensource release.

// One of the type traits, is_pod, makes it possible to query whether
// a type is a POD type. It is impossible for type_traits.h to get
// this right without compiler support, so it fails conservatively. It
// knows that fundamental types and pointers are PODs, but it can't
// tell whether user classes are PODs. The DECLARE_POD macro is used
// to inform the type traits library that a user class is a POD.
//
// Implementation note: the typedef at the end is just to make it legal
// to put a semicolon after DECLARE_POD(foo).
//
// The only reason this matters is that a few parts of the google3
// code base either require their template arguments to be PODs
// (e.g. compact_vector) or are able to use a more efficient code path
// when their template arguments are PODs (e.g. sparse_hash_map). You
// should use DECLARE_POD if you have written a class that you intend
// to use with one of those components, and if you know that your
// class satisfies all of the conditions to be a POD type.
//
// So what's a POD?  The C++ standard (clause 9 paragraph 4) gives a
// full definition, but a good rule of thumb is that a struct is a POD
// ("plain old data") if it doesn't use any of the features that make
// C++ different from C.  A POD struct can't have constructors,
// destructors, assignment operators, base classes, private or
// protected members, or virtual functions, and all of its member
// variables must themselves be PODs.

#define DECLARE_POD(TypeName)                       \
namespace base {                                    \
template<> struct is_pod<TypeName> : true_type { }; \
}                                                   \
typedef int Dummy_Type_For_DECLARE_POD              \

// We once needed a different technique to assert that a nested class
// is a POD. This is no longer necessary, and DECLARE_NESTED_POD is
// just a synonym for DECLARE_POD. We continue to provide
// DECLARE_NESTED_POD only so we don't have to change client
// code. Regardless of whether you use DECLARE_POD or
// DECLARE_NESTED_POD: use it after the outer class. Using it within a
// class definition will give a compiler error.
#define DECLARE_NESTED_POD(TypeName) DECLARE_POD(TypeName)

// Declare that TemplateName<T> is a POD whenever T is
#define PROPAGATE_POD_FROM_TEMPLATE_ARGUMENT(TemplateName)             \
namespace base {                                                       \
template <typename T> struct is_pod<TemplateName<T> > : is_pod<T> { }; \
}                                                                      \
typedef int Dummy_Type_For_PROPAGATE_POD_FROM_TEMPLATE_ARGUMENT

// Macro that does nothing if TypeName is a POD, and gives a compiler
// error if TypeName is a non-POD.  You should put a descriptive
// comment right next to the macro call so that people can tell what
// the compiler error is about.
//
// Implementation note: this works by taking the size of a type that's
// complete when TypeName is a POD and incomplete otherwise.

template <bool IsPod> struct ERROR_TYPE_MUST_BE_POD;
template <> struct ERROR_TYPE_MUST_BE_POD<true> { };
#define ENFORCE_POD(TypeName)                                             \
  enum { dummy_##TypeName                                                 \
           = sizeof(ERROR_TYPE_MUST_BE_POD<                               \
                      ::base::is_pod<TypeName>::value>) }

/* MOE:end_strip_and_replace
// Right now these macros are no-ops, and mostly just document the fact
// these types are PODs, for human use.  They may be made more contentful
// later.  The typedef is just to make it legal to put a semicolon after
// these macros.
#define DECLARE_POD(TypeName) typedef int Dummy_Type_For_DECLARE_POD
#define DECLARE_NESTED_POD(TypeName) DECLARE_POD(TypeName)
#define PROPAGATE_POD_FROM_TEMPLATE_ARGUMENT(TemplateName)             \
    typedef int Dummy_Type_For_PROPAGATE_POD_FROM_TEMPLATE_ARGUMENT
#define ENFORCE_POD(TypeName) typedef int Dummy_Type_For_ENFORCE_POD
*/

#endif  // BASE_TYPE_TRAITS_H_

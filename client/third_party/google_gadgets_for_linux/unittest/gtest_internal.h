/*
  Copyright 2008, Google Inc.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

      * Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the following disclaimer
  in the documentation and/or other materials provided with the
  distribution.
      * Neither the name of Google Inc. nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// gTest -- the Google C++ Unit Testing Framework
//
// This header file declares functions and macros used internally by
// gTest.  They are subject to change without notice.

#ifndef UNITTEST_GTEST_INTERNAL_H__
#define UNITTEST_GTEST_INTERNAL_H__

#define GTEST_SCOPED_LOCK(mutex_ptr)

#include <string.h>

#ifdef OS_LINUX
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif  // OS_LINUX

#ifndef GTEST_HAS_STD_STRING
// The user didn't tell us whether ::std::string is available, so we
// need to figure it out.

#ifdef _MSC_VER  // We are on Windows.
// Assumes that exceptions are enabled by default.
#ifndef _HAS_EXCEPTIONS
#define _HAS_EXCEPTIONS 1
#endif  // _HAS_EXCEPTIONS
// GTEST_HAS_EXCEPTIONS is non-zero iff exceptions are enabled.  It is
// always defined, while _HAS_EXCEPTIONS is defined only on Windows.
#define GTEST_HAS_EXCEPTIONS _HAS_EXCEPTIONS
// On Windows, we can use ::std::string only if exceptions are
// enabled.
#define GTEST_HAS_STD_STRING GTEST_HAS_EXCEPTIONS
#else  // We are on Linux or Mac OS.
#define GTEST_HAS_EXCEPTIONS 0
#define GTEST_HAS_STD_STRING 1
#endif  // _MSC_VER

#endif  // GTEST_HAS_STD_STRING

#if GTEST_HAS_STD_STRING
#include <string>
#endif

#ifndef GTEST_HAS_GLOBAL_STRING
// The user didn't tell us whether ::string is available, so we need
// to figure it out.

#ifdef HAS_GLOBAL_STRING
#define GTEST_HAS_GLOBAL_STRING 1
#else
#define GTEST_HAS_GLOBAL_STRING 0
#endif  // HAS_GLOBAL_STRING

#endif  // GTEST_HAS_GLOBAL_STRING

#include <iomanip>
#include <limits>

#ifdef _MSC_VER
#include <strstream>  // NOLINT
#else
#include <sstream>
#endif  // _MSC_VER

// std::strstream is deprecated.  However, we have to use it on
// Windows as std::stringstream won't compile on Windows when
// exceptions are disabled.  We use std::stringstream on other
// platforms to avoid compiler warnings there.
#ifdef _MSC_VER
typedef std::strstream StrStream;
#else
typedef std::stringstream StrStream;
#endif  // _MSC_VER

// The GNU compiler emits a warning if nested "if" statements are followed by
// an "else" statement and braces are not used to explicitly disambiguate the
// "else" binding.  This leads to problems with code like:
//
//   if (gate)
//     ASSERT_*(condition) << "Some message";
//
// The "switch (0) case 0:" idiom is used to suppress this.
#ifdef __INTEL_COMPILER
#define GTEST_AMBIGUOUS_ELSE_BLOCKER
#else
#define GTEST_AMBIGUOUS_ELSE_BLOCKER switch (0) case 0:  // NOLINT
#endif

// Due to C++ preprocessor weirdness, we need double indirection to
// concatenate two tokens when one of them is __LINE__.  Writing
//
//   foo ## __LINE__
//
// will result in the token foo__LINE__, instead of foo followed by
// the current line number.  For more details, see
// http://www.parashift.com/c++-faq-lite/misc-technical-issues.html#faq-39.6
#define GTEST_CONCAT_TOKEN(foo, bar) GTEST_CONCAT_TOKEN_IMPL(foo, bar)
#define GTEST_CONCAT_TOKEN_IMPL(foo, bar) foo ## bar

// gTest defines the testing::Message class to allow construction of
// test messages via the << operator.  The idea is that anything
// streamable to std::ostream can be streamed to a testing::Message.
// This allows a user to use his own types in gTest assertions by
// overloading the << operator.
//
// util/gtl/stl_logging-inl.h overloads << for STL containers.  These
// overloads cannot be defined in the std namespace, as that will be
// undefined behavior.  Therefore, they are defined in the global
// namespace instead.
//
// C++'s symbol lookup rule (i.e. Koenig lookup) says that these
// overloads are visible in either the std namespace or the global
// namespace, but not other namespaces, including the testing
// namespace which gTest's Message class is in.
//
// To allow STL containers (and other types that has a << operator
// defined in the global namespace) to be used in gTest assertions,
// testing::Message must access the custom << operator from the global
// namespace.  Hence this helper function.
//
// Note: Jeffrey Yasskin suggested an alternative fix by "using
// ::operator<<;" in the definition of Message's operator<<.  That fix
// doesn't require a helper function, but unfortunately doesn't
// compile with MSVC.
template <typename T>
inline void GTestStreamToHelper(std::ostream* os, const T& val) {
  *os << val;
}

// Use this annotation at the end of a struct / class definition to
// prevent the compiler from optimizing away instances that are never
// used.  This is useful when all interesting logic happens inside the
// c'tor and / or d'tor.  Example:
//
//   struct Foo {
//     Foo() { ... }
//   } GTEST_ATTRIBUTE_UNUSED;
#if defined(_MSC_VER) || (defined(OS_LINUX) && defined(SWIG))
#define GTEST_ATTRIBUTE_UNUSED
#else
#define GTEST_ATTRIBUTE_UNUSED __attribute__ ((unused))
#endif

// Tell the compiler to warn about unused return values for functions declared
// with this macro.  The macro should be used on function declarations
// following the argument list:
//
//   Sprocket* AllocateSprocket() GTEST_MUST_USE_RESULT;
#if defined(__GNUC__) \
  && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)) \
  && !defined(COMPILER_ICC)
#define GTEST_MUST_USE_RESULT __attribute__ ((warn_unused_result))
#else
#define GTEST_MUST_USE_RESULT
#endif  // (__GNUC__ > 3 || __GNUC__ == 3 && __GNUC_MINOR__ >= 4)

// A macro to disallow the evil copy constructor and operator= functions
// This should be used in the private: declarations for a class.
//
#define GTEST_DISALLOW_EVIL_CONSTRUCTORS(type)\
  type(const type &);\
  void operator=(const type &)

namespace testing {

// Forward declaration of classes.

struct TraceInfo;                      // Information about a trace point.
template <typename E> class List;      // A generic list.
template <typename E> class ListNode;  // A node in a generic list.
class Message;                         // Represents a failure message.
class TestPartResult;                  // Result of a test part.
class TestResult;                      // Result of a single Test.
class TestInfo;                        // Information about a test.
class TestInfoImpl;                    // Opaque implementation of TestInfo
class TestCase;                        // A collection of related tests.
class UnitTest;                        // A collection of test cases.
class UnitTestImpl;                    // Opaque implementation of UnitTest
class AssertionResult;                 // Result of an assertion.

namespace internal {

#ifdef __SYMBIAN32__
// Symbian does not have tr1::type_traits, so we define our own is_pointer
// These are needed as the Nokia Symbian Compiler cannot decide between
// const T& and const T* in a function template.

template <bool bool_value>
struct bool_constant {
  typedef bool_constant<bool_value> type;
  static const bool value = bool_value;
};
template <bool bool_value> const bool bool_constant<bool_value>::value;

typedef bool_constant<false> false_type;
typedef bool_constant<true> true_type;

template <typename T>
struct is_pointer : public false_type {};

template <typename T>
struct is_pointer<T*> : public true_type {};

#endif  // __SYMBIAN32__

// Defines BiggestInt as the biggest signed integer type the compiler
// supports.
#ifdef _MSC_VER
typedef __int64 BiggestInt;
#else
typedef long long BiggestInt;  // NOLINT
#endif

// The maximum number a BiggestInt can represent.  This definition
// works no matter BiggestInt is represented in one's complement or
// two's complement.
//
// We cannot rely on numeric_limits in STL, as __int64 and long long
// are not part of standard C++ and numeric_limits doesn't need to be
// defined for them.
const BiggestInt kMaxBiggestInt =
    ~(static_cast<BiggestInt>(1) << (8*sizeof(BiggestInt) - 1));

// A secret type that gTest users don't know about.  It has no
// definition on purpose.  Therefore it's impossible to create a
// Secret object, which is what we want.
class Secret;

// Two overloaded helpers for checking at compile time whether an
// expression is a null pointer literal (i.e. NULL or any 0-valued
// compile-time integral constant).  Their return values have
// different sizes, so we can use sizeof() to test which version is
// picked by the compiler.  These helpers have no implementations, as
// we only need their signatures.
//
// Given IsNullLiteralHelper(x), the compiler will pick the first
// version if x can be implicitly converted to Secret*, and pick the
// second version otherwise.  Since Secret is a secret and incomplete
// type, the only expression a user can write that has type Secret* is
// a null pointer literal.  Therefore, we know that x is a null
// pointer literal if and only if the first version is picked by the
// compiler.
char IsNullLiteralHelper(Secret* p);
char (&IsNullLiteralHelper(...))[2];  // NOLINT

// A compile-time bool constant that is true if and only if x is a
// null pointer literal (i.e. NULL or any 0-valued compile-time
// integral constant).
#ifdef __SYMBIAN32__  // Symbian
// Passing non-POD classes through ellipsis (...) crashes the ARM compiler.
// The Nokia Symbian compiler tries to instantiate a copy constructor for
// objects passed through ellipsis (...), failing for uncopyable objects.
// Hence we define this to false (and lose support for NULL detection).
#define GTEST_IS_NULL_LITERAL(x) false
#else  // ! __SYMBIAN32__
#define GTEST_IS_NULL_LITERAL(x) \
    (sizeof(::testing::internal::IsNullLiteralHelper(x)) == 1)
#endif  // __SYMBIAN32__

}  // namespace internal

// String - a UTF-8 string class.
//
// We cannot use std::string as Microsoft's STL implementation in
// Visual C++ 7.1 has problems when exception is disabled.  There is a
// hack to work around this, but we've seen cases where the hack fails
// to work.
//
// Also, String is different from std::string in that it can represent
// both NULL and the empty string, while std::string cannot represent
// NULL.
//
// NULL and the empty string are considered different.  NULL is less
// than anything (including the empty string) except itself.
//
// This class only provides minimum functionality necessary for
// implementing gTest.  We do not intend to implement a full-fledged
// string class here.
//
// Since the purpose of this class is to provide a substitute for
// std::string on platforms where it cannot be used, we define a copy
// constructor and assignment operators such that we don't need
// conditional compilation in a lot of places.
//
// In order to make the representation efficient, the d'tor of String
// is not virtual.  Therefore DO NOT INHERIT FROM String.
class String {
 public:
  // Static utility methods

  // Returns the input if it's not NULL, otherwise returns "(null)".
  // This function serves two purposes:
  //
  // 1. ShowCString(NULL) has type 'const char *', instead of the
  // type of NULL (which is int).
  //
  // 2. In MSVC, streaming a null char pointer to StrStream generates
  // an access violation, so we need to convert NULL to "(null)"
  // before streaming it.
  static inline const char * ShowCString(const char * c_str) {
    return c_str ? c_str : "(null)";
  }

  // Returns the input enclosed in double quotes if it's not NULL;
  // otherwise returns "(null)".  For example, "\"Hello\"" is returned
  // for input "Hello".
  //
  // This is useful for printing a C string in the syntax of a literal.
  //
  // Known issue: escape sequences are not handled yet.
  static String ShowCStringQuoted(const char* c_str);

  // Clones a 0-terminated C string, allocating memory using new.  The
  // caller is responsible for deleting the return value using
  // delete[].  Returns the cloned string, or NULL if the input is
  // NULL.
  //
  // This is different from strdup() in string.h, which allocates
  // memory using malloc().
  static const char * CloneCString(const char * c_str);

  // Compares two C strings.  Returns true iff they have the same content.
  //
  // Unlike strcmp(), this function can handle NULL argument(s).  A
  // NULL C string is considered different to any non-NULL C string,
  // including the empty string.
  static bool CStringEquals(const char * lhs, const char * rhs);

  // Converts a wide C string to a String using the UTF-8 encoding.
  // NULL will be converted to "(null)".  If an error occurred during
  // the conversion, "(failed to convert from wide string)" is
  // returned.
  static String ShowWideCString(const wchar_t* wide_c_str);

  // Similar to ShowWideCString(), except that this function encloses
  // the converted string in double quotes.
  static String ShowWideCStringQuoted(const wchar_t* wide_c_str);

  // Compares two wide C strings.  Returns true iff they have the same
  // content.
  //
  // Unlike wcscmp(), this function can handle NULL argument(s).  A
  // NULL C string is considered different to any non-NULL C string,
  // including the empty string.
  static bool WideCStringEquals(const wchar_t * lhs, const wchar_t * rhs);

  // Compares two C strings, ignoring case.  Returns true iff they
  // have the same content.
  //
  // Unlike strcasecmp(), this function can handle NULL argument(s).
  // A NULL C string is considered different to any non-NULL C string,
  // including the empty string.
  static bool CaseInsensitiveCStringEquals(const char * lhs,
                                           const char * rhs);

  // Formats a list of arguments to a String, using the same format
  // spec string as for printf.
  //
  // We do not use the StringPrintf class as it is not universally
  // available.
  //
  // The result is limited to 4096 characters (including the tailing
  // 0).  If 4096 characters are not enough to format the input,
  // "<buffer exceeded>" is returned.
  static String Format(const char * format, ...);

  // C'tors

  // The default c'tor constructs a NULL string.
  String() : c_str_(NULL) {}

  // Constructs a String by cloning a 0-terminated C string.
  explicit String(const char * c_str) : c_str_(NULL) {
    *this = c_str;
  }

  // Constructs a String by copying a given number of chars from a
  // buffer.  E.g. String("hello", 3) will create the string "hel".
  String(const char * buffer, size_t len);

  // The copy c'tor creates a new copy of the string.  The two
  // String objects do not share content.
  String(const String & string) : c_str_(NULL) {
    *this = string;
  }

  // D'tor.  String is intended to be a final class, so the d'tor
  // doesn't need to be virtual.
  ~String() { delete[] c_str_; }

  // Returns true iff this is an empty string (i.e. "").
  bool is_empty() const {
    return (c_str_ != NULL) && (*c_str_ == '\0');
  }

  // Compares this with another String.
  // Returns < 0 if this is less than rhs, 0 if this is equal to rhs, or > 0
  // if this is greater than rhs.
  int Compare(const String & rhs) const;

  // Returns true iff this String equals the given C string.  A NULL
  // string and a non-NULL string are considered not equal.
  bool Equals(const char* c_str) const {
    return CStringEquals(c_str_, c_str);
  }

  // Returns true iff this String ends with the given suffix.  *Any*
  // String is considered to end with a NULL or empty suffix.
  bool EndsWith(const char* suffix) const;

  // Returns the length of the encapsulated string, or -1 if the
  // string is NULL.
  int GetLength() const {
    return c_str_ ? static_cast<int>(strlen(c_str_)) : -1;
  }

  // Gets the 0-terminated C string this String object represents.
  // The String object still owns the string.  Therefore the caller
  // should NOT delete the return value.
  const char * c_str() const { return c_str_; }

  // Sets the 0-terminated C string this String object represents.
  // The old string in this object is deleted, and this object will
  // own a clone of the input string.  This function copies only up to
  // length bytes (plus a terminating null byte), or until the first
  // null byte, whichever comes first.
  //
  // This function works even when the c_str parameter has the same
  // value as that of the c_str_ field.
  void Set(const char* c_str, size_t length);

  // Assigns a C string to this object.  Self-assignment works.
  const String& operator=(const char* c_str);

  // Assigns a String object to this object.  Self-assignment works.
  const String& operator=(const String &rhs) {
    *this = rhs.c_str_;
    return *this;
  }

 private:
  const char* c_str_;
};

// Streams a String to an ostream.
inline std::ostream& operator <<(std::ostream& os, const String& str) {
  // We call String::ShowCString() to convert NULL to "(null)".
  // Otherwise we'll get an access violation on Windows.
  return os << String::ShowCString(str.c_str());
}

// Gets the content of the StrStream's buffer as a String.  Each '\0'
// character in the buffer is replaced with "\\0".
String StrStreamToString(StrStream *);

// Appends the user-supplied message to the gTest-generated message.
String AppendUserMessage(const String& gtest_msg,
                         const Message& user_msg);

// A helper class for creating scoped traces in user programs.
class ScopedTrace {
 public:
  // The c'tor pushes the given source file location and message onto
  // a trace stack maintained by gTest.
  ScopedTrace(const char* file, int line, const Message& message);

  // The d'tor pops the info pushed by the c'tor.
  //
  // Note that the d'tor is not virtual in order to be efficient.
  // Don't inherit from ScopedTrace!
  ~ScopedTrace();

 private:
  GTEST_DISALLOW_EVIL_CONSTRUCTORS(ScopedTrace);
} GTEST_ATTRIBUTE_UNUSED;  // A ScopedTrace object does its job in its
                           // c'tor and d'tor.  Therefore it doesn't
                           // need to be used otherwise.

// This template class serves as a compile-time function from size to
// type.  It maps a size in bytes to a primitive type with that
// size. e.g.
//
//   TypeWithSize<4>::UInt
//
// is typedef-ed to be unsigned int (unsigned integer made up of 4
// bytes).
//
// Such functionality should belong to STL, but I cannot find it
// there.
//
// gTest uses this class in the implementation of floating-point
// comparison.
//
// For now it only handles UInt (unsigned int) as that's all gTest
// needs.  Other types can be easily added in the future if need
// arises.
template <size_t size>
class TypeWithSize {
 public:
  // This prevents the user from using TypeWithSize<N> with incorrect
  // values of N.
  typedef void UInt;
};

// The specialization for size 4.
template <>
class TypeWithSize<4> {
 public:
  // unsigned int has size 4 in both gcc and MSVC.
  //
  // As base/basictypes.h doesn't compile on Windows, we cannot use
  // uint32, uint64, and etc here.
  typedef int Int;
  typedef unsigned int UInt;
};

// The specialization for size 8.
template <>
class TypeWithSize<8> {
 public:
#ifdef _MSC_VER  // On Windows?
  typedef __int64 Int;
  typedef unsigned __int64 UInt;
#else
  typedef long long Int;  // NOLINT
  typedef unsigned long long UInt;  // NOLINT
#endif  // _MSC_VER
};

// A type that represents a number of elapsed milliseconds.
typedef TypeWithSize<8>::Int TimeInMillis;

// Converts a streamable value to a String.  A NULL pointer is
// converted to "(null)".  When the input value is a ::string,
// ::std::string, ::wstring, or ::std::wstring object, each NUL
// character in it is replaced with "\\0".
// Declared here but defined in gtest.h, so that it has access
// to the definition of the Message class, required by the ARM
// compiler.
template <typename T>
String StreamableToString(const T& streamable);

// Formats a value to be used in a failure message.

#ifdef __SYMBIAN32__

// These are needed as the Nokia Symbian Compiler cannot decide between
// const T& and const T* in a function template. The Nokia compiler _can_
// decide between class template specializations for T and T*, so a
// tr1::type_traits-like is_pointer works, and we can overload on that.

// This overload makes sure that all pointers (including
// those to char or wchar_t) are printed as raw pointers.
template <typename T>
inline String FormatValueForFailureMessage(internal::true_type dummy,
                                           T* pointer) {
  return StreamableToString(static_cast<const void*>(pointer));
}

template <typename T>
inline String FormatValueForFailureMessage(internal::false_type dummy,
                                           const T& value) {
  return StreamableToString(value);
}

template <typename T>
inline String FormatForFailureMessage(const T& value) {
  return FormatValueForFailureMessage(
      typename internal::is_pointer<T>::type(), value);
}

#else

template <typename T>
inline String FormatForFailureMessage(const T& value) {
  return StreamableToString(value);
}

// This overload makes sure that all pointers (including
// those to char or wchar_t) are printed as raw pointers.
template <typename T>
inline String FormatForFailureMessage(T* pointer) {
  return StreamableToString(static_cast<const void*>(pointer));
}

#endif  // __SYMBIAN32__

// These overloaded versions handle narrow and wide characters.
String FormatForFailureMessage(char ch);
String FormatForFailureMessage(wchar_t wchar);

// When this operand is a const char* or char*, and the other operand
// is a ::std::string or ::string, we print this operand as a C string
// rather than a pointer.  We do the same for wide strings.

// This internal macro is used to avoid duplicated code.
#define GTEST_FORMAT_IMPL(operand2_type, operand1_printer)\
inline String FormatForComparisonFailureMessage(\
    operand2_type::value_type* str, const operand2_type& operand2) {\
  (void)(operand2);\
  return operand1_printer(str);\
}\
inline String FormatForComparisonFailureMessage(\
    const operand2_type::value_type* str, const operand2_type& operand2) {\
  (void)(operand2);\
  return operand1_printer(str);\
}

#if GTEST_HAS_STD_STRING
GTEST_FORMAT_IMPL(::std::string, String::ShowCStringQuoted)
GTEST_FORMAT_IMPL(::std::wstring, String::ShowWideCStringQuoted)
#endif  // GTEST_HAS_STD_STRING

#if GTEST_HAS_GLOBAL_STRING
GTEST_FORMAT_IMPL(::string, String::ShowCStringQuoted)
GTEST_FORMAT_IMPL(::wstring, String::ShowWideCStringQuoted)
#endif  // GTEST_HAS_GLOBAL_STRING

#undef GTEST_FORMAT_IMPL

// Constructs and returns the message for an equality assertion
// (e.g. ASSERT_EQ, EXPECT_STREQ, etc) failure.
//
// The first four parameters are the expressions used in the assertion
// and their values, as strings.  For example, for ASSERT_EQ(foo, bar)
// where foo is 5 and bar is 6, we have:
//
//   expected_expression: "foo"
//   actual_expression:   "bar"
//   expected_value:      "5"
//   actual_value:        "6"
//
// The ignoring_case parameter is true iff the assertion is a
// *_STRCASEEQ*.  When it's true, the string " (ignoring case)" will
// be inserted into the message.
AssertionResult EqFailure(const char* expected_expression,
                          const char* actual_expression,
                          const String& expected_value,
                          const String& actual_value,
                          bool ignoring_case);


AssertionResult AssertionSuccess();

// This template class represents an IEEE floating-point number
// (either single-precision or double-precision, depending on the
// template parameters).
//
// The purpose of this class is to do more sophisticated number
// comparison.  (Due to round-off error, etc, it's very unlikely that
// two floating-points will be equal exactly.  Hence a naive
// comparison by the == operation often doesn't work.)
//
// Format of IEEE floating-point:
//
//   The most-significant bit being the leftmost, an IEEE
//   floating-point looks like
//
//     sign_bit exponent_bits fraction_bits
//
//   Here, sign_bit is a single bit that designates the sign of the
//   number.
//
//   For float, there are 8 exponent bits and 23 fraction bits.
//
//   For double, there are 11 exponent bits and 52 fraction bits.
//
//   More details can be found at
//   http://en.wikipedia.org/wiki/IEEE_floating-point_standard.
//
// Template parameter:
//
//   RawType: the raw floating-point type (either float or double)
template <typename RawType>
class FloatingPoint {
 public:
  // Defines the unsigned integer type that has the same size as the
  // floating point number.
  typedef typename TypeWithSize<sizeof(RawType)>::UInt Bits;

  // Constants.

  // # of bits in a number.
  static const size_t kBitCount = 8*sizeof(RawType);

  // # of fraction bits in a number.
  static const size_t kFractionBitCount =
    std::numeric_limits<RawType>::digits - 1;

  // # of exponent bits in a number.
  static const size_t kExponentBitCount = kBitCount - 1 - kFractionBitCount;

  // The mask for the sign bit.
  static const Bits kSignBitMask = static_cast<Bits>(1) << (kBitCount - 1);

  // The mask for the fraction bits.
  static const Bits kFractionBitMask =
    ~static_cast<Bits>(0) >> (kExponentBitCount + 1);

  // The mask for the exponent bits.
  static const Bits kExponentBitMask = ~(kSignBitMask | kFractionBitMask);

  // How many ULP's (Units in the Last Place) we want to tolerate when
  // comparing two numbers.  The larger the value, the more error we
  // allow.  A 0 value means that two numbers must be exactly the same
  // to be considered equal.
  //
  // The maximum error of a single floating-point operation is 0.5
  // units in the last place.  On Intel CPU's, all floating-point
  // calculations are done with 80-bit precision, while double has 64
  // bits.  Therefore, 4 should be enough for ordinary use.
  //
  // See the following article for more details on ULP:
  // http://www.cygnus-software.com/papers/comparingfloats/comparingfloats.htm.
  static const size_t kMaxUlps = 4;

  // Constructs a FloatingPoint from a raw floating-point number.
  //
  // On an Intel CPU, passing a non-normalized NAN (Not a Number)
  // around may change its bits, although the new value is guaranteed
  // to be also a NAN.  Therefore, don't expect this constructor to
  // preserve the bits in x when x is a NAN.
  explicit FloatingPoint(const RawType& x) : value_(x) {}

  // Static methods

  // Reinterprets a bit pattern as a floating-point number.
  //
  // This function is needed to test the AlmostEquals() method.
  static RawType ReinterpretBits(const Bits bits) {
    FloatingPoint fp(0);
    fp.bits_ = bits;
    return fp.value_;
  }

  // Returns the floating-point number that represent positive infinity.
  static RawType Infinity() {
    return ReinterpretBits(kExponentBitMask);
  }

  // Non-static methods

  // Returns the bits that represents this number.
  const Bits &bits() const { return bits_; }

  // Returns the exponent bits of this number.
  Bits exponent_bits() const { return kExponentBitMask & bits_; }

  // Returns the fraction bits of this number.
  Bits fraction_bits() const { return kFractionBitMask & bits_; }

  // Returns the sign bit of this number.
  Bits sign_bit() const { return kSignBitMask & bits_; }

  // Returns true iff this is NAN (not a number).
  bool is_nan() const {
    // It's a NAN if the exponent bits are all ones and the fraction
    // bits are not entirely zeros.
    return (exponent_bits() == kExponentBitMask) && (fraction_bits() != 0);
  }

  // Returns true iff this number is at most kMaxUlps ULP's away from
  // rhs.  In particular, this function:
  //
  //   - returns false if either number is (or both are) NAN.
  //   - treats really large numbers as almost equal to infinity.
  //   - thinks +0.0 and -0.0 are 0 DLP's apart.
  bool AlmostEquals(const FloatingPoint& rhs) const {
    // The IEEE standard says that any comparison operation involving
    // a NAN must return false.
    if (is_nan() || rhs.is_nan()) return false;

    return DistanceBetweenSignAndMagnitudeNumbers(bits_, rhs.bits_) <= kMaxUlps;
  }

 private:
  // Converts an integer from the sign-and-magnitude representation to
  // the biased representation.  More precisely, let N be 2 to the
  // power of (kBitCount - 1), an integer x is represented by the
  // unsigned number x + N.
  //
  // For instance,
  //
  //   -N + 1 (the most negative number representable using
  //          sign-and-magnitude) is represented by 1;
  //   0      is represented by N; and
  //   N - 1  (the biggest number representable using
  //          sign-and-magnitude) is represented by 2N - 1.
  //
  // Read http://en.wikipedia.org/wiki/Signed_number_representations
  // for more details on signed number representations.
  static Bits SignAndMagnitudeToBiased(const Bits &sam) {
    if (kSignBitMask & sam) {
      // sam represents a negative number.
      return ~sam + 1;
    } else {
      // sam represents a positive number.
      return kSignBitMask | sam;
    }
  }

  // Given two numbers in the sign-and-magnitude representation,
  // returns the distance between them as an unsigned number.
  static Bits DistanceBetweenSignAndMagnitudeNumbers(const Bits &sam1,
                                                     const Bits &sam2) {
    const Bits biased1 = SignAndMagnitudeToBiased(sam1);
    const Bits biased2 = SignAndMagnitudeToBiased(sam2);
    return (biased1 >= biased2) ? (biased1 - biased2) : (biased2 - biased1);
  }

  union {
    RawType value_;  // The raw floating-point number.
    Bits bits_;      // The bits that represent the number.
  };
};

// Typedefs the instances of the FloatingPoint template class that we
// care to use.
typedef FloatingPoint<float> Float;
typedef FloatingPoint<double> Double;

// In order to catch the mistake of putting tests that use different
// test fixture classes in the same test case, we need to assign
// unique IDs to fixture classes and compare them.  The TypeId type is
// used to hold such IDs.  The user should treat TypeId as an opaque
// type: the only operation allowed on TypeId values is to compare
// them for equality using the == operator.
typedef void* TypeId;

// GetTypeId<T>() returns the ID of type T.  Different values will be
// returned for different types.  Calling the function twice with the
// same type argument is guaranteed to return the same ID.
//
// We don't use a similar utility in util/gtl/typeid.h, as that
// doesn't work outside google3.  The version defined here doesn't
// have some of the properties of the one in util/gtl/typeid.h, but is
// adequate for gTest's need.
template <typename T>
inline TypeId GetTypeId() {
  static bool dummy = false;
  // The compiler is required to create an instance of the static
  // variable dummy for each T used to instantiate the template.
  // Therefore, the address of dummy is guaranteed to be unique.
  return &dummy;
}

#ifdef _MSC_VER

namespace internal {

// Predicate-formatters for implementing the HRESULT checking macros
// {ASSERT|EXPECT}_HRESULT_{SUCCEEDED|FAILED}
// We pass a long instead of HRESULT to avoid causing an
// include dependency for the HRESULT type.
AssertionResult IsHRESULTSuccess(const char* expr, long hr);
AssertionResult IsHRESULTFailure(const char* expr, long hr);

}  // namespace internal

#endif  // _MSC_VER

}  // namespace testing

#define GTEST_MESSAGE(message, result_type) \
  ::testing::AssertHelper(result_type, __FILE__, __LINE__, message) = \
    ::testing::Message()

#define GTEST_FATAL_FAILURE(message) \
  return GTEST_MESSAGE(message, ::testing::TPRT_FATAL_FAILURE)

#define GTEST_NONFATAL_FAILURE(message) \
  GTEST_MESSAGE(message, ::testing::TPRT_NONFATAL_FAILURE)

#define GTEST_SUCCESS(message) \
  GTEST_MESSAGE(message, ::testing::TPRT_SUCCESS)

#define GTEST_TEST_BOOLEAN(boolexpr, booltext, actual, expected, fail) \
  GTEST_AMBIGUOUS_ELSE_BLOCKER \
  if (boolexpr) \
    ; \
  else \
    fail("Value of: " booltext "\n  Actual: " #actual "\nExpected: " #expected)


// Helper macro for defining tests.
#define GTEST_TEST(test_case_name, test_name, parent_class)\
class test_case_name##_##test_name##_Test : public parent_class {\
 public:\
  test_case_name##_##test_name##_Test() {}\
  static ::testing::Test* NewTest() {\
    return new test_case_name##_##test_name##_Test;\
  }\
 private:\
  virtual void TestBody();\
  static ::testing::TestInfo* const test_info_;\
  GTEST_DISALLOW_EVIL_CONSTRUCTORS(test_case_name##_##test_name##_Test);\
};\
\
::testing::TestInfo* const test_case_name##_##test_name##_Test::test_info_ =\
  ::testing::TestInfo::MakeAndRegisterInstance(\
    #test_case_name, \
    #test_name, \
    ::testing::GetTypeId< parent_class >(), \
    parent_class::SetUpTestCase, \
    parent_class::TearDownTestCase, \
    test_case_name##_##test_name##_Test::NewTest);\
void test_case_name##_##test_name##_Test::TestBody()


#endif  // UNITTEST_GTEST_INTERNAL_H__

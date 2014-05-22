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

#ifndef GGADGET_STRING_UTILS_H__
#define GGADGET_STRING_UTILS_H__

#include <cstring>
#include <cstdarg>
#include <map>
#include <string>
#include <vector>
#include <ggadget/common.h>
#include <ggadget/light_map.h>
#include <ggadget/unicode_utils.h>

namespace ggadget {

/**
 * @defgroup StringUtilities String utilities
 * @ingroup Utilities
 * @{
 */

/**
 * Use these functions to compare strings in gadget library code.
 * These strings include property names, file names, XML element and attribute
 * names, etc., but not include gadget element names.
 * Define @c GADGET_CASE_SENSITIVE to make the comparison case sensitive.
 * Don't define @c GADGET_CASE_SENSITIVE if compatibility with the Windows
 * version is required.
 */
int GadgetStrCmp(const char *s1, const char *s2);
int GadgetStrNCmp(const char *s1, const char *s2, size_t n);
int GadgetCharCmp(char c1, char c2);

struct CharPtrComparator {
  bool operator()(const char* s1, const char* s2) const {
    return strcmp(s1, s2) < 0;
  }
};

/**
 * A comparison functor for <code>const char *</code> parameters.
 * It's useful when using <code>const char *</code> as the key of a map.
 */
struct GadgetCharPtrComparator {
  bool operator()(const char* s1, const char* s2) const {
    return GadgetStrCmp(s1, s2) < 0;
  }
};

struct GadgetStringComparator {
  bool operator()(const std::string &s1, const std::string &s2) const {
    return GadgetStrCmp(s1.c_str(), s2.c_str()) < 0;
  }
};

/**
 * Default gadget string map, case sensitivity controled by
 * @c GADGET_CASE_SENSITIVE.
 */
typedef LightMap<std::string, std::string,
                 GadgetStringComparator> GadgetStringMap;

/**
 * Case sensitive string map.
 */
typedef LightMap<std::string, std::string> StringMap;

struct CaseInsensitiveCharPtrComparator {
  bool operator()(const char* s1, const char* s2) const {
    return strcasecmp(s1, s2) < 0;
  }
};

struct CaseInsensitiveStringComparator {
  bool operator()(const std::string &s1, const std::string &s2) const {
    return strcasecmp(s1.c_str(), s2.c_str()) < 0;
  }
};

typedef LightMap<std::string, std::string,
                 CaseInsensitiveStringComparator> CaseInsensitiveStringMap;

typedef std::vector<std::string> StringVector;

/**
 * Assigns a <code>const char *</code> string to a std::string if they are
 * different.
 * @param source source string. If it is @c NULL, dest will be cleared to blank.
 * @param dest destination string.
 * @param comparator the comparator used to compare the strings, default is
 *     @c GadgetStrCmp().
 * @return @c true if assignment occurs.
 */
bool AssignIfDiffer(
    const char *source, std::string *dest,
    int (*comparator)(const char *s1, const char *s2) = GadgetStrCmp);

/**
 * Removes the starting and ending spaces from a string.
 */
std::string TrimString(const std::string &s);

/**
 * Converts a string into lowercase.
 * Note: Only converts ASCII chars.
 */
std::string ToLower(const std::string &s);

/**
 * Converts a string into uppercase.
 * Note: Only converts ASCII chars.
 */
std::string ToUpper(const std::string &s);

/**
 * Format data into a C++ string.
 */
std::string StringPrintf(const char *format, ...)
  // Tell the compiler to do printf format string checking.
  PRINTF_ATTRIBUTE(1,2);
std::string StringVPrintf(const char *format, va_list ap);

/**
 * Append result to a supplied string
 */
void StringAppendPrintf(std::string *dst, const char *format, ...)
  PRINTF_ATTRIBUTE(2,3);
void StringAppendVPrintf(std::string *dst, const char *format, va_list ap);

/**
 * URI-encode the source string.
 * Note: don't encode a valid uri twice, which will give wrong result.
 */
std::string EncodeURL(const std::string &source);

/**
 * URI-encode the source string as a URI component.
 * Note: don't encode a valid uri twice, which will give wrong result.
 */
std::string EncodeURLComponent(const std::string &source);

/** URI-decode the source string. */
std::string DecodeURL(const std::string &source);

/** Returns whether the given character is valid in a URI. See RFC2396. */
bool IsValidURLChar(char c);
bool IsValidURLComponentChar(char c);

/** Returns the scheme of a url, eg. http, https, etc. */
std::string GetURLScheme(const char *url);

/**
 * Returns whether the given url scheme is valid or not.
 *
 * Valid url schemes are:
 * http, https, feed, file, mailto
 */
bool IsValidURLScheme(const char *scheme);

/**
 * Returns whether the given string has a valid url prefixes.
 *
 * Valid url prefixes are:
 * http://
 * https://
 * feed://
 * file://
 * mailto:
 */
bool HasValidURLPrefix(const char *url);

/**
 * Returns whether the given string is a valid url component,
 * or in another word, if it only contains valid url chars.
 */
bool IsValidURLComponent(const char *url);

/**
 * Returns whether the given string only contains valid url chars.
 */
bool IsValidURLString(const char *url);

/**
 * Returns whether the given string is a valid URL.
 *
 * Equals to HasValidURLPrefix(url) && IsValidURLString(url)
 */
bool IsValidURL(const char *url);

/** Returns whether the given string is a valid URL for a RSS feed. */
bool IsValidRSSURL(const char *url);

/**
 * Returns whether the given string is a valid URL.
 * Only http:// and https:// prefix are allowed.
 * Not a very complete check at the moment.
 */
bool IsValidWebURL(const char *url);

/**
 * Returns whether the given string is a valid file URL,
 * which has file:// prefix.
 */
bool IsValidFileURL(const char *url);

/**
 * Returns the host part of a URL in common Internet scheme syntax:
 * <scheme>://[<user>:<password>@]<host>[:<port>][/<url_path>].
 * Returns blank string if the URL is invalid.
 */
std::string GetHostFromURL(const char *url);

/**
 * Returns the filename part of a file URL, or blank string if the url is not a
 * valid file url.
 */
std::string GetPathFromFileURL(const char *url);

/**
 * Returns the username:password part of a URL, or blank string if the url has
 * no username:password part.
 */
std::string GetUsernamePasswordFromURL(const char *url);

/**
 * @param base_url the base URL (must be absolute).
 * @param url an relative or absolute URL.
 * @return the absolute URL computed from @a base_url and @a url.
 */
std::string GetAbsoluteURL(const char *base_url, const char *url);

/**
 * Encode a string into a JavaScript string literal enclosed with the specified
 * quote char, by escaping special characters in the source string.
 */
std::string EncodeJavaScriptString(const UTF16Char *source, char quote);
std::string EncodeJavaScriptString(const std::string &source, char quote);

/**
 * Decode a JavaScript string literal (with the beginning and ending quotes).
 * Returns false if the source is invalid JavaScript string literal.
 */
bool DecodeJavaScriptString(const char *source, UTF16String *dest);
bool DecodeJavaScriptString(const char *source, std::string *dest);

/**
 * Splits a string into two parts. For convenience, the impl allows
 * @a result_left or @a result_right is the same object as @a source.
 *
 * @param source the source string to split.
 * @param separator the source string will be split at the first occurrence
 *     of the separator.
 * @param[out] result_left the part before the separator. If separator is not
 *     found, result_left will be set to source. Can be @c NULL if the caller
 *     doesn't need it.
 * @param[out] result_right the part after the separator. If separator is not
 *     found, result_right will be cleared. Can be @c NULL if the caller doesn't
 *     need it.
 * @return @c true if separator found.
 */
bool SplitString(const std::string &source, const char *separator,
                 std::string *result_left, std::string *result_right);

/**
 * Splits a string into a list with specified separator.
 * @param source the source string to split.
 * @param separator the separator string to be used to split the source string.
 * @param[out] result store the result string list.
 * @return @c true if separator found.
 */
bool SplitStringList(const std::string &source, const char *separator,
                     StringVector *result);

/**
 * Compresses white spaces in a string using the rule like HTML formatting:
 *   - Removing leading and trailing white spaces;
 *   - Converting all consecutive white spaces into single spaces;
 * Only ASCII spaces (isspace(ch) == true) are handled.
 */
std::string CompressWhiteSpaces(const char *source);

/**
 * Removes tags and converts entities to their utf8 representation.
 * If everything is removed, this will return an empty string.
 * Whitespaces are compressed as in @c CompressWhiteSpaces().
 */
std::string ExtractTextFromHTML(const char *source);

/**
 * Guess if a string contains HTML. We basically check for common constructs
 * in html such as </, />, <br>, <hr>, <p>, etc.
 * This function only scans the 50k characters as an optimization.
 */
bool ContainsHTML(const char *s);

/**
 * Converts all '\r's, '\n's and '\r\n's into spaces. Useful to display
 * multi-line text in a single-line container.
 */
std::string CleanupLineBreaks(const char *source);

/**
 * Matches an XPath string against an XPath pattern, and returns whether
 * matches. This function only supports simple xpath grammar, containing only
 * element tag name, element index (not allowed in pattern) and attribute
 * names, no wildcard or other xpath expressions.
 *
 * @param xpath an xpath string returned as one of the keys in the result of
 *     XMLParserInterface::ParseXMLIntoXPathMap().
 * @param pattern an xpath pattern with only element tag names and (optional)
 *     attribute names.
 * @return true if the xpath matches the pattern. The matching rule is: first
 *     remove all [...]s in the xpath, and test if the result equals to the
 *     pattern.
 */
bool SimpleMatchXPath(const char *xpath, const char *pattern);

/**
 * Compares two versions.
 * @param version1 version string in "x", "x.x", "x.x.x" or "x.x.x.x" format
 *     where 'x' is an integer.
 * @param version2
 * @param[out] result on success: -1 if version1 < version2, 0 if
 *     version1 == version2, or 1 if version1 > version2.
 * @return @c false if @a version1 or @a version2 is invalid version string,
 *     or @c true on success.
 */
bool CompareVersion(const char *version1, const char *version2, int *result);

/** Checks if a string has a specified prefix. */
bool StartWith(const char *string, const char *prefix);

/** Checks if a string has a specified prefix, ignoring the case. */
bool StartWithNoCase(const char *string, const char *prefix);

/** Checks if a string has a specified suffix. */
bool EndWith(const char *string, const char *suffix);

/** Checks if a string has a specified suffix, ignoring the case. */
bool EndWithNoCase(const char *string, const char *suffix);

/** Parse an string contains at most four double numbers split by space.
 * @param values original string contains the string numbers consisting
 * of 4/2/1 double numbers. if two numbers are specified , both @a d1
 * and @a d3 will be set to the first value, while @a d2 and @a d4 will be set
 * to the second value. if one number is specified, all four parameters are set
 * to the specified value.
 * @param left first parameter usually indicates left border or margin.
 * @param top sedoncd parameter usually indicates top border or margin.
 * @param right third parameter usually indicates right border or margin.
 * @param bottom fourth parameter usually indicates bottom border or margin.
 * @return @c false if @1 values doesn't have valid format.
 */
bool StringToBorderSize(const std::string &values,
                        double *left, double *top,
                        double *right, double *bottom);

/** @} */

} // namespace ggadget

#endif // GGADGET_STRING_UTILS_H__

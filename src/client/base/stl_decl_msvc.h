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
//

//
// In most .h files, we would rather include a declaration of an stl
// rather than including the appropriate stl h file (which brings in
// lots of crap).  For many STL classes this is ok (eg pair), but for
// some it's really annoying.  We define those here, so you can
// just include this file instead of having to deal with the annoyance.
//
// Most of the annoyance, btw, has to do with the default allocator.

#ifndef _STL_DECL_MSVC_H
#define _STL_DECL_MSVC_H

// The version of STL that ships with Visual Studio 2010 is compatible with
// the GCC version of STL, so most of this file is unnecessary when using
// MSVC 2010.  The only thing that is still needed when compiling with
// MSVC 2010 is to bring STL into the global namespace (see below).
//
// The #if test below could also key off of STL_MSVC.  However, some projects
// are directly including stl_decl_msvc.h (not going through stl_decl.h),
// so it's possible that those projects aren't defining STL_MSVC before
// including this file.
// _MSC_VER is defined by the MSVC compiler, so it is always safe to use.
#if _MSC_VER < 1600

// VC++ namespace / STL issues; make them explicit
#include <wchar.h>
#include <string>
#include <vector>
#include <functional>
#include <utility>
#include <set>
#include <list>
#define slist list
#include <algorithm>
#include <deque>
#include <iostream>
#include <map>
#include <queue>
#include <stack>

// copy_n isn't to be found anywhere in MSVC's STL
template <typename InputIterator, typename Size, typename OutputIterator>
std::pair<InputIterator, OutputIterator>
copy_n(InputIterator in, Size count, OutputIterator out) {
  for ( ; count > 0; --count) {
    *out = *in;
    ++out;
    ++in;
  }
  return std::make_pair(in, out);
}

// Nor are the following selectors
template <typename T>
struct identity {
  inline const T& operator()(const T& t) const { return t; }
};

// Copied from STLport
template <class _Pair>
struct select1st : public std::unary_function<_Pair, typename _Pair::first_type> {
  const typename _Pair::first_type& operator()(const _Pair& __x) const {
    return __x.first;
  }
};

template <class _Pair>
struct select2nd : public std::unary_function<_Pair, typename _Pair::second_type>
{
  const typename _Pair::second_type& operator()(const _Pair& __x) const {
    return __x.second;
  }
};

#if _MSC_VER >= 1300

// If you compile on Windows and get a compile-time error because
// some google3 code specifies a 3rd or 4th parameter to one of
// these template classes, then you have to put in some #ifdefs
// and use the NATIVE_HASH_NAMESPACE::hash_(set|map) implementation.
namespace msvchash {
  template <typename Key>
  struct hash;

  template <class Key,
            class HashFcn = hash<Key> >
  class hash_set;

  template <class Key, class Val,
            class HashFcn = hash<Key> >
  class hash_map;

  template <class Key,
            class HashFcn = hash<Key> >
  class hash_multiset;

  template <class Key, class Val,
            class HashFcn = hash<Key> >
  class hash_multimap;
}  // end namespace  msvchash

using msvchash::hash_set;
using msvchash::hash_map;
using msvchash::hash;
using msvchash::hash_multimap;
using msvchash::hash_multiset;

#else
#define hash_map map
#define hash_set set
#endif
#endif // _MSC_VER < 1600

using namespace std;

#endif   /* #ifdef _STL_DECL_MSVC_H */

/*
  Copyright 2008 Google Inc.

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

// Part of this file (LokiAllocator) is from the Loki Library.
////////////////////////////////////////////////////////////////////////////////
// The Loki Library
// Copyright (c) 2001 by Andrei Alexandrescu
// This code accompanies the book:
// Alexandrescu, Andrei. "Modern C++ Design: Generic Programming and Design
//     Patterns Applied". Copyright (c) 2001. Addison-Wesley.
// Permission to use, copy, modify, distribute and sell this software for any
//     purpose is hereby granted without fee, provided that the above copyright
//     notice appear in all copies and that both that copyright notice and this
//     permission notice appear in supporting documentation.
// The author or Addison-Wesley Longman make no representations about the
//     suitability of this software for any purpose. It is provided "as is"
//     without express or implied warranty.
////////////////////////////////////////////////////////////////////////////////

#ifndef GGADGET_LIGHT_MAP_H__
#define GGADGET_LIGHT_MAP_H__

#include <map>
#include <set>
#include <ggadget/common.h>
#include <ggadget/small_object.h>

namespace ggadget {

#if !defined(OS_WIN)

// The following code is from Loki(0.1.7)'s Allocator.h
//-----------------------------------------------------------------------------

/** @class LokiAllocator
 Adapts Loki's Small-Object Allocator for STL container classes.
 This class provides all the functionality required for STL allocators, but
 uses Loki's Small-Object Allocator to perform actual memory operations.
 Implementation comes from a post in Loki forums (by Rasmus Ekman?).
 */
template
<
    typename Type,
    typename AllocT = AllocatorSingleton<>
>
class LokiAllocator
{
public:

    typedef ::std::size_t size_type;
    typedef ::std::ptrdiff_t difference_type;
    typedef Type * pointer;
    typedef const Type * const_pointer;
    typedef Type & reference;
    typedef const Type & const_reference;
    typedef Type value_type;

    /// Default constructor does nothing.
    inline LokiAllocator( void ) throw() { }

    /// Copy constructor does nothing.
    inline LokiAllocator( const LokiAllocator & ) throw() { }

    /// Type converting allocator constructor does nothing.
    template < typename Type1 >
    inline LokiAllocator( const LokiAllocator< Type1 > & ) throw() { }

    /// Destructor does nothing.
    inline ~LokiAllocator() throw() { }

    /// Convert an allocator<Type> to an allocator <Type1>.
    template < typename Type1 >
    struct rebind
    {
        typedef LokiAllocator< Type1 > other;
    };

    /// Return address of reference to mutable element.
    pointer address( reference elem ) const { return &elem; }

    /// Return address of reference to const element.
    const_pointer address( const_reference elem ) const { return &elem; }

    /** Allocate an array of count elements.  Warning!  The true parameter in
     the call to Allocate means this function can throw exceptions.  This is
     better than not throwing, and returning a null pointer in case the caller
     assumes the return value is not null.
     @param count # of elements in array.
     @param hint Place where caller thinks allocation should occur.
     @return Pointer to block of memory.
     */
    pointer allocate( size_type count, const void * hint = 0 )
    {
        (void)hint;  // Ignore the hint.
        void * p = AllocT::Instance().Allocate( count * sizeof( Type ), true );
        return reinterpret_cast< pointer >( p );
    }
    /// Ask allocator to release memory at pointer with size bytes.
    void deallocate( pointer p, size_type size )
    {
        AllocT::Instance().Deallocate( p, size * sizeof( Type ) );
    }

    /// Calculate max # of elements allocator can handle.
    size_type max_size( void ) const throw()
    {
        // A good optimizer will see these calculations always produce the same
        // value and optimize this function away completely.
        const size_type max_bytes = size_type( -1 );
        const size_type bytes = max_bytes / sizeof( Type );
        return bytes;
    }

    /// Construct an element at the pointer.
    void construct( pointer p, const Type & value )
    {
        // A call to global placement new forces a call to copy constructor.
        ::new( p ) Type( value );
    }

    /// Destruct the object at pointer.
    void destroy( pointer p )
    {
        // If the Type has no destructor, then some compilers complain about
        // an unreferenced parameter, so use the void cast trick to prevent
        // spurious warnings.
        (void)p;
        p->~Type();
    }

};

//-----------------------------------------------------------------------------

/** All equality operators return true since LokiAllocator is basically a
 monostate design pattern, so all instances of it are identical.
 */
template < typename Type >
inline bool operator == ( const LokiAllocator< Type > &, const LokiAllocator< Type > & )
{
    return true;
}

/** All inequality operators return false since LokiAllocator is basically a
 monostate design pattern, so all instances of it are identical.
 */
template < typename Type >
inline bool operator != ( const LokiAllocator< Type > & , const LokiAllocator< Type > & )
{
    return false;
}

//-----------------------------------------------------------------------------
// GGL code below.

/**
 * Customized version of std::map that uses LokiAllocator to improve
 * performance allocation/deallocation of small tree nodes.
 */
template <typename Key, typename Value, typename Compare = std::less<Key> >
class LightMap : public std::map<Key, Value, Compare,
    LokiAllocator<std::pair<const Key, Value> > > {
 public:
  typedef std::map<Key, Value, Compare,
                   LokiAllocator<std::pair<const Key, Value> > > Super;
  LightMap() { }
  LightMap(const Compare &comp) : Super(comp) { }
  LightMap(const LightMap &x) : Super(x) { }
  template <typename I> LightMap(I first, I last) : Super(first, last) { }
  LightMap &operator=(const LightMap &x)
  { Super::operator=(x); return *this; }
};

/**
 * Customized version of std::multimap that uses LokiAllocator to improve
 * performance allocation/deallocation of small tree nodes.
 */
template <typename Key, typename Value, typename Compare = std::less<Key> >
class LightMultiMap : public std::multimap<Key, Value, Compare,
    LokiAllocator<std::pair<const Key, Value> > > {
 public:
  typedef std::multimap<Key, Value, Compare,
                        LokiAllocator<std::pair<const Key, Value> > > Super;
  LightMultiMap() { }
  LightMultiMap(const Compare &comp) : Super(comp) { }
  LightMultiMap(const LightMultiMap &x) : Super(x) { }
  template <typename I> LightMultiMap(I first, I last) : Super(first, last) { }
  LightMultiMap &operator=(const LightMultiMap &x)
  { Super::operator=(x); return *this; }
};

/**
 * Customized version of std::set that uses LokiAllocator to improve
 * performance allocation/deallocation of small tree nodes.
 */
template <typename Key, typename Compare = std::less<Key> >
class LightSet : public std::set<Key, Compare, LokiAllocator<Key> > {
 public:
  typedef std::set<Key, Compare, LokiAllocator<Key> > Super;
  LightSet() { }
  LightSet(const Compare &comp) : Super(comp) { }
  LightSet(const LightSet &x) : Super(x) { }
  template <typename I> LightSet(I first, I last) : Super(first, last) { }
  LightSet &operator=(const LightSet &x)
  { Super::operator=(x); return *this; }
};

#else

template <typename Key, typename Value, typename Compare = std::less<Key> >
class LightMap : public std::map<Key, Value, Compare> {
 public:
  typedef std::map<Key, Value, Compare> Super;
  LightMap() { }
  LightMap(const Compare &comp) : Super(comp) { }
  LightMap(const LightMap &x) : Super(x) { }
  template <typename I> LightMap(I first, I last) : Super(first, last) { }
  LightMap &operator=(const LightMap &x)
  { Super::operator=(x); return *this; }
};

template <typename Key, typename Value, typename Compare = std::less<Key> >
class LightMultiMap : public std::multimap<Key, Value, Compare> {
 public:
  typedef std::multimap<Key, Value, Compare> Super;
  LightMultiMap() { }
  LightMultiMap(const Compare &comp) : Super(comp) { }
  LightMultiMap(const LightMultiMap &x) : Super(x) { }
  template <typename I> LightMultiMap(I first, I last) : Super(first, last) { }
  LightMultiMap &operator=(const LightMultiMap &x)
  { Super::operator=(x); return *this; }
};

template <typename Key, typename Compare = std::less<Key> >
class LightSet : public std::set<Key, Compare> {
 public:
  typedef std::set<Key, Compare> Super;
  LightSet() { }
  LightSet(const Compare &comp) : Super(comp) { }
  LightSet(const LightSet &x) : Super(x) { }
  template <typename I> LightSet(I first, I last) : Super(first, last) { }
  LightSet &operator=(const LightSet &x)
  { Super::operator=(x); return *this; }
};

#endif  // OS_WIN

} // namespace ggadget

#endif // end file guardian

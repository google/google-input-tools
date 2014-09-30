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

// Tailored by Google to remove the dependency on Loki's thread and singleton
// modules.

#ifndef GGADGET_SMALL_OBJECT_H__
#define GGADGET_SMALL_OBJECT_H__

#include <cstddef>
#include <new> // needed for std::nothrow_t parameter.

#include <ggadget/common.h>

#ifndef LOKI_DEFAULT_CHUNK_SIZE
#define LOKI_DEFAULT_CHUNK_SIZE 4096
#endif

#ifndef LOKI_MAX_SMALL_OBJECT_SIZE
#define LOKI_MAX_SMALL_OBJECT_SIZE 256
#endif

#ifndef LOKI_DEFAULT_OBJECT_ALIGNMENT
#define LOKI_DEFAULT_OBJECT_ALIGNMENT 4
#endif

#if defined(LOKI_SMALL_OBJECT_USE_NEW_ARRAY) && defined(_MSC_VER)
#pragma message("Don't define LOKI_SMALL_OBJECT_USE_NEW_ARRAY when using a Microsoft compiler to prevent memory leaks.")
#pragma message("now calling '#undef LOKI_SMALL_OBJECT_USE_NEW_ARRAY'")
#undef LOKI_SMALL_OBJECT_USE_NEW_ARRAY
#endif

///  \defgroup  SmallObjectGroup Small objects
///  \ingroup   Utilities
///
///  \defgroup  SmallObjectGroupInternal Internals
///  \ingroup   SmallObjectGroup

namespace ggadget
{

#if !defined(OS_WIN)
    class FixedAllocator;

    /** @class SmallObjAllocator
        @ingroup SmallObjectGroupInternal
     Manages pool of fixed-size allocators.
     Designed to be a non-templated base class of AllocatorSingleton so that
     implementation details can be safely hidden in the source code file.
     */
    class SmallObjAllocator
    {
    protected:
        /** The only available constructor needs certain parameters in order to
         initialize all the FixedAllocator's.  This throws only if
         @param pageSize # of bytes in a page of memory.
         @param maxObjectSize Max # of bytes which this may allocate.
         @param objectAlignSize # of bytes between alignment boundaries.
         */
        SmallObjAllocator( std::size_t pageSize, std::size_t maxObjectSize,
            std::size_t objectAlignSize );

        /** Destructor releases all blocks, all Chunks, and FixedAllocator's.
         Any outstanding blocks are unavailable, and should not be used after
         this destructor is called.  The destructor is deliberately non-virtual
         because it is protected, not public.
         */
        ~SmallObjAllocator( void );

        /** Returns reference to the singleton. */
        static SmallObjAllocator & Instance( std::size_t pageSize,
            std::size_t maxObjectSize, std::size_t objectAlignSize );
    public:
        /** Allocates a block of memory of requested size.  Complexity is often
         constant-time, but might be O(C) where C is the number of Chunks in a
         FixedAllocator.

         @par Exception Safety Level
         Provides either strong-exception safety, or no-throw exception-safety
         level depending upon doThrow parameter.  The reason it provides two
         levels of exception safety is because it is used by both the nothrow
         and throwing new operators.  The underlying implementation will never
         throw of its own accord, but this can decide to throw if it does not
         allocate.  The only exception it should emit is std::bad_alloc.

         @par Allocation Failure
         If it does not allocate, it will call TrimExcessMemory and attempt to
         allocate again, before it decides to throw or return NULL.  Many
         allocators loop through several new_handler functions, and terminate
         if they can not allocate, but not this one.  It only makes one attempt
         using its own implementation of the new_handler, and then returns NULL
         or throws so that the program can decide what to do at a higher level.
         (Side note: Even though the C++ Standard allows allocators and
         new_handlers to terminate if they fail, the Loki allocator does not do
         that since that policy is not polite to a host program.)

         @param size # of bytes needed for allocation.
         @param doThrow True if this should throw if unable to allocate, false
          if it should provide no-throw exception safety level.
         @return NULL if nothing allocated and doThrow is false.  Else the
          pointer to an available block of memory.
         */
        void * Allocate( std::size_t size, bool doThrow );

        /** Deallocates a block of memory at a given place and of a specific
        size.  Complexity is almost always constant-time, and is O(C) only if
        it has to search for which Chunk deallocates.  This never throws.
         */
        void Deallocate( void * p, std::size_t size );

        /** Deallocates a block of memory at a given place but of unknown size
        size.  Complexity is O(F + C) where F is the count of FixedAllocator's
        in the pool, and C is the number of Chunks in all FixedAllocator's.  This
        does not throw exceptions.  This overloaded version of Deallocate is
        called by the nothow delete operator - which is called when the nothrow
        new operator is used, but a constructor throws an exception.
         */
        void Deallocate( void * p );

        /// Returns max # of bytes which this can allocate.
        inline std::size_t GetMaxObjectSize() const
        { return maxSmallObjectSize_; }

        /// Returns # of bytes between allocation boundaries.
        inline std::size_t GetAlignment() const { return objectAlignSize_; }

        /** Releases empty Chunks from memory.  Complexity is O(F + C) where F
        is the count of FixedAllocator's in the pool, and C is the number of
        Chunks in all FixedAllocator's.  This will never throw.  This is called
        by AllocatorSingleto::ClearExtraMemory, the new_handler function for
        Loki's allocator, and is called internally when an allocation fails.
        @return True if any memory released, or false if none released.
         */
        bool TrimExcessMemory( void );

        /** Returns true if anything in implementation is corrupt.  Complexity
         is O(F + C + B) where F is the count of FixedAllocator's in the pool,
         C is the number of Chunks in all FixedAllocator's, and B is the number
         of blocks in all Chunks.  If it determines any data is corrupted, this
         will return true in release version, but assert in debug version at
         the line where it detects the corrupted data.  If it does not detect
         any corrupted data, it returns false.
         */
        bool IsCorrupt( void ) const;

    private:
        /// Default-constructor is not implemented.
        SmallObjAllocator( void );
        /// Copy-constructor is not implemented.
        SmallObjAllocator( const SmallObjAllocator & );
        /// Copy-assignment operator is not implemented.
        SmallObjAllocator & operator = ( const SmallObjAllocator & );

        /// Pointer to array of fixed-size allocators.
        FixedAllocator * pool_;

        /// Largest object size supported by allocators.
        const std::size_t maxSmallObjectSize_;

        /// Size of alignment boundaries.
        const std::size_t objectAlignSize_;
    };


    /** @class AllocatorSingleton
        @ingroup SmallObjectGroupInternal
     This template class is derived from
     SmallObjAllocator in order to pass template arguments into it, and still
     have a default constructor for the singleton.  Each instance is a unique
     combination of all the template parameters, and hence is singleton only
     with respect to those parameters.  The template parameters have default
     values and the class has typedefs identical to both SmallObject and
     SmallValueObject so that this class can be used directly instead of going
     through SmallObject or SmallValueObject.  That design feature allows
     clients to use the new_handler without having the name of the new_handler
     function show up in classes derived from SmallObject or SmallValueObject.
     Thus, the only functions in the allocator which show up in SmallObject or
     SmallValueObject inheritance hierarchies are the new and delete
     operators.
    */
    template
    <
        std::size_t chunkSize = LOKI_DEFAULT_CHUNK_SIZE,
        std::size_t maxSmallObjectSize = LOKI_MAX_SMALL_OBJECT_SIZE,
        std::size_t objectAlignSize = LOKI_DEFAULT_OBJECT_ALIGNMENT
    >
    class AllocatorSingleton : public SmallObjAllocator
    {
    public:

        /// Returns reference to the singleton.
        inline static SmallObjAllocator & Instance( void )
        {
          return SmallObjAllocator::Instance(chunkSize, maxSmallObjectSize,
                                             objectAlignSize);
        }

    private:
        /// Copy-constructor is not implemented.
        AllocatorSingleton( const AllocatorSingleton & );
        /// Copy-assignment operator is not implemented.
        AllocatorSingleton & operator = ( const AllocatorSingleton & );
    };


    /** @class SmallObjectBase
        @ingroup SmallObjectGroup
     Base class for small object allocation classes.
     The shared implementation of the new and delete operators are here instead
     of being duplicated in both SmallObject or SmallValueObject, later just
     called Small-Objects.  This class is not meant to be used directly by clients,
     or derived from by clients. Class has no data members so compilers can
     use Empty-Base-Optimization.
     */
    template
    <
        std::size_t chunkSize,
        std::size_t maxSmallObjectSize,
        std::size_t objectAlignSize
    >
    class SmallObjectBase
    {

#if (LOKI_MAX_SMALL_OBJECT_SIZE != 0) && (LOKI_DEFAULT_CHUNK_SIZE != 0) && (LOKI_DEFAULT_OBJECT_ALIGNMENT != 0)

    public:
        /// Defines type of allocator singleton, must be public
        /// to handle singleton lifetime dependencies.
        typedef AllocatorSingleton< chunkSize,
            maxSmallObjectSize, objectAlignSize > ObjAllocatorSingleton;

    public:

        /// Throwing single-object new throws bad_alloc when allocation fails.
#ifdef _MSC_VER
        /// @note MSVC complains about non-empty exception specification lists.
        static void * operator new ( std::size_t size )
#else
        static void * operator new ( std::size_t size ) throw ( std::bad_alloc )
#endif
        {
            return ObjAllocatorSingleton::Instance().Allocate( size, true );
        }

        /// Non-throwing single-object new returns NULL if allocation fails.
        static void * operator new ( std::size_t size, const std::nothrow_t & ) throw ()
        {
            return ObjAllocatorSingleton::Instance().Allocate( size, false );
        }

        /// Placement single-object new merely calls global placement new.
        inline static void * operator new ( std::size_t size, void * place )
        {
            return ::operator new( size, place );
        }

        /// Single-object delete.
        static void operator delete ( void * p, std::size_t size ) throw ()
        {
            ObjAllocatorSingleton::Instance().Deallocate( p, size );
        }

        /** Non-throwing single-object delete is only called when nothrow
         new operator is used, and the constructor throws an exception.
         */
        static void operator delete ( void * p, const std::nothrow_t & ) throw()
        {
            ObjAllocatorSingleton::Instance().Deallocate( p );
        }

        /// Placement single-object delete merely calls global placement delete.
        inline static void operator delete ( void * p, void * place )
        {
            ::operator delete ( p, place );
        }

#ifdef LOKI_SMALL_OBJECT_USE_NEW_ARRAY

        /// Throwing array-object new throws bad_alloc when allocation fails.
#ifdef _MSC_VER
        /// @note MSVC complains about non-empty exception specification lists.
        static void * operator new [] ( std::size_t size )
#else
        static void * operator new [] ( std::size_t size )
            throw ( std::bad_alloc )
#endif
        {
            return ObjAllocatorSingleton::Instance().Allocate( size, true );
        }

        /// Non-throwing array-object new returns NULL if allocation fails.
        static void * operator new [] ( std::size_t size,
            const std::nothrow_t & ) throw ()
        {
            return ObjAllocatorSingleton::Instance().Allocate( size, false );
        }

        /// Placement array-object new merely calls global placement new.
        inline static void * operator new [] ( std::size_t size, void * place )
        {
            return ::operator new( size, place );
        }

        /// Array-object delete.
        static void operator delete [] ( void * p, std::size_t size ) throw ()
        {
            ObjAllocatorSingleton::Instance().Deallocate( p, size );
        }

        /** Non-throwing array-object delete is only called when nothrow
         new operator is used, and the constructor throws an exception.
         */
        static void operator delete [] ( void * p,
            const std::nothrow_t & ) throw()
        {
            ObjAllocatorSingleton::Instance().Deallocate( p );
        }

        /// Placement array-object delete merely calls global placement delete.
        inline static void operator delete [] ( void * p, void * place )
        {
            ::operator delete ( p, place );
        }
#endif  // #if use new array functions.

#endif  // #if default template parameters are not zero

    protected:
        inline SmallObjectBase() {}
        inline SmallObjectBase( const SmallObjectBase & ) {}
        inline SmallObjectBase & operator = ( const SmallObjectBase & )
        { return *this; }
        inline ~SmallObjectBase() {}
    }; // end class SmallObjectBase


    /** @class SmallObject
        @ingroup SmallObjectGroup
     SmallObject Base class for polymorphic small objects, offers fast
     allocations & deallocations.  Destructor is virtual and public.  Default
     constructor is trivial.   Copy-constructor and copy-assignment operator are
     not implemented since polymorphic classes almost always disable those
     operations.  Class has no data members so compilers can use
     Empty-Base-Optimization.
     */
    template
    <
        std::size_t chunkSize = LOKI_DEFAULT_CHUNK_SIZE,
        std::size_t maxSmallObjectSize = LOKI_MAX_SMALL_OBJECT_SIZE,
        std::size_t objectAlignSize = LOKI_DEFAULT_OBJECT_ALIGNMENT
    >
    class SmallObject : public SmallObjectBase< chunkSize,
            maxSmallObjectSize, objectAlignSize >
    {

    public:
        virtual ~SmallObject() {}
    protected:
        inline SmallObject() {}

    private:
        /// Copy-constructor is not implemented.
        SmallObject( const SmallObject & );
        /// Copy-assignment operator is not implemented.
        SmallObject & operator = ( const SmallObject & );
    }; // end class SmallObject


    /** @class SmallValueObject
        @ingroup SmallObjectGroup
     SmallValueObject Base class for small objects with value-type
     semantics - offers fast allocations & deallocations.  Destructor is
     non-virtual, inline, and protected to prevent unintentional destruction
     through base class.  Default constructor is trivial.   Copy-constructor
     and copy-assignment operator are trivial since value-types almost always
     need those operations.  Class has no data members so compilers can use
     Empty-Base-Optimization.
     */
    template
    <
        std::size_t chunkSize = LOKI_DEFAULT_CHUNK_SIZE,
        std::size_t maxSmallObjectSize = LOKI_MAX_SMALL_OBJECT_SIZE,
        std::size_t objectAlignSize = LOKI_DEFAULT_OBJECT_ALIGNMENT
    >
    class SmallValueObject : public SmallObjectBase< chunkSize,
            maxSmallObjectSize, objectAlignSize >
    {
    protected:
        inline SmallValueObject() {}
        inline SmallValueObject( const SmallValueObject & ) {}
        inline SmallValueObject & operator = ( const SmallValueObject & )
        { return *this; }
        inline ~SmallValueObject() {}
    }; // end class SmallValueObject

#else

    template
    <
        std::size_t chunkSize = LOKI_DEFAULT_CHUNK_SIZE,
        std::size_t maxSmallObjectSize = LOKI_MAX_SMALL_OBJECT_SIZE,
        std::size_t objectAlignSize = LOKI_DEFAULT_OBJECT_ALIGNMENT
    >
    class SmallObject {
    protected:
        inline SmallObject() {}
    private:
        /// Copy-constructor is not implemented.
        SmallObject( const SmallObject & );
        /// Copy-assignment operator is not implemented.
        SmallObject & operator = ( const SmallObject & );
    }; // end class SmallObject

    template
    <
        std::size_t chunkSize = LOKI_DEFAULT_CHUNK_SIZE,
        std::size_t maxSmallObjectSize = LOKI_MAX_SMALL_OBJECT_SIZE,
        std::size_t objectAlignSize = LOKI_DEFAULT_OBJECT_ALIGNMENT
    >
    class SmallValueObject {
    protected:
        inline SmallValueObject() {}
        inline SmallValueObject( const SmallValueObject & ) {}
        inline SmallValueObject & operator = ( const SmallValueObject & )
        { return *this; }
        inline ~SmallValueObject() {}
    }; // end class SmallValueObject

#endif  // OS_WIN

} // namespace ggadget

#endif // end file guardian

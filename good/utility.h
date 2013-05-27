//----------------------------------------------------------------------------------------------------------------
// Util classes: allocator, equal, less, pair and swap template function.
// Copyright (c) 2011 Borzh.
//----------------------------------------------------------------------------------------------------------------

#ifndef __GOOD_UTILITY_H__
#define __GOOD_UTILITY_H__


#include <new>
#include "good/defines.h"


// Disable unreferenced formal parameter warnings (at destroy() if T no need for destructor).
#pragma warning(push)
#pragma warning(disable: 4100)


namespace good
{


	//************************************************************************************************************
	/// Util class to compare elements.
	//************************************************************************************************************
	template <typename T>
	class equal
	{
	public:
		/// Default operator to compare 2 elements.
		bool operator() ( const T& tLeft, const T& tRight ) const
		{
			return (tLeft == tRight);
		}
	};


	//************************************************************************************************************
	/// Util class to know elements order.
	//************************************************************************************************************
	template <typename T>
	class less
	{
	public:
		/// Default operator to compare 2 elements.
		bool operator() ( const T& tLeft, const T& tRight ) const
		{
			return (tLeft < tRight);
		}
	};


	//************************************************************************************************************
	/// Util class to store 2 elements.
	//************************************************************************************************************
	template <typename T1, typename T2>
	class pair
	{
	public:
		typedef T1 first_type;
		typedef T2 second_type;

		T1 first;
		T2 second;
		
		/// Default constructor.
		pair(): first(T1()), second(T2()) {}

		/// Constructor by arguments.
		pair(const T1& t1, const T2& t2): first(t1), second(t2) {}

		/// Copy constructor.
		pair (const pair& other): first(other.first), second(other.second) {}
	};

	
	//************************************************************************************************************
	/// Util function to swap variables values.
	//************************************************************************************************************
	template <typename T>
	void swap(T& t1, T& t2)
	{
		T tmp(t1);
		t1 = t2;
		t2 = tmp;
	}


	//************************************************************************************************************
	/// Util function search for element in container.
	//************************************************************************************************************
	template <typename T, typename Iterator>
	Iterator find(Iterator itBegin, Iterator itEnd, const T& elem)
	{
		for (; itBegin != itEnd; ++itBegin)
			if (*itBegin == elem)
				return itBegin;
		return itEnd;
	}


	//************************************************************************************************************
	/** @brief Class for contruct/destroy and allocating/deallocating objects.
	 * Contains functions to allocate/deallocate enough space to contain some objects of type T. Note that 
	 * constructors/destructors of that objects are not being called when allocated/deallocated.
	 */
	//************************************************************************************************************
	template <typename T>
	class allocator
	{

	public:
		typedef T* pointer_t;
		typedef T& reference_t;
		typedef const T& const_reference_t;
		
		template <class _Other>
		struct rebind
		{
			typedef allocator<_Other> other;
		};

		//--------------------------------------------------------------------------------------------------------
		/// Call copy constructor for object allocated in memory 'where'.
		//--------------------------------------------------------------------------------------------------------
		void construct(pointer_t pWhere, const_reference_t tCopy) const
		{
			new (pWhere) T(tCopy);
		}
		
		//--------------------------------------------------------------------------------------------------------
		/// Call destructor for object allocated in memory at location pWhere.
		//--------------------------------------------------------------------------------------------------------
		void destroy(pointer_t pWhere) const
		{
			pWhere->~T();
		}

		//--------------------------------------------------------------------------------------------------------
		/// Allocate memory for iCount objects of type T.
		//--------------------------------------------------------------------------------------------------------
		pointer_t allocate( int iSize ) const
		{
			return (pointer_t) malloc( iSize * sizeof(T) );
		}
		
		//--------------------------------------------------------------------------------------------------------
		/// Reallocate memory for iNewCount objects of type T.
		//--------------------------------------------------------------------------------------------------------
		pointer_t reallocate( pointer_t pOld, int iNewSize, int /*iOldSize*/ ) const
		{
			return (pointer_t) realloc( pOld, iNewSize * sizeof(T) );
		}
		
		//--------------------------------------------------------------------------------------------------------
		/// Deallocate memory pointing to an object of type T.
		//--------------------------------------------------------------------------------------------------------
		void deallocate( pointer_t pPtr, int /*iSize*/ ) const
		{
			free(pPtr);
		}
	};


	//************************************************************************************************************
	/// Class that holds pointer that is erased at destructor.
	//************************************************************************************************************
	template < typename T >
	class auto_ptr
	{

	public:
		//--------------------------------------------------------------------------------------------------------
		/// Default contructor (null pointer).
		//--------------------------------------------------------------------------------------------------------
		auto_ptr(): ptr(NULL) {}
		
		//--------------------------------------------------------------------------------------------------------
		/// Contructor by copy, other looses pointer.
		//--------------------------------------------------------------------------------------------------------
		auto_ptr(const auto_ptr& other): ptr(other.ptr) { ((auto_ptr&)(other)).ptr = NULL; }
		
		//--------------------------------------------------------------------------------------------------------
		/// Contructor using existing pointer.
		//--------------------------------------------------------------------------------------------------------
		auto_ptr(T* p): ptr(p) {}
		
		//--------------------------------------------------------------------------------------------------------
		/// Destructor. If decrementing m_iCounter gives 0, will free current pointer.
		//--------------------------------------------------------------------------------------------------------
		~auto_ptr() { reset(NULL); }
		
		//--------------------------------------------------------------------------------------------------------
		/// Get true if current pointer is not NULL.
		//--------------------------------------------------------------------------------------------------------
		bool operator()() { return (ptr); }

		//--------------------------------------------------------------------------------------------------------
		/// Get current pointer. Should be used only for checking for NULL.
		//--------------------------------------------------------------------------------------------------------
		T* get() { return ptr; }

		//--------------------------------------------------------------------------------------------------------
		/// Get current pointer. Should be used only for checking for NULL.
		//--------------------------------------------------------------------------------------------------------
		const T* get() const { return ptr; }

		//--------------------------------------------------------------------------------------------------------
		/// Reset current pointer.
		//--------------------------------------------------------------------------------------------------------
		void reset( T* p = NULL )
		{
			if (ptr)
				delete ptr;
			ptr = p;
		}

		//--------------------------------------------------------------------------------------------------------
		/// Perform operation on pointer. Assertion is used to ensure that pointer is valid.
		//--------------------------------------------------------------------------------------------------------
		T* operator->() { DebugAssert(ptr); return ptr; }
		
		//--------------------------------------------------------------------------------------------------------
		/// Perform operation on pointer. Assertion is used to ensure that pointer is valid.
		//--------------------------------------------------------------------------------------------------------
		const T* operator->() const { DebugAssert(ptr); return ptr; }
		
		//--------------------------------------------------------------------------------------------------------
		/// Copy operator.
		//--------------------------------------------------------------------------------------------------------
		auto_ptr& operator=( T* p )
		{
			reset( p );
			return *this;
		}

		//--------------------------------------------------------------------------------------------------------
		/// Copy operator.
		//--------------------------------------------------------------------------------------------------------
		auto_ptr& operator=( const auto_ptr& other )
		{
			reset( other.ptr );
			((auto_ptr&)(other)).ptr = NULL;
			return *this;
		}
		
	protected:
		T* ptr;
	};


	//************************************************************************************************************
	/// Class that holds pointer shared among application. Not thread-safe.
	//************************************************************************************************************
	template < typename T, typename Alloc = good::allocator<int> >
	class shared_ptr
	{
	public:
		typedef T* pointer_t;
		typedef const T* const_pointer_t;
		typedef T& reference_t;
		typedef const T& const_reference_t;
		typedef int* counter_t;

		//--------------------------------------------------------------------------------------------------------
		/// Default contructor (null pointer).
		//--------------------------------------------------------------------------------------------------------
		shared_ptr(): m_iCounter(0), ptr(NULL) {}
		
		//--------------------------------------------------------------------------------------------------------
		/// Contructor by copy another shared_p. Will increment m_iCounter.
		//--------------------------------------------------------------------------------------------------------
		shared_ptr(const shared_ptr& ptr): m_iCounter(ptr.m_iCounter), ptr(ptr.ptr)
		{
			if (m_iCounter)
				++(*m_iCounter);
		}
		
		//--------------------------------------------------------------------------------------------------------
		/// Contructor using existing pointer.
		//--------------------------------------------------------------------------------------------------------
		shared_ptr(pointer_t ptr): ptr(ptr)
		{
			m_iCounter = m_cAlloc.allocate(1);
			*m_iCounter = 1;
		}
		
		//--------------------------------------------------------------------------------------------------------
		/// Destructor. If decrementing m_iCounter gives 0, will free current pointer.
		//--------------------------------------------------------------------------------------------------------
		~shared_ptr()
		{
			reset(NULL, NULL);
		}
		
		//--------------------------------------------------------------------------------------------------------
		/// Get true if current pointer is not NULL.
		//--------------------------------------------------------------------------------------------------------
		bool operator()() { return ptr; }

		//--------------------------------------------------------------------------------------------------------
		/// Get current pointer. Should be used only for checking for NULL.
		//--------------------------------------------------------------------------------------------------------
		pointer_t get() { return ptr; }

		//--------------------------------------------------------------------------------------------------------
		/// Get current pointer. Should be used only for checking for NULL.
		//--------------------------------------------------------------------------------------------------------
		const_pointer_t get() const { return ptr; }

		//--------------------------------------------------------------------------------------------------------
		/// Reset current pointer.
		//--------------------------------------------------------------------------------------------------------
		void reset( pointer_t ptr = NULL, counter_t c = NULL )
		{
			if (m_iCounter)
			{
				DebugAssert(ptr); 
				if ( (--(*m_iCounter)) == 0 )
				{
					m_cAlloc.deallocate(m_iCounter, 1);
					delete ptr;
				}
			}
			ptr = ptr;
			m_iCounter = c;
		}

		//--------------------------------------------------------------------------------------------------------
		/// Perform operation on pointer. Assertion is used to ensure that pointer is valid.
		//--------------------------------------------------------------------------------------------------------
		pointer_t operator->() { DebugAssert(ptr); return ptr; }
		
		//--------------------------------------------------------------------------------------------------------
		/// Perform operation on pointer. Assertion is used to ensure that pointer is valid.
		//--------------------------------------------------------------------------------------------------------
		const_pointer_t operator->() const { DebugAssert(ptr); return ptr; }
		
		//--------------------------------------------------------------------------------------------------------
		/// Copy operator.
		//--------------------------------------------------------------------------------------------------------
		shared_ptr& operator=( pointer_t ptr )
		{
			reset( ptr, m_cAlloc.allocate(1) );
			return *this;
		}
		//--------------------------------------------------------------------------------------------------------
		/// Copy operator.
		//--------------------------------------------------------------------------------------------------------
		shared_ptr& operator=( const shared_ptr& ptr )
		{
			reset( ptr.ptr, ptr.m_iCounter );
			if (m_iCounter)
				++(*m_iCounter);
			return *this;
		}
		
	protected:
		Alloc m_cAlloc;           // Allocator class.
		counter_t m_iCounter;     // Shared m_iCounter.
		pointer_t ptr;         // Pointer himself.
	};


} // namespace good


#pragma warning(pop)


#endif // __GOOD_UTILITY_H__

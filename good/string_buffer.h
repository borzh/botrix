//----------------------------------------------------------------------------------------------------------------
// String buffer implementation.
// Copyright (c) 2011 Borzh.
//----------------------------------------------------------------------------------------------------------------

#ifndef __GOOD_STRING_BUFFER_H__
#define __GOOD_STRING_BUFFER_H__


#include "good/string.h"


#define DEFAULT_STRING_BUFFER_ALLOC  0                 ///< Size of string buffer allocated by default.


// Disable obsolete warnings.
#pragma warning(push)
#pragma warning(disable: 4996)


namespace good
{

	
	//************************************************************************************************************
	/// Class that holds mutable string.
	/** Note that assignment/copy constructor actually copies given string into buffer's
	 * content, so it is very different from string.
	 * Avoid using in arrays, as it is expensive (it will copy content, not pointer as in case of string). 
	 * Buffer will double his size when resized. */
	//************************************************************************************************************
	template< class Char, class Alloc = allocator<Char> >
	class base_string_buffer: public base_string<Char, Alloc>
	{
	public:
		typedef string base_class;



		//--------------------------------------------------------------------------------------------------------
		/// Default constructor with optional capacity parameter.
		//--------------------------------------------------------------------------------------------------------
		base_string_buffer( int iCapacity = DEFAULT_STRING_BUFFER_ALLOC )
		{
			DebugAssert( iCapacity >= 0 );
#ifdef DEBUG_STRING_PRINT
			printf( "base_string_buffer constructor, reserve %d\n", iCapacity );
#endif
			Init(iCapacity);
		}

		//--------------------------------------------------------------------------------------------------------
		/// Constructor giving buffer with capacity.
		//--------------------------------------------------------------------------------------------------------
		base_string_buffer( const Char* sbBuffer, int iCapacity, bool bDeallocate )
		{
#ifdef DEBUG_STRING_PRINT
			printf( "base_string_buffer constructor with buffer, reserved %d\n", iCapacity );
#endif
			DebugAssert(iCapacity > 0);

			m_iCapacity = iCapacity;
			m_iStatic = !bDeallocate;
			m_iSize = 0;
			m_pBuffer = (Char*)sbBuffer;
			m_pBuffer[0] = 0;
		}

		//--------------------------------------------------------------------------------------------------------
		/// Copy from string constructor.
		//--------------------------------------------------------------------------------------------------------
		base_string_buffer( const string& sOther )
		{
#ifdef DEBUG_STRING_PRINT
			printf( "base_string_buffer copy constructor, str: %s\n", sOther.c_str() );
#endif
			int len = sOther.length();
			Init( MAX2(len+1, DEFAULT_STRING_BUFFER_ALLOC) );
			copy_contents( sOther.c_str(), len );
			m_pBuffer[len] = 0;
			m_iSize = len;
		}

		//--------------------------------------------------------------------------------------------------------
		/// Operator =. Copy other string contents into buffer.
		//--------------------------------------------------------------------------------------------------------
		base_string_buffer& operator=( const string& sOther )
		{
#ifdef DEBUG_STRING_PRINT
			printf( "base_string_buffer operator=, %s\n", sOther.c_str() );
#endif
			assign(sOther);
			return *this;
		}

		//--------------------------------------------------------------------------------------------------------
		/// Copy other string contents into buffer.
		//--------------------------------------------------------------------------------------------------------
		void assign( const string& sOther )
		{
			assign( sOther.c_str(), sOther.length() );
		}

		//--------------------------------------------------------------------------------------------------------
		/// Copy other string contents into buffer.
		//--------------------------------------------------------------------------------------------------------
		void assign( const Char* szOther, int iOtherSize )
		{
			if ( iOtherSize < 0)
				iOtherSize = strlen(szOther);

			if ( m_iStatic )
			{
				DebugAssert( iOtherSize <= m_iCapacity+1 );
			}
			else
			{
				if ( iOtherSize >= m_iCapacity )
				{
					deallocate();
					reserve( iOtherSize+1 );
				}
			}
			m_iSize = 0;
			copy_contents( szOther, iOtherSize );
			m_pBuffer[iOtherSize] = 0;
		}

		//--------------------------------------------------------------------------------------------------------
		/// Get base_string_buffer allocated capacity.
		//--------------------------------------------------------------------------------------------------------
		int capacity() const { return m_iCapacity; }

		//--------------------------------------------------------------------------------------------------------
		/// Append string to buffer.
		//--------------------------------------------------------------------------------------------------------
		base_string_buffer& append( Char c )
		{
			increment(1);
			m_pBuffer[m_iSize++] = c;
			m_pBuffer[m_iSize] = 0;
			return *this;
		}

		//--------------------------------------------------------------------------------------------------------
		/// Append string to buffer.
		//--------------------------------------------------------------------------------------------------------
		base_string_buffer& append( const string& sOther )
		{
			increment( sOther.length() );
			copy_contents( sOther.c_str(), sOther.length(), m_iSize);
			m_pBuffer[m_iSize] = 0;
			return *this;
		}

		//--------------------------------------------------------------------------------------------------------
		/// Insert string to buffer at requiered position. pos must be in range [ 0 .. length() ].
		//--------------------------------------------------------------------------------------------------------
		base_string_buffer& insert( const string& sOther, int iPos )
		{
			DebugAssert( iPos <= length() );
			DebugAssert( sOther.length() > 0 );
			increment( sOther.length() );
			memmove( &m_pBuffer[iPos + sOther.length()], &m_pBuffer[iPos], (length() - iPos) * sizeof(Char) );
			copy_contents( sOther.c_str(), sOther.length(), iPos );
			m_pBuffer[m_iSize] = 0;
			return *this;
		}

		//--------------------------------------------------------------------------------------------------------
		/// Erase iCount characters from buffer at requiered position iPos.
		//--------------------------------------------------------------------------------------------------------
		base_string_buffer& erase( int iPos = 0, int iCount = npos )
		{
			if (iCount == npos)
				iCount = length() - iPos;
			if (iCount == 0)
				return *this;
			DebugAssert( (iCount > 0) && (iPos < length()) && (iPos+iCount <= length()) );
			memmove( &m_pBuffer[iPos], &m_pBuffer[iPos+iCount], (length() - iPos + 1) * sizeof(Char) );
			m_iSize -= iCount;
			return *this;
		}


		//--------------------------------------------------------------------------------------------------------
		/// Replace all occurencies of string sFrom in buffer by sTo.
		//--------------------------------------------------------------------------------------------------------
		base_string_buffer& replace( const base_string& sFrom, const base_string& sTo )
		{
			int pos = 0, size = length();
			int toL = sTo.length(), fromL = sFrom.length(), diff = toL - fromL;
			while ( (pos = find(sFrom, pos)) != npos )
			{
				if ( diff > 0 )
					increment( diff );
				memmove( &m_pBuffer[pos+fromL], &m_pBuffer[pos+toL], (size - iPos - fromL) * sizeof(Char) );
				strncpy( &m_pBuffer[pos], sTo.c_str() );
				m_iSize += diff;
				pos += toL;
			}
			return *this;
		}


		//--------------------------------------------------------------------------------------------------------
		/** Check if reserve is needed and reallocate internal buffer.
		 * Note that buffer never shrinks. */
		//--------------------------------------------------------------------------------------------------------
		void reserve( int iCapacity )
		{
			DebugAssert( iCapacity > 0 );
			if ( m_pBuffer && (m_iCapacity >= iCapacity) ) return;
			if (m_iStatic)
			{
				m_pBuffer = (Char*) malloc( iCapacity * sizeof(Char) );
				m_iStatic = false;
			}
			else
				m_pBuffer = (Char*) realloc( m_pBuffer, iCapacity * sizeof(Char) );
			m_iCapacity = iCapacity;
#ifdef DEBUG_STRING_PRINT
			printf( "base_string_buffer reserve(): %d\n", m_iCapacity );
#endif
		}


	protected:
		//--------------------------------------------------------------------------------------------------------
		// malloc() buffer of size iAlloc.
		//--------------------------------------------------------------------------------------------------------
		void Init( int iCapacity )
		{
			if ( iCapacity > 0 )
			{
				m_pBuffer = (Char*) malloc( iCapacity * sizeof(Char) );
				m_pBuffer[0] = 0;
			}
			else
				m_pBuffer = NULL;
			m_iCapacity = iCapacity;
			m_iSize = 0;
			m_iStatic = false;
		}

		//--------------------------------------------------------------------------------------------------------
		// Copy string to already allocated buffer at pos. Don't include trailing 0. Increment size.
		//--------------------------------------------------------------------------------------------------------
		void copy_contents( const Char* szOther, int iOtherLen, int pos = 0 )
		{
			DebugAssert( iOtherLen >= 0 && pos >= 0 );
			DebugAssert( m_iCapacity >= pos + iOtherLen + 1 );
			strncpy( &m_pBuffer[pos], szOther, iOtherLen * sizeof(Char) );
			m_iSize += iOtherLen;
		}

		//--------------------------------------------------------------------------------------------------------
		/// Increment buffer size.
		//--------------------------------------------------------------------------------------------------------
		void increment( int iBySize )
		{
			int desired = m_iSize + iBySize + 1; // Trailing 0.
			if ( desired > m_iCapacity )
			{
				DebugAssert( !m_iStatic );
				iBySize = m_iCapacity;
				if (iBySize == 0)
					iBySize = DEFAULT_STRING_BUFFER_ALLOC;
				do {
					iBySize = iBySize<<1; // Double buffer size.
				} while (iBySize < desired);
				reserve(iBySize);
			}
		}

		//--------------------------------------------------------------------------------------------------------
		// Deallocate if needed.
		//--------------------------------------------------------------------------------------------------------
		void deallocate()
		{
#ifdef DEBUG_STRING_PRINT
			DebugPrint( "base_string_buffer deallocate(): %s; free: %d\n", m_pBuffer?m_pBuffer:"null", !m_iStatic && m_pBuffer );
#endif
			if ( !m_iStatic )
				free(m_pBuffer);
			m_pBuffer = NULL;
		}

	protected:
		int m_iCapacity; // Size of allocated buffer.
	};


	typedef good::base_string_buffer<char> string_buffer;

} // namespace good


#pragma warning(pop) // Restore warnings.


#endif // __GOOD_STRING_BUFFER_H__
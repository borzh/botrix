//----------------------------------------------------------------------------------------------------------------
// String implementation.
// Copyright (c) 2011 Borzh.
//----------------------------------------------------------------------------------------------------------------

#ifndef __GOOD_STRING_H__
#define __GOOD_STRING_H__


#include <stdlib.h>
#include <string.h>


#include "good/utility.h"


// Define this to see debug prints of string memory allocation and deallocation.
//#define DEBUG_STRING_PRINT


// Disable obsolete warnings.
#pragma warning(push)
#pragma warning(disable: 4996)


namespace good
{


	/*enum asdasd {
		EStringCopy,      ///<
		EStringMove,      ///<
		EStringForceCopy, ///<
		EStringForceMove, ///<
	};
	typedef int TStringasdasd;*/

	//************************************************************************************************************
	/// Class that holds a string of characters.
	/** Note that in this version of string assignment or copy constructor "steal" internal buffer so use 
	 *  carefully (i.e. it behaves like std::auto_ptr). Use duplicate() to obtain newly allocated string.
	 *  Note that c_str() will never return NULL. */
	//************************************************************************************************************
	template <
		typename Char = Char,
		typename Alloc = allocator<Char>
	>
	class base_string
	{
	public:
		
		static const int npos = -1;   ///< Invalid position in string.

		//--------------------------------------------------------------------------------------------------------
		/// Default constructor.
		//--------------------------------------------------------------------------------------------------------
		base_string(): m_pBuffer(""), m_iSize(), m_iStatic(1)
		{
#ifdef DEBUG_STRING_PRINT
			DebugPrint( "base_string default constructor\n" );
#endif
		}

		//--------------------------------------------------------------------------------------------------------
		/// Copy constructor. Move all parameters into constructed string.
		//--------------------------------------------------------------------------------------------------------
		base_string( const base_string& other, bool bCopy = false ): m_pBuffer(other.m_pBuffer), m_iSize(other.m_iSize), m_iStatic(other.m_iStatic)
		{
#ifdef DEBUG_STRING_PRINT
			DebugPrint( "base_string copy constructor: %s, copy %d.\n", other.c_str() );
#endif
			if ( !m_iStatic )
			{
				if ( bCopy )
				{
					m_iStatic = true; // Force not to deallocate other string.
					assign(other, true);
				}
				else
				{
					((base_string&)other).m_iSize = 0;
					((base_string&)other).m_pBuffer = "";
					((base_string&)other).m_iStatic = 1;
				}
			}
		}

		//--------------------------------------------------------------------------------------------------------
		/// Constructor by 0-terminating string. Make sure that iSize reflects string size properly or is npos.
		//--------------------------------------------------------------------------------------------------------
		base_string( const Char* szStr, bool bCopy = false, bool bDealloc = false, int iSize = npos )
		{
#ifdef DEBUG_STRING_PRINT
			DebugPrint( "base_string constructor: %s, copy %d, dealloc %d\n", szStr, bCopy, bDealloc );
#endif
			m_iSize = ( iSize == npos ) ? strlen(szStr) : iSize;

			if ( m_iSize > 0 ) // szStr is not null nor empty.
			{
				if ( bCopy )
				{
					copy_contents(szStr);
					m_iStatic = 0;
				}
				else
				{
					m_pBuffer = (Char*)szStr;
					m_iStatic = !bDealloc;
				}
			}
			else
			{
				m_pBuffer = "";
				m_iStatic = 1;
			}
		}

		//--------------------------------------------------------------------------------------------------------
		/// Destructor.
		//--------------------------------------------------------------------------------------------------------
		virtual ~base_string() { deallocate(); }

		//--------------------------------------------------------------------------------------------------------
		/// Get length of this string.
		//--------------------------------------------------------------------------------------------------------
		int length() const { return m_iSize; }
		
		//--------------------------------------------------------------------------------------------------------
		/// Get length of this string.
		//--------------------------------------------------------------------------------------------------------
		int size() const { return m_iSize; }
		
		//--------------------------------------------------------------------------------------------------------
		/// Get 0-terminating string.
		//--------------------------------------------------------------------------------------------------------
		const Char* c_str() const { return m_pBuffer; }
		
		//--------------------------------------------------------------------------------------------------------
		/// Const array subscript.
		//--------------------------------------------------------------------------------------------------------
		const Char& operator[] ( int iIndex ) const
		{
			DebugAssert( (0 <= iIndex) && (iIndex < m_iSize) );
			return m_pBuffer[iIndex];
		}
		
		//--------------------------------------------------------------------------------------------------------
		/// Array subscript.
		//--------------------------------------------------------------------------------------------------------
		Char& operator[] ( int iIndex )
		{
			DebugAssert( (0 <= iIndex) && (iIndex < m_iSize) );
			return m_pBuffer[iIndex];
		}
		
		//--------------------------------------------------------------------------------------------------------
		/// Assign this string to another one. Note that by default this will move content, not copy it.
		//--------------------------------------------------------------------------------------------------------
		base_string& assign( const Char* s, int iSize = npos, bool bCopy = false )
		{
			if ( iSize == npos )
				iSize = strlen(s);

			if ( bCopy )
			{
				if ( m_iStatic )
					m_pBuffer = m_cAlloc.allocate(iSize+1);
				else if (m_iSize < iSize)
					m_pBuffer = m_cAlloc.reallocate(m_pBuffer, (iSize+1)*sizeof(Char), (m_iSize+1)*sizeof(Char));
				memcpy(m_pBuffer, s, (iSize + 1)*sizeof(Char) );
			}
			else
				m_pBuffer = (Char*)s;

			m_iStatic = !bCopy;
			m_iSize = iSize;

			return *this;
		}

		//--------------------------------------------------------------------------------------------------------
		/// Assign this string to another one. Note that by default this will move content, not copy it.
		//--------------------------------------------------------------------------------------------------------
		base_string& assign( const base_string& other, bool bCopy = false )
		{
#ifdef DEBUG_STRING_PRINT
			DebugPrint( "base_string assign: %s\n", other.c_str() );
#endif
			if ( bCopy )
				return assign( other.c_str(), other.size(), bCopy );

			deallocate();
			m_pBuffer = other.m_pBuffer;
			m_iSize = other.m_iSize;
			m_iStatic = other.m_iStatic;
			if ( !m_iStatic )
			{
				((base_string&)other).m_pBuffer = "";
				((base_string&)other).m_iSize = 0;
				((base_string&)other).m_iStatic = 1;
			}
			return *this;
		}
		
		//--------------------------------------------------------------------------------------------------------
		/// Operator =. Note that this operator moves content, not copies it.
		//--------------------------------------------------------------------------------------------------------
		base_string& operator= ( const base_string& other )
		{
			return assign(other);
		}
		
		//--------------------------------------------------------------------------------------------------------
		/// String comparison.
		//--------------------------------------------------------------------------------------------------------
		bool operator< ( const base_string& other ) const
		{
			DebugAssert( (c_str() !=  NULL) && (other.c_str() !=  NULL) );
			return strcmp(c_str(), other.c_str()) < 0;
		}

		//--------------------------------------------------------------------------------------------------------
		/// Equality operator.
		//--------------------------------------------------------------------------------------------------------
		bool operator== ( const base_string& other ) const
		{
			return ( m_iSize == other.m_iSize ) && ( ( c_str() == other.c_str() )  ||  ( strcmp( c_str(), other.c_str() ) == 0 ) );
		}

		//--------------------------------------------------------------------------------------------------------
		/// Equality operator.
		//--------------------------------------------------------------------------------------------------------
		bool operator== ( const Char* other ) const
		{
			DebugAssert(other != NULL);
			return ( c_str() == other )  ||  ( strcmp( c_str(), other ) == 0 );
		}

		//--------------------------------------------------------------------------------------------------------
		/// Not equality operator.
		//--------------------------------------------------------------------------------------------------------
		bool operator!= ( const base_string& other ) const
		{
			return !(*this == other);
		}

		//--------------------------------------------------------------------------------------------------------
		/// Not equality operator.
		//--------------------------------------------------------------------------------------------------------
		bool operator!= ( const Char* other ) const
		{
			return !(*this == other);
		}

		//--------------------------------------------------------------------------------------------------------
		/// Get new string concatinating this string and sRight.
		//--------------------------------------------------------------------------------------------------------
		base_string operator+ ( const base_string& sRight ) const
		{
			return concat_with( sRight.c_str(), sRight.m_iSize );
		}
		
		//--------------------------------------------------------------------------------------------------------
		/// Get new string concatinating this string and szRight.
		//--------------------------------------------------------------------------------------------------------
		base_string operator+ ( const Char* szRight ) const
		{
			DebugAssert(szRight);
			return concat_with( szRight, strlen(szRight) );
		}
		
		//--------------------------------------------------------------------------------------------------------
		/// Return true if this string starts with sStr.
		//--------------------------------------------------------------------------------------------------------
		bool starts_with( const base_string& sStr ) const
		{
			return strncmp( c_str(), sStr.c_str(), MIN2(sStr.m_iSize, m_iSize) ) == 0;
		}
		
		//--------------------------------------------------------------------------------------------------------
		/// Return true if this string starts with Char c.
		//--------------------------------------------------------------------------------------------------------
		bool starts_with( Char c ) const
		{
			DebugAssert( m_iSize > 0 );
			return m_pBuffer[0] == c;
		}
		
		//--------------------------------------------------------------------------------------------------------
		/// Return true if this string ends with sStr.
		//--------------------------------------------------------------------------------------------------------
		bool ends_with( const base_string& sStr ) const
		{
			if (sStr.m_iSize > m_iSize) return false;
			return strncmp( &m_pBuffer[m_iSize - sStr.m_iSize], sStr.c_str(), sStr.m_iSize ) == 0;
		}
		
		//--------------------------------------------------------------------------------------------------------
		/// Return true if a string ends with Char c.
		//--------------------------------------------------------------------------------------------------------
		bool ends_with( Char c ) const
		{
			DebugAssert( m_iSize > 0 );
			return m_pBuffer[m_iSize-1] == c;
		}

		//--------------------------------------------------------------------------------------------------------
		/// Return lowercase string.
		//--------------------------------------------------------------------------------------------------------
		base_string& lower_case()
		{
			for ( int i = 0; i < m_iSize; ++i )
				if ( ('A' <= m_pBuffer[i]) && (m_pBuffer[i] <= 'Z') )
					m_pBuffer[i] = m_pBuffer[i] - 'A' + 'a';
			return *this;
		}

		//--------------------------------------------------------------------------------------------------------
		/// Get string by duplicating this string.
		//--------------------------------------------------------------------------------------------------------
		base_string duplicate() const
		{
			int len = m_iSize;
			Char* buffer = m_cAlloc.allocate(len+1);
			strncpy( buffer, m_pBuffer, (len + 1) * sizeof(Char) );
			return base_string( buffer, false, true, len );
		}

		//--------------------------------------------------------------------------------------------------------
		/// Remove leading and trailing whitespaces(space, tab, line feed - LF, carriage return - CR).
		//--------------------------------------------------------------------------------------------------------
		base_string& trim()
		{
			if (m_iSize == 0)
				return *this;
			int begin = 0, end = m_iSize - 1;

			for (; begin <= end; ++begin)
			{
				Char c = m_pBuffer[begin];
				if ( (c != ' ') && (c != '\t') && (c != '\n') && (c != '\r') )
					break;
			}

			for (; begin <= end; --end)
			{
				Char c = m_pBuffer[end];
				if ( (c != ' ') && (c != '\t') && (c != '\n') && (c != '\r') )
					break;
			}

			m_iSize = end - begin + 1;
			if (begin > 0)
				memmove(m_pBuffer, &m_pBuffer[begin], m_iSize);
			m_pBuffer[m_iSize] = 0;
			return *this;
		}

		//--------------------------------------------------------------------------------------------------------
		/// Process escape characters: \n, \r, \t, \0, else \ and next Char are transformed to that Char.
		//--------------------------------------------------------------------------------------------------------
		base_string& escape()
		{
			Char* buf = m_pBuffer;

			// Skip start sequence.
			int start = find('\\'), end = start, count = 0;
			while ( 0 <= end && end < m_iSize)
			{
				end++;
				count++;
				if (buf[end] == 'n')
					buf[start] = '\n';
				else if (buf[end] == 'r')
					buf[start] = '\r';
				else if (buf[end] == 't')
					buf[start] = '\t';
				else if (buf[end] == '0')
					buf[start] = 0;
				else
					buf[start] = buf[end];

				start++; end++;
				while ( (end < m_iSize) && (buf[end] != '\\') )
					buf[start++] = buf[end++];
			};

			m_iSize -= count;
			buf[m_iSize] = 0;
			return *this;
		}

		//--------------------------------------------------------------------------------------------------------
		/// Find first occurrence of string str in this string.
		//--------------------------------------------------------------------------------------------------------
		int find( const base_string& str, int iFrom = 0 ) const
		{
			DebugAssert( iFrom <= m_iSize );
			if ( m_iSize - iFrom < str.m_iSize )
				return npos;

			Char* result = strstr( &m_pBuffer[iFrom], str.c_str() );
			if (result)
				return (int)( result - m_pBuffer );
			else
				return npos;
		}

		//--------------------------------------------------------------------------------------------------------
		/// Find first occurrence of Char c in this string.
		//--------------------------------------------------------------------------------------------------------
		int find( Char c, int iFrom = 0 ) const
		{
			DebugAssert( iFrom <= m_iSize );
			Char* result = strchr( &m_pBuffer[iFrom], (int)c );
			if (result)
				return (int)( result - m_pBuffer );
			else
				return npos;
		}

		//--------------------------------------------------------------------------------------------------------
		/// Find last occurrence of Char c in this string.
		//--------------------------------------------------------------------------------------------------------
		int rfind( Char c ) const
		{
			for ( Char* pCurr = m_pBuffer + m_iSize - 1; pCurr >= m_pBuffer; --pCurr )
				if ( *pCurr == c )
					return pCurr - m_pBuffer;
			return npos;
		}

		//--------------------------------------------------------------------------------------------------------
		/// Get substring from position iFrom and size iSize.
		/** If bAlloc is false then it will modify current string inserting \0 character at iFrom + iSize + 1. */
		//--------------------------------------------------------------------------------------------------------
		base_string substr( int iFrom, int iSize = npos, bool bAlloc = true ) const
		{
			int maxSize = m_iSize - iFrom;
			if ( iSize < 0 )
				iSize = maxSize;

			DebugAssert( iFrom + iSize <= m_iSize );

			if (bAlloc)
			{
				Char* buffer = m_cAlloc.allocate(iSize+1);
				strncpy( buffer, &m_pBuffer[iFrom], iSize );
				buffer[iSize] = 0;
				return base_string( buffer, false, true, iSize );
			}
			else
			{
				m_pBuffer[iFrom + iSize] = 0;
				return base_string( &m_pBuffer[iFrom], false, false, iSize );
			}
		}

		//--------------------------------------------------------------------------------------------------------
		/// Split string into several strings by separator, optionally trimming resulting strings. Put result in given array.
		//--------------------------------------------------------------------------------------------------------
		template <template <typename, typename> class Container>
		void split( Container<base_string, typename Alloc::template rebind<base_string>::other>& aContainer, Char separator = ' ', bool bTrim = false/*, bool bAlloc = true*/ ) const
		{
			int start = 0;
			int end;
			do
			{
				end = find(separator, start);
				base_string s = substr(start, end - start);
				if (bTrim)
					s.trim();
				aContainer.push_back(s);
				start = end+1;
			} while ( end != npos );
		}


		//--------------------------------------------------------------------------------------------------------
		/// Split string into several strings by separator, optionally trimming resulting strings.
		//--------------------------------------------------------------------------------------------------------
		template <template <typename, typename> class Container>
		Container<base_string, typename Alloc::template rebind<base_string>::other> split( Char separator = ' ', bool bTrim = false/*, bool bAlloc = true*/ ) const
		{
			Container<base_string, typename Alloc::template rebind<base_string>::other> result;
			split<Container>(result, separator, bTrim);
			return result;
		}


		//--------------------------------------------------------------------------------------------------------
		/// Split string into several strings by separators, optionally trimming resulting strings.
		//--------------------------------------------------------------------------------------------------------
		template <template <typename, typename> class Container>
		Container<base_string, typename Alloc::template rebind<base_string>::other> split( const base_string& separators, bool bTrim/*, bool bAlloc = true*/ ) const
		{
			Container<base_string, typename Alloc::template rebind<base_string>::other> result;
			int start = 0, end = 0;
			while ( end < size() )
			{
				bool bFound = false;
				for ( int i=0; i<separators.size(); ++i )
					if ( separators[i] == m_pBuffer[end] )
					{
						base_string s = substr(start, end - start);
						if (bTrim)
							s.trim();
						result.push_back(s);
						start = end+1;
						break;
					}
				end++;
			}
			return result;
		}


	public: // Static methods.
		//--------------------------------------------------------------------------------------------------------
		/// Get string by adding two 0-terminating strings.
		//--------------------------------------------------------------------------------------------------------
		static base_string concatenate( const base_string& s1, const base_string& s2 )
		{
			int len3 = s1.m_iSize + s2.m_iSize;
			Char* buffer = m_cAlloc.allocate(len3 + 1);
			if (s1.size() > 0)
				strncpy( buffer, s1.c_str(), s1.m_iSize * sizeof(Char) );
			if (s2.size() > 0)
				strncpy( &buffer[s1.m_iSize], s2.c_str(), s2.m_iSize * sizeof(Char) );
			buffer[len3] = 0;
			base_string result;
			result.m_pBuffer = buffer;
			result.m_iSize = len3;
			return result;
		}


	protected: // Methods.
		//--------------------------------------------------------------------------------------------------------
		// Deallocate if needed.
		//--------------------------------------------------------------------------------------------------------
		void deallocate()
		{
#ifdef DEBUG_STRING_PRINT
			DebugPrint( "base_string deallocate(): %s; free: %d\n", m_pBuffer?m_pBuffer:"null", !m_iStatic && m_pBuffer );
#endif
			if ( !m_iStatic )
				free(m_pBuffer);
		}

		//--------------------------------------------------------------------------------------------------------
		// Copy 0-termination base_string.
		//--------------------------------------------------------------------------------------------------------
		void copy_contents(const Char* szFrom)
		{
			m_pBuffer = m_cAlloc.allocate(m_iSize+1);
			strncpy( m_pBuffer, szFrom, (m_iSize+1) * sizeof(Char) );
		}

		//--------------------------------------------------------------------------------------------------------
		// Return base_string = this + szRight.
		//--------------------------------------------------------------------------------------------------------
		base_string concat_with( const Char* szRight, int iRightSize ) const
		{
			int len = m_iSize;
			Char* buffer = m_cAlloc.allocate(len + iRightSize + 1);
			strncpy( buffer, m_pBuffer, len * sizeof(Char) );
			strncpy( &buffer[m_iSize], szRight, (iRightSize+1) * sizeof(Char) );
			return base_string ( buffer, false, true );
		}

	protected:
		typedef typename Alloc::template rebind<Char>::other alloc_t; // Allocator object for Char.

		alloc_t m_cAlloc;         // Allocator for Char.
		Char* m_pBuffer;          // Allocated space is m_iSize+1 bytes (for trailing 0).
		int m_iSize:31;           // String size.
		int m_iStatic:1;          // If true, then base_string must NOT be deallocated.
	};

	
	typedef base_string<char> string; ///< Typedef for base_string of chars.


} // namespace good


#pragma warning(pop) // Restore warnings.


#endif // __GOOD_STRING_H__
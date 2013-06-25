#ifndef __GOOD_BIT_SET_H__
#define __GOOD_BIT_SET_H__


#include "good/vector.h"
#include "good/utility.h"


namespace good
{

	
	//************************************************************************************************************
	/// Small class for handling set of bits.
	//************************************************************************************************************
	template < typename Alloc >
	class base_bitset
	{
	public:
		typedef good::vector< unsigned char, Alloc > container_t;

		/// Constructor with optional size of set.
		base_bitset( int iSize = 0 ): m_iSize(iSize) { m_cContainer.resize(BIT_ARRAY_SIZE(iSize)); }

		/// Constructor with data.
		base_bitset( const char* data, int iSize ): m_iSize(iSize)
		{
			int iCharSize = BIT_ARRAY_SIZE(iSize);
			m_cContainer.resize(iCharSize);
			memcpy(m_cContainer.data(), data, iCharSize);
		}

		/// Get size of bitset.
		int size() const { return m_cContainer.size(); }

		/// Returns true if any bits are set.
		bool any() const
		{
			for ( int i=0; i<m_cContainer.size(); ++i )
				if ( m_cContainer[i] )
					return true;
			return false;
		}

		/// Returns true if no bits are set.
		bool none() const { !any(); }

		/// Resize set to given size. Note that new bits will be undefined.
		void resize( int iNewSize ) { m_cContainer.resize( BIT_ARRAY_SIZE(iNewSize) ); m_iSize = iNewSize; }

		/// Clear all bits.
		void clear() { memset( m_cContainer.data(), 0, m_cContainer.size() ); }

		/// Set all bits.
		void set() { memset( m_cContainer.data(), 0xFF, m_cContainer.size() ); }

		/// Returns true if bit n is set.
		bool operator[]( int iIndex ) const { return test(iIndex); }
	
		/// Returns true if bit n is set.
		bool test( int iIndex ) const { DebugAssert( iIndex < m_iSize ); return BIT_ARRAY_IS_SET(iIndex, m_cContainer) != 0; }

		/// Set bit at given position.
		void set( int iIndex, bool bValue = true )
		{
			DebugAssert( iIndex < m_iSize );
			if ( bValue )
				BIT_ARRAY_SET(iIndex, m_cContainer);
			else
				BIT_ARRAY_CLEAR(iIndex, m_cContainer);
		}

		/// Clear bit at given position.
		void clear( int iIndex ) { set(iIndex, false); }

		/// Get count of set bits.
		int count()
		{
			                            // 0000 0001 0010 0011 0100 0101 0110 0111 1000 1001 1010 1011 1100 1101 1110 1111
			static int char_counts[16] = { 0,   1,   1,   2,   1,   2,   2,   3,   1,   2,   2,   3,   2,   2,   3,   4 };
			int count = 0;
			for ( int i=0; i < m_cContainer.size(); ++i )
			{
				unsigned char c = m_cContainer[i];
				count += char_counts[c & 0x0F];
				count += char_counts[(c >> 4 ) & 0x0F];
			}
			return count;
		}

	protected:
		container_t m_cContainer;
		int m_iSize;
	};


	typedef base_bitset< good::allocator<unsigned char> > bitset; ///< Typedef for default bitset.


} // namespace good


#endif // __GOOD_BIT_SET_H__

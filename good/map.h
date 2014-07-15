//----------------------------------------------------------------------------------------------------------------
// Map implementation, based on Anderssion tree.
// Copyright (c) 2011 Borzh.
//----------------------------------------------------------------------------------------------------------------

#ifndef __GOOD_MAP_H__
#define __GOOD_MAP_H__


#include "good/utility.h"
#include "good/aatree.h"


namespace good
{


    //************************************************************************************************************
    /**
     * @brief Util class to perform binary operations between two elements of type pair<T1, T2>.
     * Binary operation applies to first elements.
     */
    //************************************************************************************************************
    template <typename T, typename Op>
    class pair_first_op
    {
    public:
        /// Default operatorto compare 2 elements.
        bool operator() ( const T& tLeft, const T& tRight ) const
        {
            return op(tLeft.first, tRight.first);
        }
    protected:
        Op op;
    };


    //************************************************************************************************************
    /// Map from Key to Value.
    //************************************************************************************************************
    template <
        typename Key,
        typename Value,
        typename Less = less<Key>,
        typename Alloc = allocator< pair<Key, Value> >
    >
    class map: public aatree<
                             pair<Key, Value>,
                             pair_first_op< pair<Key, Value>, Less >,
                             Alloc
                            >
    {
    public:
        typedef pair<Key, Value> key_value_t;
        typedef aatree< pair<Key, Value>, pair_first_op<pair<Key, Value>, Less>, Alloc> base_class;
        typedef typename base_class::node_t node_t;

        //--------------------------------------------------------------------------------------------------------
        /// Operator =. Note that this operator moves content, not copies it.
        //--------------------------------------------------------------------------------------------------------
        //map& operator= (map const& tOther)
        //{
        //	assign(tOther);
        //	return *this;
        //}

        //--------------------------------------------------------------------------------------------------------
        /// Get constant iterator to a key.
        //--------------------------------------------------------------------------------------------------------
        typename base_class::const_iterator find( const Key& key ) const
        {
            return this->find( key_value_t(key, Value()) );
        }

        //--------------------------------------------------------------------------------------------------------
        /// Get iterator to a key.
        //--------------------------------------------------------------------------------------------------------
        typename base_class::iterator find( const Key& key )
        {
            return this->find( key_value_t(key, Value()) );
        }

        //--------------------------------------------------------------------------------------------------------
        /// Remove element at map iterator. Return next element.
        //--------------------------------------------------------------------------------------------------------
        typename base_class::iterator erase( typename base_class::iterator it )
        {
            return this->erase(it);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Remove key->value association.
        //--------------------------------------------------------------------------------------------------------
        typename base_class::iterator erase( const Key& key )
        {
            node_t* succ;
            int succDir;
            node_t* cand = _search( this->m_pHead->parent, key_value_t(key, Value()), succ, succDir );

            if ( this->_is_nil(cand) )
                return iterator(this->nil);

            // Found key->value.
            return iterator( _erase( cand, cand->parent->child[1] == cand, succ, succDir ) );
        }

        //--------------------------------------------------------------------------------------------------------
        /// Get mutable value for a key.
        /**
         * If key not found, insert (key, empty value) into map, returning reference to that value.
         * Use find() function if you need to know if key has some value.
         */
        //--------------------------------------------------------------------------------------------------------
        Value& operator[]( const Key& key )
        {
            key_value_t tmp( key, Value() );
            typename base_class::iterator it = insert(tmp, false); // Insert if not exists but don't replace.
            return it->second;
        }

        //--------------------------------------------------------------------------------------------------------
        // Get const value for a key.  Use find() function if you need to know if key has some value.
        // TODO: why doesn't work.
        //--------------------------------------------------------------------------------------------------------
        //const Value& operator[]( const Key& key ) const
        //{
        //	key_value_t tmp( key, Value() );
        //	const_iterator it = find(tmp);
        //	return it->second;
        //}

    };


};

#endif // __GOOD_MAP_H__

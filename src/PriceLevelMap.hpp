#ifndef __ORDER_MAP_HPP__
#define __ORDER_MAP_HPP__

#include <assert.h>
#include <map>
#include <unordered_map>

#include "OrderList.hpp"
#include "Trade.hpp"

namespace JumpInterview {
	namespace OrderBook {

		/*
		 * A map+table that has constant time lookups, but still O(logN) only when we create a new price level
		 */
		template <class T>
		class PriceLevelMap
		{
		public:
			typedef typename std::map < uint32_t, OrderList_ptr, T > LevelsTree;

			/* Add( O(1) ) or Find ( O(logN) ) the price level in the map */
			OrderList_ptr & add ( uint32_t price )
			{
				typename LevelsTable::iterator iter ( m_table.find ( price ) );
				if ( iter != m_table.end() )
					return iter->second->second;
				else
				{
					OrderList_ptr node_list = std::make_shared < OrderList > ();
					typename LevelsTree::iterator iter = m_tree.insert ( std::make_pair ( price, node_list ) ).first;
					m_table.insert ( std::make_pair ( price, iter ) );
					return iter->second;
				}
			}

			/* Remove ( O(1) ) the price level from the map */
			void remove ( uint32_t price )
			{
				typename LevelsTable::const_iterator iter ( m_table.find ( price ) );
				assert ( iter != m_table.end() );
				assert ( iter->second->second->empty() );
				m_tree.erase ( iter->second );
				m_table.erase ( iter );
			}

			bool empty() const
			{
				assert ( m_tree.empty() == m_table.empty() );
				return m_tree.empty();
			}

			size_t size() const
			{
				assert ( m_tree.size() == m_table.size() );
				return m_tree.size();
			}

			void clear()
			{
				m_table.clear();
				m_tree.clear();
			}

			typename LevelsTree::const_iterator begin() const
			{
				return m_tree.begin();
			}

			typename LevelsTree::const_iterator end() const
			{
				return m_tree.end();
			}

			void print ( std::ostream& os ) const
			{
				static const char * empty ( "<empty>" );
				if ( !m_tree.empty() )
					for ( auto iter = m_tree.begin();
							iter != m_tree.end();
							iter++ )
					{
						OrderList_ptr const & list ( iter->second );
						os << *list;
					}
				else
					os << empty << std::endl;
			}

			/*
			 * Starting with the top price level, tries to get 'volume_to_go' down to 0
			 * This is, if it would have still matched on this side. ( different comparer for B/S )
			 *
			 * Does not remove the orders or anything, simply fills in a vector with expected trades
			 */
			void matchTrades ( Trade_vct & vct, uint32_t price, uint32_t & volume_to_go )
			{
				static T t;
				assert ( m_tree.size() > 0 );
				for ( auto iter = m_tree.begin();
						iter != m_tree.end() && t ( iter->first, price ) && volume_to_go > 0;
						iter++ )
				{
					OrderList_ptr const & list ( iter->second );
					for ( OrderNode_list::iterator iter = list->begin();
							iter != list->end() && volume_to_go > 0;
							iter ++ )
					{
						Order_ptr const & order ( ( *iter )->order() );
						Trade_ptr new_trade ( new Trade ( std::min ( volume_to_go, order->volume() ), order->price() ) );
						vct.push_back ( new_trade );
						assert ( volume_to_go >= new_trade->volume() );
						volume_to_go -= new_trade->volume();
					}
				}
			}

		private:
			typedef typename std::unordered_map < uint32_t, typename LevelsTree::iterator > LevelsTable;
			LevelsTree m_tree;
			LevelsTable m_table;
		};
	}
}

#endif
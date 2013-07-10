#include <algorithm>
#include <assert.h>
#include <functional>

#include "Constants.hpp"
#include "OrderBook.hpp"

namespace JumpInterview {
	namespace OrderBook {

		OrderBook::OrderBook ( ErrorSummary & error_summary ) :
			m_error_summary ( error_summary ),
			m_sequence_id ( 0 ),
			m_mid_price ( std::numeric_limits<double>::max() ),
			m_am_expecting_trades ( false )
		{
			m_add_functors[ OrderSide::BUY ] = std::bind ( &OrderBook::add<BuyPriceLevelMap>, this, std::ref ( m_buys ), std::placeholders::_1 );
			m_add_functors[ OrderSide::SELL ] = std::bind ( &OrderBook::add<SellPriceLevelMap>, this, std::ref ( m_sells ), std::placeholders::_1 );
			m_remove_functors [ OrderSide::BUY ] = std::bind ( &OrderBook::remove<BuyPriceLevelMap>, this, std::ref ( m_buys ), std::placeholders::_1, std::placeholders::_2 );
			m_remove_functors [ OrderSide::SELL ] = std::bind ( &OrderBook::remove<SellPriceLevelMap>, this, std::ref ( m_sells ), std::placeholders::_1, std::placeholders::_2 );
			m_modify_functors [ OrderSide::BUY ] = std::bind ( &OrderBook::modify<BuyPriceLevelMap>, this, std::ref ( m_buys ), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 );
			m_modify_functors [ OrderSide::SELL ] = std::bind ( &OrderBook::modify<SellPriceLevelMap>, this, std::ref ( m_sells ), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 );
			// Unlike the other functions, this is located in the map itself.
			m_match_functors [ OrderSide::SELL ] = std::bind ( &BuyPriceLevelMap::matchTrades, std::ref ( m_buys ), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 );
			m_match_functors [ OrderSide::BUY ] = std::bind ( &SellPriceLevelMap::matchTrades, std::ref ( m_sells ), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 );
		}

		/*
		* The orderbook knows all about our orders, so should dealloc them here
		*/
		OrderBook::~OrderBook()
		{
			m_buys.clear();
			m_sells.clear();
			clearExpectedTrades();
		}

		void OrderBook::clearExpectedTrades()
		{
			for ( Trade_vct::const_iterator iter = m_expected_trades.begin();
					iter != m_expected_trades.end();
					iter++ )
				delete ( *iter );
			m_expected_trades.clear();
		}

		/*
		* Create a new 'price level' if we have to,
		* and add the order to it.
		* Returns true if succesful, false if the order already exists
		*/
		bool OrderBook::add ( Order_ptr const & order )
		{
			assert ( order->price() > 0 );
			OrderDict::iterator iter ( m_all_orders.find ( order->orderId() ) );
			if ( iter == m_all_orders.end() )
			{
				m_all_orders.insert ( std::make_pair ( order->orderId(), m_add_functors [ order->side() ] ( order ) ) );
				return true;
			}
			else
			{
				m_error_summary.duplicate_order_id++;
				return false;
			}
		}

		template <class T>
		OrderNode_list::iterator OrderBook::add ( T & map, Order_ptr const & order )
		{
			OrderList_ptr & list ( map.add ( order->price() ) );
			OrderNode_list::iterator return_iter = list->add ( order, m_sequence_id++ );
			// if we are the top level, and there's just our new price in it, surely the mid price has changed ( if there's something on the other side .. )
			if ( map.begin()->second == list && list->size() == 1 )
			{
				calculateMidPrice();
				m_expected_trades.clear();
				m_am_expecting_trades = isCrossed();
			}
			assert ( ( *return_iter )->order() == order );
			return return_iter;
		}

		/*
		* Removes the order from the map and list.
		* Is still up to the calling function to dispose
		*/
		Order_ptr OrderBook::remove ( uint32_t order_id,
									  OrderSide::Side side,
									  uint32_t volume,
									  uint32_t price )
		{
			OrderDict::iterator iter ( m_all_orders.find ( order_id ) );
			// don't check the volume - that might have changed without the user realising it
			if ( iter != m_all_orders.end() &&
					( *iter->second )->order()->side() == side &&
					( *iter->second )->order()->price() == price )
			{
				Order_ptr order ( ( *iter->second )->order() );
				m_remove_functors [ order->side() ] ( iter->second, price );
				m_all_orders.erase ( iter );
				return order;
			}
			else
				m_error_summary.removes_with_no_corresponding_order++;
			return 0;
		}

		template <class T>
		void OrderBook::remove ( T & map,
								 OrderNode_list::iterator & order_iter,
								 uint32_t price )
		{
			assert ( !map.empty() );
			OrderList_ptr price_level ( map.add ( price ) );
			bool was_top_level ( map.begin()->second == price_level );
			price_level->remove ( order_iter );
			if ( price_level->empty() )
			{
				map.remove ( price );
				if ( was_top_level )
					calculateMidPrice();
			}
		}

		/*
		* Find the old order. A couple of things can happen here:
		*  -> Price ( and optionally, Volume as well ) changed - remove and add new order ( lose time priority )
		*  -> Volume changed down - doesn't effect place. Definitely the case for after a trade, debatable when it's because of a user modification.
		*  -> Volume changed up - update the order and move to the back of the queue
		* ( Unexpected ) -> A new order gets created because we don't know about the original order
		* ( Unexpected ) -> If the side doesn't match, we just note this
		*/
		void OrderBook::modify ( uint32_t order_id,
								 OrderSide::Side side,
								 uint32_t volume,
								 uint32_t price )
		{
			OrderDict::iterator iter ( m_all_orders.find ( order_id ) );
			if ( iter != m_all_orders.end() )
			{
				Order_ptr const & order ( ( *iter->second )->order() );
				if ( order->side() == side )
					iter->second = m_modify_functors [ side ] ( iter->second, volume, price );
				else
					m_error_summary.order_modify_on_wrong_side ++;
			}
			else
			{
				// if we try to modify an order that we don't know about yet, just treat it as a new order.
				// that's better than nothing.
				Order_ptr new_order = new Order ( order_id,
												  side,
												  volume,
												  price );
				add ( new_order );
				m_error_summary.order_modify_on_order_i_dont_know ++;
			}
		}

		template <class T>
		OrderNode_list::iterator OrderBook::modify ( T & map,
				OrderNode_list::iterator & order_iter,
				uint32_t volume,
				uint32_t price )
		{
			Order_ptr order ( ( *order_iter )->order() );
			if ( order->volume() < volume ||
					order->price() != price )
			{
				remove ( map, order_iter, order->price() ); 	/* remove at the old level */
				order->modify ( volume, price ); 		/* modify the contents */
				return add ( map, order ); 			/* and add at new level */
			}
			else
			{
				// volume goes down ( either execution or user change ) - keep priority
				order->modify ( volume, price );
				return order_iter;
			}
		}

		void OrderBook::handleTrade ( uint32_t volume,
									  uint32_t price,
									  std::ostream &os )
		{
			// Even if the trade's unexpected, we print this ..
			static const char at ( '@' );
			if ( m_trade_summary.last_level == price )
				m_trade_summary.last_volume++;
			else
				m_trade_summary.reset ( price, volume ) ;
			os << m_trade_summary.last_volume << at << ( m_trade_summary.last_level / Constants::round_size ) << std::endl;
			// at the first trade that arrives since we crossed, we calculate our vector of expected trades.
			// we will now match every trade with the top of this vector.
			if ( isCrossed() )
			{
				if ( m_expected_trades.empty() )
				{
					if ( m_am_expecting_trades )
					{
						// make sure we only fill this vector once for every time we reach a new top price level that crosses
						calculateExpectedTrades();
						m_am_expecting_trades = false;
						assert ( !m_expected_trades.empty() );
					}
					else
					{
						// we are crossing but we've already traded all volume we expected at this level
						m_error_summary.trades_with_no_corresponding_order++;
						return;
					}
				}
				assert ( !m_expected_trades.empty() );
				Trade_ptr first_expected ( *m_expected_trades.begin() );
				if ( first_expected->price() == price &&
						first_expected->volume() == volume )
				{
					// great stuff, this one matches!
					delete ( first_expected );
					m_expected_trades.erase ( m_expected_trades.begin() );
				}
				else
					m_error_summary.trades_with_no_corresponding_order++;
			}
			else
				m_error_summary.trades_with_no_corresponding_order++;
		}

		double const & OrderBook::midPrice() const
		{
			return m_mid_price;
		}

		/*
		* There is a special case when the order book is crossed. I figured there's a couple of things we can do here, including just using the
		* expected trade price as the mid price. Or, see what the new mid price would be after we actually trade. Those are not a real reflection of
		* what we see here though, so I decided to just use the average anyway.
		*/
		void OrderBook::calculateMidPrice()
		{
			m_mid_price = ( m_buys.empty() || m_sells.empty() ) ?
						  std::numeric_limits<double>::max() :
						  ( m_buys.begin()->first + m_sells.begin()->first ) / ( Constants::round_size * 2.0 ) ;
		}

		/*
		* Will be called whenever we add a new pricelevel, because that's exactly when we expect to trade potentially.
		*/
		void OrderBook::calculateExpectedTrades()
		{
			assert ( m_am_expecting_trades );
			if ( isCrossed() )
			{
				clearExpectedTrades();
				OrderNode_ptr buy_node ( *m_buys.begin()->second->begin() );
				OrderNode_ptr sell_node ( *m_sells.begin()->second->begin() );
				Order_ptr const & most_recent_order ( buy_node->sequence_id() > sell_node->sequence_id() ?
													  buy_node->order() :
													  sell_node->order() );
				// now that we know the most recent order, find out which orders get matched against this on the other side
				unsigned int volume_to_go ( most_recent_order->volume() );
				m_match_functors [ most_recent_order->side() ] ( m_expected_trades, most_recent_order->price(), volume_to_go );
			}
		}

		bool OrderBook::isCrossed() const
		{
			return ( !m_buys.empty() &&
					 !m_sells.empty() &&
					 ( *m_buys.begin()->second->begin() )->order()->price() >= ( *m_sells.begin()->second->begin() )->order()->price() );
		}

		bool OrderBook::waitingForTrades() const
		{
			return m_am_expecting_trades || !m_expected_trades.empty();
		}

		void OrderBook::print ( std::ostream &os ) const
		{
			static const char * buys ( "Buys:" );
			static const char * sells ( "Sells:" );
			os << buys << std::endl;
			m_buys.print ( os );
			os << sells << std::endl;
			m_sells.print ( os );
		}
	}
}

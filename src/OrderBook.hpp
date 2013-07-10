#ifndef __ORDER_BOOK_HPP__
#define __ORDER_BOOK_HPP__

#include <map>
#include <unordered_map>
#include <functional>

#include "Order.hpp"
#include "PriceLevelMap.hpp"
#include "Trade.hpp"
#include "OrderList.hpp"
#include "ErrorSummary.hpp"

namespace JumpInterview {
	namespace OrderBook {

		struct TradeSummary
		{
			TradeSummary() : last_level ( 0 ), last_volume ( 0 ) {}
			void reset ( uint32_t const & new_price, uint32_t const & new_volume )
			{
				last_level = new_price;
				last_volume = new_volume;
			}
			uint32_t last_level;
			uint32_t last_volume;
		};

		class OrderBook
		{
		public:
			typedef PriceLevelMap < std::greater<uint32_t> > BuyPriceLevelMap;
			typedef PriceLevelMap < std::less<uint32_t> > SellPriceLevelMap;

			OrderBook ( ErrorSummary & error_summary );
			~OrderBook();

			bool add ( Order_ptr const & order ) ;
			Order_ptr remove ( uint32_t order_id,
							   OrderSide::Side side,
							   uint32_t volume,
							   uint32_t price );
			void modify ( uint32_t order_id,
						  OrderSide::Side side,
						  uint32_t volume,
						  uint32_t price ) ;

			void handleTrade ( uint32_t volume,
							   uint32_t price,
							   std::ostream &os ) ;

			double const & midPrice() const;
			void print ( std::ostream &os ) const;
			bool isCrossed() const;
			bool waitingForTrades() const;

			BuyPriceLevelMap const & buys() const
			{
				return m_buys;
			}

			SellPriceLevelMap const & sells() const
			{
				return m_sells;
			}
		private:
			typedef std::unordered_map < uint32_t, OrderNode_list::iterator > OrderDict;

			ErrorSummary & m_error_summary;
			uint32_t m_sequence_id;
			double m_mid_price;
			BuyPriceLevelMap m_buys;
			SellPriceLevelMap m_sells;
			OrderDict m_all_orders;
			TradeSummary m_trade_summary;
			Trade_vct m_expected_trades;
			bool m_am_expecting_trades;

			/*
			 * When we need to operate on an (Buy/Sell)OrderMap, we just use these bound functions.
			 * They are indexed by order type, and we don't have to supply the map or comparison operator anymore.
			 */
			typedef std::function<OrderNode_list::iterator ( Order_ptr const & ) > Add_functor;
			typedef std::function<void ( OrderNode_list::iterator &, uint32_t ) > Remove_functor;
			typedef std::function<OrderNode_list::iterator ( OrderNode_list::iterator &, uint32_t, uint32_t ) > Modify_functor;
			typedef std::function<void ( Trade_vct & vct, uint32_t, uint32_t & ) > Match_functor;
			Add_functor m_add_functors[2];
			Remove_functor m_remove_functors[2];
			Modify_functor m_modify_functors[2];
			Match_functor m_match_functors[2];

			void calculateMidPrice();
			void calculateExpectedTrades();
			void clearExpectedTrades();

			template <class T>
			OrderNode_list::iterator add ( T & map,
										   Order_ptr const & order );

			template <class T>
			void remove ( T & map,
						  OrderNode_list::iterator & order_iter,
						  uint32_t price );

			template <class T>
			OrderNode_list::iterator modify ( T & map,
											  OrderNode_list::iterator & order,
											  uint32_t volume,
											  uint32_t price );
		};

		typedef OrderBook * OrderBook_ptr;

	}
}


#endif
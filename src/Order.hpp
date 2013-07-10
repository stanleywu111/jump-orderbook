#ifndef __ORDER_HPP__
#define __ORDER_HPP__

#include <vector>
#include <stdint.h>
#include <ostream>
#include <memory>
#include <iostream>

#include "PoolAllocator.hpp"

namespace JumpInterview {
	namespace OrderBook {

		namespace OrderSide
		{
			enum Side
			{
				BUY,
				SELL
			};
		}

		class Order;
		typedef Order * Order_ptr;

		class Order
		{
		public:
			friend class OrderList;
			Order ( uint32_t order_id,
					OrderSide::Side side,
					uint32_t volume,
					uint32_t price );

			friend class OrderBook;
			uint32_t orderId() const;
			OrderSide::Side side() const;
			uint32_t volume() const;
			uint32_t price() const;

			static inline void* operator new ( std::size_t sz )
			{
				return PoolAllocator<Order>::instance().allocate ( sz ) ;
			}

			static inline void operator delete ( void* p )
			{
				PoolAllocator<Order>::instance().deallocate ( static_cast< Order * > ( p ), sizeof ( Order ) ) ;
			}

			void print ( std::ostream& os );
		private:
			Order ( Order const & rhs ) {}
			uint32_t m_order_id;
			OrderSide::Side m_side;
			uint32_t m_volume;
			uint32_t m_price;
			char m_formatted_string[ 40 ];

			void modify ( uint32_t volume, uint32_t price );
			inline char * format();
		};

		std::ostream& operator<< ( std::ostream& os, Order& order );
	}
}

#endif

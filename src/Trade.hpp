#ifndef __TRADE_HPP__
#define __TRADE_HPP__

#include <stdint.h>
#include <vector>
#include <memory>

#include "PoolAllocator.hpp"

namespace JumpInterview {
	namespace OrderBook {

		class Trade
		{
		public:
			Trade ( uint32_t volume,
					uint32_t price );
			~Trade();
			uint32_t volume() const;
			uint32_t price() const;

			static inline void* operator new ( std::size_t sz )
			{
				return PoolAllocator<Trade>::instance().allocate ( sz ) ;
			}

			static inline void operator delete ( void* p )
			{
				PoolAllocator<Trade>::instance().deallocate ( static_cast< Trade * > ( p ), sizeof ( Trade ) ) ;
			}
		private:
			Trade ( Trade const & rhs ) {}

			uint32_t m_volume;
			uint32_t m_price;
		};
		typedef Trade * Trade_ptr;
		typedef std::vector < Trade * > Trade_vct;
	}
}

#endif
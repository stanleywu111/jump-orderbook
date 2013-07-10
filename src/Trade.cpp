#include <assert.h>
#include <cmath>
#include <iostream>

#include "Trade.hpp"
#include "Constants.hpp"

namespace JumpInterview {
	namespace OrderBook {
		Trade::Trade ( uint32_t volume,
					   uint32_t price ) : m_volume ( volume ),
			m_price ( price )
		{
			assert ( m_volume != 0 );
			assert ( m_price != 0 );
		}

		Trade::~Trade()
		{
		}

		uint32_t Trade::volume() const {
			return m_volume;
		}

		uint32_t Trade::price() const {
			return m_price;
		}
	}
}

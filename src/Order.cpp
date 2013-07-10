#include <assert.h>
#include <cstring>
#include <cmath>
#include <sstream>

#include "Constants.hpp"
#include "Order.hpp"

namespace JumpInterview {
	namespace OrderBook {

		/*
		 * I am converting the price from double into a uint32_t. This is to make comparisons further down the easier.
		 * Otherwise, I would have to result to ' std::abs(a-b) < std::numeric_limits<double>::epsilon() ' for just about
		 * every operation involving doubles.
		 *
		 * We obviously have to pick Constants::round_size properly.
		 */
		Order::Order ( uint32_t order_id,
					   OrderSide::Side side,
					   uint32_t volume,
					   uint32_t price ) :
			m_order_id ( order_id ),
			m_side ( side ),
			m_volume ( volume ),
			m_price ( price )
		{
			assert ( m_volume > 0 );
			assert ( m_price > 0 );
			m_formatted_string[0] = 0;
		}

		uint32_t Order::orderId() const  {
			return m_order_id;
		}

		OrderSide::Side Order::side() const  {
			return m_side;
		}

		uint32_t Order::volume() const  {
			return m_volume;
		}

		uint32_t Order::price() const  {
			return m_price;
		}

		void Order::modify ( uint32_t volume, uint32_t price )
		{
			assert ( m_volume != volume || m_price != price );
			assert ( volume > 0 && price > 0 );
			m_volume = volume;
			m_price = price;
			// the next time we want to print this order, we have to reformat it
			m_formatted_string[0] = 0;
		}

		char * Order::format()
		{
			static const char * semicolon ( ":" );
			static const char * buy ( "Buy" );
			static const char * sell ( "Sell" );
			static const char * space ( " " );
			static const char * at ( "@" );
			assert ( !m_formatted_string[0] );
			std::stringstream ss;
			// let this do the rounding.
			ss.precision ( 8 );
			ss <<
			   m_order_id << semicolon << space <<
			   ( m_side == OrderSide::BUY ? buy : sell ) << space <<
			   m_volume << space <<
			   at << space <<
			   ( m_price / Constants::round_size );
			assert ( ss.tellp() < 40 );
			ss.read ( m_formatted_string, ss.tellp() );
			// this is now the end of the string
			m_formatted_string[ss.tellp()] = 0;
			return m_formatted_string;
		}

		void Order::print ( std::ostream& os )
		{
			os << ( !m_formatted_string[0] ? format() : m_formatted_string ) ;
		}

		/*
		 * We store price as uint32_t locally, but price it in the old format.
		 */
		std::ostream& operator<< ( std::ostream& os, Order& order )
		{
			order.print ( os );
			return os;
		}
	}
}


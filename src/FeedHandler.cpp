#include <algorithm>
#include <iostream>
#include <limits>
#include <assert.h>
#include <stdlib.h>
#include <stdexcept>
#include <cmath>

#include "FeedHandler.hpp"

namespace JumpInterview {
	namespace OrderBook {

		// valid order actions (A,X,M) and trade action (T)
		const char FeedHandler::f_add ( 'A' );
		const char FeedHandler::f_remove ( 'X' );
		const char FeedHandler::f_modify ( 'M' );
		const char FeedHandler::f_trade ( 'T' );

		// valid sides are (B,S)
		const char FeedHandler::f_buy ( 'B' );
		const char FeedHandler::f_sell ( 'S' );

		// fields seperated by (,)
		const char FeedHandler::f_sep ( ',' );

		// we might write comments '/', possibly started with a whitespace
		const char FeedHandler::f_comment ( '/' );
		const char FeedHandler::f_whitespace ( ' ' );
		// also, allow dos style formatting .. where our lines still have a \r at the end
		const char FeedHandler::f_return ( '\r' );

		FeedHandler::FeedHandler( ) :
			m_book ( m_error_summary )
		{
		}

		FeedHandler::~FeedHandler()
		{
		}

		void FeedHandler::processMessage ( const std::string &line, std::ostream &os )
		{
			static const char * nan ( "NAN" );
			try
			{
				size_t i = line.find ( f_sep, 0 );
				// orders are more likely, let's help predictive branching and process those first
				if ( i == 1 && line.size() > 3 &&
						(
							line[0] == f_add ||
							line[0] == f_remove ||
							line[0] == f_modify ) )
					processOrderMessage ( line, os );
				else if ( i == 1 && line.size() > 3 && line[0] == f_trade )
					processTradeMessage ( line, os );
				else
					m_error_summary.corrupted_messages++;
			} catch ( std::runtime_error & )
			{
				// ouch - I really shouldn't get here
				m_error_summary.unexpected_exception++;
			}
			double const & mid ( m_book.midPrice() );
			if ( mid == std::numeric_limits<double>::max() )
				os << nan << std::endl;
			else
				os << mid << std::endl;
		}

		// I could have split the string into tokens, but this way I don't have to allocate memory for strings..
		// I also could have decided to use only 2 size_t's and keep on checking that we're on the right track, I decided
		// not to do that to make predictive branching easier - we process everything in one go and carry on. We expect
		// most messages to be well-formed anyway.
		void FeedHandler::processOrderMessage ( const std::string &line, std::ostream &os )
		{
			assert ( line[0] == f_add ||
					 line[0] == f_remove ||
					 line[0] == f_modify );
			size_t order_id_begin ( 2 );
			size_t order_id_end ( line.find ( f_sep, order_id_begin ) );
			size_t side_begin ( order_id_end + 1 );
			size_t volume_begin ( side_begin + 2 );
			size_t volume_end ( line.find ( f_sep, volume_begin ) );
			size_t price_begin ( volume_end + 1 );
			size_t comment ( line.find ( f_comment, price_begin ) );
			size_t whitespace ( line.find ( f_whitespace, price_begin ) );
			assert ( ( whitespace == std::string::npos && comment == std::string::npos ) ||	( whitespace != comment ) );
			size_t return_chr ( line.find ( f_return, price_begin ) );
			// read until the end of the line or the first whitespace, comma
			size_t price_end ( std::min ( comment, std::min ( whitespace, return_chr ) ) != std::string::npos ?
							   std::min ( comment, std::min ( whitespace , return_chr ) ) : line.size() );
			uint32_t order_id;
			uint32_t volume;
			double price;
			bool failure = (
							   /* parse the order id */
							   order_id_end <= order_id_begin ||
							   !tryParse ( line.data() + order_id_begin,
										   order_id_end - order_id_begin,
										   order_id ) ||
							   /* check that the side is B or S */
							   ( line[side_begin] != f_buy && line[side_begin] != f_sell ) ||
							   /* parse the volume */
							   volume_end <= volume_begin ||
							   !tryParse ( line.data() + volume_begin,
										   volume_end - volume_begin,
										   volume ) ||
							   /* finally, parse the price */
							   price_end <= price_begin ||
							   !tryParse ( line.data() + price_begin,
										   price_end - price_begin,
										   price ) || /* Why not, allow and order id of 0 */
							   price <= 0 ||
							   price > maxPrice() ||
							   volume == 0 /* An order with a volume of 0? I don't think so! If that's a modify it should be an 'X' instead! */ );
			if ( !failure )
			{
				// once the book is crossed we generate expected trades.
				// before we see any more order messages, we expect new trades to match those we expected
				if ( m_book.isCrossed() && m_book.waitingForTrades() )
					m_error_summary.no_trades_when_they_should_happen++;
				switch ( line[0] )
				{
				case f_add:
				{
					Order_ptr order ( new Order  (
										  order_id,
										  line[side_begin] == f_buy ? OrderSide::BUY : OrderSide::SELL,
										  volume,
										  static_cast < uint32_t > ( std::floor ( price * Constants::round_size ) ) ) );
					assert ( order != 0 );
					assert ( order->volume() == volume );
					assert ( order->orderId() == order_id );
					// if we can't add this order, we have to dispose it ourselves
					if ( !m_book.add ( order ) )
						delete ( order );
					break;
				}
				case f_remove:
				{
					// order will be disposed as soon as this goes out of scope
					Order_ptr order ( m_book.remove ( order_id,
													  line[side_begin] == f_buy ? OrderSide::BUY : OrderSide::SELL,
													  volume,
													  static_cast < uint32_t > ( std::floor ( price * Constants::round_size ) ) ) );
					if ( order )
						delete ( order );
					break;
				}
				case f_modify:
				{
					m_book.modify ( order_id,
									line[side_begin] == f_buy ? OrderSide::BUY : OrderSide::SELL,
									volume,
									static_cast < uint32_t > ( std::floor ( price * Constants::round_size ) ) );
					break;
				}
				default:
					// error!
					break;
				}
			}
			else if ( price_begin == std::string::npos )
				m_error_summary.corrupted_messages ++;
			else
				m_error_summary.out_of_bounds_or_weird_numbers++;
		}

		void FeedHandler::processTradeMessage ( const std::string &line, std::ostream &os )
		{
			assert ( line[0] == f_trade );
			size_t volume_begin ( 2 );
			size_t volume_end ( line.find ( f_sep, volume_begin ) );
			size_t price_begin ( volume_end + 1 );
			size_t comment ( line.find ( f_comment, price_begin ) );
			size_t whitespace ( line.find ( f_whitespace, price_begin ) );
			size_t return_chr ( line.find ( f_return, price_begin ) );
			// read until the end of the line or the first whitespace, comma
			size_t price_end ( std::min ( comment, std::min ( whitespace, return_chr ) ) != std::string::npos ?
							   std::min ( comment, std::min ( whitespace, return_chr ) ) : line.size() );
			uint32_t volume;
			double price;
			bool failure = (
							   /* parse the volume */
							   volume_end <= volume_begin ||
							   !tryParse ( line.data() + volume_begin,
										   volume_end - volume_begin,
										   volume ) ||
							   /* parse the price */
							   price_end <= price_begin ||
							   !tryParse ( line.data() + price_begin,
										   price_end - price_begin,
										   price ) ||
							   price < 0 ||
							   price > maxPrice() );
			if ( !failure )
				m_book.handleTrade ( volume, static_cast < uint32_t > ( std::floor ( price * Constants::round_size ) ), os );
			else if ( price_begin == std::string::npos )
				m_error_summary.corrupted_messages++;
			else
				m_error_summary.out_of_bounds_or_weird_numbers++;
		}

		void FeedHandler::printCurrentOrderBook ( std::ostream &os ) const
		{
			m_book.print ( os );
		}

		void FeedHandler::printErrorSummary ( std::ostream & os ) const
		{
			os << "Errors:" << std::endl;
			os << m_error_summary;
		}

		OrderBook const & FeedHandler::book() const
		{
			return m_book;
		}

		ErrorSummary const & FeedHandler::errors() const
		{
			return m_error_summary;
		}

		bool FeedHandler::tryParse ( const char * input, size_t len, double & out )
		{
			char* endptr;
			out = strtod ( input, &endptr );
			// success if we processed exactly the number of characters we expected
			return ( endptr == input + len );
		}

		bool FeedHandler::tryParse ( const char * input, size_t len, uint32_t & out )
		{
			char * endptr;
			out = strtoul ( input, &endptr, 10 );
			// success if we processed exactly the number of characters we expected and there's no '-' in there
			return ( std::find ( input, input + len, '-' ) == input + len &&
					 endptr == input + len &&
					 ( len < 10 || !isUIntOverflow ( input, len ) ) );
		}

		/*
		 * A not so quick check to see if the value's bigger than uint32_t::max
		 */
		bool FeedHandler::isUIntOverflow ( const char * input, size_t len )
		{
			static const char * max_size ( "4294967295" );
			if ( len > 10 )
				return true;
			for ( size_t i = 0; i < len && input[i] >= max_size[i] ; i++ )
			{
				if ( input[i] > max_size[i] )
					return true;
			}
			return false;
		}

		constexpr double FeedHandler::maxPrice()
		{
			return std::floor ( std::numeric_limits<uint32_t>::max() / Constants::round_size );
		}
	}
}

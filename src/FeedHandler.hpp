#ifndef __FEED_HANDLER_HPP
#define __FEED_HANDLER_HPP

#include "Constants.hpp"
#include "Order.hpp"
#include "OrderBook.hpp"
#include "ErrorSummary.hpp"

namespace JumpInterview {
	namespace OrderBook {

		class FeedHandler
		{
		public:
			FeedHandler( );
			~FeedHandler();
			void processMessage ( const std::string &line, std::ostream &os );
			void printCurrentOrderBook ( std::ostream &os ) const;
			void printErrorSummary ( std::ostream & os ) const;
			OrderBook const & book() const;
			ErrorSummary const & errors() const;
		private:
			static const char f_add ;
			static const char f_remove;
			static const char f_modify;
			static const char f_trade;

			static const char f_buy;
			static const char f_sell;

			static const char f_sep;
			static const char f_comment;
			static const char f_whitespace;
			static const char f_return;
			FeedHandler ( FeedHandler const & rhs ) : m_book ( m_error_summary ) {}

			inline void processOrderMessage ( const std::string &line, std::ostream &os );
			inline void processTradeMessage ( const std::string &line, std::ostream &os );

			static bool tryParse ( const char * input, size_t len, double & out );
			static bool tryParse ( const char * input, size_t len, uint32_t & out );
			static bool isUIntOverflow ( const char * input, size_t len );
			static constexpr double maxPrice();

			ErrorSummary m_error_summary;
			OrderBook m_book;
		};
	}
}

#endif
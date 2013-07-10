#ifndef __ERROR_SUMMARY_HPP__
#define __ERROR_SUMMARY_HPP__

#include <iostream>
#include <stdint.h>

namespace JumpInterview {
	namespace OrderBook {

		struct ErrorSummary
		{
		public:
			ErrorSummary();
			// completely malformed messages
			uint32_t corrupted_messages;
			// somewhat malformed messages, like negative order ids, prices, volume
			uint32_t out_of_bounds_or_weird_numbers;
			// unexpected order state messages
			uint32_t order_modify_on_order_i_dont_know;
			uint32_t order_modify_on_wrong_side;
			uint32_t duplicate_order_id;
			uint32_t removes_with_no_corresponding_order;
			// unexpected trade state messages
			uint32_t trades_with_no_corresponding_order;
			uint32_t no_trades_when_they_should_happen;
			// worst nightmare - something I didn't think about
			uint32_t unexpected_exception;

			bool empty() const;
		};
		std::ostream& operator<< ( std::ostream& os, const ErrorSummary& sum );
	}
}

#endif
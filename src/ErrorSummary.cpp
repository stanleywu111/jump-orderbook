#include "ErrorSummary.hpp"

namespace JumpInterview {
	namespace OrderBook {

		ErrorSummary::ErrorSummary() :
			corrupted_messages ( 0 ),
			out_of_bounds_or_weird_numbers ( 0 ),
			order_modify_on_order_i_dont_know ( 0 ),
			order_modify_on_wrong_side ( 0 ),
			duplicate_order_id ( 0 ),
			removes_with_no_corresponding_order ( 0 ),
			trades_with_no_corresponding_order ( 0 ),
			no_trades_when_they_should_happen ( 0 ),
			unexpected_exception ( 0 )
		{
		}

		std::ostream& operator<< ( std::ostream& os, const ErrorSummary& sum )
		{
			os << "[ GLOBAL] Corrupted messages: " << sum.corrupted_messages << std::endl;
			os << "[ GLOBAL] Out of bounds or otherwise weird data: " << sum.out_of_bounds_or_weird_numbers << std::endl;
			os << "[  ORDER] Modify without corresponding order: " << sum.order_modify_on_order_i_dont_know << std::endl;
			os << "[  ORDER] Modify that's changing side: " << sum.order_modify_on_wrong_side << std::endl;
			os << "[  ORDER] Duplicate order id: " << sum.duplicate_order_id << std::endl;
			os << "[  ORDER] Removes without corresponding order: " << sum.removes_with_no_corresponding_order << std::endl;
			os << "[  TRADE] Trades without corresponding order: " << sum.trades_with_no_corresponding_order << std::endl;
			os << "[  TRADE] No trades when they should happen: " << sum.no_trades_when_they_should_happen << std::endl;
			os << "[SERIOUS] Unexpected exception: " << sum.unexpected_exception << std::endl;
			return os;
		}

		bool ErrorSummary::empty() const
		{
			return !corrupted_messages &&
				   !duplicate_order_id &&
				   !trades_with_no_corresponding_order &&
				   !removes_with_no_corresponding_order &&
				   !no_trades_when_they_should_happen &&
				   !out_of_bounds_or_weird_numbers &&
				   !order_modify_on_order_i_dont_know &&
				   !order_modify_on_wrong_side &&
				   !unexpected_exception;
		}
	}
}
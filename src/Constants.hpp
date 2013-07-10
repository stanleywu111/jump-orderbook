#ifndef __CONSTANTS_HPP__
#define __CONSTANTS_HPP__

namespace JumpInterview {
	namespace OrderBook {
		namespace Constants	{
			// Every price is multiplied by 1000 and then treated as a uint32_t.
			// I've made two assumptions here:
			// * Prices are always positive ( not neccessarily the case, for instance a put spread or irs can have a negative price )
			// * The tick size is more than 0.001.
			// If that's not the case, this number should be higher.
			static const double round_size ( 1000.0 );
		}
	}
}


#endif
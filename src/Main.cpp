#include <fstream>
#include <cstring>
#include <string>
#include <iomanip>
#include <iostream>

#include "FeedHandler.hpp"

using namespace JumpInterview::OrderBook;

int main ( int argc, char **argv )
{
	FeedHandler feed;
	std::string line;
	std::iostream null_str ( 0 );
	std::cout.precision ( 8 );
	if ( argc < 2 )
	{
		std::cerr << "You have to supply a valid filename." << std::endl;
		return 1; // failure
	}
	const std::string filename ( argv[1] );
	// the 'silent' flag will still do all processing and everything, but instead of writing
	// to std::cout we now write to an empty sink. I do this to test the speed of the program itself because
	// even diverting the output to /dev/null would take up a lot of time.
	// obviously it would be faster to skip printing messages alltogether when we supply 'silent' but that
	// would be cheating and not particularly helpful when profiling
	bool silent ( argc >= 3 && !strcmp ( argv[2], "silent" ) );
	std::ostream & os ( silent ? null_str : std::cout );
	// will close on destruction
	std::ifstream infile ( filename.c_str(), std::ios::in );
	if ( !infile.good() )
	{
		std::cerr << "Problems finding/opening file [" << filename << "]" << std::endl;
		return 1; // another failure.
	}
	uint32_t counter = 0;
	while ( std::getline ( infile, line ) ) {
		feed.processMessage ( line, os );
		if ( ++counter % 10 == 0 ) {
			feed.printCurrentOrderBook ( os );
		}
	}
	feed.printCurrentOrderBook ( os );
	( os ) << std::endl;
	// errors are pretty relevant - you can't silence the truth
	feed.printErrorSummary ( std::cout );
	return !feed.errors().empty();
}

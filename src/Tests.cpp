#ifndef _WIN32
#define BOOST_TEST_DYN_LINK
#endif
#define BOOST_TEST_MODULE JumpBookTests

#include <algorithm>
#include <limits>
#include <iomanip>

#include <boost/test/unit_test.hpp>
#include <boost/format.hpp>

#ifdef PROFILE
#include <gperftools/profiler.h>
#endif

#include "OrderList.hpp"
#include "OrderBook.hpp"
#include "FeedHandler.hpp"

using namespace JumpInterview::OrderBook;

static std::string getFirst ( std::stringstream& ss )
{
	std::string str ( ss.str() );
	std::string line;
	std::getline ( ss, line );
	return line;
}

BOOST_AUTO_TEST_CASE ( processIncorrectLinesTest )
{
	FeedHandler handler;
	OrderBook::BuyPriceLevelMap buys ( handler.book().buys() );
	OrderBook::SellPriceLevelMap sells ( handler.book().sells() );
	ErrorSummary const & errors ( handler.errors() );
	std::ostringstream os;
	handler.processMessage ( "", os );
	BOOST_CHECK_EQUAL ( errors.corrupted_messages, ( size_t ) 1 );
	BOOST_CHECK ( buys.empty() );
	BOOST_CHECK ( sells.empty() );
	handler.processMessage ( ",,,,,,", os );
	BOOST_CHECK_EQUAL ( errors.corrupted_messages, ( size_t ) 2 );
	BOOST_CHECK ( buys.empty() );
	BOOST_CHECK ( sells.empty() );
	// order message with no numbers
	handler.processMessage ( "A,A,A,A,A", os );
	BOOST_CHECK_EQUAL ( errors.out_of_bounds_or_weird_numbers, ( size_t ) 1 );
	BOOST_CHECK ( buys.empty() );
	BOOST_CHECK ( sells.empty() );
	// or with s space
	handler.processMessage ( " A,1000,B,1,1000", os );
	BOOST_CHECK_EQUAL ( errors.corrupted_messages, ( size_t ) 3 );
	// order message with corrupted numbers
	handler.processMessage ( "X,10,B,a,1aa1", os );
	BOOST_CHECK_EQUAL ( errors.out_of_bounds_or_weird_numbers, ( size_t ) 2 );
	BOOST_CHECK ( buys.empty() );
	BOOST_CHECK ( sells.empty() );
	// don't allow negative order ids, volumes, prices or side that isn't B/S
	handler.processMessage ( "A,-1000,B,1,1000", os );
	handler.processMessage ( "A,1000,B,-1,1000", os );
	handler.processMessage ( "A,1000,B,1,-1000", os );
	handler.processMessage ( "A,1000,F,1,1000", os );
	BOOST_CHECK_EQUAL ( errors.out_of_bounds_or_weird_numbers, ( size_t ) 6 );
	BOOST_CHECK ( buys.empty() );
	BOOST_CHECK ( sells.empty() );
	// don't allow trades with a negative volume or price
	handler.processMessage ( "T,-2,100", os );
	handler.processMessage ( "T,2,-100", os );
	BOOST_CHECK_EQUAL ( errors.out_of_bounds_or_weird_numbers, ( size_t ) 8 );
	// or missing price but because there's a comma we expect to parse it
	handler.processMessage ( "T,2,", os );
	BOOST_CHECK_EQUAL ( errors.out_of_bounds_or_weird_numbers, ( size_t ) 9 );
	BOOST_CHECK ( buys.empty() );
	BOOST_CHECK ( sells.empty() );
	// or missing price ( again ) no comma so a corrupt message instead
	handler.processMessage ( "T,2", os );
	BOOST_CHECK_EQUAL ( errors.corrupted_messages, ( size_t ) 4 );
	BOOST_CHECK ( buys.empty() );
	BOOST_CHECK ( sells.empty() );
	// clearly this is wrong as well
	handler.processMessage ( "T", os );
	BOOST_CHECK_EQUAL ( errors.corrupted_messages, ( size_t ) 5 );
	// as is this
	handler.processMessage ( "!", os );
	BOOST_CHECK_EQUAL ( errors.corrupted_messages, ( size_t ) 6 );
	// don't allow doubles when we expect integers
	handler.processMessage ( "T,2.0,100", os );
	handler.processMessage ( "A,2.000,B,1,1000", os );
	handler.processMessage ( "A,211,B,10.0,1000", os );
	BOOST_CHECK_EQUAL ( errors.out_of_bounds_or_weird_numbers, ( size_t ) 12 );
	// or whitespace in random places
	handler.processMessage ( "A, 211,B,10,1000", os ); // actually, this we allow
	handler.processMessage ( "A,211, B,10,1000", os ); // this we definitely don't - we expect to see B/S in that spot
	handler.processMessage ( "A,211,B, 10,1000", os ); // actually, this we allow
	handler.processMessage ( "A,211,B,10, 1000", os ); // we look for whitespace to see where a comment starts, so this is pretty confusing
	BOOST_CHECK_EQUAL ( errors.out_of_bounds_or_weird_numbers, ( size_t ) 14 );
}

/*
 * We are using valgrind later on, so I can't make this number too ridiculously high.
 */
BOOST_AUTO_TEST_CASE ( processManyOrdersSmokeTest )
{
	FeedHandler handler;
	std::ostringstream os;
#ifdef PROFILE
	// Google perftools - if we compile this in we want to gather more samples and actually profile
	ProfilerStart ( "Smoke test" );
	for ( uint32_t i = 1 ; i < 100000; i ++ )
#else
	for ( uint32_t i = 1 ; i < 1000; i ++ )
#endif
	{
		static std::string actions ( "AMX" );
		char action ( actions[i % 3] );
		uint32_t order_id ( i % 30 );
		std::string side ( order_id < 15 ? "B" : "S" );
		uint32_t quantity ( i );
		double price ( ( double ) i / 10 );
		std::string line ( ( boost::format ( "%1%,%2%,%3%,%4%,%5%" )
							 % action
							 % order_id
							 % side
							 % quantity
							 % price ).str() );
		handler.processMessage ( line, os );
		os.clear();
	}
#ifdef PROFILE
	ProfilerStop();
#endif
	//handler.printCurrentOrderBook ( std::cout );
	BOOST_CHECK ( true );
}

BOOST_AUTO_TEST_CASE ( processNonCrossingOrdersTest )
{
	FeedHandler handler;
	OrderBook::BuyPriceLevelMap const & buys ( handler.book().buys() );
	OrderBook::SellPriceLevelMap const & sells ( handler.book().sells() );
	std::stringstream ss;
	Order_ptr order = 0;
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 0 );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 0 );
	handler.processMessage ( "A,000001,B,1,1000", ss );
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 1 );
	order = ( *buys.begin()->second->begin() )->order();
	BOOST_CHECK_EQUAL ( order->orderId(), 000001 );
	BOOST_CHECK_EQUAL ( order->price(), 1000000 );
	BOOST_CHECK_EQUAL ( order->side(), OrderSide::BUY );
	BOOST_CHECK_EQUAL ( getFirst ( ss ), "NAN" );
	ss.str ( "" );
	ss.clear();
	handler.processMessage ( "A,000002,S,1,1010", ss );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 1 );
	order = ( *sells.begin()->second->begin() )->order();
	BOOST_CHECK_EQUAL ( order->orderId(), 000002 );
	BOOST_CHECK_EQUAL ( order->price(), 1010000 );
	BOOST_CHECK_EQUAL ( order->side(), OrderSide::SELL );
	BOOST_CHECK_EQUAL ( getFirst ( ss ), "1005" );
	ss.str ( "" );
	ss.clear();
	handler.processMessage ( "A,000003,S,1,1020", ss );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 2 );
	order = ( *sells.begin()->second->begin() )->order();
	BOOST_CHECK_EQUAL ( order->orderId(), 000002 ); // hasn't changed
	BOOST_CHECK_EQUAL ( order->price(), 1010000 ); // hasn't changed
	BOOST_CHECK_EQUAL ( order->side(), OrderSide::SELL );
	BOOST_CHECK_EQUAL ( getFirst ( ss ), "1005" ); // hasn't changed
	ss.str ( "" );
	ss.clear();
	handler.processMessage ( "A,000004,S,1,1005", ss );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 3 );
	order = ( *sells.begin()->second->begin() )->order();
	BOOST_CHECK_EQUAL ( order->orderId(), 000004 ); // has changed
	BOOST_CHECK_EQUAL ( order->price(), 1005000 ); // has changed
	BOOST_CHECK_EQUAL ( order->side(), OrderSide::SELL );
	BOOST_CHECK_EQUAL ( getFirst ( ss ), "1002.5" ); // has changed
	ss.str ( "" );
	ss.clear();
	BOOST_REQUIRE ( handler.errors().empty() );
}

BOOST_AUTO_TEST_CASE ( processModifyingOrdersTest )
{
	FeedHandler handler;
	OrderBook::BuyPriceLevelMap const & buys ( handler.book().buys() );
	OrderBook::SellPriceLevelMap const & sells ( handler.book().sells() );
	std::stringstream ss;
	Order_ptr order = 0;
	ErrorSummary const & errors ( handler.errors() );
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 0 );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 0 );
	handler.processMessage ( "A,000001,B,1,1000", ss );
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 1 );
	order = ( *buys.begin()->second->begin() )->order();
	BOOST_CHECK_EQUAL ( order->orderId(), 000001 );
	BOOST_CHECK_EQUAL ( order->price(), 1000000 );
	BOOST_CHECK_EQUAL ( order->side(), OrderSide::BUY );
	BOOST_CHECK_EQUAL ( getFirst ( ss ), "NAN" );
	ss.str ( "" );
	ss.clear();
	handler.processMessage ( "A,000002,S,1,1010", ss );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 1 );
	order = ( *sells.begin()->second->begin() )->order();
	BOOST_CHECK_EQUAL ( order->orderId(), 000002 );
	BOOST_CHECK_EQUAL ( order->price(), 1010000 );
	BOOST_CHECK_EQUAL ( order->side(), OrderSide::SELL );
	BOOST_CHECK_EQUAL ( getFirst ( ss ), "1005" );
	ss.str ( "" );
	ss.clear();
	// change price
	handler.processMessage ( "M,000002,S,1,1020", ss );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 1 );
	order = ( *sells.begin()->second->begin() )->order();
	BOOST_CHECK_EQUAL ( order->orderId(), 000002 );
	BOOST_CHECK_EQUAL ( order->price(), 1020000 );
	BOOST_CHECK_EQUAL ( order->side(), OrderSide::SELL );
	BOOST_CHECK_EQUAL ( getFirst ( ss ), "1010" );
	ss.str ( "" );
	ss.clear();
	// change volume
	handler.processMessage ( "M,000002,S,1000,1020", ss );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 1 );
	order = ( *sells.begin()->second->begin() )->order();
	BOOST_CHECK_EQUAL ( order->orderId(), 000002 );
	BOOST_CHECK_EQUAL ( order->price(), 1020000 );
	BOOST_CHECK_EQUAL ( order->volume(), 1000 );
	BOOST_CHECK_EQUAL ( order->side(), OrderSide::SELL );
	BOOST_CHECK_EQUAL ( getFirst ( ss ), "1010" );
	ss.str ( "" );
	ss.clear();
	BOOST_CHECK ( errors.empty() );
}

BOOST_AUTO_TEST_CASE ( processUnknownModifyTest )
{
	// modify an order that doesn't exist. we don't like this but accept it as an insert anyway
	FeedHandler handler;
	ErrorSummary const & errors ( handler.errors() );
	OrderBook::BuyPriceLevelMap const & buys ( handler.book().buys() );
	OrderBook::SellPriceLevelMap const & sells ( handler.book().sells() );
	std::stringstream ss;
	Order_ptr order = 0;
	handler.processMessage ( "M,000002,S,1000,1020", ss );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 1 );
	order = ( *sells.begin()->second->begin() )->order();
	BOOST_CHECK_EQUAL ( order->orderId(), 000002 );
	BOOST_CHECK_EQUAL ( order->price(), 1020000 );
	BOOST_CHECK_EQUAL ( order->volume(), 1000 );
	BOOST_CHECK_EQUAL ( order->side(), OrderSide::SELL );
	BOOST_CHECK_EQUAL ( getFirst ( ss ), "NAN" );
	ss.str ( "" );
	ss.clear();
	BOOST_CHECK_EQUAL ( errors.order_modify_on_order_i_dont_know, ( uint32_t ) 1 );
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 0 );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 1 );
}

BOOST_AUTO_TEST_CASE ( processModifyWrongSideTest )
{
	// this we really don't allow - we try to change sides of an order
	FeedHandler handler;
	ErrorSummary const & errors ( handler.errors() );
	OrderBook::BuyPriceLevelMap const & buys ( handler.book().buys() );
	OrderBook::SellPriceLevelMap const & sells ( handler.book().sells() );
	std::stringstream ss;
	handler.processMessage ( "A,000002,S,1000,1020", ss );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 1 );
	handler.processMessage ( "M,000002,B,1000,1020", ss );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 1 );
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 0 );
	BOOST_CHECK_EQUAL ( errors.order_modify_on_wrong_side, ( uint32_t ) 1 );
}

BOOST_AUTO_TEST_CASE ( processDeleteUnknownOrderTest )
{
	FeedHandler handler;
	ErrorSummary const & errors ( handler.errors() );
	OrderBook::BuyPriceLevelMap const & buys ( handler.book().buys() );
	OrderBook::SellPriceLevelMap const & sells ( handler.book().sells() );
	std::stringstream ss;
	handler.processMessage ( "X,000002,S,1000,1020", ss );
	BOOST_CHECK_EQUAL ( errors.removes_with_no_corresponding_order, ( uint32_t ) 1 );
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 0 );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 0 );
}

BOOST_AUTO_TEST_CASE ( processInsertDuplicateOrderTest )
{
	FeedHandler handler;
	ErrorSummary const & errors ( handler.errors() );
	OrderBook::BuyPriceLevelMap const & buys ( handler.book().buys() );
	OrderBook::SellPriceLevelMap const & sells ( handler.book().sells() );
	std::stringstream ss;
	handler.processMessage ( "A,000002,S,1000,1020", ss );
	BOOST_CHECK ( errors.empty() );
	handler.processMessage ( "A,000002,S,1000,1020", ss );
	BOOST_CHECK_EQUAL ( errors.duplicate_order_id, ( size_t ) 1 );
	handler.processMessage ( "A,000002,B,1000,1020", ss );
	BOOST_CHECK_EQUAL ( errors.duplicate_order_id, ( size_t ) 2 );
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 0 );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 1 );
}

BOOST_AUTO_TEST_CASE ( processDeletingOrdersTest )
{
	FeedHandler handler;
	OrderBook::BuyPriceLevelMap const & buys ( handler.book().buys() );
	OrderBook::SellPriceLevelMap const & sells ( handler.book().sells() );
	std::stringstream ss;
	Order_ptr order = 0;
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 0 );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 0 );
	handler.processMessage ( "A,000001,B,1,1000", ss );
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 1 );
	order = ( *buys.begin()->second->begin() )->order();
	BOOST_CHECK_EQUAL ( order->orderId(), 000001 );
	BOOST_CHECK_EQUAL ( order->price(), 1000000 );
	BOOST_CHECK_EQUAL ( order->side(), OrderSide::BUY );
	BOOST_CHECK_EQUAL ( getFirst ( ss ), "NAN" );
	ss.str ( "" );
	ss.clear();
	handler.processMessage ( "A,000002,S,1,1010", ss );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 1 );
	order = ( *sells.begin()->second->begin() )->order();
	BOOST_CHECK_EQUAL ( order->orderId(), 000002 );
	BOOST_CHECK_EQUAL ( order->price(), 1010000 );
	BOOST_CHECK_EQUAL ( order->side(), OrderSide::SELL );
	BOOST_CHECK_EQUAL ( getFirst ( ss ), "1005" );
	ss.str ( "" );
	ss.clear();
	handler.processMessage ( "A,000003,S,20,1010", ss );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 1 /* still 1 price level */ );
	BOOST_CHECK_EQUAL ( sells.begin()->second->size(), ( size_t ) 2 /* with 2 orders */ );
	order = ( *sells.begin()->second->begin() )->order();
	BOOST_CHECK_EQUAL ( order->orderId(), 000002 );
	BOOST_CHECK_EQUAL ( order->price(), 1010000 );
	BOOST_CHECK_EQUAL ( order->volume(), 1 );
	BOOST_CHECK_EQUAL ( order->side(), OrderSide::SELL );
	BOOST_CHECK_EQUAL ( getFirst ( ss ), "1005" );
	ss.str ( "" );
	ss.clear();
	// delete the first one, second should stay in and
	// mid price shouldn't change
	handler.processMessage ( "X,000002,S,1,1010", ss );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 1 );
	order = ( *sells.begin()->second->begin() )->order();
	BOOST_CHECK_EQUAL ( order->orderId(), 000003 );
	BOOST_CHECK_EQUAL ( order->price(), 1010000 );
	BOOST_CHECK_EQUAL ( order->volume(), 20 );
	BOOST_CHECK_EQUAL ( order->side(), OrderSide::SELL );
	BOOST_CHECK_EQUAL ( getFirst ( ss ), "1005" );
	ss.str ( "" );
	ss.clear();
}

BOOST_AUTO_TEST_CASE ( processCrossingOrdersTest )
{
	FeedHandler handler;
	OrderBook::BuyPriceLevelMap const & buys ( handler.book().buys() );
	OrderBook::SellPriceLevelMap const & sells ( handler.book().sells() );
	ErrorSummary const & errors ( handler.errors() );
	std::stringstream ss;
	Order_ptr order = 0;
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 0 );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 0 );
	handler.processMessage ( "A,000001,B,4,1010", ss );
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 1 );
	order = ( *buys.begin()->second->begin() )->order();
	BOOST_CHECK_EQUAL ( order->orderId(), 000001 );
	BOOST_CHECK_EQUAL ( order->price(), 1010000 );
	BOOST_CHECK_EQUAL ( order->side(), OrderSide::BUY );
	BOOST_CHECK_EQUAL ( getFirst ( ss ), "NAN" );
	ss.str ( "" );
	ss.clear();
	handler.processMessage ( "A,000002,S,1,1000", ss );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 1 );
	order = ( *sells.begin()->second->begin() )->order();
	BOOST_CHECK_EQUAL ( order->orderId(), 000002 );
	BOOST_CHECK_EQUAL ( order->price(), 1000000 );
	BOOST_CHECK_EQUAL ( order->side(), OrderSide::SELL );
	BOOST_CHECK_EQUAL ( getFirst ( ss ), "1005" ); // we are crossing, but I still take the mid market price
	ss.str ( "" );
	ss.clear();
	BOOST_REQUIRE ( errors.empty() );
	// this trade isn't what we expected! still have expected trades
	handler.processMessage ( "T,2,1010", ss );
	BOOST_REQUIRE ( !errors.empty() );
	BOOST_CHECK_EQUAL ( errors.trades_with_no_corresponding_order, ( uint32_t ) 1 );
	ss.str ( "" );
	ss.clear();
	// this trade isn't what we expected either! still have expected trades
	handler.processMessage ( "T,1,1015", ss );
	BOOST_REQUIRE ( !errors.empty() );
	BOOST_CHECK_EQUAL ( errors.trades_with_no_corresponding_order, ( uint32_t ) 2 );
	ss.str ( "" );
	ss.clear();
	// this is what we expected, so now we're done
	handler.processMessage ( "T,1,1010", ss );
	BOOST_CHECK_EQUAL ( getFirst ( ss ), "1@1010" );
	BOOST_CHECK_EQUAL ( errors.trades_with_no_corresponding_order, ( uint32_t ) 2 );
	ss.str ( "" );
	ss.clear();
	// but traded volume at this level will still go up if we trade more unexpected volumes
	handler.processMessage ( "T,1,1010", ss );
	BOOST_CHECK_EQUAL ( getFirst ( ss ), "2@1010" );
	BOOST_CHECK_EQUAL ( errors.trades_with_no_corresponding_order, ( uint32_t ) 3 );
}

BOOST_AUTO_TEST_CASE ( processCrossingMyModifyingTest )
{
	FeedHandler handler;
	OrderBook::BuyPriceLevelMap const & buys ( handler.book().buys() );
	OrderBook::SellPriceLevelMap const & sells ( handler.book().sells() );
	std::stringstream ss;
	Order_ptr order = 0;
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 0 );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 0 );
	handler.processMessage ( "A,000001,B,4,1010", ss );
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 1 );
	order = ( *buys.begin()->second->begin() )->order();
	BOOST_CHECK_EQUAL ( order->orderId(), 000001 );
	BOOST_CHECK_EQUAL ( order->price(), 1010000 );
	BOOST_CHECK_EQUAL ( order->side(), OrderSide::BUY );
	BOOST_CHECK_EQUAL ( getFirst ( ss ), "NAN" );
	ss.str ( "" );
	ss.clear();
	handler.processMessage ( "A,000002,S,1,1020", ss );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 1 );
	order = ( *sells.begin()->second->begin() )->order();
	BOOST_CHECK_EQUAL ( order->orderId(), 000002 );
	BOOST_CHECK_EQUAL ( order->price(), 1020000 );
	BOOST_CHECK_EQUAL ( order->side(), OrderSide::SELL );
	BOOST_CHECK_EQUAL ( getFirst ( ss ), "1015" );
	ss.str ( "" );
	ss.clear();
	// move the price into crossing. now we are expecting a trade to follow.
	handler.processMessage ( "M,000002,S,1,1000", ss );
	BOOST_CHECK_EQUAL ( getFirst ( ss ), "1005" );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 1 );
}

BOOST_AUTO_TEST_CASE ( processTradeWhenNotExpectedTest )
{
	FeedHandler handler;
	OrderBook::BuyPriceLevelMap const & buys ( handler.book().buys() );
	OrderBook::SellPriceLevelMap const & sells ( handler.book().sells() );
	ErrorSummary const & errors ( handler.errors() );
	std::stringstream ss;
	handler.processMessage ( "T,1,1010", ss );
	BOOST_CHECK_EQUAL ( getFirst ( ss ), "1@1010" );
	BOOST_CHECK_EQUAL ( errors.trades_with_no_corresponding_order, ( uint32_t ) 1 );
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 0 );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 0 );
}

BOOST_AUTO_TEST_CASE ( processDeletePriceLevels )
{
	FeedHandler handler;
	OrderBook::BuyPriceLevelMap const & buys ( handler.book().buys() );
	OrderBook::SellPriceLevelMap const & sells ( handler.book().sells() );
	ErrorSummary const & errors ( handler.errors() );
	std::stringstream ss;
	handler.processMessage ( "A,90,S,1,110", ss );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 1 );
	handler.processMessage ( "A,100,B,1,100", ss );
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 1 );
	BOOST_CHECK_CLOSE ( handler.book().midPrice(), 105, 0.0001 );
	handler.processMessage ( "A,101,B,1,101", ss );
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 2 );
	BOOST_CHECK_CLOSE ( handler.book().midPrice(), 105.5, 0.0001 );
	handler.processMessage ( "A,102,B,1,102", ss );
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 3 );
	BOOST_CHECK_CLOSE ( handler.book().midPrice(), 106, 0.0001 );
	// a middle one takes the top spot
	handler.processMessage ( "M,101,B,1,108", ss );
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 3 );
	BOOST_CHECK_CLOSE ( handler.book().midPrice(), 109, 0.0001 );
	// that doesn't work, wrong price ..
	handler.processMessage ( "X,101,B,1,101", ss );
	BOOST_CHECK ( errors.removes_with_no_corresponding_order == 1 );
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 3 );
	// ok, delete the top spot
	handler.processMessage ( "X,101,B,1,108", ss );
	BOOST_CHECK ( errors.removes_with_no_corresponding_order == 1 );
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 2 );
	BOOST_CHECK_CLOSE ( handler.book().midPrice(), 106, 0.0001 );
	handler.processMessage ( "X,100,B,1,100", ss );
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 1 );
	BOOST_CHECK_CLOSE ( handler.book().midPrice(), 106, 0.0001 );
	handler.processMessage ( "X,102,B,1,102.0", ss );
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 0 );
	BOOST_CHECK_CLOSE ( handler.book().midPrice(), std::numeric_limits<double>::max(), 0.0001 );
	// add to top again
	handler.processMessage ( "A,102,B,1,102.00000000000001      // we'll round this", ss );
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 1 );
	BOOST_CHECK_CLOSE ( handler.book().midPrice(), 106, 0.0001 );
	handler.processMessage ( "X,102,B,1,102", ss );
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 0 );
	BOOST_CHECK_CLOSE ( handler.book().midPrice(), std::numeric_limits<double>::max(), 0.0001 );
}

BOOST_AUTO_TEST_CASE ( processNoTradeWhenExpectedTest )
{
	FeedHandler handler;
	ErrorSummary const & errors ( handler.errors() );
	OrderBook::BuyPriceLevelMap const & buys ( handler.book().buys() );
	OrderBook::SellPriceLevelMap const & sells ( handler.book().sells() );
	std::stringstream ss;
	Order_ptr order = 0;
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 0 );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 0 );
	handler.processMessage ( "A,000001,B,1,1020", ss );
	BOOST_CHECK_EQUAL ( buys.size(), ( size_t ) 1 );
	order = ( *buys.begin()->second->begin() )->order();
	BOOST_CHECK_EQUAL ( order->orderId(), 000001 );
	BOOST_CHECK_EQUAL ( order->price(), 1020000 );
	BOOST_CHECK_EQUAL ( order->side(), OrderSide::BUY );
	BOOST_CHECK_EQUAL ( getFirst ( ss ), "NAN" );
	ss.str ( "" );
	ss.clear();
	handler.processMessage ( "A,000002,S,2,1010", ss );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 1 );
	order = ( *sells.begin()->second->begin() )->order();
	BOOST_CHECK_EQUAL ( order->orderId(), 000002 );
	BOOST_CHECK_EQUAL ( order->price(), 1010000 );
	BOOST_CHECK_EQUAL ( order->volume(), 2 );
	BOOST_CHECK_EQUAL ( order->side(), OrderSide::SELL );
	BOOST_CHECK_EQUAL ( getFirst ( ss ), "1015" );
	ss.str ( "" );
	ss.clear();
	// at this stage we would expect a trade message to come in.
	// instead, we get an order modify ( we've traded 1 lot )
	// that's unexpected
	handler.processMessage ( "M,000002,S,1,1010", ss );
	BOOST_CHECK_EQUAL ( sells.size(), ( size_t ) 1 );
	order = ( *sells.begin()->second->begin() )->order();
	BOOST_CHECK_EQUAL ( order->orderId(), 000002 );
	BOOST_CHECK_EQUAL ( order->price(), 1010000 );
	BOOST_CHECK_EQUAL ( order->volume(), 1 );
	BOOST_CHECK_EQUAL ( order->side(), OrderSide::SELL );
	BOOST_CHECK_EQUAL ( getFirst ( ss ), "1015" );
	ss.str ( "" );
	ss.clear();
	BOOST_CHECK_EQUAL ( errors.no_trades_when_they_should_happen, ( size_t ) 1 );
}





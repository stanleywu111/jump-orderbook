# Intro
This program builds its own representation of an order book based on messages such as 

* A,100000,S,1,1075 ( add an order with id 100000, sell 1@1075 )
* X,100004,B,10,950 ( delete an order with id 100004, sell 10@950 )
* T,1,1025 ( process a trade of 1@1025 )
* X,100008,B,3,1050 (delete an order with id 100008, buy 3@1050 )

After every message we display the mid price ( NAN if we don't have it ), which is the simple average of the top bid and top ask. Every 10th message we print the current order book, and after every trade we print the total volume traded on that level since we've been trading there. 

The program is meant to be quick, robust and easy enough to understand. 

# Usage

main [input file] [optionally:silent]
There's two input files provided; smaller.txt ( which I copied from the email ) and bigger.txt which is generated.
Bigger.txt first creates a bunch of orders, and modifies them up straight away. Then, some orders are deleted and finally trades come in.
It turns out that actually printing takes the most time. By far. So, if you set it to silent if will still format the messages but not show them.

# Dependencies

* gcc - should support C++11. My version is [ gcc (Ubuntu/Linaro 4.7.2-2ubuntu1) 4.7.2 ]
* valgrind - we run unit tests in valgrind
* strip - we strip the release binary
* boost unit test headers and lib. I used the one in the Ubuntu libboost1.50-dev package
* (optional) google perftools, pprof if you want to run profiling on a larger smoke test

# Building
make/make all will:

* clean any old executables/objects
* build the tests ( with 'debug' flags )
* runs them within valgrind ( set to break if there's a problem )
* build the main file ( with 'release' flags )
* runs it on the sample provided in the email
whereas 'make profile' will:
* build the tests ( optimised and with more to do, and support for google perftols )
* run the tests
* display the output

# Design

All prices are converted into uint32_t. This is because we need to compare these in a binary tree and testing for double equality can be tricky. To go from double to uint32_t, we multiply the price by 1000.0 ( defined in Constants.hpp ) and round it down. If that's not sufficient, this 1000.0 needs to be incremented.

I seperate the B/S sides. Each side gets its own PriceLevelMap. This is a ( std::map, std::unordered_map ) combination that lets us quickly O(1) jump to existing price levels. Levels are deleted or created at a panalty of O(logN), making that the most expensive operation we can have. Each item in a PriceLevelMap is an OrderList. This is a linked list of orders. Orders are simply inserted at the back, and we assume that when we trade, the ones at the front get their turn first. Those operations take O(1). Finally, we have the orders, which we actually store with a sequence_id. We need those to compare timestamps between both sides, to see where we expect to trade. To allow quick access to our orders, we have a seperate hash table that stores the OrderList::iterators. This way, we can easily jump to the order to change say it's volume. Also, this will let us remove an order from an orderlist without having to step through it. This operation now also takes O(1).

Hierarchically this might look like

* OrderBook
    * Buy PriceLevelMap ( std::map and std::unordered_map )
        * 1000 OrderList ( std::list )
    		* B 3 x 1000 ( t0 )
    		* B 4 x 1000 ( t2 )
    	* 900  OrderList
    		* B 1 x 900  ( t3 )
    * Sell PriceLevelMap
    	* 1025 OrderList
    		* S 3 x 1025 ( t1 )
    		* S 2 x 1025 ( t4 )
    	* 1020 OrderLit
    		* S 5 x 1020 ( t5 )
    * Constant time look up table from order_id to OrderList::iterator ( std::unordered_map )

Side node; I do know that we can't guarantee that the std collections are actually implemented the way I said they are. I've never
come across an implementation that purposefully choses to do something else though..

For orders I have the following time complexities ( this table doesn't display well in a markdown viewer - use a tesxt editor for that ):

			    PriceLevelMap		List		Helper Hashtable
* Add			O(logN)			    O(1)		O(1)		-> Only O(logN) when we're creating a new price level. O(1) otherwise
* Remove		O(1)			    O(1)		O(1)
* Modify Volume
		Down	-	    	    	-		    O(1)
		Up	    O(1)	    		O(1)		O(1)
* Modify Price	O(logN)			    O(1)		O(1)		-> Same as above

The very worst case performance of a binary tree is O(N). This makes our worst case performance the same O(N), when we insert an order at a new price level and the tree isn't balanced. This will happen when all nodes have been added in the reverse-sorted order. This way, our tree basically looks like a linked list, for which we know we've got O(N) performance. But, most operations ( including most order add/modifications ) happen in constant time.

** Please let me know if there's a better way to do this - now I'm really intrigued. **

# Memory allocation

I like tcmalloc and boost pool allocator. In this case however I decided to roll my own. This is a simple recycling pool which I used for all orders, order nodes, lists and trades. Initially we allocate memory normally, but when it comes to returning memory we don't actually do that if there's still space on the queue. The next time we have to allocate memory and there is still some available in the queue, we return that. This works best when orders a typically added and removed in quick succession. If however it looks like we'll be adding a whole lot of orders in one go, we'd still be allocating memory often. In that case, it would make sense to allocate objects in whole chunks, say 25 at a time, still within the PoolAllocator.

String formatting now takes up most time. That's because for every ten lines, I'm going to write down the complete book. To make this quicker, I only format my order when something's changed and keep re-using a char[] when I can. 

# Exception handling

I'm expecting weird messages to come in, or messages to come in in the wrong order. I wouldn't want to throw exceptions when this happens as that would be very slow. Rather, I directly increment the relevant error counter. If we do get an exception, we do catch it and increment the 'unexpected_exception' counter. I really don't want that to happen. The coding excersize listed six specific areas of interest. I've added three more to make it even more specific.

# On ptr const & 

In my orderbook I have the function 'bool add ( Order_ptr const & order ) ;' When my Order_ptr is a simple pointer, this actually doesn't help us in the slightest. I prefer this anyway because I typically use more smart pointers, in which case you want to pass them along const & to prevent copies. Not adding the const & just makes it look wrong then.

# On detecting missing/wrong trades

To detect missing or wrong trades this happens:

* When an order is responsible for crossing ( reaches a new top price level that will trade ) , it will clear out the existing list of expected trades and set a boolean flag.
* At every trade entry we check to see if we are crossed. If we're not, that's a problem.
* If we are crossed and we haven't yet filled a new vector of expected trades ( checking that boolean I mentioned earlier ), we do so now.
	 * If the vector is empty and we had already filled it in ( flag is now false ), we've already traded our volume at this level which is an error.
* We have a vector of expected trades and our new trade - this trade should match the top of that vector in price and volume otherwise it's an error.

This works very quickly when things happen as expected, but not quite so when we keep on reaching a new crossing level, and get wrong trades inbetween. Every time that happens, we will calculate a new list of expected levels.

# Ambiguity

I think the following things are open to interpretation. This is how I resolved it:

I wasn't sure if 'X,100004,B,10,950 // cancel' would actually be valid so I decided to allow such messages. This is obviously a little bit slower than if I had treated that as an invalid message.

* When we cancel an order, we expect the price and side to match ( volume can have changed, and the user might be unaware ).
* When we modify an order, we only check for the order id and side, as of course this can't change but volume/price can.
I believe these assumptions are ok, but obviously open to interpretation.

I am assuming that when we submit a crossing order, the order of events will be:

* Insert the new order ( or move price of existing order up )
* Get one or more fills back
* Get two or more order modifications back
If that's not the order in which things happen, I will not detect the state correctly.

I make no assumption that order ids are in fact ordered. It doesn't matter anyway, their priority changes on a modify so we have to keep track of ordering ourselves.

On every message I am to write both the mid price and if there's a trade the traded volume and price. If this is a trade, I write that first.

# Limitations

* None of this is meant to be thread-safe. This is actually the main reason I chose not to use boost (fast) pool allocator. We can do with an easier, custom allocator.
* Order_ids, prices are stored as uint32_t. I do expect this to be enough but obviously any type can overflow if you want it to.

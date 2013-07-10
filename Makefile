RELEASE_FLAGS = "-O3 -Wall -DNDEBUG"
DEBUG_FLAGS = "-O0 -g -Wall -Werror"
RELEASE_PROFILE_FLAGS = "-O3 -Wall -DNDEBUG -DPROFILE -lprofiler"

all: clean debug release

lib/$(VERSION)/ErrorSummary.o : src/ErrorSummary.cpp
	g++ -std=c++11 -c $< -pipe $(FLAGS) -o $@

lib/$(VERSION)/FeedHandler.o : src/FeedHandler.cpp
	g++ -std=c++11 -c $< -pipe $(FLAGS) -o $@

lib/$(VERSION)/Main.o : src/Main.cpp
	g++ -std=c++11 -c $< -pipe $(FLAGS) -o $@

lib/$(VERSION)/Order.o : src/Order.cpp
	g++ -std=c++11 -c $< -pipe $(FLAGS) -o $@

lib/$(VERSION)/OrderBook.o : src/OrderBook.cpp
	g++ -std=c++11 -c $< -pipe $(FLAGS) -o $@

lib/$(VERSION)/OrderList.o : src/OrderList.cpp
	g++ -std=c++11 -c $< -pipe $(FLAGS) -o $@

lib/$(VERSION)/Tests.o : src/Tests.cpp
	g++ -std=c++11 -c $< -pipe $(FLAGS) -o $@

lib/$(VERSION)/Trade.o : src/Trade.cpp
	g++ -std=c++11 -c $< -pipe $(FLAGS) -o $@

release:
	mkdir lib;mkdir lib/release;/bin/true
	VERSION=release FLAGS=$(RELEASE_FLAGS) make main
	# Every little helps .. ( runtime performance, this will make debugging much harder )
	strip main
debug:
	mkdir lib;mkdir lib/debug;/bin/true
	VERSION=debug FLAGS=$(DEBUG_FLAGS) make main-valgrind
	VERSION=debug FLAGS=$(DEBUG_FLAGS) make tests-valgrind

profile:
	mkdir lib;mkdir lib/profile;/bin/true
	VERSION=profile FLAGS=$(RELEASE_PROFILE_FLAGS) make tests-profile
	CPUPROFILE_FREQUENCY=1000 CPUPROFILE=tests.prof ./tests
	VERSION=profile FLAGS=$(RELEASE_PROFILE_FLAGS) make main
	strip main
	google-pprof --text ./tests ./tests.prof
	echo Measuring how long it takes to run 23445 operations, with output disabled
	time --quiet ./main ./bigger.txt silent

style:
	# This is my coding standard. There are many like it, but this is mine
	astyle --indent=force-tab --pad-oper --pad-paren --delete-empty-lines --suffix=none --indent-namespaces --indent-col1-comments -n --recursive *.cpp *.hpp

tests: lib/$(VERSION)/ErrorSummary.o lib/$(VERSION)/FeedHandler.o lib/$(VERSION)/Order.o lib/$(VERSION)/OrderBook.o lib/$(VERSION)/OrderList.o lib/$(VERSION)/Tests.o lib/$(VERSION)/Trade.o 
	g++ $^ -lboost_unit_test_framework -o tests

tests-profile: lib/$(VERSION)/ErrorSummary.o lib/$(VERSION)/FeedHandler.o lib/$(VERSION)/Order.o lib/$(VERSION)/OrderBook.o lib/$(VERSION)/OrderList.o lib/$(VERSION)/Tests.o lib/$(VERSION)/Trade.o -lprofiler
	g++ $^ -lboost_unit_test_framework -o tests

tests-valgrind: tests
	valgrind --error-exitcode=1 ./tests
	
main: lib/$(VERSION)/ErrorSummary.o lib/$(VERSION)/FeedHandler.o lib/$(VERSION)/Main.o lib/$(VERSION)/Order.o lib/$(VERSION)/OrderBook.o lib/$(VERSION)/OrderList.o lib/$(VERSION)/Trade.o
	g++ $^ -o main -pipe
	
main-valgrind: main
	valgrind --error-exitcode=1 ./main smaller.txt
	
clean:
	rm -Rf tests main lib/*/*.o orderbook_michiel_van_slobbe.tgz tests.prof
	
package: clean style debug release
	find . -name "*~" -exec rm {} \;
	rm -Rf tests main lib/* orderbook_michiel_van_slobbe_1.1.tgz
	tar cvzf orderbook_michiel_van_slobbe_1.1.tgz src Makefile smaller.txt bigger.txt README.md order_gen.R
	
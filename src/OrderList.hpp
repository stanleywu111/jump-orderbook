#ifndef __ORDER_LIST_HPP__
#define __ORDER_LIST_HPP__

#include <stdint.h>
#include <list>
#include <memory>

#include "Order.hpp"
#include "PoolAllocator.hpp"


namespace JumpInterview {
	namespace OrderBook {

		// An order node only contains a pointer to an order. It is not in charge of disposing it
		// when we remove it from the list.
		class OrderNode
		{
		public:
			OrderNode ( Order_ptr const & order,
						uint32_t sequence_id ) : m_order ( order ),
				m_sequence_id ( sequence_id )
			{
			}

			Order_ptr const & order() const
			{
				return m_order;
			}

			uint32_t const & sequence_id() const
			{
				return m_sequence_id;
			}

			static inline void* operator new ( std::size_t sz )
			{
				return PoolAllocator<OrderNode>::instance().allocate ( sz ) ;
			}

			static inline void operator delete ( void* p )
			{
				PoolAllocator<OrderNode>::instance().deallocate ( static_cast< OrderNode * > ( p ), sizeof ( OrderNode ) ) ;
			}
		private:
			Order_ptr m_order;
			// when crossing, we use this to find out at what level that should happen
			uint32_t m_sequence_id;
		};
		typedef std::shared_ptr < OrderNode > OrderNode_ptr;
		typedef std::list < OrderNode_ptr > OrderNode_list;

		class OrderList
		{
		public:
			OrderList();
			~OrderList();
			OrderNode_list::iterator add ( Order_ptr const & order, uint32_t sequence_id );
			void remove ( OrderNode_list::iterator order_iter );
			bool empty() const;
			size_t size() const;
			OrderNode_list::iterator begin();
			OrderNode_list::iterator end();
			static inline void* operator new ( std::size_t sz )
			{
				return PoolAllocator<OrderList>::instance().allocate ( sz ) ;
			}

			static inline void operator delete ( void* p )
			{
				PoolAllocator<OrderList>::instance().deallocate ( static_cast< OrderList * > ( p ), sizeof ( OrderList ) ) ;
			}
		private:
			;
			OrderList ( OrderList const & rhs ) {}
			OrderNode_list m_list;
		};
		typedef std::shared_ptr < OrderList > OrderList_ptr;

		std::ostream& operator<< ( std::ostream& os, OrderList& order );
	}
}

#endif
#include <assert.h>
#include <algorithm>

#include "OrderList.hpp"

namespace JumpInterview {
	namespace OrderBook {

		OrderList::OrderList()
		{
		}

		OrderList::~OrderList()
		{
			for ( OrderNode_list::iterator iter = m_list.begin();
					iter != m_list.end();
					iter++ )
				delete ( ( *iter )->order() );
		}

		OrderNode_list::iterator OrderList::begin()
		{
			return m_list.begin();
		}

		OrderNode_list::iterator OrderList::end()
		{
			return m_list.end();
		}

		bool OrderList::empty() const
		{
			return m_list.empty();
		}

		size_t OrderList::size() const
		{
			return m_list.size();
		}

		OrderNode_list::iterator OrderList::add ( Order_ptr const & order,
				uint32_t sequence_id )
		{
			OrderNode_ptr node = std::make_shared < OrderNode > ( order, sequence_id );
			m_list.push_back ( node );
			OrderNode_list::iterator last ( m_list.end() );
			return --last;
		}

		void OrderList::remove ( OrderNode_list::iterator order_iter )
		{
			OrderNode_list::iterator c_iter ( m_list.begin() );
			m_list.erase ( order_iter );
		}

		std::ostream& operator<< ( std::ostream& os, OrderList& list )
		{
			assert ( !list.empty() );
			for ( OrderNode_list::iterator iter = list.begin();
					iter != list.end();
					iter++ )
				os << * ( *iter )->order() << std::endl;
			return os;
		}
	}
}

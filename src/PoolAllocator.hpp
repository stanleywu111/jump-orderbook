#ifndef __POOL_ALLOCATOR_HPP__
#define __POOL_ALLOCATOR_HPP__

#include <assert.h>
#include <stack>
#include <memory>

namespace JumpInterview {
	namespace OrderBook {

		/*
		 * We expect the number of objects to stay relatively stable once we're in business. If that's not the case,
		 * it makes more sense to allocate memory in chunks. For instance by creating a T* chunk = new T[25] and filling
		 * the stack with those whenever it's empty. There are two downsides to this:
		 *
		 * 1. The T objects need to have a default () constructor that's accessible by the PoolAllocator.
		 * 2. Slightly more memory overhead because every time we allocate 25, we've got 24 waiting there to be taken up.
		 *
		 * I decided not to do this, but if the situation calls for it, that would definitely be possible.
		 */
		template <class T>
		class PoolAllocator : public std::allocator<T>
		{
		public:
			/* The pool allocator cuts down on the number of mallocs we have to do. On the flip-side, it is less memory efficient.  */
			PoolAllocator<T> ( )
			{
			}

			static PoolAllocator<T> & instance()
			{
				static PoolAllocator<T> instance;
				return instance;
			}

			~PoolAllocator<T>()
			{
				while ( !m_stack.empty() )
				{
					T* top = m_stack.top();
					assert ( top );
					std::allocator<T>::deallocate ( top, sizeof ( T ) );
					m_stack.pop();
				}
			}

			T* allocate ( size_t n, const void * hint = 0 )
			{
				if ( m_stack.empty() )
					return std::allocator<T>::allocate ( n, hint );
				T* t = m_stack.top();
				m_stack.pop();
				return t;
			}

			void deallocate ( T* t, size_t n )
			{
				static const size_t max_size ( 1000 );
				assert ( t );
				if ( m_stack.size() < max_size )
					m_stack.push ( t );
				else
					std::allocator<T>::deallocate ( t, n );
			}

		private:
			std::stack<T*> m_stack;
			PoolAllocator<T> ( PoolAllocator<T> const & rhs ) {}
		};
	}
}

#endif

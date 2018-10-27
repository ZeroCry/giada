/* -----------------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * -----------------------------------------------------------------------------
 *
 * Copyright (C) 2010-2018 Giovanni A. Zuliani | Monocasual
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * -------------------------------------------------------------------------- */


#ifndef G_QUEUE_H
#define G_QUEUE_H


#include <array>
#include <atomic>


namespace giada {
namespace m {
template <typename T, size_t fixedSize>
class Queue
{
public:
 
 	Queue() {}
	Queue(const Queue& src) = delete; // Can't be copied
 
 	/* isEmpty
 	Returns whether the queue is empty. */

	bool isEmpty() const noexcept
	{
		const size_t readPosition  = m_readPosition.load();
		const size_t writePosition = m_writePosition.load();	
		return readPosition == writePosition;
	}
 
	/* push
	Pushes an element to the queue. Returns true when the element was added, 
	false when the queue is full. */

	bool push(const T& element)
	{
		const size_t oldWritePosition = m_writePosition.load();
		const size_t newWritePosition = getPositionAfter(oldWritePosition);
		const size_t readPosition     = m_readPosition.load();
		
		if (newWritePosition == readPosition) // The queue is full
			return false;
		
		m_ringBuffer[oldWritePosition] = element;
		m_writePosition.store(newWritePosition);
		
		return true;
	}

	/* pop
	Pops an element from the queue. Returns true when succeeded, false when the 
	queue is empty. */

	bool pop(T& element)
	{
		if (isEmpty())
			return false;
		
		const size_t readPosition = m_readPosition.load();
		element = std::move(m_ringBuffer[readPosition]);
		m_readPosition.store(getPositionAfter(readPosition));
		
		return true;
	}
 
	/* clear
	Clears the content from the queue. */

	void clear() noexcept
	{
		const size_t readPosition  = m_readPosition.load();
		const size_t writePosition = m_writePosition.load();
		
		if (readPosition != writePosition)
			m_readPosition.store(writePosition);
	}
 
	/* getMaxSize
	Returns the maximum size of the queue. */

	constexpr size_t getMaxSize() const noexcept
	{
		return RingBufferSize - 1;
	}
 
	/* getSize
	Returns the actual number of elements in the queue. */

	size_t getSize() const noexcept
	{
		const size_t readPosition  = m_readPosition.load();
		const size_t writePosition = m_writePosition.load();
		
		if (readPosition == writePosition)
			return 0;
		
		size_t size = 0;
		if (writePosition < readPosition)
			size = RingBufferSize - readPosition + writePosition;
		else
			size = writePosition - readPosition;

		return size;
	}
 
	static constexpr size_t getPositionAfter(size_t pos) noexcept
	{
		return ((pos + 1 == RingBufferSize) ? 0 : pos + 1);
	}
	 
private:

	static constexpr size_t RingBufferSize = fixedSize + 1;
	std::array<T, RingBufferSize> m_ringBuffer;
	std::atomic<size_t> m_readPosition  = {0};
	std::atomic<size_t> m_writePosition = {0};    
};
}}; // giada::m::


#endif

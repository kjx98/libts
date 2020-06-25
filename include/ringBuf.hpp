#pragma once
#ifndef	__RINGBUF_HPP__
#define	__RINGBUF_HPP__

#include <cstddef>
#include <cstdint>
#include <array>
#include <memory>
#include <thread>
#include <assert.h>
#include <boost/lockfree/spsc_queue.hpp>
#include "ts3/timestamp.hpp"

using namespace ts3;

template <typename T, size_t rgSize=1024> 
class RingBuffer {
public:
	RingBuffer<T,rgSize>(const int timeout_Ms=1000, const bool bYield=true):
			nWaits_(0), nSlowWaits_(0),
			yield_(bYield),
			timeout_(timeout_Ms*1e6) {}
	RingBuffer<T,rgSize>(RingBuffer&& ringB):
			nWaits_(ringB.nWaits_), nSlowWaits_(ringB.nSlowWaits_),
			yield_(ringB.yield_),
			timeout_(ringB.timeout_)
	{
		queue_ = std::move(ringB.queue_);
	}
	bool push(const T &dat) noexcept {
		timespec tpS;
		clock_gettime(CLOCK_REALTIME, &tpS);
		while (ts3_unlikely(!queue_.push(dat))) {
			++nWaits_;
			if (yield_) std::this_thread::yield();
			if (timeout_ > 0) {
				timespec tpE;
				clock_gettime(CLOCK_REALTIME, &tpE);
				if (tpE - tpS > timeout_) return false;
			}
		}
		{
			timespec tpE;
			clock_gettime(CLOCK_REALTIME, &tpE);
			if (ts3_unlikely(tpE - tpS > 1000)) ++nSlowWaits_;
		}
		return true;
	}
	bool pop(T &dat) noexcept {
		return queue_.pop(dat);
	}
	bool isEmpty() noexcept { return queue_.empty(); }
#if	__GNUC__ > 6
	size_t	write_available() const { return queue_.write_available(); }
#endif
	bool is_lockfree() noexcept { return queue_.is_lock_free(); }

	size_t	waitCount() const { return nWaits_; }
	size_t	waitSlowCount() const { return nSlowWaits_; }

private:
	boost::lockfree::spsc_queue<T, boost::lockfree::capacity<rgSize> > queue_;
	size_t	volatile nWaits_;
	size_t	volatile nSlowWaits_;
	bool	yield_;
	int64_t	timeout_;
};
#endif	//	__RINGBUF_HPP__

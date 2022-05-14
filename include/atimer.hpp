/*
 * Copyright (C) 2020 Jesse Kuang <jkuang@21cn.com>
 *
 * This is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#ifndef	__ACCURATETIMER_HPP__
#define	__ACCURATETIMER_HPP__

#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <map>
#include <functional>
// <format.h> for C++20
#include <fmt/format.h>
#include "ts3/timestamp.hpp"


namespace ts3::Timer {

typedef std::function<void ()> voidFunc;
using diff_t = int64_t;
#ifdef	WITH_MILLISEC
using duration_t = std::chrono::milliseconds;
#else
using duration_t = std::chrono::microseconds;
#endif
using sclock_t = std::chrono::steady_clock;

struct timerKey_t {
	int		id_;
	diff_t	nextRun_;
	timerKey_t(int id, diff_t nextRun): id_(id), nextRun_(nextRun) {}
	timerKey_t(const timerKey_t&) = default;
};

bool operator< (const timerKey_t& lhs, const timerKey_t& rhs) noexcept {
	if (ts3_unlikely(lhs.nextRun_ == rhs.nextRun_))
		return lhs.id_ < rhs.id_;
	return lhs.nextRun_ < rhs.nextRun_;
}

bool operator== (const timerKey_t& lhs, const timerKey_t& rhs) noexcept {
	return lhs.id_ == rhs.id_ && lhs.nextRun_ == rhs.nextRun_;
}

struct Event {
	int id_;			// Unique id for each event object
	diff_t timeout_;	// Interval for execution(in microsec)
	diff_t nextRun_;	// Next scheduled execution time(measured w.r.t start of loop)
	bool repeat_;		// Repeat event indefinitely if true
	bool running_;
	int overrun_;
	// Event handler callback
	voidFunc eventHandler_;
	Event(int id, diff_t timeout, diff_t nextRun, bool repeat,
			voidFunc eventHandler) : id_(id), timeout_(timeout),
		nextRun_(nextRun), repeat_(repeat), running_(false), overrun_(0),
		eventHandler_(eventHandler) {
	}
	Event(const Event&) = default;
};

struct hms_t {
	int	hour_;
	int	min_;
	int sec_;
	hms_t(const int hour, const int min=0, const int sec=0) :
		hour_(hour), min_(min), sec_(sec)
	{ }
};

class TimerQueue {
public:
	using time_point_t = std::chrono::time_point<sclock_t>;
	TimerQueue() : stopThread_(false),
		fallThrough_(false),
		currMin_(0),
		waitTime_(0),
		nExpires_(0),
		nSpinWait_(0),
		nextId_(1),
		startTime_(sclock_t::now())
	{
#ifndef	WITH_MILLISEC
		//ts3::tsc_clock&	tscc(ts3::tsc_clock::Instance());
		//jitter_ = tscc.jitter() + 50;
		jitter_ = 200;
#endif
	}
	~TimerQueue() {
		if (!stopThread_) {
			shutdown();
		}
	}

	// f(args...) should return as soon as possible, especialy if repeat false
	template<class F, class... Args>
	int create(diff_t timeout, bool repeat, F&& f, Args&&... args);
	int cancel(int id);
	// f(args...) MUST return as soon as possible
	template<class F, class... Args>
	int expire_on(hms_t && hms, F&& f, Args&&... args) noexcept {
		return expire_impl(std::forward<hms_t>(hms), f, args...);
	}
	template<class F, class... Args>
	int expire_on(hms_t& hms, F&& f, Args&&... args) noexcept {
		return expire_impl(std::forward<hms_t>(hms), f, args...);
	}

	static TimerQueue & Instance() {
		static TimerQueue accuTimer;
		return accuTimer;
	}
	int timerLoop();
	void shutdown() {
		stopThread_ = true;
		eventQCond_.notify_all();
		emptyQCond_.notify_all();
	}
	int	SpinWaits() noexcept { return nSpinWait_; }
	int Timeouts() noexcept { return nExpires_; }

	// Delete all constructors since this is singleton pattern
	TimerQueue(const TimerQueue &)=delete;
	TimerQueue(TimerQueue &&)=delete;
	TimerQueue & operator=(const TimerQueue &)=delete;
	TimerQueue & operator=(TimerQueue &&)=delete;
private:
	template<class F, class... Args>
	int expire_impl(hms_t && hms, F&& f, Args&&... args) noexcept {
		struct timespec tp;
		clock_gettime(CLOCK_REALTIME, &tp);
		ts3::LocalTime	ltm(tp);
		auto timeo = ltm.us_next_hm(hms.hour_, hms.min_, hms.sec_);
#ifdef	WITH_MILLISEC
		timeo /= 1000;
#endif
		return create(timeo, false, f, args...);
	}

	std::mutex eventQMutex_;
	std::condition_variable eventQCond_;
	std::mutex emptyQMutex_;
	std::condition_variable emptyQCond_;

	volatile bool stopThread_;
	volatile bool fallThrough_;
	diff_t currMin_;
	diff_t waitTime_;
#ifndef	WITH_MILLISEC
	int64_t	jitter_;
#endif
	int	nExpires_;
	int	nSpinWait_;
	int nextId_;

	time_point_t startTime_;
	std::map<timerKey_t, std::shared_ptr<Event> > eventMap_;
};


template<class F, class... Args>
int TimerQueue::create(diff_t timeout, bool repeat, F&& f, Args&&... args) {
	diff_t prevMinTimeout; 

	// Define generic funtion with any number / type of args
	// and return type, make it callable without args(void func())
	// using std::bind
	auto diff = std::chrono::duration_cast<duration_t>(
							sclock_t::now() - startTime_).count();
	auto func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
	std::function<void()> task = func;

	// Create new event object
	{
		std::unique_lock<std::mutex> lock(eventQMutex_);
		auto id = nextId_++;

		// Save previous timeout value to ensure fall through
		// incase 'this' timeout is lesser than previous timeout
		prevMinTimeout = currMin_;

		// Chain events with same timeout value
		diff_t nextRun = diff + timeout;
		auto ev=std::make_shared<Event>(id, timeout, nextRun, repeat, task);
		timerKey_t	key(id, nextRun);
#ifdef	_NTEST
		std::cout << fmt::format("create timer {} timeout: {}, nextRun: {}\n",
						id, timeout, nextRun);
#endif
		eventMap_.insert(eventMap_.begin(), std::make_pair(key, ev));

		// For first run trigger wait asap else find min
		// timeout value and wait till it expires or event
		// with lesser timeout is queued
		if (!currMin_) {
			currMin_ = nextRun;
		} else {
			auto minElem = eventMap_.begin();
			currMin_ = (*minElem).second->nextRun_;

			// The timer loop may already be waiting for longer
			// timeout value. If new event with lesser timeout
			// gets added to eventMap_ its necessary to notify
			// the conditional variable and fall through
			if (nextRun < prevMinTimeout) {
				fallThrough_ = true;
			}
		}

		// Notify timer loop thread to process event
		emptyQCond_.notify_one();
		return id;
	}
}


int TimerQueue::timerLoop() {
	while (true) {
		// Block till queue is empty
		{
			std::unique_lock<std::mutex> lock(emptyQMutex_);
			emptyQCond_.wait(lock, [this]{ return this->stopThread_ ||
							!this->eventMap_.empty(); });
		}

		if (stopThread_) {
			std::cout << "Timer loop has been stopped\n";
			return 0;
		}

		if (!eventMap_.empty()) {
			// Block till least timeout value in eventMap_ is expired or
			// new event gets added to eventMap_ with lesser value
			std::unique_lock<std::mutex> lock(eventQMutex_);

			// XXX Ocasionally below wait doesn't fall through
			// immediately even though timer loop thread has
			// already been started. Looks like notify doesn't
			// reach the thread. Is this bug in STL ?
			auto diff = std::chrono::duration_cast<duration_t>(
								sclock_t::now() - startTime_).count();

			// Find extra time which has to be spent sleeping
			waitTime_ = currMin_ - diff;
#ifndef	WITH_MILLISEC
			if (waitTime_ >= jitter_) waitTime_ -= jitter_;
#endif
			if (waitTime_ > 0)
			eventQCond_.wait_until(lock, sclock_t::now()+duration_t(waitTime_),
							[this] { return this->stopThread_ ||
									this->fallThrough_; });

			// Enough time may not have been spent in case of
			// fall through. So calculate diff and wait for
			// some more time
			if (fallThrough_) {
				// Find time spent
				auto diff = std::chrono::duration_cast<duration_t>(
									sclock_t::now() - startTime_).count();

				// Find extra time which has to be spent sleeping
				waitTime_ = currMin_ - diff;
				// Reset fallThrough_
				fallThrough_ = false;

				if (waitTime_ > 0) {
					// XXX will block thread, multiple fallThrough_
#ifdef	WITH_MILLISEC
					if (waitTime_ >= 10) continue;
					std::this_thread::sleep_for(duration_t(waitTime_));
#else
					if (waitTime_ > jitter_) continue;
					++nSpinWait_;
					ts3::usleep(waitTime_);
#endif
				}
			}

			auto elapsedTime = std::chrono::duration_cast<duration_t>(
										sclock_t::now() - startTime_).count();

#ifndef	WITH_MILLISEC
			waitTime_ = currMin_ - elapsedTime;
			if (waitTime_ > jitter_) continue;
#ifdef	ommit
			ts3::usleep(waitTime_);
#else
			if (waitTime_ > 0) {
				++nSpinWait_;
				do {
					std::this_thread::yield();
					auto elapsedTime = std::chrono::duration_cast<duration_t>(
										sclock_t::now() - startTime_).count();
					waitTime_ = currMin_ - elapsedTime;
				} while (waitTime_ > 0);
			}
#endif
#endif
			// Trigger all events in event chain
			std::map<timerKey_t, std::shared_ptr<Event> > evMap;
			for (auto it = eventMap_.begin() ;it != eventMap_.end();) {
				auto ev = it->second;
				if (ev->nextRun_ > elapsedTime) break;
				if (ev->repeat_) {
					if (ev->running_) {
						++(ev->overrun_);
					} else {
						ev->running_ = true;
						std::thread	tRun([ev]{
							ev->eventHandler_();
							ev->running_ = false;
						});
						tRun.detach();
					}
				} else ev->eventHandler_();
				++nExpires_;

				// If event has to be run once then delete
				// from vector else increment nextRun_ which
				// will be compared below to get next event
				// to be run
#ifdef	_NTEST
				auto id = ev->id_;
				auto timeout = ev->timeout_;
				auto nextRun = ev->nextRun_;
				std::cerr << fmt::format(
						"{} expire timer {} timeout: {} , nextRun: {}\n",
						elapsedTime, id, timeout, nextRun);
#endif
				if (ev->repeat_) {
					auto nextRun = elapsedTime + ev->timeout_;
					timerKey_t	key(ev->id_, nextRun);
					ev->nextRun_ = nextRun;
					evMap.insert(std::make_pair(key, ev));
				}
				it = eventMap_.erase(it);
			}

			if (!evMap.empty()) {
				eventMap_.insert(evMap.begin(), evMap.end());
			}
			// If map is empty continue and block at start of loop
			if (eventMap_.empty()) {
				continue;
			}

			// Get next event chain to be run
			auto elem = eventMap_.begin();
			waitTime_ = (*elem).second->nextRun_ - elapsedTime;
			if (waitTime_ < 0) {
				std::cout <<
#ifdef	WITH_MILLISEC
					fmt::format("Event delayed : {} (ms)\n",waitTime_);
#else
					fmt::format("Event delayed : {} (us)\n",waitTime_);
#endif
				waitTime_ = 0;
			}
			// Next event chain to be run
			currMin_ = (*elem).second->nextRun_;
#ifndef	WITH_MILLISEC
			if (waitTime_ > jitter_) waitTime_ -= jitter_;
#endif
		}
	}
}

inline int TimerQueue::cancel(int id) {
	std::unique_lock<std::mutex> lock(eventQMutex_);

	for (auto & eventPair : eventMap_) {
		if (eventPair.second->id_ != id) continue;
		std::cout << fmt::format("Event with id {} deleted succesfully\n", id);
		if (eventPair.second->running_ || eventPair.second->overrun_ > 0) {
			std::cout <<
				fmt::format("WARNING!!! running: {} overruns: {}\n",
						eventPair.second->running_, eventPair.second->overrun_);
		}
		eventMap_.erase(eventPair.first);
		return 0;
	}
	return -1;
}

}
#endif	//	__ACCURATETIMER_HPP__

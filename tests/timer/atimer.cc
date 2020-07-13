#include <unistd.h>
#include <numeric>
#include <algorithm>
#include <vector>
#include <signal.h>
#include <sched.h>
#include "ts3/timestamp.hpp"
#include "atimer.hpp"

#define CLOCKID CLOCK_REALTIME
//#define CLOCKID CLOCK_MONOTONIC
auto start = std::chrono::system_clock::now();
std::vector<double>	samples;
volatile bool	running;
using namespace ts3;
using namespace ts3::Timer;

void sig_term(int sig) {
	running = false;
	std::cerr << "Got signal term\n";
}

static inline void printTime(struct timespec *tpp = nullptr) {
	struct timespec tp;
	if (tpp == nullptr) {
		clock_gettime(CLOCKID, &tp);
		tpp = &tp;
	}
	struct tm *tmp=klocaltime(tpp->tv_sec);
	std::cout << fmt::format("{:02}:{:02}:{:02}.{:03}\n", tmp->tm_hour,
			tmp->tm_min, tmp->tm_sec, (int)(tpp->tv_nsec/1000000));
}

int handler1() {
	printTime();
	std::cout << "handler1 ";
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>( \
			std::chrono::system_clock::now() - start).count() << std::endl;
	return 0;
}

int handler2() {
	printTime();
	std::cout << "handler2 ";
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>( \
			std::chrono::system_clock::now() - start).count() << std::endl;
	std::this_thread::sleep_for(std::chrono::seconds(4));
	return 0;
}

int handler2b() {
	printTime();
	std::cout << "handler2b ";
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>( \
			std::chrono::system_clock::now() - start).count() << std::endl;
	return 0;
}

int handler4() {
	struct timespec tp;
	clock_gettime(CLOCKID, &tp);
	printTime(&tp);
	std::cout << "handler4 diff ";
#ifdef	WITH_MILLISEC
	std::cout << (tp.tv_nsec / 1000000) << "ms sleep(ms) ";
#else
	std::cout << (tp.tv_nsec / 1000) << "us sleep(us) ";
#endif
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>( \
			std::chrono::system_clock::now() - start).count() << std::endl;
	return 0;
}

class foo {
public:
	void handler3() {
		printTime();
		std::cout << "handler3 ";
		std::cout << std::chrono::duration_cast<std::chrono::milliseconds>( \
				std::chrono::system_clock::now() - start).count() << std::endl;
	}
};

static void dump_stats() noexcept {
	if (samples.size() == 0) return;
	double tsum = std::accumulate(samples.begin(), samples.end(), (double)0);
	std::sort(samples.begin(), samples.end());
	auto mean = tsum / samples.size();
	auto max = samples.back();
	auto min = samples.front();
	auto p99 = samples[0.99 * samples.size()];
	std::cout << "Mean:   " << mean << " us" << std::endl;
	std::cout << "99%:    " << p99 << " us" << std::endl;
	std::cout << "Max:    " << max << " us" << std::endl;
	std::cout << "Min:    " << min << " us" << std::endl;
}

#define timerObj Timer::TimerQueue::Instance()

int main()
{
	// Start timer loop
	tzset();
	samples.reserve(100000);
	std::thread asyncthread(&Timer::TimerQueue::timerLoop, &timerObj);
	foo f;
	signal( SIGINT, sig_term );
	signal( SIGTERM, sig_term );
	signal( SIGHUP, sig_term );
	struct timespec tp;
	clock_gettime(CLOCKID, &tp);
	std::cout << "Start create timers ";
	printTime(&tp);
	//clock_gettime(CLOCKID, &tp);
	LocalTime	ltm(tp);
	hms_t	us(ltm.ltime()->tm_hour, ltm.ltime()->tm_min + 1);
	//int eventId4 = timerObj.expire_on(std::move(us), &handler4);
	int eventId4 = timerObj.expire_on(us, &handler4);
	//int eventId4 = timerObj.expire_on(hms_t(ltm.ltime()->tm_hour, ltm.ltime()->tm_min + 1), &handler4);
#ifdef	WITH_MILLISEC
	int eventId1 = timerObj.create(5000, true, &handler1);
	int eventId2 = timerObj.create(3000, true, &handler2);
	int eventId3 = timerObj.create(6000, false, &foo::handler3, &f);
#else
	int eventId1 = timerObj.create(5000000, true, &handler1);
	int eventId2 = timerObj.create(3000000, true, &handler2);
	int eventId3 = timerObj.create(6000000, false, &foo::handler3, &f);
#endif

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
#ifdef	WITH_MILLISEC
	int eventId2b = timerObj.create(3000, true, &handler2b);
#else
	int eventId2b = timerObj.create(3000000, true, &handler2b);
#endif
	std::cout << "End create timers ";
	printTime();

	running = true;
	time_t	endT = tp.tv_sec + 60;
	while (running) {
	    std::this_thread::sleep_for(std::chrono::seconds(5));
		clock_gettime(CLOCKID, &tp);
		if (tp.tv_sec > endT) break;
	}

	if (timerObj.cancel(eventId1) == -1) {
		std::cout << "Failed to cancel id" << std::endl;
	}

#ifdef	WITH_MILLISEC
	if (timerObj.cancel(eventId2) == -1) {
		std::cout << "Failed to cancel id" << std::endl;
	}
	if (timerObj.cancel(eventId2b) == -1) {
		std::cout << "Failed to cancel id" << std::endl;
	}
#else
	if (timerObj.cancel(eventId2) == -1) {
		std::cout << "Failed to cancel id" << std::endl;
	}
	if (timerObj.cancel(eventId2b) == -1) {
		std::cout << "Failed to cancel id" << std::endl;
	}
#endif

	if (timerObj.cancel(eventId3) == -1) {
		std::cout << "Failed to cancel id" << std::endl;
	}
	if (timerObj.cancel(eventId4) == -1) {
		std::cout << "Failed to cancel id" << std::endl;
	}

	std::cout << fmt::format("Spinwait: {} timeouts: {}\n",
			timerObj.SpinWaits(), timerObj.Timeouts());
	timerObj.shutdown();

	asyncthread.join();

	ts3::tsc_clock&   tscc(ts3::tsc_clock::Instance());
	std::cout << fmt::format("rdtscp overhead: {} ticks: {} per ms\n",
				tscc.overhead(), (int64_t)(1000 / tscc.us_pertick()) );
	std::cout << "Start create timers ";
	clock_gettime(CLOCKID, &tp);
	samples.clear();
	for(int i=0; i < 10000; ++i) {
		auto tk1 = ts3::rdtscp();
		std::this_thread::sleep_for(std::chrono::microseconds(100));
		auto tk2 = ts3::rdtscp();
		double v = tscc.elapse(tk1, tk2);
		samples.push_back(v);
	}
	std::cout << fmt::format("Samples {} for sleep_for 100us\n",
					samples.size());
	dump_stats();

	samples.clear();
	for(int i=0; i < 100000; ++i) {
		auto tk1 = ts3::rdtscp();
		std::this_thread::yield();
		auto tk2 = ts3::rdtscp();
		double v = tscc.elapse(tk1, tk2);
		samples.push_back(v);
	}
	std::cout << fmt::format("Samples {} for std::this_thread::yield\n",
					samples.size());
	dump_stats();

	samples.clear();
	for(int i=0; i < 100000; ++i) {
		auto tk1 = ts3::rdtscp();
		sched_yield();
		auto tk2 = ts3::rdtscp();
		double v = tscc.elapse(tk1, tk2);
		samples.push_back(v);
	}
	std::cout << fmt::format("Samples {} for sched_yield\n",
					samples.size());
	dump_stats();

	return 0;
}

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>

using namespace std;
volatile bool	running;
std::vector<int64_t>	samples;
constexpr long interval = 10000000L;

#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                        } while (0)

static inline void printTime() {
	struct timespec tp;
	clock_gettime(CLOCKID, &tp);
	struct tm *tmp=localtime(&tp.tv_sec);
	char	bbuf[128];
	sprintf(bbuf, "%02d:%02d:%02d.%03d", tmp->tm_hour, tmp->tm_min,
			tmp->tm_sec, (int)(tp.tv_nsec/1000000));
	std::cout << bbuf << std::endl;
}

static void
print_siginfo(siginfo_t *si)
{
	timer_t *tidp;
	int oor;

	tidp = (timer_t *)si->si_value.sival_ptr;

	printf("    sival_ptr = %p; ", si->si_value.sival_ptr);
	printf("    *sival_ptr = 0x%lx\n", (long) *tidp);

	oor = timer_getoverrun(*tidp);
	if (oor == -1)
		errExit("timer_getoverrun");
	else
		printf("    overrun count = %d\n", oor);
}

void sig_term(int sig) {
	running = false;
	std::cerr << "Got signal term\n";
}

static void
handler(int sig, siginfo_t *si, void *uc)
{
    /* Note: calling printf() from a signal handler is not safe
       (and should not be done in production programs), since
       printf() is not async-signal-safe; see signal-safety(7).
       Nevertheless, we use printf() here as a simple way of
       showing that the handler was called. */
	if (!running) {
		printf("Caught signal %d\n", sig);
		print_siginfo(si);
		signal(sig, SIG_IGN);
		return;
	}
	struct timespec	tp;
	clock_gettime(CLOCKID, &tp);
	int64_t	v = tp.tv_nsec % interval;
	samples.push_back(v);
}

int
main(int argc, char *argv[])
{
	timer_t timerid;
	struct sigevent sev;
	struct itimerspec its;
	sigset_t mask;
	struct sigaction sa;

	signal( SIGINT, sig_term );
	signal( SIGTERM, sig_term );
	signal( SIGHUP, sig_term );
	samples.reserve(10000);

	/* Establish handler for timer signal */
	printf("Establishing handler for signal %d\n", SIG);
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = handler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIG, &sa, NULL) == -1)
		errExit("sigaction");

	/* Block timer signal temporarily */
	printf("Blocking signal %d\n", SIG);
	sigemptyset(&mask);
	sigaddset(&mask, SIG);
	if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
		errExit("sigprocmask");

	/* Create the timer */
	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIG;
	sev.sigev_value.sival_ptr = &timerid;
	if (timer_create(CLOCKID, &sev, &timerid) == -1)
		errExit("timer_create");

	printf("timer ID is 0x%lx\n", (long) timerid);

	/* Start the timer */
	struct timespec	tp;
	clock_gettime(CLOCKID, &tp);
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = interval;
	if (tp.tv_nsec > 0) {
		its.it_value.tv_nsec = 1000000000 - tp.tv_nsec;
		its.it_value.tv_sec = 0;
	} else {
		its.it_value.tv_nsec = 0;
		its.it_value.tv_sec = 1;
	}
	printTime();
	running = true;
	if (timer_settime(timerid, 0, &its, NULL) == -1)
		errExit("timer_settime");

	/* Unlock the timer signal, so that timer notification
	 * can be delivered */
	printf("Unblocking signal %d\n", SIG);
	if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
		errExit("sigprocmask");

	/* Sleep for a while; meanwhile, the timer may expire
	 * multiple times */
	time_t	oT = time(nullptr);
	while(running){
		sleep(1);
		time_t nT = time(nullptr);
		if (nT == oT) continue;
		oT = nT;
		printTime();
	}
	/* Block timer signal temporarily */
	printf("Blocking signal %d\n", SIG);
	sigemptyset(&mask);
	sigaddset(&mask, SIG);
	if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
		errExit("sigprocmask");
	if (samples.size() == 0) return 0;

	std::cout << "Samples " << samples.size() << " for itimer\n";
	int64_t tsum = std::accumulate(samples.begin(), samples.end(), (int64_t)0);
	std::sort(samples.begin(), samples.end());
	auto mean = tsum / samples.size();
	auto max = samples.back();
	auto min = samples.front();
	auto p99 = samples[0.99 * samples.size()];
	std::cout << "Mean:   " << mean << " ns" << std::endl;
	std::cout << "99%:    " << p99 << " ns" << std::endl;
	std::cout << "Max:    " << max << " ns" << std::endl;
	std::cout << "Min:    " << min << " ns" << std::endl;

	exit(EXIT_SUCCESS);
}

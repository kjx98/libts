#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <sys/time.h>
#include <signal.h>
#include <vector>
#include <algorithm>
#include <numeric>
 
using namespace std;
volatile bool	running;
std::vector<int>	samples;
constexpr	int interval = 10000;

void printTime() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	struct tm *tmp=localtime(& tv.tv_sec);
	char	bbuf[128];
	sprintf(bbuf, "%02d:%02d:%02d.%03d", tmp->tm_hour, tmp->tm_min, tmp->tm_sec, (int)tv.tv_usec/1000);
	std::cout<<bbuf<<std::endl;
}

void sig_term(int sig) {
	running = false;
	std::cerr << "Got signal term\n";
}

void my_alarm_handler(int a){
	if (!running) return;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	int	udiff = tv.tv_usec % interval;
	samples.push_back(udiff);
	//printTime();
	//std::cerr<<"test "<<std::endl;
}
 
int main(){
	struct itimerval t;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	t.it_interval.tv_usec = interval;
	t.it_interval.tv_sec = 0;
	if (tv.tv_usec > 0) {
		t.it_value.tv_usec = 1000000 - tv.tv_usec;
		t.it_value.tv_sec = 0;
	} else {
		t.it_value.tv_usec = 0;
		t.it_value.tv_sec = 1;
	}

	signal( SIGALRM, my_alarm_handler );
	signal( SIGINT, sig_term );
	signal( SIGTERM, sig_term );
	signal( SIGHUP, sig_term );
	printTime();
	samples.reserve(10000);
	running = true;
	if( setitimer( ITIMER_REAL, &t, NULL) < 0 ){
		std::cerr<<"settimer error."<<std::endl;
		return -1;
	}

	time_t	oT = time(nullptr);
	while(running){
		sleep(1);
		time_t nT = time(nullptr);
		if (nT == oT) continue;
		oT = nT;
		printTime();
	}
	signal( SIGALRM, SIG_IGN );
	if (samples.size() == 0) return 0;

	std::cout << "Samples " << samples.size() << " for itimer\n";
	int64_t tsum = std::accumulate(samples.begin(), samples.end(), (int64_t)0);
	std::sort(samples.begin(), samples.end());
	auto mean = tsum / samples.size();
	auto max = samples.back();
	auto min = samples.front();
	auto p99 = samples[0.99 * samples.size()];
	std::cout << "Mean:   " << mean << " us" << std::endl;
	std::cout << "99%:    " << p99 << " us" << std::endl;
	std::cout << "Max:    " << max << " us" << std::endl;
	std::cout << "Min:    " << min << " us" << std::endl;

	return 0;
}

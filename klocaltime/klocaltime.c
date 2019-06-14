//
// $Revision$
// $Log$

#include <stdbool.h>
#include <stdatomic.h>
#include <time.h>


static atomic_bool	inited=false;

static void initialize_functions(void)
{
	if (inited) return;
	tzset();
	inited = true;
}

struct tm *localtime(const time_t *timep)
{
	if (timep == 0) return gmtime(timep);
	if (!inited) initialize_functions();
	time_t	rtime=*timep - timezone;
	struct tm* res = gmtime(&rtime);
	if (res != 0) {
		res->tm_isdst = daylight;
	}
	return res;
}

struct tm	*localtime_r(const time_t *timep, struct tm *result)
{
	if (timep == 0) return gmtime_r(timep, result);
	if (!inited) initialize_functions();
	time_t	rtime=*timep - timezone;
	struct tm* res = gmtime_r(&rtime, result);
	if (res != 0) {
		res->tm_isdst = daylight;
	}
	return res;
}

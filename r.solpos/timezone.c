/*
 * timezone: supporting functions for time zone
 * 
 * Carlos A. Rueda
 * 2003-05-01
 *********************************************************************/

#include <time.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "timezone.h"


//////////////////////////////////////////////////////////////////////
double get_current_timezone()
{
	double tz;
	time_t t = time(NULL);
	localtime(&t);
	tz = (double) -timezone / 3600.0;
	return daylight ? tz + 1.0 : tz;
}

//////////////////////////////////////////////////////////////////////
double get_timezone(int year, int month, int day)
{
	struct tm rtm;
	struct tm *tm = &rtm;
	char out[1024];
	double time_zone = 9999.0;
	
	tm->tm_year = year - 1900;
	tm->tm_mon = month - 1;
	tm->tm_mday = day;
	tm->tm_hour = 12;
	tm->tm_min = 0;
	tm->tm_sec = 0;
	mktime(tm);
	
	strftime(out, sizeof(out) - 1, "%z", tm);
	scan_timezone(out, &time_zone);

	return time_zone;
}

  


//////////////////////////////////////////////////////////////////////
// Scans a string for a timezone.
// Returns 0 iff OK
int scan_timezone(const char* str_tz, double *time_zone)
{
	char local[64];
	char* ptr = local;
	int sgn, len;
	int ok = 1;
	

	*time_zone = 0;
	assert ( strlen(str_tz) > 0 );
	strcpy(local, str_tz);

	sgn = 1;
	if ( *ptr == '-' || *ptr == '+' )
	{
		sgn = *ptr == '-' ? -1 : 1;
		ptr++;
	}
	len = strlen(ptr);
	ok = ok && len == 4;
	if ( ok )
	{
		int tz_h, tz_m;
		ok = ok && 1 == sscanf(ptr + len - 2, "%d", &tz_m);
		*(ptr + len - 2) = 0;
		ok = ok && 1 == sscanf(ptr, "%d", &tz_h);
		if ( ok )
			*time_zone = sgn * (tz_h + tz_m / 60.0);
	}
	
	return ok ? 0 : 1;
}


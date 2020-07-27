#ifndef __JTIME_H__
#define __JTIME_H__

#include <sys/time.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

typedef struct timeval jtime_t;

//typedef struct jtime_s jtime_t;
//struct jtime_s{
//	time_t tv_sec;
//	int tv_usec;
//};

void jtime_set_time( jtime_t *tval, time_t sec, int usec);
void jtime_set_msec( jtime_t *tval, uint64_t msec);
int jtime_get_current( jtime_t *tval);
int jtime_set_current( jtime_t *tval);
int64_t jtime_diff( jtime_t *tval1, jtime_t *tval2);
int jtime_msec_sleep( uint64_t msec);

#endif


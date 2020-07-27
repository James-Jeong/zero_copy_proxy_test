#include "jtime.h"

void jtime_set_time( jtime_t *tval, time_t sec, int usec){
	tval->tv_sec = sec;
	tval->tv_usec = usec;
}

void jtime_set_msec( jtime_t *tval, uint64_t msec){
	tval->tv_sec = msec / 1000;
	tval->tv_usec = msec - ( ( ( uint64_t)( tval->tv_sec) * 1000)) * 1000;
}

int jtime_get_current( jtime_t *tval){
	return gettimeofday(tval, NULL);
}

int jtime_set_current( jtime_t *tval){
	return settimeofday(tval, NULL);
}

int64_t jtime_diff( jtime_t *tval1, jtime_t *tval2){
	return ( ( int64_t)( tval2->tv_sec - tval1->tv_sec)) * 1000000 + ( tval2->tv_usec - tval1->tv_usec);
}

/**
 * @fn int jtime_msec_sleep( uint64_t msec)
 * @brief 전달된 msec 동안 기다리는 함수이다.
 * @param msec 기다릴 시간 (단위:msec)
 * @return 성공여부, 0이면 성공 아니면 실패
 */
int jtime_msec_sleep( uint64_t msec){
	jtime_t tval;
	jtime_set_msec( &tval, msec);
	select(0, NULL, NULL, NULL, &tval);
	return 0;
}


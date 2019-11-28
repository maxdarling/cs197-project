/* 
 * File:   Timer.hpp
 * Author: stefan
 *
 * Created on 6. Februar 2014, 11:26
 */

#ifndef TIMER_HPP
#define	TIMER_HPP
#include <sys/time.h>  // gettime

static double getTimeSec(void) {
    struct timeval now_tv;
    gettimeofday(&now_tv, NULL);
    return ((double) now_tv.tv_sec) + ((double) now_tv.tv_usec) / 1000000.0;
}

#endif	/* TIMER_HPP */


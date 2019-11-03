//
//  Adapter.hpp
//  IndexStructure
//
//  Created by Stefan Richter on 11.02.14.
//  Copyright (c) 2014 Stefan Richter. All rights reserved.
//
#ifndef TYPES_HPP
#define    TYPES_HPP

#include <sys/time.h>  // gettime

#if __x86_64__
typedef uintptr_t MWord;
#else
typedef uintptr_t MWord;
#endif

#endif	/* TIMER_HPP */

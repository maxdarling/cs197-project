#ifndef AVALANCHE_HPP
#define AVALANCHE_HPP

/* avalanche.c
 *
 * Measures the avalanche property of a hash function
 *
 * 2 types of iterations: sequential, and Pollard-rho
 *
 * Danko Ilik, 2003
 *
 */

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "hashFunctions.hpp"
#include "HashTestCommons.hpp"


/* - take an original and its hash value
 * - for each bit change in the original calculate the
 *   hamming distance of the original's hash and the
 *   one-bit-changed-original's hash
 * - (calculate an average, and variance of the sequence of
 *   hamming distances)
 * - go take another original, in one of the following 2 ways,
 *   depending on the mode of operation of the avalanche tool:
 *   1. sequentially
 *   2. Pollard-rho
 */

u16 bytes_input;
u16 bytes_output;
byte *original_data, *original_hash;
byte *modified_data, *modified_hash;

void iter_begin() {
    char *hashName = "HASHNAME";
    int inBitLen = 64;
    int outBitLen = 64;
    int i;

    bytes_input = inBitLen / 8;
    bytes_output = outBitLen / 8;

    original_data = (byte *) malloc(bytes_input);
    original_hash = (byte *) malloc(bytes_output);
    modified_data = (byte *) malloc(bytes_input);
    modified_hash = (byte *) malloc(bytes_output);
    if (original_data == NULL || original_hash == NULL
            || modified_data == NULL || modified_hash == NULL) {
        printf("Can not allocate memory for essential buffers.\n");
        exit(EXIT_FAILURE);
    }

    /* This is the only needed zero-zation of the input buffers.
     * Any further eventual copying from hashesh to data will
     * leave the trailing zeros alone. May not seem important
     * to an observer who does not see the point of doing this. :)
     */
    for (i = 0; i < bytes_input; i++) original_data[i] = modified_data[i] = 0;
    for (i = 0; i < bytes_output; i++) original_hash[i] = modified_hash[i] = 0;

    printf("%s\n", hashName);
    printf("identifier = %s\ninput bits = %i\noutput bits = %i\n\n",
            hashName, inBitLen, outBitLen);
    printf("0%%  10%%  20%%  30%%  40%%  50%%  60%%  70%%  80%%  90%%  100%%\n");

}

void iter_end() {
    //free(original_data);
    //free(original_hash);
    //free(modified_data);
    //free(modified_hash);

}

void iter_original_with_rho() {
    memcpy(original_data, original_hash, bytes_output);
    hash_transform(original_data, original_hash);
}

void iter_original_sequentially() {
    increment_bitstring(original_data, bytes_input);
    hash_transform(original_data, original_hash);
}

int avalancheMain(int argc, char *argv[]) {
    i64 iterations;
    i64 i;
    double onetick;
    i64 lastick;
    i64 x, sum_x, sum_x_square; /* to calculate mean and variance in the end */
    unsigned min_x, max_x; /* min. and max. Hamming distance encountered */
    double mean, variance;
    int j;
    byte iteration_type;

    if (argc != 4) {
        printf("\n%s [seq|rho] iteracii hash.dll\n\n", argv[0]);
        exit(EXIT_FAILURE);
    }

#define SEQUENTIAL 1
#define RHO 2

#ifdef MSVC
  if(strcmp(strupr(argv[1]),"SEQ")==0) {
#else
    if (strcasecmp(argv[1], "SEQ") == 0) {
#endif
        iteration_type = SEQUENTIAL;
    } else {
        iteration_type = RHO;
    }

#ifdef MSVC
  iterations = (i64) _atoi64(argv[2]);
#else
    iterations = (i64) atoll(argv[2]);
#endif

    if (iterations <= 0) {
        /* We should lower the upper limit because of the sum_x etc. */
#ifdef MSVC
    printf("The number of iterations should be between %I64d and %I64d.\n",
	   (i64)100, (i64)_I64_MAX);
#else
        printf("The number of iterations should be between %lld and %lld.\n",
                (i64) 100, (i64) _I64_MAX);
#endif
        exit(EXIT_FAILURE);
    }

    /* At least 200 iterations should be used. (otherwise there would
     * be a problem displaying the progress-bar :)) */
    if (iterations < 200) iterations = 200;

    onetick = (double) iterations / (double) 53;

#ifdef MSVC
  printf("\n%I64u-iterations Avalanche Property Test on:\n\n", iterations);
#else
    printf("\n%llu-iterations Avalanche Property Test on:\n\n", iterations);
#endif
    iter_begin();

    max_x = 0;
    min_x = UINT_MAX;
    sum_x = sum_x_square = 0;
    lastick = 1;
    for (i = 0; i < iterations; i++) {
        if (lastick != (i64) ((double) i / onetick)) {
            printf("."); /* output progress */
            fflush(stdout);
            lastick = (i64) ((double) i / onetick);
        }

        /* generate the next iteration, either sequentially or with "rho" */
        if (iteration_type == RHO) {
            iter_original_with_rho();
        } else {
            iter_original_sequentially();
        }

        for (j = 0; j < bytes_input * 8; j++) {
            memcpy(modified_data, original_data, bytes_input);
            //print_bits(modified_data, bytes_input);
            modify_bit(modified_data, j);
            //print_bits(modified_data, bytes_input);
            hash_transform(modified_data, modified_hash);
            x = hamming_distance(original_hash, modified_hash, bytes_output);
            if (min_x > x) min_x = x; else if (max_x < x) max_x = x;
            sum_x += x;
            sum_x_square += (x * x);
        }

    }

    /* avalanche.c(156) : error C2520: conversion from unsigned __int64 to
     * double not implemented, use signed __int64 */
    /* i posle microsoft gi bivalo za neshto...! */
    mean = (double) sum_x / (double) (iterations * bytes_input * 8);
    variance = ((double) sum_x_square - mean) /
            (double) (iterations * bytes_input * 8);

    printf("\n\nOne bit change of the source resulted in\n");
    printf("mean=%lf and variance=%lf\nbit changes of the hash value.\n\n", mean, variance);
    printf("The minimum number of bit changes was %u,\nand the maximum was %u, out of %u total.\n\n", min_x, max_x, 64);

    iter_end();
    exit(EXIT_SUCCESS);
}


#endif	/* AVALANCHE_HPP */
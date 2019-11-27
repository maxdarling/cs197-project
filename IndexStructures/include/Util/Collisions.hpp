#ifndef COLLISIONS_HPP
#define COLLISIONS_HPP

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

unsigned bytes_input, bytes_output, bits_input, bits_output;
byte *yorig, *yhash, *zorig, *zhash;

void rng_begin(char *dllhashname) {
    int i;

    bits_input=64;
    bits_output=64;
    bytes_input=bits_input/8;
    bytes_output=bits_output/8;

    yorig = (byte*)malloc(bytes_input);
    yhash = (byte*)malloc(bytes_output);
    zorig = (byte*)malloc(bytes_input);
    zhash = (byte*)malloc(bytes_output);
    /* This is the only needed zero-zation of the input buffers.
     * Any further eventual copying from hashesh to data will
     * leave the trailing zeros alone. May not seem important
     * to an observer who does not see the point of doing this. :)
     */
    for(i=0; i<bytes_input; i++) yorig[i]=zorig[i]=0;
    for(i=0; i<bytes_output; i++) yhash[i]=zhash[i]=0;

}

void rng_end() {
    //free(yorig);
    //free(yhash);
    //free(zorig);
    //free(zhash);

}

int collisionsMain(int argc, char *argv[]) {
    u64 i;
    unsigned testbits;
    unsigned maxcoll;
    unsigned hd;

    if(argc!=3) {
        printf("\n%s [full|<bits>] hash.dll\n\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    testbits = (unsigned)atoi(argv[1]);

    rng_begin(argv[2]);

    if(testbits==0) testbits=bits_output;

    printf("\n");
    if(testbits==0) printf("FULL(%u bit) ", testbits);
    else printf("%u-BIT ", testbits);
    printf("Collision Search for:\n\n");
    printf("%s\n", "HASHNAME");
    printf("identifier = %s\ninput bits = %i\noutput bits = %i\n\n",
            "HASHNAME", 64, 64);
    printf("Search starting... (press CTRL+C to abort)...\n");

    i=0;
    maxcoll=0;
    do {
        /* do a single y iteration */
        memcpy(yorig, yhash, bytes_output);
        hash_transform(yorig, yhash);

        /* do a double z iteration */
        memcpy(zorig, zhash, bytes_output);
        hash_transform(zorig, zhash);
        memcpy(zorig, zhash, bytes_output);
        hash_transform(zorig, zhash);

        /* update iteration counter */
        ++i;

        hd = hamming_distance(yhash, zhash, bytes_output);
        if(hd>maxcoll) {
            maxcoll=hd;
#ifdef MSVC
		printf("%u (%I64u iter)  ", hd, i);
#else
            printf("%u (%llu iter)  ", hd, i);
#endif
            fflush(stdout);
        }
    } while(hd<testbits);

    //printf("\nFound a %u bit collision after %I64u iterations!\n\n",
    // testbits, i);
    printf("\nFound a %u bit collision after %I64u iterations!\n\n", hd, i);

    printf("  x1 :");print_hex(yorig, bytes_input);
    printf("h(x1):");print_hex(yhash, bytes_output);
    printf("\n");
    printf("  x2 :");print_hex(zorig, bytes_input);
    printf("h(x2):");print_hex(zhash, bytes_output);
    printf("\n");

    //printf("Have a nice day! ;)\n");

    rng_end();
    exit(EXIT_SUCCESS);
}

#endif	/* COLLISIONS_HPP */
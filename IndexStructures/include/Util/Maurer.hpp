#ifndef MAURER_HPP
#define MAURER_HPP

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

/* maurer.c
 *
 * Maurer's universal statistical test
 *
 * according to instructions from "The Handbook of Applied
 * Cryptography", page 184; and "A Universal Statistical Test for
 * Random Bit Generators", Ueli M. Maurer, "Journal of Cryptology",
 * vol. 5, no. 2, 1992, pp. 89-105
 *
 * Danko Ilik, 2003
 *
 */


/* Ako X e sluchajna promenliva so N(0,1) raspredelba, togash
 * P(X>x)=alpha
 *
 * tabelata e prevzemena od "Handbook of Applied Cryptography",
 * strana 177
 */
double normald[8][2] = {
        {0.1000, 1.2816},
        {0.0500, 1.6449},
        {0.0250, 1.9600},
        {0.0100, 2.3263},
        {0.0050, 2.5758},
        {0.0025, 2.8070},
        {0.0010, 3.0902},
        {0.0005, 3.2905}
};

u16 bytes_input;
u16 bytes_output;
const char *testype;
u16 *returns;
byte *hashinput;

void rng_begin(char *dllhashname) {
    int i;
    bytes_input=64/8;
    bytes_output=64/8;
    returns=(u16*)malloc(bytes_output/2);
    hashinput=(byte*)malloc(bytes_input);
    /* This is the only needed zero-zation of the input buffers.
     * Any further eventual copying from hashesh to data will
     * leave the trailing zeros alone. May not seem important
     * to an observer who does not see the point of doing this. :)
     */
    for(i=0; i<bytes_input; i++) hashinput[i]=0;
    for(i=0; i<bytes_output; i++) returns[i]=0;

    printf("%s\n", "HASH_DESC");
    printf("identifier = %s\ninput bits = %i\noutput bits = %i\n\n",
            "HASH_NAME", 64, 64);
    printf("0%%  10%%  20%%  30%%  40%%  50%%  60%%  70%%  80%%  90%%  100%%\n");

}

void rng_end() {
    // Zaradi neshto nepoznato, ova free() ja pagja programata...
    //free(returns);
    // Znachi kje mu ostavime na operativniot sistem da si gi
    // garbage collect tie nekolku bajti :)

    //free(hashinput);

}

u16 counter = 0;
u16 rng_next_u16() {
    u16 result;
    /* With each hash_transform() call, we get bytes_output random bytes.
     * As our unit of measure is u16, and that is 2 bytes, we have return-
     * data for bytes_output/2 before we issue the next hash_transform()
     */

    if(counter==0) {
        memcpy(hashinput, returns, bytes_output);
        hash_transform(hashinput, (byte*)returns);
    }
    result = returns[counter];
    counter++;
    counter%=(bytes_output/2);

    return result;
}

int maurerMain(int argc, char *argv[]) {
    //  u32 L = 16;
    u32 Q = 10*65536;
    u32 K = 1000*65536;
    u32 QplusK;
    u32 onetick;
    u32 T[65536];
    u16 bi;
    u32 i, j;
    double sum;

    if(argc!=3) {
        printf("\n%s [short|normal|long] hash.dll\n\n", argv[0]);
        exit(EXIT_FAILURE);
    }

#ifdef MSVC
  if (strcmp("SHORT", strupr(argv[1]))==0) {testype="SHORT"; K=100*65536;}
  else if (strcmp("LONG", strupr(argv[1]))==0) {testype="LONG"; K=10000*65536;}
  else {testype="NORMAL"; K=1000*65536;}
#else
    if (strcasecmp("SHORT", argv[1])==0) {testype="SHORT"; K=100*65536;}
    else if (strcasecmp("LONG", argv[1])==0) {testype="LONG"; K=10000*65536;}
    else {testype="NORMAL"; K=1000*65536;}
#endif

    QplusK = Q+K;
    onetick = K/53;

    printf("\n%s (%lu iterations) Maurer Universal Statistical Test on:\n\n", testype, QplusK);
    rng_begin(argv[2]);

    /* Zero the table T */
    for(j=0; j<65536; j++) T[j]=0;

    /* Initialize the table T */
    for(i=1; i<=Q; i++) {
        bi = rng_next_u16();
        T[bi] = (u32) i;
    }

    /* Calculate the statistic X_u */
    sum = 0.0;
    for(i=Q+1; i<=QplusK; i++) {
        if (i%onetick==0) {printf(".");fflush(stdout);}
        bi = rng_next_u16();
        sum = sum + log((double)(i-T[bi]));
        T[bi] = i;
    }
    sum = sum / (double)K;
    sum = sum / log(2.0);

    printf("\n\nX_u statistics = %lf (%lf = theoretical math. expect. for uniform)\n\n", sum, 15.167379);

    /* We normalize the statistic ( N(m,s^2) -> N(0,1) ) */
    sum = (sum-15.167379)/sqrt(3.421);
    /* printf("%lf (%lf)\n", sum, 0.0); */

    for(i=0; i<8; i++)
        if(sum < -normald[i][1] || sum > normald[i][1])
            printf("FAILS  for alpha = %lf\n", normald[i][0]);
        else
            printf("PASSES for alpha = %lf\n", normald[i][0]);

    rng_end();
    exit(EXIT_SUCCESS);
}

#endif	/* MAURER_HPP */
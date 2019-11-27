#ifndef HASH_TEST_COMMONS_HPP
#define HASH_TEST_COMMONS_HPP

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

/* one should always check these, if one tries to port
 * the package to architectures other than IA32 with
 * GCC 3.2 or MSVC 6.0
 */
typedef unsigned char byte;
/* sizeof(byte)==1 */
typedef unsigned short int u16;
/* sizeof(u16)==2 */
typedef unsigned int u32;       /* sizeof(u32)==4 */

#ifdef MSVC
typedef unsigned __int64 u64;   /* sizeof(u64)==8 */
typedef __int64 i64;   /* sizeof(i64)==8 */
#else
typedef uint64_t u64;
/* sizeof(u64)==8 */
typedef int64_t i64;   /* sizeof(i64)==8 */
#endif


#ifndef _I64_MAX
#  ifdef MSVC
#    define _I64_MAX LONG_LONG_MAX
#  else
/* zemeno od limits.h; od nekoja prichina ne beshe dovolno samo include */
#    define _I64_MAX 9223372036854775807LL
#  endif
#endif

int hash_transform(byte *in, byte *out) {

    static TabulationHash hash;
    u64 *in64 = (u64 *) in;
    u64 *out64 = (u64 *) out;
    *out64 = hash(*in64);
    return 1;
}

void print_bits(byte *a, unsigned alen) {
    unsigned i;
    byte mask;

    for (i = 0; i < alen; i++) {
        for (mask = 0x80; mask; mask >>= 1) /* 0x80 == 10000000 binary*/
            printf("%d", (a[i] & mask ? 1 : 0));
        printf(" ");
    }
    printf("\n\n");
}

void print_hex(byte *a, unsigned alen) {
    unsigned i;
    for (i = 0; i < alen; i++) printf("%.2X", a[i]);
    printf("\n");
}

/* Modify the k-th bit from the left in the bitstring a */
void modify_bit(byte *a, unsigned k) {
    byte mask;

    mask = 0x80; /* set the mask to binary 10000000 */
    mask >>= (k % 8); /* move the bit-1 a number of positions to the right */
    a[k / 8] ^= mask; /* XOR the appropriate byte in the bitstring with the mask */
}

/* Increment a bitstring by one */
void increment_bitstring(byte *a, unsigned alen) {
    int i;

    i = alen - 1;
    do {
        ++a[i];
        --i;
    } while (i >= 0 && a[i + 1] == 0);

}

unsigned hamming_distance(byte *a, byte *b, unsigned alen) {
    unsigned i;
    byte xored;
    unsigned distance;

    distance = 0;
    for (i = 0; i < alen; i++) {
        xored = (a[i] ^ b[i]); /* XOR the next byte from the sequence */
        while (xored) { /* if it has 1s inside, count how many */
            distance += (xored & 1);
            /* it is more efficient to add zeroes than branch
             * on out-of-order processors */
            xored >>= 1; /* shift-right 1 position */
        }
    }

    return distance;
}

/* Return 1 if bitstrings a and b are equal to the testbits-bit
 * from the left, or return 0 otherwise
 */
int equal_bitstrings(byte *a, byte *b, unsigned testbits) {
    unsigned len = testbits / 8;
    byte bitmask;
    unsigned i;

    /* first check the whole bytes */
    for (i = 0; i < len; i++) if (a[i] != b[i]) return 0;

    testbits %= 8;
    if (testbits == 0) return 1;

    bitmask = 0xFF; /* binary 11111111 */
    bitmask <<= (8 - testbits); /* shift left appropriately */
    if ((a[len] & bitmask) == (b[len] & bitmask)) return 1; else return 0;

}

/* sets all bits to zero */
void zeroize(byte *a, unsigned alen) {

    unsigned i;

    for (i = 0; i < alen; i++)

        a[i] = 0;
    //while((--alen)>=0) a[alen]=0;
}

/* xor to bitstrings into the first one */
void memxor(byte *a, byte *b, unsigned alen) {
    unsigned i;

    for (i = 0; i < alen; i++)

        a[i] ^= b[i];

    //while((--alen)>=0) a[alen] = a[alen]^b[alen];
}

/* Return the k-th bit from the left in the bitstring a */
byte get_bit(byte *a, unsigned k) {
    byte mask;

    mask = 0x80; /* set the mask to binary 10000000 */
    mask >>= (k % 8); /* move the bit-1 a number of positions to the right */
    return !!(a[k / 8] & mask); /* return the appropriate byte in the bitstring
			   with the mask */
}


/** for testing purposes..
main() {
byte a[3] = {0xF0, 0x0F, 0xAA};
byte b[3] = {0xF0, 0x0F, 0xAB};

print_bits(a, 3);
print_bits(b, 3);

printf("%u\n", equal_bitstrings(a,b,24));
printf("%u\n", equal_bitstrings(a,b,23));
}
*/

/** also for testing purposes..
main() {
//byte a[5] = {0xF0, 0x0F, 0xAA, 0x00, 0xFF};
byte a[5] = {0xF0, 0xFF, 0xFF, 0xFF, 0xFF};
unsigned byte_length = 5;
byte b[5];

//memcpy(b, a, byte_length);
//print_bits(a, byte_length);
//modify_bit(a, 0);
//print_bits(a, byte_length);
//modify_bit(a, 9);
//print_bits(a, byte_length);
//modify_bit(a, 13);
//print_bits(a, byte_length);
//modify_bit(a, 39);
//print_bits(a, byte_length);

increment_bitstring(a, byte_length);
print_bits(a, byte_length);
increment_bitstring(a, byte_length);
print_bits(a, byte_length);
increment_bitstring(a, byte_length);
print_bits(a, byte_length);
increment_bitstring(a, byte_length);
print_bits(a, byte_length);
increment_bitstring(a, byte_length);
print_bits(a, byte_length);

printf("h.d. = %d\n", hamming_distance(a, b, byte_length));
}
*/

#endif	/* HASH_TEST_COMMONS_HPP */
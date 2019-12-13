//
//  BenchSIMDSearch.cpp
//  IndexStructure
//
//  Created by Stefan Richter on 18.02.14.
//  Copyright (c) 2014 Stefan Richter. All rights reserved.
//
#include <iostream>
#include <emmintrin.h> // x86 SSE intrinsics
#include "Timer.hpp"

//// perf-critical: ensure this is 64-byte aligned. (a full cache line)
//union bnode
//{
//    int32_t i32[16];
//    __m256i m256[2];
//};
//
//// returns from 0 (if value < i32[0]) to 16 (if value >= i32[15])
//unsigned bsearch_avx2(bnode const* const node, __m256i const value)
//{
//    __m256i const perm_mask = _mm256_set_epi32(7, 6, 3, 2, 5, 4, 1, 0);
//
//    // compare the two halves of the cache line.
//
//    __m256i cmp1 = _mm256_load_si256(&node->m256[0]);
//    __m256i cmp2 = _mm256_load_si256(&node->m256[1]);
//
//    cmp1 = _mm256_cmpgt_epi32(cmp1, value); // PCMPGTD
//    cmp2 = _mm256_cmpgt_epi32(cmp2, value); // PCMPGTD
//
//    // merge the comparisons back together.
//    //
//    // a permute is required to get the pack results back into order
//    // because AVX-256 introduced that unfortunate two-lane interleave.
//    //
//    // alternately, you could pre-process your data to remove the need
//    // for the permute.
//
//    __m256i cmp = _mm256_packs_epi32(cmp1, cmp2); // PACKSSDW
//    cmp = _mm256_permutevar8x32_epi32(cmp, perm_mask); // PERMD
//
//    // finally create a move mask and count trailing
//    // zeroes to get an index to the next node.
//
//    unsigned mask = _mm256_movemask_epi8(cmp); // PMOVMSKB
//    return _tzcnt_u32(mask) / 2; // TZCNT
//}

const uint32_t FAN_OUT = 256;

struct Node {
    Node *next;
    size_t payload[FAN_OUT];
};

inline void check(int *h, long *k) {
    *h = 5;
    *k = 6;
    if (*h == 5)
        printf("strict aliasing problem\n");
}

int main4() {
    long k[1];
    check((int*) k, k);
    return 0;
}

void main3() {
    int nodeCount = 32;
    for (int k = 0; k < 16; ++k) {
        size_t *pv = new size_t[nodeCount - 1];
        for (int i = 0; i < nodeCount - 1; ++i) {
            pv[i] = i + 1;
        }
        //std::random_shuffle(pv, pv + nodeCount - 1);
        Node *list = new Node[nodeCount];
        Node *cur = list;
        for (int i = 0; i < nodeCount - 1; ++i) {
            cur->payload[0] = cur - list;
            cur->next = &list[pv[i]];
            cur = cur->next;
        }
        cur->next = NULL;
        cur->payload[0] = cur - list;

        delete[] pv;

        uint64_t sum = 0;
        double start = getTimeSec();
        for (int i = 0; i < 100; ++i) {
            cur = list;
            while (cur != NULL) {
                //sum += cur->payload[reinterpret_cast<size_t>(cur) % 64];
                sum += 1;
                cur = cur->next;
            }
        }
        printf("size: %f, time: %f | %lu\n", sizeof(Node) * nodeCount / (float) 1024 / 1024, 100000 * (getTimeSec() - start) / 100 / nodeCount, sum);
        nodeCount *= 2;
        delete[] list;
    }
}

void main2() {
    int nodeCount = 32;
    Node *head = new Node;
    Node *cur = head;
    for (int i = 0; i < nodeCount; ++i) {
        Node *newNode = new Node;
        cur->next = newNode;
        cur = newNode;
    }
    cur->next = NULL;
    uint64_t sum = 0;
    double start = getTimeSec();
    for (int i = 0; i < 10; ++i) {
        cur = head;
        while (cur->next != NULL) {
            sum += cur->payload[0];
            cur = cur->next;
        }
    }
    printf("xxx: %f | %lu\n", (getTimeSec() - start) / 100, sum);
}

int main(int argc, const char *argv[]) {
    main4();
    return 0;
}

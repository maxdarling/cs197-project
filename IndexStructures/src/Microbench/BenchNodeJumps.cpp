//
//  main.cpp
//  MicroBench
//
//  Created by Stefan Richter on 14.02.14.
//  Copyright (c) 2014 Stefan Richter. All rights reserved.
//

#include <iostream>
#include "Timer.hpp"

const uint32_t FAN_OUT = 256;

struct Node {
    Node *next;
    char payload[FAN_OUT * sizeof(size_t)];
};

//#define NOP_LOOP 10;

uint64_t nodeSizes[] = {2064, 656, 160, 52};
uint64_t size = 512 * 1024 * 1024;
uint64_t iter = 100 * 1024 * 1024;

void main2() {
    Node *head = new Node;
    Node *cur = head;
    for (int i = 0; i < 1024 * 1024; ++i) {
        Node *newNode = new Node;
        cur->next = newNode;
        cur = newNode;
    }
    cur->next = NULL;
}

void runBig(char *data, uint64_t nodeSize) {
#ifdef NOP_LOOP
    int nopCount = 0;
    const int max = NOP_LOOP + (reinterpret_cast<uint64_t >(data) & 1);
#endif
    uint64_t jumpDistSum = 0;
    uint64_t sum = 0;
    double start = getTimeSec();
    uint64_t *pos = reinterpret_cast<uint64_t *>(data);
    for (int i = 0; i < iter; ++i) {
        uint64_t v = *pos;
        sum += *(pos + 1) + (2 + (v % (nodeSize / sizeof(uint64_t) - 2))); //enriched with additional computational crap
        //big jump
        uint64_t *oldPos = pos;
        pos = reinterpret_cast<uint64_t *>((data + (v % (size - sizeof(uint64_t)))));
        jumpDistSum += oldPos > pos ? (oldPos - pos) : (pos - oldPos);

#ifdef NOP_LOOP
        for (int i = 0; i < max; ++i) {
            nopCount += max;
        }
#endif
    }
    printf("time big (judy-style): %f | %lu\n", getTimeSec() - start, sum);
    printf("avg dist: %f\n", jumpDistSum * 8 / (double) iter);
#ifdef NOP_LOOP
    printf("NOP: %lu\n", nopCount);
#endif
}

void runBigSmall(char *data, uint64_t nodeSize) {
#ifdef NOP_LOOP
    int nopCount = 0;
    const int max = NOP_LOOP + (reinterpret_cast<uint64_t >(data) & 1);
#endif
    uint64_t jumpDistSum = 0;
    uint64_t sum = 0;
    double start = getTimeSec();
    uint64_t *pos = reinterpret_cast<uint64_t *>(data);
    for (int i = 0; i < iter; ++i) {
        uint64_t v = *pos;
        //small jump
        uint64_t *smallJump = pos + 2 + (v % ((nodeSize / sizeof(uint64_t) - 2)));
        sum += *smallJump;
        //big jump
        uint64_t *oldPos = pos;
        pos = reinterpret_cast<uint64_t *>((data + (v % (size - sizeof(uint64_t)))));
        jumpDistSum += oldPos > pos ? (oldPos - pos) : (pos - oldPos);
#ifdef NOP_LOOP
        for (int i = 0; i < max; ++i) {
            nopCount += max;
        }
#endif
    }
    printf("time small+big (art-style): %f | %lu\n", getTimeSec() - start, sum);
    printf("avg dist: %f\n", jumpDistSum * 8 / (double) iter);
#ifdef NOP_LOOP
    printf("NOP: %lu\n", nopCount);
#endif
}

int main(int argc, const char *argv[]) {

    char *data = reinterpret_cast<char *>(valloc(size));
    srand(123);
    for (int i = 0; i < size; ++i) {
        data[i] = rand();
    }
    for (int k = 0; k < sizeof(nodeSizes) / sizeof(uint64_t); ++k) {
        uint64_t nodeSize = nodeSizes[k];
        printf("---------------- simulating using node size of: %lu ----------------\n", nodeSize);
        runBig(data, nodeSize);
        //------------------------
        runBigSmall(data, nodeSize);
        //------------------------

    }
    return 0;
}



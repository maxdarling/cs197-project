//
//  test.cpp
//  
//
//  Created by Max  on 11/5/19.
//
#include <iostream>
#include <stdio.h>

#include "Types.hpp"
#include "ChainedHashMap.hpp"
#include "LinearHashTable.hpp"

#include "AdaptiveTable.hpp"

//all includes below copied from main.cpp...
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <set>
#include <random>
#include <map>

#include <unordered_set>
#include <algorithm>
#include "ArrayHashTable.hpp"

#include "hashFunctions.hpp"
#include "Timer.hpp"
#include "Adapters.hpp"
#include "MemoryMeasurement.hpp"
#include "Types.hpp"
#include "Zipf.h"
#include "Defines.hpp"
#include "LinearTombstoneHashTable.hpp"
#include "LinearTombstoneHashTableSIMD.hpp"
#include "QuadraticHashTable.hpp"
#include "CompactInlinedChainedHashMap.hpp"
#include "InlineChainedHashMap.hpp"
#include "CompactInlinedChainedHashMap.hpp"

int POW_2 = 20;

//class AdaptiveTable {
//private:
//    enum table_type {CH, LP};
//    ChainedHashMap<MultiplicativeHash>* chained_table;
//    LinearHashTable<int, int, MultiplicativeHash, 0L, false>* lp_table;
//    table_type curr_type;
//
//public:
//    AdaptiveTable(table_type t = LP) {
//        curr_type = t;
//        //std::cout<<"constructor finished"<<std::endl;
//    }
//    void printElems(){
//        //case 1: chained
//        if (curr_type == CH){
//            chained_table->printElems();
//        }
//    }
//};

//control experiment. tests only LP table, no switching. 75% unsucc ratio.
void experiment1 () {
    
    //step 1: fill the table with a fixed set of elements
    LinearHashTable<int, int, MultiplicativeHash, 0L, false> lp_table; //2^18
    for (int i = 0; i < (int)pow(2, POW_2-4); i++) {
        lp_table.put((std::rand() % RAND_MAX-1) + 1, i); // [1,RAND_MAX-1].
    }
    
    //step 2: prepare the query set
    
    //step 3: start time.
    auto start = std::chrono::high_resolution_clock::now();
    
    //step 4: perform operations (one insert, one unsuccessful lookup)
    for (int i = 0; i < (int)pow(2, POW_2); i++) {
        lp_table.put((std::rand() % RAND_MAX-1) + 1, i);
//        if (i%4 == 0) {
//            lp_table.get((std::rand() % RAND_MAX-1) + 1); // <- successful
//        } else {
//            lp_table.get(RAND_MAX); // <- unsuccessful
//        }
        lp_table.get(RAND_MAX);
    }
    
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop-start);
    
    std::cout<<" LP Table (no-switch)\nExperiment duration: "<<duration.count()<<std::endl;
}

//control experiment. tests CH table, no switching. 75% unsucc ratio.
void experiment2 () {
    ChainedHashMap<MultiplicativeHash> map (10); // todo: more rigorous about size
    for (int i = 0; i < (int)pow(2, POW_2-4); i++) {
        map.putValue((std::rand() % RAND_MAX-1) + 1, i); // [1,RAND_MAX-1].
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < (int)pow(2, POW_2); i++) {
        map.putValue((std::rand() % RAND_MAX-1) + 1, i);
//        if (i%4 == 0) {
//            map.getValue((std::rand() % RAND_MAX-1) + 1); // <- successful
//        } else {
//            map.getValue(RAND_MAX); // <- unsuccessful
//        }
        map.getValue(RAND_MAX);
    }
    
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop-start);
    
    std::cout<<" CH Table (no-switch)\nExperiment duration: "<<duration.count()<<std::endl;
}

//resizing control #1. switch from one table to the same version.
//void experiment3 () {
//    ChainedHashMap<MultiplicativeHash> map (8); // 8 matches default for LP.
//        for (int i = 0; i < (int)pow(2, POW_2-4); i++) {
//            map.putValue((std::rand() % RAND_MAX-1) + 1, i); // [1,RAND_MAX-1].
//        }
//
//        auto start = std::chrono::high_resolution_clock::now();
//
//        for (int i = 0; i < (int)pow(2, POW_2); i++) {
//            bool b = map.putValue2((std::rand() % RAND_MAX-1) + 1, i);
//            if (b) { //resize
//                ChainedHashMap<MultiplicativeHash> new_map =
//            }
//    //        if (i%4 == 0) {
//    //            map.getValue((std::rand() % RAND_MAX-1) + 1); // <- successful
//    //        } else {
//    //            map.getValue(RAND_MAX); // <- unsuccessful
//    //        }
//            map.getValue(RAND_MAX);
//        }
//
//        auto stop = std::chrono::high_resolution_clock::now();
//        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop-start);
//
//        std::cout<<" CH Table (no-switch)\nExperiment duration: "<<duration.count()<<std::endl;
//}

//playing around with stuff.
void test () {
    std::cout << "program start" << std::endl;
    ChainedHashMap<MultiplicativeHash> map (8); //size should be power of 2

    for (int i = 1; i<9; i++) {
        std::cout<<"about to insert "<<i<<std::endl;
        map.putValue(i, 1);
    }
    map.printElems();
    
    //testing for making chained hash map private Entry ** map member to public
    LinearHashTable<int, int, MultiplicativeHash, 0L, false> lp_table;
    for (size_t i = 0; i<map.getArraySize(); i++) {
        Entry* p = map.map[i];
        if (p) {
            while (p!=nullptr) {
                lp_table.put(p->getKey(), p->getValue());
                std::cout<<"added ("<<p->getKey()<<", "<<p->getValue()<<")"<<std::endl;
                p = p->next;
            }
        }
    }
    
    lp_table.printElems();
    
    std::cout << "program end" << std::endl;
}
int main() {
//    experiment1();
//    experiment2();
    
    //below: test for transferHash function.
//    ChainedHashMap<MultiplicativeHash> m1 (8);
//    for (int i = 0; i<8; i++) {
//        m1.putValue(i, 0);
//    }
//    std::cout<<"elements of ch: \n"<<std::endl;
//    m1.printElems();
//    LinearHashTable<int, int, MultiplicativeHash, 0L, false> lp_table(16);
//    m1.transferHash(lp_table);
//    std::cout<<"elements of lp: "<<std::endl;
//    lp_table.printElems();
    
    AdaptiveTable table (LP);
    table.dummy();
    std::cout<<"starting insertions"<<std::endl;
    for (int i = 1; i<9; i++) {
        table.insert(i, 1);
    }
    std::cout<<"done with insertions"<<std::endl;
    table.printElems();
    //table.lookup(10);

}

//
//  AdaptiveTable.hpp
//  
//
//  Created by Max  on 11/30/19.
//

#include <iostream>
#include <stdio.h>
#include "ChainedHashMap.hpp"
#include "Types.hpp"

#include "LinearHashTable.hpp"


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
#include "LinearHashTable.hpp"
#include "LinearTombstoneHashTable.hpp"
#include "LinearTombstoneHashTableSIMD.hpp"
#include "QuadraticHashTable.hpp"
#include "CompactInlinedChainedHashMap.hpp"
#include "InlineChainedHashMap.hpp"
#include "CompactInlinedChainedHashMap.hpp"

enum table_type {LP, CH}; //want to make this global so users can pass type into contructor(eg. AdaptiveTable(LP) table;)

class AdaptiveTable {
    //note: <int, int> is assumed, since ChainedHashMap is set as such.
private:
    LinearHashTable<int, int, MultiplicativeHash, 0L, false>* lp_table = nullptr;
    ChainedHashMap<MultiplicativeHash>* ch_table = nullptr;
    
    table_type type;
    int sizeLog2;
    size_t num_successful;
    size_t num_unsuccessful;
    
public:
    AdaptiveTable(table_type t = LP, int size = 16) {
        std::cout<<"began constructor"<<std::endl;
        type = t;
        sizeLog2 = size;
        //init table
        if (type == LP) {
            //is this the best way to init?
            lp_table = new LinearHashTable<int, int, MultiplicativeHash, 0L, false>(sizeLog2);
            std::cout<<lp_table->isFull()<<std::endl;

        }
        else if (type == CH) {
            ch_table = new ChainedHashMap<MultiplicativeHash>(pow(sizeLog2,2));
        }
        else {
            //Todo: add more tables
        }
        std::cout<<"exited constructor"<<std::endl;
    }
    
    void dummy() {
        std::cout<<"dummy() called"<<std::endl;
    }
    
    //returns optimal table type according to flowchart + heuristics
    table_type getOptimalTableType() {
        if (num_unsuccessful / (num_unsuccessful + num_successful) >= 0.75) {
            return CH;
        }
        else {
            return LP;
        }
        
        //todo: add
    }
    
    //todo: somehow make this cleaner with templates, subclass, void* curr_table, etc
    void insert(int key, int val) {
        if (type == LP) { // <- this adds unwanted code complexity!
            if (lp_table->isFull() && (getOptimalTableType() == CH)) {
                ch_table = new ChainedHashMap<MultiplicativeHash> (pow(2, lp_table->getSizeLog2() + 1));
                lp_table->transferHash(ch_table);
                delete lp_table;
                type = CH;
                ch_table->INS(key, val);
                std::cout<<"swapped from LP to CH!"<<std::endl;
            }
            else {
                //lp_table->INS(key, val);
                std::cout<<"about to insert"<<std::endl;
                lp_table->INS(key, val);
                std::cout<<"basic insertion completed"<<std::endl;
            }
        }
        else if (type == CH) {
            if (ch_table->isFull() && (getOptimalTableType() == LP)) {
                lp_table = new LinearHashTable<int, int, MultiplicativeHash, 0L, false>(ch_table->getSizeLog2() + 1);
                ch_table->transferHash(lp_table);
                delete ch_table;
                type = LP;
                lp_table->INS(key, val);
                std::cout<<"swapped from CH to LP!"<<std::endl;
            }
            else {
                ch_table->INS(key, val);
            }
        }
    }
    
    void lookup() {
        if (type == LP) {
            
        }
    }
    
    void remove() {
        
    }
    
    void printElems() {
        if (type == LP) {
            lp_table->printElems();
        }
        else if (type == CH) {
            ch_table->printElems();
        }
        else {
            
        }
    }
};

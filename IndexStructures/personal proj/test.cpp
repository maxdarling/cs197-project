//
//  test.cpp
//  
//
//  Created by Max  on 11/5/19.
//
#include <iostream>
#include <stdio.h>
#include "../include/Structures/Cuckoo/ChainedHashMap.hpp"
#include "../include/Util/Types.hpp"
#include "../include/Util/hashFunctions.hpp"
#include "../include/Structures/Cuckoo/LinearHashTable.hpp"


//all includes below copied from main.cpp...
/*#include <vector>
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
*/

class AdaptiveTable {
private:
    enum table_type {CH, LP};
    ChainedHashMap<MultiplicativeHash>* chained_table;
    LinearHashTable<int, int, MultiplicativeHash, 0L, false>* lp_table;
    table_type curr_type;
    
public:
    AdaptiveTable(table_type t = LP) {
        curr_type = t;
        //std::cout<<"constructor finished"<<std::endl;
    }
    
    void printElems(){
        //case 1: chained
        if (curr_type == CH){
            chained_table->printElems();
        }
    }
    
    
};

int main() {
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

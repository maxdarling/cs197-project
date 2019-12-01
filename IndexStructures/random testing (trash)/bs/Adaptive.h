#include "HashTable.h"


class AdaptiveHashTable {
  enum workflow {RW, WORM};
  enum distribution {DENSE, SPARSE};
  enum table_layout {SoA, AoS};
  enum hash_config {LPMult, QPMult, ChainedH4};

 private: 
  HashTable* table;
  workflow wf;
  distribution distr;
  table_layout layout;
  
  int getConfig(int* arr);
  void adaptiveResize();
  
 public:
  AdaptiveHashTable(workflow work = RW, distribution dist = DENSE, table_layout lay = AoS) {
    wf = work;
    distr = dist;
    layout = lay;
    adaptiveResize();
  }
    
  //Todo: add more combinations
  int getConfig(arr) { 
    if (arr[0] == RW && arr[1] == DENSE) return LPMult;
  }

  //decide what table to use based on member variables
  void adaptiveResize() { 
    int metrics [3] = ht.getMetadata(); //returns array of metrics tracked 
    int config = getConfig(metrics);
    HashTable* new_table = new HashTable(config); //initializes HT class with correct scheme/function and 2x capacity
    
    //copy old elements over
    for (auto it = table.begin(); it!=table.end(); it++) { 
      new_table.insert(it->first, it->second);
    }
    
    delete table; //free up old memory
    table = new_table; //reassign instance variable
  }
}

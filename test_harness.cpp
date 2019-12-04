#include "adaptive_hash.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
//#include "Timer.hpp"

using std::vector;
using std::string;

//given an empty vector (already allocated), insert appropriate keys
void generateKeys(vector<uint64_t>& keys, size_t size, string type = "DENSE") {
    std::mt19937 gen(197); //random func with seed as 197.
    
    if (type == "DENSE") {
        for (int i = 0; i<size; i++) {
            keys[i] = i+1; //b/c 0 isnt valid.
        }
        
        std::shuffle(keys.begin(), keys.end(), gen);
    }
    //todo: generateIndexData() in main.cpp ensures no dupes
    else if (type == "SPARSE") {
        uint64_t max = uint64_t(pow(2,64) - 1);
        std::uniform_int_distribution<uint64_t> dis(1, max);
        for (int i = 0; i<size; i++) {
            keys[i] = dis(gen);
        }
    }
    else {
        throw "error - invalid distribution type argument";
    }
}


/*---------------------------Experiments Below------------------------*/



//EXP 1
void exp1_helper(size_t size, size_t capacity, string type) {
    //ChainedHashMap<MultiplicativeHash> map(capacity);
    GenericHashTable* map;
    if (type == "LP") {
        map = new LinearHashTable<int, int, MultiplicativeHash, 0L, false> (std::log2(capacity)); //care to do log2 here
    }
    else if (type == "CH") {
        map = new ChainedHashMap<MultiplicativeHash>(capacity);
    }
    else { throw "invalid type arg";}
    
    vector<uint64_t> keys(size);
    generateKeys(keys, size, "DENSE");
    
    //step1: populate table
    for (int i = 0; i<size; i++) {
        map->put(keys[i], 1);
    }
    
    //step2: perform lookups
    size_t num_operations = 16*size;
    double start = getTimeSec();
    for (size_t i = 0; i<num_operations; i++){
        map->get(0); // todo: make more robust?
    }
    
    double time = getTimeSec() - start;
    std::cout<<type<<" - time elapsed: "<<time<<std::endl;
    
}

//benchmark chained and LP, 100% unsucc ratio. no switching. dense. WORM.
void experiment1() {
    size_t size = 1000000;
    size_t capacity = size_t(pow(2,21));//LF will be ~47%, just like experiment
    
    std::cout<<"start of experiment 1"<<std::endl;
    exp1_helper(size, capacity, "LP");
    exp1_helper(size, capacity, "CH");
    std::cout<<"end of experiment 1"<<std::endl;
}

int main(int argc, char*argv[]) {
    
    experiment1();
    return 0;
}



#include "adaptive_hash.hpp"
#include <iostream>
#include <vector>
#include <algorithm>

using std::vector;
using std::string;

//basic test to ensure nothing is broken
void create_simple_hash_table() { //assumes all input is valid
	AdaptiveHashTable test(20);
    for (int i = 0; i<5; i++) {
        test.insert(i,1);
    }
    
    test.printElems();
    
}

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

//test to see if generateKeys() works. It does!
void experiment1() {
    vector<uint64_t> vec (16);
    generateKeys(vec, vec.size(), "SPARSE");
    for (auto it = vec.begin(); it!=vec.end(); it++)
        std::cout<<*it<<std::endl;
}


int main(int argc, char*argv[]) {
    
    //create_simple_hash_table();
    
    experiment1();
    return 0;
}



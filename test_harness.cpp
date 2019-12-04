#include "adaptive_hash.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
//#include "Timer.hpp"

using std::vector;
using std::string;

//given an empty vector (already allocated), insert appropriate keys
void generateKeys(vector<uint64_t>& keys, size_t size, string dist = "DENSE") {
    std::mt19937 gen(198); //random func with seed as 197.
    
    if (dist == "DENSE") {
        for (int i = 0; i<size; i++) {
            keys[i] = i+1; //b/c 0 isnt valid.
        }
        
        std::shuffle(keys.begin(), keys.end(), gen);
    }
    //todo: generateIndexData() in main.cpp ensures no dupes
    else if (dist == "SPARSE") {
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

//given all params, will conduct an experiment
double lookup_helper(size_t size, size_t capacity, string type, string distr, float unsucc_ratio) {
    GenericHashTable* map;
    if (type == "LP") {
        map = new LinearHashTable<int, int, MultiplicativeHash, 0L, false> (std::log2(capacity)); //care to do log2 here
    }
    else if (type == "CH") {
        map = new ChainedHashMap<MultiplicativeHash>(capacity);
    }
    else { throw "invalid type arg";}
    
    vector<uint64_t> keys(size);
    generateKeys(keys, size, distr);
    
    //step1: populate table
    for (int i = 0; i<size; i++) {
        map->put(keys[i], 1);
    }
    
    //plant fail keys
    if (unsucc_ratio > 0) {
        std::random_shuffle(keys.begin(), keys.end());
        size_t numFailKeys = size * unsucc_ratio;
        std::mt19937_64 gen(325246);
        unsigned long long int max = ~0ULL;
        std::uniform_int_distribution<MWord> dis_key(1, max);
        for (size_t i = 0; i < numFailKeys; ++i) {
            keys[i] = dis_key(gen);
        }
    }
    std::random_shuffle(keys.begin(), keys.end());
    
    //step2: perform lookups
    //size_t num_operations = 16*size;
    
    double start = getTimeSec();
    
    //below: method 1 - same as paper
//    int repeat = 16;
//    for (int r = 0; r<repeat; r++) {
//        for (size_t i = 0; i<size; i++){
//            map->get(keys[i]);
//        }
//    }
    //below: method 2 - custom
    size_t num_operations = 16*size;
    for (size_t i = 0; i<num_operations; i++){
        map->get(keys[i%size]); //mod size to not exceed
    }
    
    
    double time = getTimeSec() - start;
    //std::cout<<type<<" - time elapsed: "<<time<<std::endl;
    std::cout<<time<<std::endl;
    return time;
}

/*---------------------------Experiments Below------------------------*/

/*IMPORTANT: it became clear that calling univ_helper in a loop produced very similar results. instead, need to just run the executable mulitple times.

example:
 start of experiment 1 w/ ratio 0
 5 times for LP:
 1.1688
 1.16658
 1.16018
 1.16013
 1.15448
 5 times for CH:
 1.99127
 1.9974
 1.99549
 1.98744
 1.99352
 
 ^clearly, this is fucked.
 */

//reproduces simple WORM paper results for CH vs LP for unsucc. ratio
void experiment1(float ratio) {
    std::cout<<"start of experiment 1 w/ ratio "<<ratio<<std::endl;
    size_t size = 1000000;
    size_t capacity = size_t(pow(2,21)); //~47% LF, just like paper

    double LP_time = lookup_helper(size, capacity, "LP", "DENSE", ratio);
    double CH_time = lookup_helper(size, capacity, "CH", "DENSE", ratio);

    std::cout<<"LP: "<<(LP_time)<<", CH: "<<(CH_time)<<std::endl;
}

//deprecated version below:

//void experiment1(float ratio, int trials) {
//    std::cout<<"start of experiment 1 w/ ratio "<<ratio<<std::endl;
//    size_t size = 1000000;
//    size_t capacity = size_t(pow(2,21)); //~47% LF, just like paper
//
//    std::cout<<trials<<" times for LP: "<<std::endl;
//    double LP_time = 0;
//    for (int i = 0; i<trials; i++)
//        LP_time += lookup_helper(size, capacity, "LP", "DENSE", ratio);
//
//    std::cout<<trials<<" times for CH: "<<std::endl;
//    double CH_time = 0;
//    for (int i = 0; i<trials; i++)
//        CH_time += lookup_helper(size, capacity, "CH", "DENSE", ratio);
//
//    std::cout<<"avg CH: "<<(CH_time/trials)<<", avg LP: "<<(LP_time/trials)<<std::endl<<std::endl;
//}

int main(int argc, char*argv[]) {
    
    experiment1(0);
    
}



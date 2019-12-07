#include "adaptive_hash.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
//#include "Timer.hpp"

using std::vector;
using std::string;


//given an empty vector (already allocated), insert appropriate keys
void generateKeys(vector<uint64_t>& keys, size_t size, string dist = "DENSE", size_t begin = 0, size_t end = 0) {
    std::mt19937 gen(198); //random func with seed as 197.
    
    if (dist == "DENSE") {
        if (end == 0) end = size;
        for (int i = begin; i<end; i++) {
            keys[i] = i+1;
        }
//        for (int i = 0; i<size; i++) {
//            keys[i] = i+1; //b/c 0 isnt valid.
//        }
        
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

//measure the time it takes to rehash vs. transferHash()
//type args: "rehash" or "transfer"
void exp2_helper(string type = "transfer") {
    
    //Chained Rehash
    int power = 21;
    size_t size = size_t(pow(2,power-1));
    size_t capacity = int(pow(2,power));
    vector<uint64_t> keys(size);
    generateKeys(keys, keys.size(), "DENSE");
    
    //GenericHashTable* map = new ChainedHashMap<MultiplicativeHash>(capacity);
    GenericHashTable* map = new LinearHashTable<int, int, MultiplicativeHash, 0L, false>(std::log2(capacity));
    for (int i = 0; i<keys.size(); i++) {
        map->put(keys[i], i+42);
    }
    double start = getTimeSec();
    double time = 0;
    if (type == "rehash") {
        map->put(1,1);
        time = getTimeSec() - start;
    }
    else if (type == "transfer") {
        ChainedHashMap<MultiplicativeHash>* new_map = new ChainedHashMap<MultiplicativeHash>(2*map->size());
        //((ChainedHashMap<MultiplicativeHash>*)map)->transferHash(new_map);
        ((LinearHashTable<int, int, MultiplicativeHash, 0L, false>*)map)->transferHash(new_map);
        GenericHashTable* trash = map;
        map = new_map;
        delete trash;
        //free(static_cast<void *> (trash));
        time = getTimeSec() - start;
    }
    
    std::cout<<type<<": "<<time<<std::endl;
}

void experiment2() {
    exp2_helper("rehash");
    exp2_helper("transfer");
}

////full attempt. doesn't work currently (segfault)
//double ins_helper(size_t size, size_t capacity, string type, string distr, float unsucc_ratio) {
//    GenericHashTable* map;
//    if (type == "LP") {
//        map = new LinearHashTable<int, int, MultiplicativeHash, 0L, false> (std::log2(capacity)); //care to do log2 here
//    }
//    else if (type == "CH") {
//        map = new ChainedHashMap<MultiplicativeHash>(capacity);
//    }
//    else { throw "invalid type arg";}
//
//
//    vector<vector<uint64_t>> ins_keys = {vector<uint64_t>(size), vector<uint64_t>(2*size), vector<uint64_t>(4*size), vector<uint64_t>(8*size)};
//    size_t begin = 0;
//    size_t end = size;
//    for (int i = 0; i<ins_keys.size(); i++){
//        generateKeys(ins_keys[i], ins_keys[i].size(), "DENSE", begin, end);
//        begin = size+1;
//        end = 2*size;
//    }
//
//    vector<uint64_t> keys(size);
//    generateKeys(keys, size, distr);
//
//    vector<uint64_t> lookup_keys = keys; //<-todo: make this an array, mimicking ins_keys
//
//    //plant fail keys
//    if (unsucc_ratio > 0) {
//        std::random_shuffle(lookup_keys.begin(), lookup_keys.end());
//        size_t numFailKeys = size * unsucc_ratio;
//        std::mt19937_64 gen(325246);
//        unsigned long long int max = ~0ULL;
//        std::uniform_int_distribution<MWord> dis_key(1, max);
//        for (size_t i = 0; i < numFailKeys; ++i) {
//            lookup_keys[i] = dis_key(gen);
//        }
//    }
//    std::random_shuffle(lookup_keys.begin(), lookup_keys.end());
//
//
//    //step2: perform lookups
//    //size_t num_operations = 16*size;
//    std::cout<<"got here!"<<std::endl;
//    double start = getTimeSec();
//
//
//    for (int j = 0; j<ins_keys.size(); j++){
//        //below: method 2 - custom
//        int ratio = 4; //lookup/ins
//        size_t num_operations = ratio * ins_keys[j].size();
//        for (size_t i = 0; i<num_operations; i++){
//            std::cout<<"("<<j<<", "<<i<<")"<<std::endl;
//            if (i%ratio == 0){ //every 'ratio' # of iters, perform an insert
//                map->put(ins_keys[j][i/ratio], i+42);
//            }
//            else {
//                map->get(lookup_keys[i/ratio]); //lookup same key 'ratio' times
//            }
//        }
//    }
//
//
//    double time = getTimeSec() - start;
//    //std::cout<<type<<" - time elapsed: "<<time<<std::endl;
//    std::cout<<time<<std::endl;
//    return time;
//}
//
////test the ratio of insertions to lookups that will still yield results from exp. 1
//void experiment3(float ratio) {
//    std::cout<<"start of experiment 3 w/ ratio "<<ratio<<std::endl;
//    //size_t size = 1000000;
//    int power = 18;
//    size_t size = size_t(pow(2,power-1));
//    size_t capacity = size_t(pow(2,power));
//
//    double LP_time = ins_helper(size, capacity, "LP", "DENSE", ratio);
//    double CH_time = ins_helper(size, capacity, "CH", "DENSE", ratio);
//
//    std::cout<<"LP: "<<(LP_time)<<", CH: "<<(CH_time)<<std::endl;
//}


//slightly modified lookup_helper
double hybrid_helper(size_t size, size_t capacity, string type, string distr, float unsucc_ratio) {
    GenericHashTable* map;
    if (type == "LP") {
        map = new LinearHashTable<int, int, MultiplicativeHash, 0L, false> (std::log2(capacity)); //care to do log2 here
    }
    else if (type == "CH") {
        map = new ChainedHashMap<MultiplicativeHash>(capacity);
    }
    else { throw "invalid type arg";}
    
    vector<uint64_t> put_keys(size);
    generateKeys(put_keys, size, distr);
    
    vector<uint64_t> get_keys = put_keys;
    
    //plant fail keys
    if (unsucc_ratio > 0) {
        std::random_shuffle(get_keys.begin(), get_keys.end());
        size_t numFailKeys = size * unsucc_ratio;
        std::mt19937_64 gen(325246);
        unsigned long long int max = ~0ULL;
        std::uniform_int_distribution<MWord> dis_key(1, max);
        for (size_t i = 0; i < numFailKeys; ++i) {
            get_keys[i] = dis_key(gen);
        }
    }
    std::random_shuffle(get_keys.begin(), get_keys.end());
    
    
    double start = getTimeSec();
    
    //below: method 2 - custom
    int factor = 6;
    size_t num_operations = factor*size;
    for (size_t i = 0; i<num_operations; i++){
        if (i%factor==0){
            map->put(put_keys[i/factor], i+1);
        }
        else {
            map->get(get_keys[i%size]);
        }
    }
    double pretime = getTimeSec() - start;
    std::cout<<type<<" before rehash: "<<pretime<<std::endl;
    map->put(num_operations, 999); //rehash here!
    double finaltime = getTimeSec() - start;
    std::cout<<"rehash duration: "<<finaltime-pretime<<std::endl;
    //std::cout<<type<<" - time elapsed: "<<time<<std::endl;
    std::cout<<"total: "<<finaltime<<std::endl<<std::endl;
    return finaltime;
}

//hybrid ins and lookup
void experiment4(float ratio) {
    std::cout<<"start of experiment 4 w/ ratio "<<ratio<<std::endl;
    //note: 2^20 is a bit over 1 million
    int power = 22;
    size_t size = size_t(pow(2,power-1));
    size_t capacity = size_t(pow(2,power));

    double LP_time = hybrid_helper(size, capacity, "LP", "DENSE", ratio);
    double CH_time = hybrid_helper(size, capacity, "CH", "DENSE", ratio);

    //std::cout<<"LP: "<<(LP_time)<<", CH: "<<(CH_time)<<std::endl;
}

//test insertion overhead of AHT
void exp5helper(size_t size, size_t capacity) {
    GenericHashTable* map = map = new LinearHashTable<int, int, MultiplicativeHash, 0L, false> (std::log2(capacity));
    
    //AdaptiveHashTable* map = new AdaptiveHashTable(capacity, "LP");
    
    vector<uint64_t> put_keys(size);
    generateKeys(put_keys, size, "DENSE");
    
    double start = getTimeSec();
    for (int i = 0; i<size-2; i++) {
        map->put(put_keys[i], i+42);
    }
    double end = getTimeSec() - start;
    
    std::cout<<"total time: "<<end<<std::endl;
}

void experiment5() {
    std::cout<<"start of experiment 5"<<std::endl;
    int power = 22;
    size_t size = size_t(pow(2, power-1));
    size_t capacity = size_t(pow(2, power));
    exp5helper(size, capacity);
}

int main(int argc, char*argv[]) {
    
    //experiment1(0);
    
    //experiment2();
    
    //experiment3(0.00);
    
    //experiment2();
    
    //experiment4(1);
    
    experiment5();
    
}



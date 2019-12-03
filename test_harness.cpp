#include "adaptive_hash.hh"
#include <iostream>
#include <vector>



//basic test to ensure nothing is broken
void create_simple_hash_table() { //assumes all input is valid
	AdaptiveHashTable test(20);
    for (int i = 0; i<5; i++) {
        test.insert(i,1);
    }
    
    test.printElems();
    
}

int main(int argc, char*argv[]) {
    
    create_simple_hash_table();
    return 0;
}



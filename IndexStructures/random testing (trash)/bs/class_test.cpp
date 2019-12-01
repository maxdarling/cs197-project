//
//  class_test.cpp
//  
//
//  Created by Max  on 11/30/19.
//

#include <stdio.h>
#include <iostream>

template <class C>
void printNums (C obj) {
    std::cout<<"val: "<<obj.LOOKUP()<<std::endl;
}

class c1 {
private:
    int x;
public:
    c1 () {
        x = 5;
    }
    
    int LOOKUP () {
        return x;
    }
};



int main () {
    std::cout<<"program start"<<std::endl;
    c1 var;
    printNums(var);
    
    std::cout<<"program end"<<std::endl;

}

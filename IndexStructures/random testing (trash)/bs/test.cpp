//about: this code tests polymorphism to potentially be used in the final project.
//after testing this, i'm thinking that using templates is a better idea.

#include "BaseTable.hpp"
#include <vector>
#include <string>
#include <iostream>


class ChainedTable : public BaseTable {
private:
    std::vector<std::pair<int, std::string> > map;
public:
    ChainedTable () {
        //nothing in constructor
    }
    
    void insert(int key, int val) {
        map.push_back(std::make_pair(key, "chained"));
    }
    
    
    //template<class C>
    void switchTable (BaseTable& new_table) {
        for (int i = 0; i<map.size(); i++) {
            new_table.insert(map[i].first, 1);
        }
    }
                      
};

class LinearTable : public BaseTable {
    
private:
    int length;
    int* arr;
    int* curr;
    
public:
    LinearTable (size_t arr_size = 10) {
        length = 2*arr_size;
        arr = new int[length];
        curr = arr;
    }
    
    void insert(int key, int value) {
        if (curr >= arr + length - 1) throw "exception: out of bounds";
        *curr++ = key;
        *curr++ = value;
    }
    
    void switchTable (BaseTable& new_table) {
        for (int i = 0; i<length; i+=2) {
            new_table.insert(arr[i], arr[i+1]);
        }
    }
    
    void printElems () {
        for (int i = 0; i<length; i+=2) {
            std::cout<<"key: "<<arr[i]<<", value: "<<arr[i+1]<<std::endl;
        }
    }
    
};


int main () {

    //BaseTable* table = new ChainedTable;
    ChainedTable* table = new ChainedTable;
    table->insert(1,1);
    table->insert(2,1);
    table->insert(3,1);
    
//    BaseTable* temp = new LinearTable(6);
//    (ChainedTable*)table->switchTable(temp);
    
    LinearTable* temp = new LinearTable(6);
    table->switchTable(*(BaseTable*)temp);
    
    delete table;
    table = temp;
    
    table->printElems();
    
    std::cout<<"program end"<<std::endl;
    return 0;
}

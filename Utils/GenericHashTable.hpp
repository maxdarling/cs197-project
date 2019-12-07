#include <vector>
#include <any>

class GenericHashTable 
{
  public:
    // add error checking to remove, put fxns
    virtual size_t size() = 0;
    virtual void remove(uint64_t K) = 0;
    virtual uint64_t get(uint64_t K) = 0;
    virtual void put(uint64_t K, uint64_t H) = 0;
    //returns bool when it is full. idea is that it lets you rehash manually.
    virtual bool put2(uint64_t K, uint64_t H) = 0;
    virtual void printElems() = 0;
    //virtual void transferHash(ChainedHashMap* new_table) = 0;
    virtual ~GenericHashTable(){};
};

#include <vector>
#include <any>
class GenericHashTable 
{
  public:
    // add error checking to remove, put fxns
    virtual void remove(uint64_t K) = 0;
    virtual uint64_t get(uint64_t K) = 0;
    //add error checking to remove & put function
    virtual void put(uint64_t K, uint64_t H) = 0;	  
    virtual void printElems() = 0;
    virtual ~GenericHashTable(){};
};

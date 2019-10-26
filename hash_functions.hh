#ifndef HASH_FUNCTIONS_HH
#define HASH_FUNCTIONS_HH


#include <math.h>
#include <string>

class HashFunctions{
	private:
		std::unordered_set<std::string> valid_funcs{"M-S", "M-A-S"};
	public:
		HashFunctions(std::string);
		unsigned int countBits (unsigned int number);
		size_t multishift(size_t input);
		size_t multiaddshift(size_t input);
};

#endif

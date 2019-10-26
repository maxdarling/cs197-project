#include <math.h>
#include <algorithms>


class HashFunctions{
	private:
		std::unordered_set<std::string> valid_funcs{"M-S", "M-A-S"};
	public:
		unsigned int countBits (unsigned int number);
		size_t multishift(size_t input);
		size_t multiaddshift(size_t input);
}

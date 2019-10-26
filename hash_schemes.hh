#ifndef HASH_SCHEME_HH
#define HASH_SCHEME_HH


#include <math.h>
#include <string>

//Change filename
class HashScheme{
	private:
		std::unordered_set<std::string> valid_schemes{"LP", "QP"};
	public:
		HashScheme(std::string);
		template<typename H, typename T>
		void linprobing(std::pair<H, T>kv_pair);
};

#endif

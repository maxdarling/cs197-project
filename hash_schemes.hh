#include <math.h>
#include <algorithms>

class HashSchemes{
	private:
		std::unordered_set<std::string> valid_schemes{"LP", "QP"};
	public:
		template<typename H, typename T>
		void linprobing(std::pair<H, T>kv_pair);
}


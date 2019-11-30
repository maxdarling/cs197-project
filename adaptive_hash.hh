#ifndef ADAPTIVE_HASH_TABLE_HH
#define ADAPTIVE_HASH_TABLE_HH

#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include "Utils/Hash_Functions.hpp"
#include "Utils/LinearHashTable.hpp"
#include "Utils/QuadraticHashTable.hpp"
#include "Utils/ChainedHashTable.hpp"
//#include "Utils/RobinDistanceHashTable.hpp"
#include <math.h>

//TODO: Think of all variations with resizing, key-value pairs, etc.
class AdaptiveHashTable{
private:
	int distrType; //Dense = -1, Sparse = 0, Grid = 1
	std::string currHash;
	std::string currFunc;
	std::unordered_set<std::string> valid_schemes{"LP", "QP", "CH"};
	std::unordered_set<std::string> valid_funcs{"M-S", "M-A-S"};
	size_t lookup_ratio;
	size_t load_factor;
	size_t table_size;
	size_t table_capacity;
	size_t table_density; // sparse = 0 <= t_d <= 1 = dense
	size_t read_time;
	size_t write_time;
	enum HashFunction {
		MultiplicativeHash, 
		TabulationHash, 
		MultiplyAddHash, 
		MurmurHash
	};
	enum HashSchemes(HashFunction) {
		ChainedHashMap<HashFunction>,
		LinearHashTable<int, int, HashFunction, 0L, false>,
		QuadraticHashTable<int, int, HashFunction, 0L, false>
	};
	struct HashTable{
		size_t index;
		//std::unordered_map<size_t index, vector< ChaindHashMap<HashFunction> > table;
		ChainedHashMap<HashFunction> table;
	};
	HashTable the_table; //better way to declare this
public:
	//AdaptiveHashTable(some data);
	AdaptiveHashTable(); 
	//get all the appropriate variables
	std::string get_hash_scheme() {return currFunc; };
	std::string get_hash_function() {return currHash; };
	size_t get_lookup_ratio() {return lookup_ratio; };
	size_t get_load_factor() {return load_factor; };
	size_t get_table_size() {return the_table.table.size(); };
	size_t get_table_capacity() {return table_capacity; };
	size_t get_density() {return table_density; };
	size_t get_read_time() {return read_time; };
	size_t get_write_time() {return write_time; };
 	
	//update the appropriate variables
	void update_load_factor();
	void update_lookup_ratio();
	void update_density();
	void update_read_time();
	void update_write_time();

	//other important functions
	void make_hash_table();
	void print_out_hash_table() {the_table.table.printElems()};
	void insert_into_table(int Key, int Value) {the_table.table.put(Key, Value);};
	void remove_from_table(int Key) {the_table.table.remove(Key);};
	void read_from_table(int Key) {the_table.table.get(Key);};
	bool assess_switching();
};

#endif

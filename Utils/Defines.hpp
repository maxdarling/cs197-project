#ifndef DEFINES_HPP
#define DEFINES_HPP

//
//#define FOR_PLOT
#define PRINT_STATISTICS
//#define PRINT_NODE_SIZES
//#define LH_DEBUG_PROBE_COUNT
//
#define COVERING_IDX

#define UNORDERED_CONTROLLED_LOAD_FACTOR

#define FIBONACCI_DENSE  // This has effect only on Cuckoo
//#define FAST_MORE_TABLES // This has effect only on Cuckoo
#define PRIMARY_KEY_ONLY_CUCKOO // This has effect only on Cuckoo

#define INIT_WITH_SIZE

#define DO_POINT_QUERIES

//#define PROFILE_LOOKUPS
//#define PROFILE_INSERTIONS

//#define PRINT_STATISTIC

#define STEFAN_ZIPF

//#define MULTIPLE_MULTHASHING

#define LINEAR
#define ROBINHT
//#define CUCKOO2
//#define CUCKOO3
#define CUCKOO4
#define QUADRATIC
#define UNORDERED
//#define ARRAY

#define OUTER_RELATION 30

//this is to make chained hashing go beyond lf 1
//#define HIGH_LOAD_FACTOR

//#define SIMD_LOOKUP
//#define SIMD_INSERT
//#define SIMD_WIDTH 4
//
//#define LOOKUP_DEPENDENCY

#endif	/* DEFINES_HPP */

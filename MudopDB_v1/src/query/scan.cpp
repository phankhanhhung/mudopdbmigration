#include "query/scan.hpp"

// Scan is an abstract base class with pure virtual methods.
// All concrete scan implementations (ProductScan, TableScan, etc.)
// will inherit from this class and provide implementations.
//
// This file is intentionally minimal as Scan provides only the interface.
// Concrete implementations are in separate files:
//   - ProductScan (query/productscan.cpp)
//   - ProjectScan (query/projectscan.cpp)
//   - SelectScan (query/selectscan.cpp)
//   - TableScan (record/tablescan.cpp)
//   - IndexSelectScan (index/query/indexselectscan.cpp)
//   - IndexJoinScan (index/query/indexjoinscan.cpp)
//   - ChunkScan (multibuffer/chunkscan.cpp)
//   - MultibufferProductScan (multibuffer/multibufferproductscan.cpp)
//   - SortScan (materialize/sortscan.cpp)
//   - GroupByScan (materialize/groupbyscan.cpp)
//   - MergeJoinScan (materialize/mergejoinscan.cpp)

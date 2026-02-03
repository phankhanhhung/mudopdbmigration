# MudopDB Storage Engine Architecture - Complete Phase Overview

**Project:** NMDB2 (Rust) → MudopDB_v1 (C++) Migration
**Date:** October 12, 2025
**Status:** Phase 0-2 Complete, Phase 3-7 Planned

---

## Architecture Layers (Bottom-Up)

The storage engine follows a layered architecture, where each layer builds upon the one below it:

```
┌─────────────────────────────────────────────┐
│  Phase 7: SQL Interface (JDBC-style API)   │ ✅ Complete
├─────────────────────────────────────────────┤
│  Phase 6: Query Processing & Planning      │ 🔲 Planned
├─────────────────────────────────────────────┤
│  Phase 5: Metadata Management              │ 🔲 Planned
├─────────────────────────────────────────────┤
│  Phase 4: Record Management                │ 🔲 Planned
├─────────────────────────────────────────────┤
│  Phase 3: Buffer Management                │ 🔲 Planned
├─────────────────────────────────────────────┤
│  Phase 2: Logging Layer (WAL)              │ ✅ Complete
├─────────────────────────────────────────────┤
│  Phase 1: File Layer (Disk I/O)            │ ✅ Complete
└─────────────────────────────────────────────┘
```

**Legend:**
- ✅ Complete: Implemented, tested (100% pass rate), Docker verified
- 🔲 Planned: Designed but not yet implemented

---

## Phase 0: API Layer (Previously Completed)

### Status: ✅ Complete (79 tests passing)

### Components
- **Driver Interface:** Abstract driver for database connections
- **Connection Interface:** Connection management (embedded, network)
- **Statement Interface:** SQL statement execution
- **ResultSet Interface:** Query result iteration
- **Constant:** Typed value container (int, string)
- **Scan Interface:** Abstract table/query scan interface

### Purpose
Provides JDBC-style API for client applications. This is the top layer that users interact with.

### Test Coverage
79 tests covering connection lifecycle, statement execution, result iteration, and driver management.

---

## Phase 1: File Layer (Disk I/O)

### Status: ✅ Complete (27 tests passing)

### Components

#### BlockId (`include/file/blockid.hpp`, `src/file/blockid.cpp`)
- **Purpose:** Unique identifier for disk blocks
- **Key Feature:** Filename + block number pair
- **Hash Support:** Can be used in `std::unordered_set` and `std::unordered_map`

#### Page (`include/file/page.hpp`, `src/file/page.cpp`)
- **Purpose:** In-memory representation of a disk block
- **Key Features:**
  - Big-endian integer encoding
  - String storage (4-byte length prefix + data)
  - Byte array operations
  - Bounds checking
- **Format:**
  - Integer: 4 bytes big-endian
  - String: [4-byte length][UTF-8 data]
  - Bytes: [4-byte length][raw data]

#### FileMgr (`include/file/filemgr.hpp`, `src/file/filemgr.cpp`)
- **Purpose:** File system interface for database files
- **Key Features:**
  - Block-level read/write/append
  - Thread-safe with mutex protection
  - File size caching
  - Automatic temp file cleanup
- **Concurrency:** `std::mutex` protects all file operations

### Dependencies
- C++ Standard Library (`<fstream>`, `<filesystem>`)
- No external dependencies

### Test Coverage
27 tests covering:
- BlockId: construction, equality, ordering, hashing (6 tests)
- Page: integer/string/byte operations, big-endian, bounds (11 tests)
- FileMgr: read/write, persistence, multi-file, temp cleanup (10 tests)

### Lines of Code
~600 lines (headers + implementation + tests)

---

## Phase 2: Logging Layer (Write-Ahead Log)

### Status: ✅ Complete (17 tests passing)

### Components

#### LogIterator (`include/log/logiterator.hpp`, `src/log/logiterator.cpp`)
- **Purpose:** Iterate through log records for recovery
- **Key Features:**
  - Backward iteration (most recent to oldest)
  - Forward within each page
  - Automatic block boundary crossing
  - Exception on exhausted iterator

#### LogMgr (`include/log/logmgr.hpp`, `src/log/logmgr.cpp`)
- **Purpose:** Write-Ahead Log manager
- **Key Features:**
  - Log Sequence Numbers (LSN)
  - Backward-growing page format
  - Automatic page allocation
  - Flush semantics (flush if LSN >= last_saved_lsn)
- **Page Format:**
  ```
  [boundary: 4 bytes][empty space][record1][record2][...]
                                   ^--- records grow backward from end
  ```

### Write-Ahead Logging (WAL) Principles
1. **Durability:** Log records must be flushed before data pages
2. **Recovery:** Iterator reads from newest to oldest for undo/redo
3. **Efficiency:** Backward-growing pages avoid record shifting

### Dependencies
- Phase 1 (File Layer): FileMgr, Page, BlockId

### Test Coverage
17 tests covering:
- LogIterator: empty/single/multiple records, cross-block (6 tests)
- LogMgr: append, LSN, flush, page wrapping, persistence (11 tests)

### Lines of Code
~700 lines (headers + implementation + tests)

---

## Phase 3: Buffer Management (Planned)

### Status: 🔲 Planned

### Planned Components

#### Buffer (`include/buffer/buffer.hpp`)
- **Purpose:** Wrapper around Page with pinning semantics
- **Key Features:**
  - Pin/unpin reference counting
  - Associated with BlockId
  - Modification tracking (dirty flag)
  - LSN tracking for recovery

#### BufferMgr (`include/buffer/buffermgr.hpp`)
- **Purpose:** Buffer pool manager
- **Key Features:**
  - LRU eviction policy
  - Pin/unpin tracking
  - Limited pool size (configurable)
  - Thread-safe buffer allocation
  - Dirty page flushing

### Design Goals
- **Performance:** Reduce disk I/O through caching
- **Concurrency:** Thread-safe buffer allocation and eviction
- **Durability:** Respect WAL (flush log before data)

### Estimated Effort
- **Time:** 3-4 days
- **Tests:** ~25 tests
- **Lines of Code:** ~1500 lines

### Dependencies
- Phase 1 (File Layer): FileMgr, Page, BlockId
- Phase 2 (Logging Layer): LogMgr (for WAL compliance)

---

## Phase 4: Record Management (Planned)

### Status: 🔲 Planned

### Planned Components

#### Schema (`include/record/schema.hpp`)
- **Purpose:** Table schema definition
- **Key Features:**
  - Field names and types
  - Field offsets for record layout
  - Schema serialization

#### Layout (`include/record/layout.hpp`)
- **Purpose:** Physical record layout
- **Key Features:**
  - Calculate field offsets
  - Record size computation
  - Support for variable-length fields (strings)

#### RecordPage (`include/record/recordpage.hpp`)
- **Purpose:** Manage records within a page
- **Key Features:**
  - Insert/delete records
  - Slot-based record storage
  - Free space tracking
  - Record ID (RID) generation

#### TableScan (`include/record/tablescan.hpp`)
- **Purpose:** Sequential scan of table records
- **Key Features:**
  - Implements Scan interface
  - Multi-page scanning
  - Record insertion/deletion
  - Field get/set operations

### Design Goals
- **Fixed-length fields:** Predictable offsets for fast access
- **Variable-length fields:** Support for strings (stored at end of record)
- **Deletion:** Use tombstone/slot recycling

### Estimated Effort
- **Time:** 4-5 days
- **Tests:** ~30 tests
- **Lines of Code:** ~2000 lines

### Dependencies
- Phase 1 (File Layer): Page
- Phase 3 (Buffer Management): BufferMgr

---

## Phase 5: Metadata Management (Planned)

### Status: 🔲 Planned

### Planned Components

#### TableMgr (`include/metadata/tablemgr.hpp`)
- **Purpose:** Manage table metadata
- **Key Features:**
  - Create/drop tables
  - Store table schemas in system catalog
  - Track table statistics (record count, block count)

#### ViewMgr (`include/metadata/viewmgr.hpp`)
- **Purpose:** Manage view definitions
- **Key Features:**
  - Create/drop views
  - Store view definitions (SQL text)

#### StatMgr (`include/metadata/statmgr.hpp`)
- **Purpose:** Track table statistics
- **Key Features:**
  - Block count per table
  - Record count per table
  - Used for query optimization

#### IndexMgr (`include/metadata/indexmgr.hpp`)
- **Purpose:** Manage index metadata
- **Key Features:**
  - Create/drop indexes
  - Track index type (B-tree, hash)
  - Store index schema

### Design Goals
- **System Catalog:** Metadata stored as regular tables (bootstrap problem)
- **Caching:** In-memory metadata cache for performance
- **Consistency:** Metadata updates within transactions

### Estimated Effort
- **Time:** 5-6 days
- **Tests:** ~35 tests
- **Lines of Code:** ~2500 lines

### Dependencies
- Phase 4 (Record Management): TableScan, Schema, Layout

---

## Phase 6: Query Processing & Planning (Planned)

### Status: 🔲 Planned

### Planned Components

#### Query Parser
- **Purpose:** Parse SQL into AST
- **Key Features:**
  - SELECT, INSERT, UPDATE, DELETE
  - CREATE TABLE, CREATE INDEX
  - WHERE clause parsing

#### Query Planner
- **Purpose:** Generate execution plans
- **Key Features:**
  - Table scan plans
  - Index scan plans
  - Join algorithms (nested loop, hash join)
  - Predicate pushdown
  - Cost-based optimization

#### Execution Engine
- **Purpose:** Execute query plans
- **Key Features:**
  - Iterator-based execution (Volcano model)
  - Projection, selection, join operators
  - Aggregation (COUNT, SUM, AVG, etc.)
  - Sorting

### Design Goals
- **Modularity:** Operators as composable iterators
- **Optimization:** Cost-based plan selection
- **Extensibility:** Easy to add new operators

### Estimated Effort
- **Time:** 10-14 days
- **Tests:** ~50 tests
- **Lines of Code:** ~4000 lines

### Dependencies
- Phase 4 (Record Management): TableScan
- Phase 5 (Metadata Management): TableMgr, StatMgr

---

## Phase 7: SQL Interface (JDBC-style API)

### Status: ✅ Complete (Phase 0 - previously implemented)

### Components
Already implemented as Phase 0:
- Driver, Connection, Statement, ResultSet
- Constant (typed value)
- Scan interface

### Integration with Storage Engine
- **Connection** will create transactions (uses BufferMgr, LogMgr)
- **Statement** will invoke query planner/executor
- **ResultSet** wraps query scan results

---

## Complete Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    Client Application                        │
└───────────────────────────┬─────────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────────┐
│  Phase 0/7: JDBC-style API (Driver, Connection, Statement)  │ ✅
│  - EmbeddedDriver, NetworkDriver                            │
│  - EmbeddedConnection, NetworkConnection                    │
│  - EmbeddedStatement, EmbeddedResultSet                     │
└───────────────────────────┬─────────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────────┐
│  Phase 6: Query Processing (Parser, Planner, Executor)      │ 🔲
│  - SQL Parser → AST                                         │
│  - Planner → Execution Plan (Scan, Join, Sort, Aggregate)  │
│  - Executor → Iterator-based execution (Volcano model)     │
└───────────────────────────┬─────────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────────┐
│  Phase 5: Metadata Management (System Catalog)              │ 🔲
│  - TableMgr (create/drop tables, schemas)                  │
│  - ViewMgr (view definitions)                              │
│  - StatMgr (table statistics for optimization)             │
│  - IndexMgr (index metadata)                               │
└───────────────────────────┬─────────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────────┐
│  Phase 4: Record Management (Heap Files, Table Scans)       │ 🔲
│  - Schema (field definitions)                              │
│  - Layout (physical record layout, offsets)                │
│  - RecordPage (slot-based record storage)                  │
│  - TableScan (sequential scan, insert/delete)              │
└───────────────────────────┬─────────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────────┐
│  Phase 3: Buffer Management (Buffer Pool, LRU Eviction)     │ 🔲
│  - Buffer (Page wrapper with pin/unpin)                    │
│  - BufferMgr (pool manager, eviction policy)               │
│  - Dirty page tracking, WAL compliance                     │
└───────────────────────────┬─────────────────────────────────┘
                            │
                ┌───────────┴───────────┐
                │                       │
┌───────────────▼───────┐   ┌───────────▼──────────────┐
│  Phase 2: WAL         │   │  Phase 1: File Layer     │
│  - LogMgr             │   │  - FileMgr (block I/O)   │ ✅
│  - LogIterator        │   │  - Page (byte buffer)    │
│  - LSN tracking       │   │  - BlockId (identifier)  │
│  - Flush semantics    │   │  - Big-endian encoding   │
└───────────────────────┘   └──────────────────────────┘
         ✅                            ✅
```

---

## Implementation Timeline

### Completed (October 12, 2025)
- ✅ **Phase 0:** API Layer (79 tests) - Previously completed
- ✅ **Phase 1:** File Layer (27 tests) - Completed today
- ✅ **Phase 2:** Logging Layer (17 tests) - Completed today

**Total Tests Passing:** 124/124 (100%)

### Planned Implementation Order

1. **Phase 3: Buffer Management** (3-4 days, ~25 tests)
   - Critical for performance
   - Required by all upper layers

2. **Phase 4: Record Management** (4-5 days, ~30 tests)
   - Builds on buffer pool
   - Provides table storage

3. **Phase 5: Metadata Management** (5-6 days, ~35 tests)
   - System catalog for tables/indexes
   - Statistics for query optimization

4. **Phase 6: Query Processing** (10-14 days, ~50 tests)
   - Parser, planner, executor
   - Largest and most complex phase

5. **Integration & Testing** (3-5 days)
   - End-to-end tests
   - Performance benchmarks
   - Bug fixes

**Total Estimated Time:** 25-34 days for Phases 3-6

---

## Key Technical Decisions

### 1. Layered Architecture
- **Rationale:** Modularity, testability, matches Rust reference
- **Benefit:** Each layer can be tested independently

### 2. Big-Endian Encoding
- **Rationale:** Cross-platform compatibility
- **Trade-off:** Byte swapping overhead (negligible)

### 3. Backward-Growing WAL Pages
- **Rationale:** Efficient append (no record shifting)
- **Trade-off:** Slightly complex iteration logic

### 4. Thread-Safe Design
- **Rationale:** Prepare for concurrent transactions
- **Strategy:** Mutex protection in FileMgr, future: fine-grained locking in BufferMgr

### 5. Smart Pointers
- **Rationale:** Memory safety, clear ownership
- **Usage:** `std::shared_ptr` for shared ownership, `std::unique_ptr` for exclusive

---

## Testing Strategy

### Unit Tests
Each component has comprehensive unit tests:
- **BlockId:** 6 tests (construction, equality, hashing)
- **Page:** 11 tests (int/string/byte ops, bounds)
- **FileMgr:** 10 tests (read/write, persistence)
- **LogIterator:** 6 tests (iteration, cross-block)
- **LogMgr:** 11 tests (append, LSN, flush, wrapping)

### Integration Tests
- File layer: Table simulation (multi-record pages)
- Log layer: Persistence across sessions

### Future Testing
- **Phase 3+:** Performance benchmarks (buffer hit rate, eviction)
- **Phase 6:** End-to-end SQL tests (TPC-C style)

---

## Dependencies and Build System

### External Dependencies
- **GoogleTest:** Testing framework (v1.14.0)
- **CMake:** Build system (≥3.10)
- **C++17:** Language standard

### Internal Dependencies
```
Phase 6 (Query) → Phase 5 (Metadata) → Phase 4 (Record)
                                            ↓
Phase 3 (Buffer) → Phase 2 (Log)
                 ↘ Phase 1 (File)
```

### Docker Integration
Multi-stage Dockerfile:
- **Build stage:** Compile with tests enabled
- **Test stage:** Run `ctest --output-on-failure`
- **Runtime stage:** Slim production image

---

## Risk Assessment

### Technical Risks

#### High Risk
- **Buffer Pool Concurrency (Phase 3):** Race conditions in pin/unpin, eviction
  - **Mitigation:** Extensive concurrency tests, use of mutexes/condition variables

#### Medium Risk
- **Query Optimizer Complexity (Phase 6):** Cost estimation, plan selection
  - **Mitigation:** Start with simple heuristics, iterate
- **Record Layout Variable-Length Fields (Phase 4):** String storage complexity
  - **Mitigation:** Fixed-length first, then add variable-length

#### Low Risk
- **Metadata Bootstrapping (Phase 5):** Chicken-and-egg problem
  - **Mitigation:** Follow Rust reference implementation exactly

### Project Risks
- **Scope Creep:** Too many features before core is solid
  - **Mitigation:** Stick to phase plan, defer advanced features
- **Testing Debt:** Skipping tests to go faster
  - **Mitigation:** Require 100% test pass rate before moving to next phase

---

## Success Criteria

### Per-Phase Criteria
- ✅ All unit tests passing (100%)
- ✅ Integration tests covering key workflows
- ✅ Docker build successful
- ✅ Code review by maintainer
- ✅ Documentation updated

### Overall Project Success
- All 8 phases complete (Phases 0-7)
- Full TPC-C style benchmark passing
- Performance comparable to Rust reference (±20%)
- Zero memory leaks (valgrind clean)
- Thread-safe under load testing

---

## Conclusion

The MudopDB storage engine migration is progressing well:
- **3 of 8 phases complete** (Phases 0, 1, 2)
- **124/124 tests passing** (100%)
- **Solid foundation** for buffer management and upper layers

The layered architecture provides clear separation of concerns, and the bottom-up implementation strategy ensures each layer is thoroughly tested before moving up.

**Next Milestone:** Phase 3 (Buffer Management) - ETA 3-4 days

---

**Document Version:** 1.0
**Last Updated:** October 12, 2025
**Author:** Storage Engine Migration Team

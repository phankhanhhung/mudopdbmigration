# MudopDB

A simple relational database engine written in C++17, based on the [SimpleDB](http://www.cs.bc.edu/~sciore/simpledb/) architecture from Edward Sciore's *Database Design and Implementation*.

## Architecture

MudopDB implements a layered database architecture with 8 core layers:

```
                    +-----------+
                    |    App    |   SQL REPL / CLI
                    +-----+-----+
                          |
                    +-----+-----+
                    |    API    |   Connection, Statement, ResultSet, Driver
                    +-----+-----+
                          |
           +--------------+--------------+
           |              |              |
     +-----+-----+  +----+----+  +------+------+
     |   Planner |  |  Parse  |  | Materialize |
     +-----+-----+  +---------+  +------+------+
           |                             |
     +-----+-----+               +------+------+
     |  Optimize |               |  Sort/Join  |
     +-----------+               +-------------+
           |
     +-----+------+
     |    Query    |   Scan, Select, Product, Project
     +-----+------+
           |
     +-----+------+
     |   Record   |   Schema, Layout, TableScan, RecordPage
     +-----+------+
           |
     +-----+------+
     |   Metadata  |   TableMgr, ViewMgr, StatMgr
     +-----+------+
           |
     +-----+------+
     | Transaction |   Concurrency (S/X locks) + Recovery (WAL)
     +-----+------+
           |
  +--------+--------+
  |        |        |
+---+  +-----+  +------+
|File|  | Log |  |Buffer|
+---+  +-----+  +------+
```

| Layer | Description |
|-------|-------------|
| **File** | Block-level disk I/O with `FileMgr`, `Page`, `BlockId` |
| **Log** | Write-ahead logging with `LogMgr` and `LogIterator` |
| **Buffer** | Buffer pool management with LRU eviction and pin/unpin |
| **Transaction** | ACID transactions with shared/exclusive locking and undo-based recovery |
| **Record** | Fixed-length record storage with `Schema`, `Layout`, `RecordPage` |
| **Metadata** | System catalog for tables, views, and statistics |
| **Query** | Relational algebra operators: select, project, product scans |
| **Parse** | SQL lexer and recursive-descent parser |
| **Plan** | Query planning with basic and heuristic optimizers |
| **Materialize** | Sort, GroupBy, MergeJoin, and temporary tables |
| **API** | JDBC-style embedded and network driver interface |

## Building

**Requirements:** CMake 3.16+, C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+)

```bash
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON
cmake --build .
```

## Usage

```bash
# Start the interactive SQL shell
./mudopdb studentdb

# Or pass the database directory as argument
./mudopdb /path/to/mydb
```

```sql
SQL> create table student (sid int, sname varchar(20), gradyear int)
SQL> insert into student (sid, sname, gradyear) values (1, 'Alice', 2025)
SQL> insert into student (sid, sname, gradyear) values (2, 'Bob', 2026)
SQL> select sname, gradyear from student where gradyear = 2025
```

## Running Tests

```bash
cd build
cmake .. -DBUILD_TESTS=ON
cmake --build .
ctest --output-on-failure
```

## Project Structure

```
.
├── CMakeLists.txt
├── include/                # Public headers
│   ├── api/                # Connection, Statement, ResultSet, Driver
│   ├── buffer/             # Buffer, BufferMgr
│   ├── file/               # BlockId, Page, FileMgr
│   ├── log/                # LogMgr, LogIterator
│   ├── materialize/        # Sort, GroupBy, MergeJoin, TempTable
│   ├── metadata/           # TableMgr, ViewMgr, StatMgr, MetadataMgr
│   ├── opt/                # HeuristicQueryPlanner, TablePlanner
│   ├── parse/              # Lexer, Parser, SQL data structures
│   ├── plan/               # Plan, Planner, BasicQueryPlanner
│   ├── query/              # Scan, Constant, Predicate, Select/Product/Project
│   ├── record/             # Schema, Layout, RecordPage, TableScan
│   ├── server/             # SimpleDB (top-level engine)
│   └── tx/                 # Transaction, ConcurrencyMgr, RecoveryMgr
├── src/                    # Implementation files
└── tests/                  # Google Test unit tests
```

## References

- Sciore, E. *Database Design and Implementation* (2nd Edition). Springer, 2020.

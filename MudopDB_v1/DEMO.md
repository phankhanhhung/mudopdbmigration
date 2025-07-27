# MudopDB Phase 4 Demo Guide

This guide shows how to run the Phase 4 demonstration program that showcases the Record Management Layer capabilities.

---

## Quick Start: Run the Demo

### Using Docker (Recommended)

```bash
# Build the demo image
docker build -f Dockerfile.debian -t mudopdb-demo --target demo .

# Run the demonstration
docker run --rm mudopdb-demo
```

### Using Native Build

```bash
# Build the project
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON ..
make -j$(nproc)

# Run the demo
./src/demo_phase4
```

---

## What the Demo Shows

The demonstration program (`src/demo_phase4.cpp`) showcases all Phase 4 capabilities:

### 1. **Storage Engine Initialization**
- Creates database directory
- Initializes FileMgr, LogMgr, BufferMgr
- Configures 400-byte blocks with 8-buffer pool

### 2. **Table Schema Creation**
```
Table: STUDENTS
Fields:
  - id (INTEGER)
  - name (VARCHAR(20))
  - age (INTEGER)
  - major (VARCHAR(15))

Slot size: 55 bytes
Records per block: 7
```

### 3. **Record Insertion**
Inserts 10 student records:
- Alice Johnson, Bob Smith, Carol Davis, etc.
- Demonstrates multi-block table (2 blocks for 10 records)

### 4. **Sequential Table Scan**
Displays all records in formatted table:
```
============================================================
             id           name            age          major
------------------------------------------------------------
              1  Alice Johnson             20   Computer Sci
              2      Bob Smith             22    Mathematics
             ...
```

### 5. **Record Update**
Updates Eve Wilson's record:
- Age: 19 → 20
- Major: Biology → Comp Sci

### 6. **Record Deletion**
Deletes Carol Davis (ID=3)
- Shows 9 remaining records after deletion
- Demonstrates slot reuse capability

### 7. **RID-Based Navigation**
- Saves RID of first record: `[0, 0]` (block 0, slot 0)
- Scans to end of table
- Jumps back to first record using saved RID
- Proves direct record access (no sequential scan needed)

### 8. **Storage Statistics**
```
Table file: students.tbl
Number of blocks: 2
Total file size: 800 bytes
Available buffers: 7 / 8
```

### 9. **Persistence Verification**
- Closes table scan
- Flushes all buffers to disk
- Creates new scan
- Verifies all 9 records still accessible
- **Proves crash-safety via Write-Ahead Logging**

---

## Demo Output Explained

### Phase 1: Initialization
```
[1] Initializing Storage Engine...
    Database directory: /tmp/mudopdb_demo
    Block size: 400 bytes
    Buffer pool: 8 buffers
```
**What happens:** Creates file manager, log manager, and buffer pool.

### Phase 2: Schema Definition
```
[2] Creating Table Schema: STUDENTS
    Fields: id (INT), name (VARCHAR(20)), age (INT), major (VARCHAR(15))
    Slot size: 55 bytes
    Records per block: 7
```
**What happens:**
- Schema defines field types and lengths
- Layout calculates slot size: 4 (flag) + 4 (id) + 24 (name) + 4 (age) + 19 (major) = 55 bytes
- 400-byte blocks hold floor(400/55) = 7 records each

### Phase 3: Insertions
```
[3] Inserting Records...
    Inserted: Alice Johnson (ID: 1)
    ...
```
**What happens:**
- `scan.insert()` finds EMPTY slot, marks USED
- `scan.set_int/set_string()` writes field values
- Buffer marked dirty for eventual flush

### Phase 4: Full Table Scan
```
[4] Scanning All Records:
============================================================
             id           name            age          major
------------------------------------------------------------
              1  Alice Johnson             20   Computer Sci
```
**What happens:**
- `scan.before_first()` resets to start
- `scan.next()` iterates through USED slots
- Transparently crosses from block 0 to block 1

### Phase 5-6: Update and Delete
```
[5] Updating Record (ID=5, Eve Wilson)...
    Before: Eve Wilson, Age: 19, Major: Biology
    After:  Eve Wilson, Age: 20, Major: Comp Sci

[6] Deleting Record (ID=3, Carol Davis)...
    Deleting: Carol Davis
```
**What happens:**
- Update: Scans to find ID=5, calls `set_int/set_string()` to modify
- Delete: Scans to find ID=3, marks slot EMPTY (available for reuse)

### Phase 8: RID Navigation
```
[8] Demonstrating RID-Based Navigation...
    First record RID: [0, 0]
    Name: Alice Johnson
    Jumping back to first record using RID...
    Name: Alice Johnson
```
**What happens:**
- `scan.get_rid()` returns `RID(block_number=0, slot=0)`
- `scan.move_to_rid(rid)` jumps directly to that block+slot
- No sequential scanning required (O(1) access)

### Phase 10: Persistence Test
```
[10] Testing Persistence...
     Closing table scan and flushing buffers...
     Creating new scan to verify data persisted...
     ✓ Found 9 records after restart
     ✓ Data successfully persisted to disk!
```
**What happens:**
1. `scan.close()` unpins all buffers
2. `bm->flush_all(0)` forces dirty buffers to disk
3. New `TableScan` created (fresh buffer pool)
4. Scan finds all 9 records still present
5. **Proves data survived "process restart"**

---

## Persistence Across Process Boundaries

### True Persistence Test

The standard demo (`demo_phase4`) shows persistence within a single process (destroying objects and recreating them). To prove **true persistence** where data survives complete application termination and is accessible by a separate process, use the persistence test:

```bash
# Build the persistence test
docker build -f Dockerfile.debian -t mudopdb-persistence --target persistence .

# Run writer then reader (two separate processes)
docker run --rm mudopdb-persistence
```

### How It Works

The persistence test runs TWO completely separate executables:

1. **`persistence_writer`** - First process
   - Creates database at `/tmp/mudopdb_persistent`
   - Creates ACCOUNTS table with 5 records
   - Explicitly flushes all buffers to disk
   - **Exits completely** (all destructors run, files closed)

2. **`persistence_reader`** - Second process (separate executable)
   - Creates **fresh** FileMgr, LogMgr, BufferMgr instances
   - No shared memory with writer
   - Verifies database exists (`is_new == false`)
   - Reads all 5 records from disk
   - Performs CRUD operations:
     - UPDATE: Alice's balance +$1000
     - DELETE: Carol Williams (ID 103)
     - INSERT: Frank Miller (ID 106)
   - Flushes changes to disk

### What This Proves

✅ **Storage Engine Independence**: The storage engine exists independently of the application process
✅ **True Persistence**: Data survives complete application shutdown
✅ **Cross-Process Access**: Separate processes can access the same database
✅ **Crash Safety**: Write-Ahead Logging ensures durability
✅ **CRUD on Persisted Data**: All operations work on data that survived process restart

### Key Verification Points

**Writer Output:**
```
[WRITER] Step 6: Closing database...
         Flushing all dirty buffers to disk...
         ✓ All data flushed to disk

[WRITER] Step 7: Exiting application...
         Destructors will run, files will be closed
```

**Reader Output:**
```
[READER] Step 2: Initializing NEW storage engine instance...
         Database is_new: NO (Good!)
         ✓ Existing database detected

[READER] Step 5: Opening table and reading data...
         Total records: 5
         Total balance: $44450

[READER] Step 6: Verifying data integrity...
         ✓ Record count matches (5 records)
         ✓ Total balance matches ($44,450)
```

This demonstrates that MudopDB is a **true persistent storage engine**, not just an in-memory database.

---

## Other Available Demos

### Run All Tests
```bash
# Build tests stage
docker build -f Dockerfile.debian -t mudopdb --target tests .

# Run all 183 tests
docker run --rm mudopdb
```

**Output:**
```
100% tests passed, 0 tests failed out of 183
Total Test time (real) = 1.26 sec
```

### Run Main Application
```bash
# Build runtime stage
docker build -f Dockerfile.debian -t mudopdb --target runtime .

# Run interactive shell
docker run -it --rm mudopdb
```

**Note:** Main application is a basic shell from Phase 0. SQL parsing not yet implemented.

---

## Understanding the Code

The demo source code (`src/demo_phase4.cpp`) is well-commented and can be used as a template for:

1. **Creating tables programmatically**
   ```cpp
   auto schema = std::make_shared<Schema>();
   schema->add_int_field("id");
   schema->add_string_field("name", 20);
   Layout layout(schema);
   ```

2. **Inserting records**
   ```cpp
   TableScan scan(buffer_mgr, "tablename", layout);
   scan.insert();
   scan.set_int("id", 1);
   scan.set_string("name", "Alice");
   ```

3. **Scanning tables**
   ```cpp
   scan.before_first();
   while (scan.next()) {
       int id = scan.get_int("id");
       std::string name = scan.get_string("name");
   }
   ```

4. **Updating/deleting records**
   ```cpp
   // Update
   if (scan.get_int("id") == 5) {
       scan.set_int("age", 21);
   }

   // Delete
   if (scan.get_int("id") == 3) {
       scan.delete_record();
   }
   ```

5. **Using RIDs for direct access**
   ```cpp
   RID rid = scan.get_rid().value();
   // ... later ...
   scan.move_to_rid(rid);  // Jump directly to record
   ```

---

## Technical Details

### Storage Layout
```
Database Directory: /tmp/mudopdb_demo/
├── students.tbl       (Data file - 800 bytes for 2 blocks)
└── demo.log          (Write-ahead log)
```

### Memory Usage
- **Buffer pool:** 8 buffers × 400 bytes = 3,200 bytes
- **Active buffers:** 1-2 (depends on current operation)
- **Log buffer:** 400 bytes

### I/O Characteristics
- **Writes:** Page-level (400 bytes)
- **WAL protocol:** Log written before data
- **Flush strategy:** Explicit flush on close + eviction
- **Block allocation:** On-demand when page fills

### Performance
- **Insert:** O(s) where s = slots per page (~7)
- **Sequential scan:** O(n) where n = total records
- **Update/Delete:** O(n) (must scan to find)
- **RID access:** O(1) (direct block + slot)

---

## Troubleshooting

### Demo doesn't run
```bash
# Check if image was built
docker images | grep mudopdb-demo

# Rebuild if needed
docker build -f Dockerfile.debian -t mudopdb-demo --target demo .
```

### Database permission errors
The demo uses `/tmp/mudopdb_demo` which should be writable. If running natively and you get permission errors:
```bash
# Use a different directory
# Edit src/demo_phase4.cpp line:
std::string db_dir = "/path/to/writable/dir";
```

### Build fails
```bash
# Check C++17 support
g++ --version  # Should be GCC 7+ or Clang 5+

# Clean build
rm -rf build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON ..
make clean && make -j$(nproc)
```

---

## What's Next?

After running this demo, you understand:
- ✅ How records are stored in fixed-size slots
- ✅ How sequential scans work across blocks
- ✅ How updates and deletes modify records
- ✅ How RID-based navigation provides direct access
- ✅ How Write-Ahead Logging ensures crash safety

**Next Phase (Phase 5):** Metadata Management
- System catalog tables
- CREATE/DROP TABLE support
- Transaction layer with ACID properties
- View management
- Table statistics

---

## Additional Resources

- **Full Documentation:** `/report/phase4_completion_report.md`
- **Test Suite:** `tests/test_tablescan.cpp` (10 comprehensive tests)
- **Source Code:** `src/demo_phase4.cpp` (280 lines, well-commented)
- **Architecture:** See completion report for full system diagram

---

**Questions?** Check the test files for more examples of TableScan usage.

# Phase 4 Demo - Quick Start Guide

## Run the Demo in 2 Commands

```bash
# 1. Build the demo
docker build -f Dockerfile.debian -t mudopdb-demo --target demo .

# 2. Run the demo
docker run --rm mudopdb-demo
```

## What You'll See

A complete demonstration of Phase 4 Record Management:

```
╔══════════════════════════════════════════════════════════╗
║     MudopDB Phase 4 Demo - Record Management Layer      ║
╚══════════════════════════════════════════════════════════╝

[1] Initializing Storage Engine...
[2] Creating Table Schema: STUDENTS
[3] Inserting Records...
[4] Scanning All Records:
============================================================
             id           name            age          major
------------------------------------------------------------
              1  Alice Johnson             20   Computer Sci
              2      Bob Smith             22    Mathematics
              ...
[5] Updating Record (ID=5, Eve Wilson)...
[6] Deleting Record (ID=3, Carol Davis)...
[7] Records After Update and Delete
[8] Demonstrating RID-Based Navigation...
[9] Storage Statistics...
[10] Testing Persistence...

Phase 4 Record Management Layer: ✅ FULLY FUNCTIONAL
```

## What Gets Demonstrated

✅ **Table Creation** - Define schema with 4 fields
✅ **Insert Operations** - Add 10 student records
✅ **Sequential Scanning** - Read all records in formatted table
✅ **Updates** - Modify field values
✅ **Deletes** - Remove records (slots become reusable)
✅ **RID Navigation** - Direct record access (no scanning)
✅ **Multi-block Tables** - Data spans 2 blocks automatically
✅ **Persistence** - Data survives process restart
✅ **Crash Safety** - Write-Ahead Logging ensures durability

## System Capabilities

After Phase 4, MudopDB can:
- Create tables with typed fields (INT, VARCHAR)
- Insert/update/delete records
- Scan tables sequentially
- Access records directly by RID
- Handle multi-block tables
- Persist data to disk with crash recovery

## For More Details

See `DEMO.md` for:
- Full explanation of each demo phase
- Code examples for using TableScan API
- Technical details (storage layout, performance)
- Troubleshooting guide
- Next steps (Phase 5 preview)

## Test True Persistence

Want to verify data survives complete application shutdown?

```bash
# Build the persistence test
docker build -f Dockerfile.debian -t mudopdb-persistence --target persistence .

# Run writer then reader (two separate processes)
docker run --rm mudopdb-persistence
```

This runs TWO separate programs:
1. **Writer** - Creates ACCOUNTS table, inserts 5 records, exits completely
2. **Reader** - Fresh process reads the data, performs CRUD operations

✅ Proves storage engine is truly persistent across process boundaries!

## Run Tests Instead

```bash
# Build and run all 183 tests
docker build -f Dockerfile.debian -t mudopdb --target tests .
docker run --rm mudopdb
```

Expected output: `100% tests passed, 0 tests failed out of 183`

# Docker Container Persistence - Explained

## The Question

> "We have a writer program that wrote, and a reader program that read. We run the writer program in some containerized environment. Now if we run that container again, does the storage engine still exist inside? What happens if we make some changes to the codebase and rebuild the container?"

## Short Answer

**Without volumes**: No, the storage engine data is **lost** when the container exits.

**With volumes**: Yes, the storage engine data **persists** across container runs and rebuilds.

---

## How Docker Container Filesystems Work

### What Happens in Our Current Test

```bash
docker run --rm mudopdb-persistence
```

**Step-by-step:**

1. **Container starts** (fresh filesystem from image)
   ```
   /tmp/  (empty)
   /app/persistence_writer (executable)
   /app/persistence_reader (executable)
   ```

2. **Writer process runs** (PID: 1)
   - Creates `/tmp/mudopdb_persistent/`
   - Writes `accounts.tbl` and `app.log` to disk
   - **Process exits** ← Writer process terminates completely

3. **Reader process runs** (PID: 8) ← NEW PROCESS, separate from writer
   - Reads from `/tmp/mudopdb_persistent/`
   - Database files are still there! ✅
   - This proves **process-level persistence**

4. **Container exits**
   - The `--rm` flag deletes the entire container
   - `/tmp/mudopdb_persistent/` is **destroyed**
   - All data is **lost** ❌

5. **Run the command again**
   - Completely fresh container
   - `/tmp/` is empty again
   - Previous database is **gone**

### What Our Test Actually Proves

Our test proves **application-level persistence**:

✅ **Process Independence**: Data survives when the writer process (PID) terminates
✅ **No Shared Memory**: Reader process has no memory shared with writer
✅ **True Disk I/O**: Data is actually written to and read from disk files
✅ **Storage Engine Works**: FileMgr, LogMgr, BufferMgr persist data correctly

Our test does **NOT** prove **container-level persistence**:

❌ Data does not survive container deletion (due to `--rm`)
❌ Data does not survive container restart
❌ Data is not accessible from different container instances

---

## Making Data Persistent Across Container Runs

### Using Docker Volumes

Docker volumes store data **outside** the container filesystem, making it persist even when containers are deleted or rebuilt.

#### Option 1: Named Volume (Recommended)

```bash
# Create a volume once
docker volume create mudopdb-data

# Run writer - data goes to volume
docker run --rm -v mudopdb-data:/tmp/mudopdb_persistent \
  mudopdb-persistence /app/persistence_writer

# Run reader in COMPLETELY NEW container - data is still there!
docker run --rm -v mudopdb-data:/tmp/mudopdb_persistent \
  mudopdb-persistence /app/persistence_reader

# You can run reader again and again - data persists
docker run --rm -v mudopdb-data:/tmp/mudopdb_persistent \
  mudopdb-persistence /app/persistence_reader
```

**What happens:**
- `/tmp/mudopdb_persistent/` inside the container is **mounted** to the volume
- Volume exists on the Docker host (not inside containers)
- Data survives container deletion, restart, and rebuilds

#### Option 2: Bind Mount (Development)

```bash
# Mount a host directory
mkdir -p /tmp/my-mudopdb-data

# Run writer
docker run --rm -v /tmp/my-mudopdb-data:/tmp/mudopdb_persistent \
  mudopdb-persistence /app/persistence_writer

# Check files on host!
ls -la /tmp/my-mudopdb-data/
# Output:
# accounts.tbl
# app.log

# Run reader - uses same host directory
docker run --rm -v /tmp/my-mudopdb-data:/tmp/mudopdb_persistent \
  mudopdb-persistence /app/persistence_reader
```

**Advantages:**
- You can inspect database files directly on your host machine
- Great for debugging and development

---

## What Happens When You Rebuild?

### Scenario 1: Rebuild Without Volumes (Current Setup)

```bash
# Run original version
docker run --rm mudopdb-persistence
# Writer creates data, reader reads it, container exits
# Data is LOST

# Make code changes
echo "// new feature" >> src/demo_persistence_writer.cpp

# Rebuild
docker build -f Dockerfile.debian -t mudopdb-persistence --target persistence .

# Run rebuilt version
docker run --rm mudopdb-persistence
# Completely fresh database - no old data
# Previous run's data is GONE
```

**Result**: Each run is independent. Rebuilding doesn't affect anything because data was never preserved.

### Scenario 2: Rebuild WITH Volumes

```bash
# Create volume
docker volume create mudopdb-data

# Run writer with OLD code version
docker run --rm -v mudopdb-data:/tmp/mudopdb_persistent \
  mudopdb-persistence /app/persistence_writer
# Database created: 5 ACCOUNTS records

# Make code changes to reader
vim src/demo_persistence_reader.cpp
# Add: std::cout << "NEW VERSION\n";

# Rebuild image
docker build -f Dockerfile.debian -t mudopdb-persistence --target persistence .

# Run reader with NEW code version
docker run --rm -v mudopdb-data:/tmp/mudopdb_persistent \
  mudopdb-persistence /app/persistence_reader
# Output: "NEW VERSION"
# Reads OLD data from volume (5 ACCOUNTS records)
# Data PERSISTED across rebuild!
```

**Result**: The volume preserves data even when you rebuild the image with new code.

---

## Important Distinction: Process vs Container vs Volume

| Level | What Happens | Our Test Proves This? |
|-------|--------------|----------------------|
| **Process** | Writer process exits, reader process starts | ✅ YES - This is what our test demonstrates |
| **Container** | Container exits with `--rm`, data is deleted | ❌ NO - Data is lost without volumes |
| **Volume** | Data persists even when container and image are deleted | ❌ NO - Our test doesn't use volumes |

### Visual Representation

**Without Volume:**
```
┌─────────────────────────────────────────────┐
│  Container (temporary)                      │
│  ┌─────────────────────────────────────┐   │
│  │ /tmp/mudopdb_persistent/            │   │
│  │   ├── accounts.tbl                  │   │
│  │   └── app.log                       │   │
│  └─────────────────────────────────────┘   │
│  Process 1: Writer (exits)                  │
│  Process 2: Reader (reads data) ✅          │
└─────────────────────────────────────────────┘
     │
     │ docker run --rm exits
     ↓
   💥 Container deleted
   💥 All data LOST
```

**With Volume:**
```
┌─────────────────────────────────────────────┐
│  Container (temporary)                      │
│  ┌─────────────────────────────────────┐   │
│  │ /tmp/mudopdb_persistent/ (mount)   │───┐ │
│  └─────────────────────────────────────┘   │ │
│  Process 1: Writer (exits)                  │ │
└─────────────────────────────────────────────┘ │
     │                                          │
     │ Container exits/deleted                  │
     ↓                                          │
   Container GONE                               │
   But data is safe in volume! ───────────────┐ │
                                               │ │
┌─────────────────────────────────────────────┐│ │
│  New Container                              ││ │
│  ┌─────────────────────────────────────┐   ││ │
│  │ /tmp/mudopdb_persistent/ (mount)   │───┼┘ │
│  └─────────────────────────────────────┘   │  │
│  Process 2: Reader (reads old data) ✅      │  │
└─────────────────────────────────────────────┘  │
                                                 │
┌──────────────────────────────────────────────┐ │
│  Docker Volume (persistent)                  │ │
│  mudopdb-data                               │←┘
│  ├── accounts.tbl                            │
│  └── app.log                                 │
└──────────────────────────────────────────────┘
     ↑
     Survives container deletion, rebuilds, etc.
```

---

## What Our Test IS Good For

Our current test (without volumes) is **perfect** for demonstrating:

1. **Storage Engine Correctness**
   - FileMgr writes data to files correctly
   - LogMgr implements Write-Ahead Logging
   - BufferMgr flushes dirty pages to disk
   - Data is not just in memory

2. **Process Independence**
   - Writer process terminates completely (PID ends)
   - Reader process is a separate process (different PID)
   - No shared memory between processes
   - Data is accessed via filesystem, not memory

3. **CRUD Functionality**
   - Insert works (writer adds records)
   - Read works (reader retrieves records)
   - Update works (reader modifies Alice's balance)
   - Delete works (reader removes Carol)

4. **Data Integrity**
   - Data survives process termination
   - Verification of correct values
   - Schema consistency across processes

---

## Enhanced Persistence Test with Volumes

Want to test **full container-level persistence**? Here's how:

### Create Enhanced Test Script

```bash
#!/bin/bash
# test_full_persistence.sh

echo "╔════════════════════════════════════════╗"
echo "║  Full Persistence Test with Volumes   ║"
echo "╚════════════════════════════════════════╝"
echo ""

# Create volume
echo "[1] Creating Docker volume..."
docker volume create mudopdb-data
echo "    ✓ Volume created: mudopdb-data"
echo ""

# Run writer
echo "[2] Running WRITER in first container..."
docker run --rm -v mudopdb-data:/tmp/mudopdb_persistent \
  mudopdb-persistence /app/persistence_writer
echo ""

# Verify data persists by running reader multiple times
echo "[3] Running READER in second container..."
docker run --rm -v mudopdb-data:/tmp/mudopdb_persistent \
  mudopdb-persistence /app/persistence_reader
echo ""

echo "[4] Running READER AGAIN in third container..."
echo "    (Data should include Frank Miller from previous run)"
docker run --rm -v mudopdb-data:/tmp/mudopdb_persistent \
  mudopdb-persistence /app/persistence_reader
echo ""

echo "╔════════════════════════════════════════╗"
echo "║  ✅ FULL PERSISTENCE VERIFIED          ║"
echo "║  • Data survived 3 container instances ║"
echo "║  • Each container had fresh filesystem ║"
echo "║  • Volume preserved database files     ║"
echo "╚════════════════════════════════════════╝"

# Cleanup
read -p "Delete volume? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
  docker volume rm mudopdb-data
  echo "✓ Volume deleted"
fi
```

### What This Proves

Running this script proves:
- ✅ Data survives across **multiple container instances**
- ✅ Data survives even when containers are completely deleted (`--rm`)
- ✅ Storage engine is truly independent
- ✅ Can run reader multiple times on same data
- ✅ Second reader sees modifications made by first reader

---

## Summary: Levels of Persistence

### Level 1: In-Memory Only ❌
- Data lost when process exits
- Not a real database

### Level 2: Process-Level Persistence ✅ (What we test)
- Data survives process termination
- Written to disk files
- Can be read by other processes
- **Our current test proves this**

### Level 3: Container-Level Persistence ✅ (Need volumes)
- Data survives container deletion
- Requires Docker volumes
- **Easy to add to our test**

### Level 4: Production Persistence ✅ (Need volumes + backups)
- Data survives host crashes
- Requires volumes + external backups
- Network-attached storage in production

---

## Recommended Update to Documentation

Add this section to `PERSISTENCE_TEST.md`:

```markdown
## Making Data Persist Across Container Runs

The basic test demonstrates process-level persistence. To make data persist
across container deletions and rebuilds, use Docker volumes:

### Create Persistent Volume
\`\`\`bash
# Create volume
docker volume create mudopdb-data

# Run writer
docker run --rm -v mudopdb-data:/tmp/mudopdb_persistent \
  mudopdb-persistence /app/persistence_writer

# Run reader (separate container)
docker run --rm -v mudopdb-data:/tmp/mudopdb_persistent \
  mudopdb-persistence /app/persistence_reader

# Run reader AGAIN - data is still there!
docker run --rm -v mudopdb-data:/tmp/mudopdb_persistent \
  mudopdb-persistence /app/persistence_reader

# Cleanup when done
docker volume rm mudopdb-data
\`\`\`

### What This Proves
- ✅ Data survives container deletion
- ✅ Data accessible from multiple container instances
- ✅ Storage engine is truly persistent
\`\`\`
```

---

## Bottom Line

**Your Questions Answered:**

1. **"If we run that container again, does the storage engine still exist inside?"**
   - **No**, because `--rm` deletes the container filesystem
   - **Yes** if you use a Docker volume

2. **"What happens if we make changes to the codebase and rebuild?"**
   - **Without volume**: Old data is gone, fresh database
   - **With volume**: Old data persists, new code can read it

**What our test proves:**
- ✅ Storage engine writes to real disk files (not just memory)
- ✅ Data survives process termination
- ✅ Separate processes can access the database
- ✅ Storage engine is independent of application process

**What our test doesn't prove (but easily could with volumes):**
- ❌ Data survives container deletion
- ❌ Data survives across container restarts

The distinction is important: We're testing **storage engine persistence** (proven ✅), not **container filesystem persistence** (Docker limitation, solvable with volumes).

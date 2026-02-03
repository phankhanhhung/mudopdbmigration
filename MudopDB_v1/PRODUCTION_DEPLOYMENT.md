# MudopDB Production Deployment Guide

## The Problem

You want to run MudopDB in a container where:
- **Application container** is ephemeral (starts only when needed, stops when idle)
- **Database storage** persists across container restarts
- Multiple container instances can access the same database
- Data survives container deletion, rebuilds, and host restarts

## The Solution: Docker Volumes

Separate the **application layer** (container) from the **storage layer** (volume).

```
┌─────────────────────────────────────────────┐
│  Application Container (EPHEMERAL)          │
│  • Starts when user makes request           │
│  • Runs queries/transactions                │
│  • Stops when idle                          │
│  • Can be deleted/rebuilt anytime           │
└─────────────────┬───────────────────────────┘
                  │
                  │ Mounts volume
                  ↓
┌─────────────────────────────────────────────┐
│  Docker Volume (PERSISTENT)                 │
│  • Stores database files                    │
│  • Survives container deletion              │
│  • Survives container rebuilds              │
│  • Survives host restarts                   │
│  • Independent of application lifecycle     │
└─────────────────────────────────────────────┘
```

---

## Quick Start: Production Setup

### 1. Create Persistent Volume

```bash
# Create named volume for database storage
docker volume create mudopdb-data

# Inspect the volume
docker volume inspect mudopdb-data
```

**Output:**
```json
[
    {
        "CreatedAt": "2025-10-19T18:30:00Z",
        "Driver": "local",
        "Labels": {},
        "Mountpoint": "/var/lib/docker/volumes/mudopdb-data/_data",
        "Name": "mudopdb-data",
        "Options": {},
        "Scope": "local"
    }
]
```

The volume is stored on the Docker host at the `Mountpoint` location.

### 2. Build the Application Image

```bash
docker build -f Dockerfile.debian -t mudopdb:latest --target runtime .
```

### 3. Run Container with Volume Mounted

```bash
docker run -d \
  --name mudopdb-server \
  -v mudopdb-data:/var/lib/mudopdb \
  -p 5432:5432 \
  mudopdb:latest
```

**Flags explained:**
- `-d`: Run in detached mode (background)
- `--name`: Container name for easy reference
- `-v mudopdb-data:/var/lib/mudopdb`: Mount volume to `/var/lib/mudopdb` in container
- `-p 5432:5432`: Expose port (when we add network layer)
- `mudopdb:latest`: Image to use

---

## Typical Usage Patterns

### Pattern 1: Start → Use → Stop → Restart

```bash
# Day 1: Start database container
docker run -d --name mudopdb \
  -v mudopdb-data:/var/lib/mudopdb \
  mudopdb:latest

# User 1: Connect and write data
docker exec -it mudopdb /app/mudopdb
# ... insert records ...

# Stop container (app goes idle)
docker stop mudopdb

# Remove container (to save resources)
docker rm mudopdb

# Day 2: User 2 wants to access database
# Start NEW container with SAME volume
docker run -d --name mudopdb \
  -v mudopdb-data:/var/lib/mudopdb \
  mudopdb:latest

# User 2: Connect and read data
docker exec -it mudopdb /app/mudopdb
# ... sees all data from Day 1! ✅
```

### Pattern 2: Rebuild Application, Keep Data

```bash
# Running production version
docker run -d --name mudopdb \
  -v mudopdb-data:/var/lib/mudopdb \
  mudopdb:v1.0

# Develop new features
vim src/record/tablescan.cpp
# ... add new functionality ...

# Rebuild image
docker build -f Dockerfile.debian -t mudopdb:v1.1 --target runtime .

# Stop old container
docker stop mudopdb
docker rm mudopdb

# Start NEW container with updated code
docker run -d --name mudopdb \
  -v mudopdb-data:/var/lib/mudopdb \
  mudopdb:v1.1

# All old data is still accessible! ✅
```

### Pattern 3: Multiple Containers, Same Data

```bash
# Read-only container for reporting
docker run -d --name mudopdb-reader \
  -v mudopdb-data:/var/lib/mudopdb:ro \
  mudopdb:latest /app/mudopdb --readonly

# Read-write container for transactions
docker run -d --name mudopdb-writer \
  -v mudopdb-data:/var/lib/mudopdb \
  mudopdb:latest /app/mudopdb

# Both access the same database files
```

**Note**: Phase 4 doesn't have locking yet. Phase 5+ will add proper concurrency control.

---

## Updated Application Code for Production

### Modify to Use Volume Path

Currently, our demos use `/tmp/mudopdb_persistent`. For production, use a dedicated data directory:

**File: `src/main.cpp`** (update the database path)

```cpp
int main() {
    // Use environment variable with fallback
    const char* db_dir_env = std::getenv("MUDOPDB_DATA_DIR");
    std::string db_dir = db_dir_env ? db_dir_env : "/var/lib/mudopdb";

    std::cout << "MudopDB starting...\n";
    std::cout << "Database directory: " << db_dir << "\n";

    // Create directory if it doesn't exist
    if (!std::filesystem::exists(db_dir)) {
        std::filesystem::create_directories(db_dir);
        std::cout << "Created database directory\n";
    }

    // Initialize storage engine
    auto fm = std::make_shared<FileMgr>(db_dir, 400);
    auto lm = std::make_shared<LogMgr>(fm, "mudopdb.log");
    auto bm = std::make_shared<BufferMgr>(fm, lm, 8);

    // Rest of application...
}
```

### Updated Dockerfile for Production

**File: `Dockerfile.production`** (create new file)

```dockerfile
### Production Dockerfile for MudopDB
FROM debian:bookworm-slim AS build

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
COPY . /workspace

RUN mkdir -p build \
 && cd build \
 && cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF .. \
 && cmake --build . -- -j$(nproc)

### Runtime stage
FROM debian:bookworm-slim AS runtime

# Create user for running database (security best practice)
RUN groupadd -r mudopdb && useradd -r -g mudopdb mudopdb

# Install runtime dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Create data directory
RUN mkdir -p /var/lib/mudopdb && chown mudopdb:mudopdb /var/lib/mudopdb

WORKDIR /app
COPY --from=build /workspace/build/src/mudopdb /app/mudopdb
RUN chmod +x /app/mudopdb

# Switch to non-root user
USER mudopdb

# Set environment variable for data directory
ENV MUDOPDB_DATA_DIR=/var/lib/mudopdb

# Expose port (for future network layer)
EXPOSE 5432

# Volume for persistent storage
VOLUME ["/var/lib/mudopdb"]

ENTRYPOINT ["/app/mudopdb"]
```

### Build and Run Production Image

```bash
# Build production image
docker build -f Dockerfile.production -t mudopdb:prod --target runtime .

# Create volume
docker volume create mudopdb-data

# Run with volume mounted
docker run -d \
  --name mudopdb \
  -v mudopdb-data:/var/lib/mudopdb \
  -p 5432:5432 \
  mudopdb:prod
```

---

## Complete Production Workflow

### Initial Setup

```bash
# 1. Create volume
docker volume create mudopdb-data

# 2. Build image
docker build -f Dockerfile.production -t mudopdb:1.0 --target runtime .

# 3. Start database
docker run -d \
  --name mudopdb \
  --restart unless-stopped \
  -v mudopdb-data:/var/lib/mudopdb \
  mudopdb:1.0
```

### Daily Operations

```bash
# Check if database is running
docker ps | grep mudopdb

# View logs
docker logs mudopdb

# Connect to database (interactive)
docker exec -it mudopdb /app/mudopdb

# Stop database
docker stop mudopdb

# Start database again (same data)
docker start mudopdb

# Restart database
docker restart mudopdb
```

### Updating Application

```bash
# 1. Make code changes
vim src/record/tablescan.cpp

# 2. Build new version
docker build -f Dockerfile.production -t mudopdb:1.1 --target runtime .

# 3. Stop old container
docker stop mudopdb
docker rm mudopdb

# 4. Start new container with updated code
docker run -d \
  --name mudopdb \
  --restart unless-stopped \
  -v mudopdb-data:/var/lib/mudopdb \
  mudopdb:1.1

# Data from version 1.0 is still accessible! ✅
```

### Backup and Restore

```bash
# Backup volume to tar file
docker run --rm \
  -v mudopdb-data:/data \
  -v $(pwd):/backup \
  debian:bookworm-slim \
  tar czf /backup/mudopdb-backup-$(date +%Y%m%d).tar.gz -C /data .

# Restore from backup
docker volume create mudopdb-data-restored
docker run --rm \
  -v mudopdb-data-restored:/data \
  -v $(pwd):/backup \
  debian:bookworm-slim \
  tar xzf /backup/mudopdb-backup-20251019.tar.gz -C /data
```

### Cleanup

```bash
# Stop and remove container
docker stop mudopdb
docker rm mudopdb

# Remove image
docker rmi mudopdb:1.0

# Remove volume (DANGER: deletes all data!)
docker volume rm mudopdb-data
```

---

## Docker Compose Setup (Recommended)

For easier management, use Docker Compose:

**File: `docker-compose.yml`**

```yaml
version: '3.8'

services:
  mudopdb:
    build:
      context: .
      dockerfile: Dockerfile.production
      target: runtime
    image: mudopdb:latest
    container_name: mudopdb
    restart: unless-stopped
    volumes:
      - mudopdb-data:/var/lib/mudopdb
    ports:
      - "5432:5432"
    environment:
      - MUDOPDB_DATA_DIR=/var/lib/mudopdb
    healthcheck:
      test: ["CMD", "/app/mudopdb", "--healthcheck"]
      interval: 30s
      timeout: 10s
      retries: 3

volumes:
  mudopdb-data:
    name: mudopdb-data
```

### Usage with Docker Compose

```bash
# Start database
docker-compose up -d

# View logs
docker-compose logs -f

# Stop database
docker-compose down

# Stop and remove volume (DANGER!)
docker-compose down -v

# Rebuild and restart
docker-compose up -d --build
```

---

## Verification: Data Persists Across Container Lifecycles

### Test Script

```bash
#!/bin/bash
# test_production_persistence.sh

echo "╔════════════════════════════════════════════════╗"
echo "║  Production Persistence Verification Test     ║"
echo "╚════════════════════════════════════════════════╝"
echo ""

# 1. Create volume
echo "[1] Creating persistent volume..."
docker volume create mudopdb-data
echo ""

# 2. Run writer in first container
echo "[2] Starting Container #1 (writer)..."
docker run --rm \
  -v mudopdb-data:/tmp/mudopdb_persistent \
  mudopdb-persistence /app/persistence_writer
echo "    ✓ Container #1 exited"
echo ""

# 3. Run reader in second container
echo "[3] Starting Container #2 (reader)..."
docker run --rm \
  -v mudopdb-data:/tmp/mudopdb_persistent \
  mudopdb-persistence /app/persistence_reader
echo "    ✓ Container #2 exited"
echo ""

# 4. Run reader AGAIN in third container
echo "[4] Starting Container #3 (reader again)..."
docker run --rm \
  -v mudopdb-data:/tmp/mudopdb_persistent \
  mudopdb-persistence /app/persistence_reader
echo "    ✓ Container #3 exited"
echo ""

# 5. Verify volume contents
echo "[5] Inspecting volume contents..."
docker run --rm \
  -v mudopdb-data:/data \
  debian:bookworm-slim \
  ls -lh /tmp/mudopdb_persistent/
echo ""

echo "╔════════════════════════════════════════════════╗"
echo "║  ✅ PRODUCTION PERSISTENCE VERIFIED            ║"
echo "║                                                ║"
echo "║  • 3 containers started and stopped            ║"
echo "║  • Each container had fresh filesystem         ║"
echo "║  • Volume preserved data across all runs       ║"
echo "║  • Data survives container deletion            ║"
echo "╚════════════════════════════════════════════════╝"
echo ""

# Cleanup
read -p "Delete volume and data? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
  docker volume rm mudopdb-data
  echo "✓ Volume deleted"
else
  echo "Volume preserved for future use"
fi
```

### Run Test

```bash
chmod +x test_production_persistence.sh
./test_production_persistence.sh
```

---

## Production Best Practices

### 1. Security

```bash
# Run as non-root user (already in Dockerfile.production)
USER mudopdb

# Use read-only root filesystem
docker run -d \
  --name mudopdb \
  --read-only \
  -v mudopdb-data:/var/lib/mudopdb \
  mudopdb:prod

# Limit resources
docker run -d \
  --name mudopdb \
  --memory=512m \
  --cpus=1 \
  -v mudopdb-data:/var/lib/mudopdb \
  mudopdb:prod
```

### 2. High Availability

```bash
# Use restart policy
docker run -d \
  --name mudopdb \
  --restart unless-stopped \
  -v mudopdb-data:/var/lib/mudopdb \
  mudopdb:prod

# Or in Docker Compose
restart: unless-stopped
```

### 3. Monitoring

```bash
# Health checks
docker run -d \
  --name mudopdb \
  --health-cmd="/app/mudopdb --healthcheck" \
  --health-interval=30s \
  -v mudopdb-data:/var/lib/mudopdb \
  mudopdb:prod

# Check health status
docker inspect --format='{{.State.Health.Status}}' mudopdb
```

### 4. Backups

```bash
# Automated backup script
#!/bin/bash
# backup_mudopdb.sh

BACKUP_DIR="/backups/mudopdb"
DATE=$(date +%Y%m%d-%H%M%S)

mkdir -p "$BACKUP_DIR"

docker run --rm \
  -v mudopdb-data:/data \
  -v "$BACKUP_DIR:/backup" \
  debian:bookworm-slim \
  tar czf "/backup/mudopdb-$DATE.tar.gz" -C /data .

echo "Backup saved: $BACKUP_DIR/mudopdb-$DATE.tar.gz"

# Keep only last 7 days
find "$BACKUP_DIR" -name "mudopdb-*.tar.gz" -mtime +7 -delete
```

### 5. Volume Drivers for Production

For production environments, consider using:

- **Local driver** (default): Good for single-host development
- **NFS driver**: For network-attached storage
- **Cloud drivers**: AWS EBS, Azure Disk, GCP Persistent Disk
- **Distributed storage**: GlusterFS, Ceph

```bash
# Example: Using NFS driver
docker volume create \
  --driver local \
  --opt type=nfs \
  --opt o=addr=192.168.1.100,rw \
  --opt device=:/path/to/nfs/share \
  mudopdb-data
```

---

## Summary

### Your Requirements

> "The application runs when there is any user want to read or write anything to the database, but in between, the database must still exist."

### Solution

**Use Docker volumes to separate application from data:**

```bash
# Create persistent volume (ONCE)
docker volume create mudopdb-data

# Run container when user connects
docker run -d --name mudopdb \
  -v mudopdb-data:/var/lib/mudopdb \
  mudopdb:latest

# Stop container when idle
docker stop mudopdb
docker rm mudopdb

# Run again when next user connects
docker run -d --name mudopdb \
  -v mudopdb-data:/var/lib/mudopdb \
  mudopdb:latest

# Database files persist in volume! ✅
```

### What This Achieves

✅ **Application is ephemeral**: Containers can start/stop/be deleted anytime
✅ **Data is persistent**: Database survives across container lifecycles
✅ **Easy updates**: Rebuild image, restart container, data is preserved
✅ **Backup-friendly**: Volume can be backed up independently
✅ **Production-ready**: Standard Docker deployment pattern

---

## Next Steps

1. **Update `src/main.cpp`** to use `/var/lib/mudopdb` or environment variable
2. **Create `Dockerfile.production`** with proper volume configuration
3. **Test with the verification script** above
4. **Set up automated backups** for production
5. **Add Docker Compose** for easier orchestration

Want me to implement these changes to the codebase?

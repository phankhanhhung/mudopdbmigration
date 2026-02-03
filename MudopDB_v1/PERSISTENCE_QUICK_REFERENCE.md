# MudopDB Persistence - Quick Reference

## Your Use Case
> "Application runs in container only when users connect, but database must persist between runs."

## Solution: Docker Volumes ✅

---

## One-Time Setup

```bash
# Create persistent volume
docker volume create mudopdb-data
```

---

## Daily Usage

```bash
# User connects → Start container
docker run -d --name mudopdb \
  -v mudopdb-data:/var/lib/mudopdb \
  mudopdb:latest

# User disconnects → Stop container
docker stop mudopdb
docker rm mudopdb

# Next user → Start fresh container (SAME volume)
docker run -d --name mudopdb \
  -v mudopdb-data:/var/lib/mudopdb \
  mudopdb:latest
# Previous data is still there! ✅
```

---

## Update Application (Keep Data)

```bash
# Rebuild image
docker build -t mudopdb:v2 .

# Stop old version
docker stop mudopdb && docker rm mudopdb

# Start new version (SAME volume)
docker run -d --name mudopdb \
  -v mudopdb-data:/var/lib/mudopdb \
  mudopdb:v2
# All data preserved! ✅
```

---

## Common Operations

### Backup Database
```bash
docker run --rm \
  -v mudopdb-data:/data \
  -v $(pwd):/backup \
  debian:bookworm-slim \
  tar czf /backup/backup.tar.gz -C /data .
```

### Restore Database
```bash
docker volume create mudopdb-data-restored
docker run --rm \
  -v mudopdb-data-restored:/data \
  -v $(pwd):/backup \
  debian:bookworm-slim \
  tar xzf /backup/backup.tar.gz -C /data
```

### Inspect Volume
```bash
docker volume inspect mudopdb-data
```

### List Files in Volume
```bash
docker run --rm -v mudopdb-data:/data \
  debian:bookworm-slim ls -lh /data
```

### Delete Volume (DANGER!)
```bash
docker volume rm mudopdb-data
```

---

## Test It Works

```bash
# Automated test
./test_production_persistence.sh

# Or build and run
docker build -f Dockerfile.debian -t mudopdb-persistence --target persistence .
docker run --rm mudopdb-persistence
```

---

## Docker Compose (Easier)

**File: `docker-compose.yml`**
```yaml
version: '3.8'
services:
  mudopdb:
    image: mudopdb:latest
    container_name: mudopdb
    restart: unless-stopped
    volumes:
      - mudopdb-data:/var/lib/mudopdb
    ports:
      - "5432:5432"

volumes:
  mudopdb-data:
```

**Usage:**
```bash
docker-compose up -d      # Start
docker-compose down       # Stop (keeps volume)
docker-compose down -v    # Stop and DELETE volume
```

---

## What You Get

| Feature | Result |
|---------|--------|
| **Application** | Ephemeral (delete anytime) ✅ |
| **Data** | Persistent (survives restarts) ✅ |
| **Updates** | Easy (rebuild, keep data) ✅ |
| **Backups** | Simple (tar the volume) ✅ |
| **Resource Usage** | Efficient (stop when idle) ✅ |

---

## Key Files

- `QUICK_START_VOLUMES.md` - Detailed guide for your use case
- `PRODUCTION_DEPLOYMENT.md` - Complete production setup
- `DOCKER_PERSISTENCE_EXPLAINED.md` - Technical deep dive
- `TEST_RESULTS_PERSISTENCE.md` - Test verification
- `test_production_persistence.sh` - Automated test script

---

## Architecture

```
┌─────────────────────────┐
│  Application Container  │ ← Ephemeral (start/stop/delete)
│  (mudopdb:latest)       │
└──────────┬──────────────┘
           │ mounts
           ↓
┌─────────────────────────┐
│  Docker Volume          │ ← Persistent (survives everything)
│  (mudopdb-data)         │
│  ├── accounts.tbl       │
│  └── app.log            │
└─────────────────────────┘
```

**Containers come and go, volume stays forever** 🎯

---

## Proof It Works

Run any of these:

```bash
# Automated
./test_production_persistence.sh

# Quick demo
docker run --rm mudopdb-persistence

# Manual verification
docker volume create test-vol
docker run --rm -v test-vol:/tmp/mudopdb_persistent \
  mudopdb-persistence /app/persistence_writer
docker run --rm -v test-vol:/tmp/mudopdb_persistent \
  mudopdb-persistence /app/persistence_reader
docker volume rm test-vol
```

All tests prove: **Data persists across container lifecycles** ✅

---

## Summary

**Before**: Container exits → data lost ❌

**After**: Container exits → data in volume → next container reads it ✅

**This is production-ready!** 🚀

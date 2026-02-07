# Review Kiến Trúc: MudopDB_v1 (C++) vs NMDB2 (Rust)

**Ngày:** 7 tháng 2, 2026
**Phạm vi:** So sánh kiến trúc toàn diện giữa hai phiên bản

---

## 1. Tổng Quan

| Tiêu chí | MudopDB_v1 (C++) | NMDB2 (Rust) |
|----------|------------------|--------------|
| Ngôn ngữ | C++17 | Rust 2021 edition |
| Build system | CMake 3.16+ | Cargo |
| Testing | GoogleTest | Rust built-in `#[test]` |
| Trạng thái | 4/7 phases (57%) | Hoàn thiện (~95%) |
| Tests | 183 (100% pass) | 169+ (100% pass) |
| Network | Không (embedded only) | gRPC + Tokio async |

---

## 2. Kiến Trúc Phân Tầng

Cả hai đều tuân theo kiến trúc **bottom-up layered** dựa trên SimpleDB:

```
┌──────────────────────────────────────────────────┐
│  Layer 7: SQL Interface (JDBC-style API)         │  C++: ✅  Rust: ✅
├──────────────────────────────────────────────────┤
│  Layer 6: Query Processing (Parse/Plan/Execute)  │  C++: ✗   Rust: ✅
├──────────────────────────────────────────────────┤
│  Layer 5: Metadata Management (System Catalog)   │  C++: ✗   Rust: ✅
├──────────────────────────────────────────────────┤
│  Layer 4: Record Management (Table/Record)       │  C++: ✅  Rust: ✅
├──────────────────────────────────────────────────┤
│  Layer 3: Buffer Management (Buffer Pool)        │  C++: ✅  Rust: ✅
├──────────────────────────────────────────────────┤
│  Layer 2: Logging (Write-Ahead Log)              │  C++: ✅  Rust: ✅
├──────────────────────────────────────────────────┤
│  Layer 1: File Layer (Disk I/O)                  │  C++: ✅  Rust: ✅
└──────────────────────────────────────────────────┘
```

**Nhận xét:** MudopDB_v1 thiếu 3 tầng quan trọng: Metadata Management, Query Processing, và Transaction layer. Đây là khoảng cách lớn nhất giữa hai phiên bản.

---

## 3. Quản Lý Bộ Nhớ & Ownership

### 3.1 C++ (MudopDB_v1)

Sử dụng smart pointers với mô hình ownership hỗn hợp:

```cpp
class Buffer {
    file::Page contents_;                    // Sở hữu trực tiếp (stack)
    std::optional<file::BlockId> blk_;       // Optional value
    std::shared_ptr<file::FileMgr> fm_;      // Shared ownership
    std::shared_ptr<log::LogMgr> lm_;        // Shared ownership
};
```

```cpp
class BufferMgr {
    std::vector<Buffer> bufferpool_;         // Direct ownership
    std::shared_ptr<file::FileMgr> fm_;     // Shared
};
```

**Ưu điểm:**
- RAII đảm bảo cleanup tự động
- Không sử dụng raw pointer
- `std::optional` cho nullable values

**Nhược điểm:**
- Mô hình ownership không đồng nhất (mixed shared_ptr / value semantics)
- Không có cơ chế compiler-enforced để ngăn data races
- Developer phải tự đảm bảo thread safety

### 3.2 Rust (NMDB2)

Ownership rõ ràng với compile-time guarantees:

```rust
pub struct Buffer {
    fm: Arc<FileMgr>,              // Shared immutable
    lm: Arc<Mutex<LogMgr>>,       // Shared mutable (guarded)
    contents: Page,                 // Owned directly
    blk: Option<BlockId>,          // Option for nullable
    pins: i32,
    txnum: Option<usize>,
}
```

**Ưu điểm:**
- Borrow checker ngăn use-after-free, double-free tại compile time
- `Arc<Mutex<T>>` bắt buộc synchronization khi chia sẻ mutable state
- Không thể truy cập data được bảo vệ mà không acquire lock

**Nhược điểm:**
- Verbose hơn (phải khai báo explicit Arc/Mutex)
- Learning curve cao hơn

### 3.3 Đánh giá

| Tiêu chí | C++ | Rust | Ghi chú |
|----------|-----|------|---------|
| Memory safety | Runtime (RAII) | Compile-time | Rust có lợi thế rõ ràng |
| Data race prevention | Manual | Automatic | Rust compiler ngăn tại build |
| Null safety | std::optional | Option enum | Tương đương |
| Resource cleanup | RAII | Drop trait | Tương đương |

---

## 4. Xử Lý Lỗi

### 4.1 C++ - Exception-based

```cpp
// filemgr.cpp - Silent failure khi file không tồn tại
void FileMgr::read(const BlockId& blk, Page& page) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!fs::exists(filepath)) {
        return;  // ⚠️ Silent failure - page giữ nguyên zeros
    }
    // ...
    if (!file) {
        throw std::runtime_error("Failed to read block: " + blk.to_string());
    }
}

// buffermgr.cpp - Custom exception
class BufferAbortException : public std::runtime_error {
    BufferAbortException()
        : std::runtime_error("Buffer abort: pool exhausted after timeout") {}
};
```

**Vấn đề:**
- Silent failure trong `read()` khi file chưa tồn tại - không báo lỗi
- Chỉ có 1 loại custom exception (`BufferAbortException`)
- Thiếu error context chi tiết
- Exception có thể bị bỏ qua (uncaught)

### 4.2 Rust - Result-based

```rust
// Typed error hierarchy
#[derive(Debug)]
pub enum TransactionError {
    Abort(AbortError),
    Recovery(RecoveryError),
    Utf8(FromUtf8Error),
    IO(Error),
    General,
}

// Explicit propagation với ? operator
pub fn read(&self, blk: &BlockId, p: &mut Page) -> Result<(), Error> {
    let mut f = self.get_file(blk.file_name())?;  // Tự động propagate
    f.seek(SeekFrom::Start(pos))?;
    f.read_exact(p.contents())?;
    Ok(())
}
```

**Ưu điểm:**
- Tất cả lỗi PHẢI được xử lý hoặc propagate
- Error type hierarchy phong phú với context
- `?` operator giúp propagation ngắn gọn
- Không có silent failure

### 4.3 Đánh giá

| Tiêu chí | C++ | Rust |
|----------|-----|------|
| Buộc xử lý lỗi | Không | Có (compiler) |
| Error context | String message | Typed enum variants |
| Silent failures | Có (FileMgr::read) | Không |
| Error composition | Thủ công | From trait auto-convert |

**Khuyến nghị cho C++:** Cân nhắc sử dụng `std::expected` (C++23) hoặc tạo Result type riêng để cải thiện error handling.

---

## 5. Concurrency & Thread Safety

### 5.1 C++ - Selective Locking

Chỉ `FileMgr` được bảo vệ bằng mutex:

```cpp
class FileMgr {
    mutable std::mutex mutex_;  // Mutex cho file I/O
};

// LogMgr, Buffer, BufferMgr: KHÔNG thread-safe
// Comment trong code:
// "This class is NOT thread-safe. Callers must synchronize access."
```

**Vấn đề:** Trách nhiệm thread safety bị đẩy lên tầng cao hơn (Transaction layer) nhưng Transaction layer chưa được implement.

### 5.2 Rust - Comprehensive Thread Safety

```rust
pub struct SimpleDB {
    fm: Arc<FileMgr>,                        // Immutable - safe to share
    lm: Arc<Mutex<LogMgr>>,                 // Mutable - mutex protected
    bm: Arc<Mutex<BufferMgr>>,              // Mutable - mutex protected
    mdm: Option<Arc<Mutex<MetadataMgr>>>,   // Optional, protected
}
```

Concurrency control tại Transaction level:

```rust
pub struct Transaction {
    recovery_mgr: RecoveryMgr,
    concur_mgr: ConcurrencyMgr,     // S-lock / X-lock per block
    bm: Arc<Mutex<BufferMgr>>,
    mybuffers: BufferList,
}

// Read operation acquires S-lock
pub fn get_int(&mut self, blk: &BlockId, offset: usize)
    -> Result<i32, TransactionError> {
    self.concur_mgr.s_lock(blk)?;   // Shared lock
    // ... read data
}

// Write operation acquires X-lock
pub fn set_int(&mut self, blk: &BlockId, offset: usize, val: i32, ok_to_log: bool)
    -> Result<(), TransactionError> {
    self.concur_mgr.x_lock(blk)?;   // Exclusive lock
    // ... write data
}
```

### 5.3 Đánh giá

| Tiêu chí | C++ | Rust |
|----------|-----|------|
| File I/O | Mutex ✅ | Arc (immutable) ✅ |
| Log Manager | Không ⚠️ | Arc<Mutex> ✅ |
| Buffer Manager | Không ⚠️ | Arc<Mutex> ✅ |
| Block-level locking | Không ✗ | S/X locks ✅ |
| Compiler enforcement | Không | Có (Send/Sync traits) |

---

## 6. Transaction Model

### 6.1 C++ - Không có Transaction Layer

TableScan truy cập trực tiếp BufferMgr mà không qua Transaction:

```cpp
class TableScan {
    std::shared_ptr<buffer::BufferMgr> bm_;  // Direct access

    void move_to_block(int32_t blknum) {
        file::BlockId blk(filename_, blknum);
        current_buffer_idx_ = bm_->pin(blk);  // Trực tiếp pin
    }

    void close() {
        if (current_buffer_idx_.has_value()) {
            bm_->unpin(current_buffer_idx_.value());  // Manual unpin
        }
    }
};
```

**Hệ quả:**
- Không có ACID guarantees
- Không có crash recovery
- Không có isolation giữa concurrent operations
- Không có rollback capability

### 6.2 Rust - Full ACID Transaction

```rust
pub struct Transaction {
    recovery_mgr: RecoveryMgr,     // Undo/redo log records
    concur_mgr: ConcurrencyMgr,   // S/X block-level locks
    bm: Arc<Mutex<BufferMgr>>,
    mybuffers: BufferList,          // Transaction-owned buffers
    txnum: usize,
}

impl Transaction {
    pub fn commit(&mut self) -> Result<(), Error> {
        self.recovery_mgr.commit()?;     // Write commit record
        self.concur_mgr.release();       // Release all locks
        self.mybuffers.unpin_all();       // Release all buffers
        Ok(())
    }

    pub fn rollback(&mut self) -> Result<(), TransactionError> {
        self.do_rollback()?;             // Undo changes
        self.bm.lock().unwrap().flush_all(self.txnum)?;
        let lsn = RollbackRecord::write_to_log(&self.lm, self.txnum)?;
        self.lm.lock().unwrap().flush(lsn)?;
        self.concur_mgr.release();       // Release locks
        self.mybuffers.unpin_all();       // Release buffers
        Ok(())
    }
}
```

TableScan sử dụng Transaction thay vì truy cập BufferMgr trực tiếp:

```rust
pub struct TableScan {
    tx: Arc<Mutex<Transaction>>,  // Qua transaction layer
    layout: Layout,
    rp: Option<RecordPage>,
}
```

### 6.3 Đánh giá

| ACID Property | C++ | Rust |
|---------------|-----|------|
| Atomicity | ✗ | ✅ (rollback support) |
| Consistency | ✗ | ✅ (constraint checks) |
| Isolation | ✗ | ✅ (S/X locks) |
| Durability | Partial (WAL exists) | ✅ (WAL + commit records) |

**Đây là khoảng cách kiến trúc quan trọng nhất.** C++ có WAL layer nhưng chưa wire vào transaction management.

---

## 7. Các Module NMDB2 Có Mà MudopDB_v1 Thiếu

### 7.1 Transaction Layer (`tx/`)

```
NMDB2/src/tx/
├── transaction.rs          # ACID transaction wrapper
├── bufferlist.rs           # Per-transaction buffer tracking
├── concurrency/
│   ├── concurrency_mgr.rs  # S/X lock management
│   └── lock_table.rs       # Block-level lock table
└── recovery/
    ├── recovery_mgr.rs     # Crash recovery coordinator
    ├── log_record.rs       # Log record types (Start, Commit, Rollback, SetInt, SetString)
    ├── start_record.rs
    ├── commit_record.rs
    ├── rollback_record.rs
    ├── set_int_record.rs
    ├── set_string_record.rs
    └── checkpoint_record.rs
```

**Ảnh hưởng:** Không có module này, C++ không thể đảm bảo data integrity khi crash hoặc concurrent access.

### 7.2 Metadata Management (`metadata/`)

```
NMDB2/src/metadata/
├── table_mgr.rs    # CREATE/DROP TABLE, schema storage
├── view_mgr.rs     # View definitions
├── stat_mgr.rs     # Table statistics (blocks, records)
├── index_mgr.rs    # Index metadata
└── metadata_mgr.rs # Unified metadata interface
```

**Ảnh hưởng:** Không thể tạo/quản lý tables dynamically, không có statistics cho query optimization.

### 7.3 SQL Parsing (`parse/`)

```
NMDB2/src/parse/
├── lexer.rs           # SQL tokenization
├── parser.rs          # SQL → AST
├── query_data.rs      # SELECT AST
├── insert_data.rs     # INSERT AST
├── modify_data.rs     # UPDATE AST
├── delete_data.rs     # DELETE AST
├── create_table_data.rs
├── create_view_data.rs
└── create_index_data.rs
```

### 7.4 Query Execution (`query/`, `plan/`, `opt/`)

```
NMDB2/src/query/
├── expression.rs      # Field/constant expressions
├── term.rs            # Comparison terms
├── predicate.rs       # WHERE clause predicates
├── select_scan.rs     # Filter operator
├── project_scan.rs    # Projection operator
└── product_scan.rs    # Cross-product operator

NMDB2/src/plan/
├── plan.rs            # Plan trait
├── table_plan.rs      # Full table scan plan
├── select_plan.rs     # Filter plan
├── project_plan.rs    # Projection plan
├── product_plan.rs    # Join plan
└── query_planner.rs   # Plan generation

NMDB2/src/opt/
├── heuristic_query_planner.rs  # Heuristic optimizer
└── table_planner.rs            # Per-table planning
```

### 7.5 Indexing (`index/`)

```
NMDB2/src/index/
├── index.rs           # Index trait
├── btree/
│   ├── btree_index.rs
│   ├── btree_dir.rs
│   ├── btree_leaf.rs
│   └── btree_page.rs
└── hash/
    └── hash_index.rs
```

### 7.6 Materialization (`materialize/`)

```
NMDB2/src/materialize/
├── sort_scan.rs
├── sort_plan.rs
├── merge_join_scan.rs
├── merge_join_plan.rs
├── group_by_scan.rs
├── group_by_plan.rs
├── count_fn.rs
├── max_fn.rs
├── min_fn.rs
├── sum_fn.rs
└── avg_fn.rs  (inferred)
```

### 7.7 Network Layer

```
NMDB2/src/api/network/   # gRPC server/client
NMDB2/proto/              # Protocol buffer definitions
```

**Tổng kết module thiếu:**

| Module | Files | Tầm quan trọng |
|--------|-------|-----------------|
| Transaction | ~12 files | **Critical** - ACID guarantees |
| Metadata | ~5 files | **High** - Dynamic table management |
| Parse | ~9 files | **High** - SQL support |
| Query/Plan | ~12 files | **High** - Query execution |
| Index | ~6 files | **Medium** - Performance |
| Materialize | ~10 files | **Medium** - Sort/Aggregate |
| Network | ~8 files | **Low** - gRPC server |

---

## 8. Design Patterns So Sánh

### 8.1 Polymorphism

**C++ - Virtual methods (vtable):**
```cpp
class Scan {
public:
    virtual ~Scan() = default;
    virtual void before_first() = 0;
    virtual bool next() = 0;
    virtual int get_int(const std::string& fldname) = 0;
    virtual std::string get_string(const std::string& fldname) = 0;
};

class TableScan : public Scan { ... };
```

**Rust - Traits (trait objects / static dispatch):**
```rust
pub trait ScanControl {
    fn before_first(&mut self) -> Result<(), TransactionError>;
    fn next(&mut self) -> Result<bool, TransactionError>;
    fn get_int(&mut self, fldname: &str) -> Result<i32, TransactionError>;
    fn get_string(&mut self, fldname: &str) -> Result<String, TransactionError>;
}

impl ScanControl for TableScan { ... }
impl UpdateScanControl for TableScan { ... }  // Multiple traits
```

**Khác biệt:** Rust cho phép implement nhiều traits riêng biệt (ScanControl + UpdateScanControl) mà không cần multiple inheritance. C++ dùng single inheritance + virtual.

### 8.2 Factory Pattern

**C++ - Abstract factory:**
```cpp
class Driver {
public:
    virtual std::unique_ptr<Connection> connect(const std::string& url) = 0;
};

class EmbeddedDriver : public Driver { ... };
```

**Rust - Trait + enum dispatch:**
```rust
// Uses enum_dispatch crate for zero-cost dynamic dispatch
```

### 8.3 Iterator Pattern (Volcano Model)

Cả hai đều dùng iterator-based execution (Volcano model) cho query processing. Mỗi operator implement `next()` interface để kéo từng tuple.

---

## 9. Build System & Infrastructure

### 9.1 C++

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(MudopDB VERSION 1.0)
set(CMAKE_CXX_STANDARD 17)

# GoogleTest fetched at build time
FetchContent_Declare(googletest ...)
```

Docker multi-stage build:
```dockerfile
# Build → Test → Runtime (slim)
```

### 9.2 Rust

```toml
# Cargo.toml
[dependencies]
tonic = "0.8.0"          # gRPC
tokio = "1.20.1"         # Async runtime
prost = "0.11.0"         # Protocol Buffers
enum_dispatch = "0.3.8"  # Zero-cost polymorphism
```

**Khác biệt chính:**
- Rust có ecosystem phong phú hơn cho networking (Tokio, Tonic)
- C++ phải tự implement hoặc dùng thư viện bên ngoài (gRPC C++)
- Rust testing built-in, C++ cần GoogleTest

---

## 10. Khuyến Nghị Cho MudopDB_v1

### 10.1 Ưu Tiên Cao (Critical Path)

1. **Implement Transaction Layer (Phase 5a)**
   - `Transaction` class wrapping BufferMgr + LogMgr
   - `RecoveryMgr` cho crash recovery
   - `ConcurrencyMgr` cho lock management
   - **Lý do:** Đây là nền tảng cho tất cả tầng trên

2. **Refactor TableScan để sử dụng Transaction**
   - Thay `std::shared_ptr<BufferMgr>` bằng `std::shared_ptr<Transaction>`
   - Thêm lock acquisition trước read/write
   - **Lý do:** Đảm bảo ACID properties

3. **Cải thiện Error Handling**
   - Loại bỏ silent failures trong `FileMgr::read()`
   - Tạo error hierarchy (thay vì chỉ `runtime_error`)
   - Cân nhắc `std::expected<T, E>` (C++23) hoặc custom Result type

### 10.2 Ưu Tiên Trung Bình

4. **Implement Metadata Management (Phase 5b)**
   - TableMgr, ViewMgr, StatMgr
   - System catalog bootstrap

5. **Thread Safety cho LogMgr và BufferMgr**
   - Thêm mutex protection (hoặc delegate rõ ràng qua Transaction)

6. **Implement SQL Parser (Phase 6a)**
   - Lexer + Parser cho basic SQL

### 10.3 Ưu Tiên Thấp

7. **Index Support**
   - B-tree và hash index

8. **Query Optimization**
   - Cost-based planner

9. **Network Layer**
   - gRPC hoặc custom protocol

---

## 11. Kết Luận

### Điểm Mạnh của MudopDB_v1 (C++)
- Foundation layers (File, Log, Buffer, Record) implemented chắc chắn
- 183 tests đều pass, test coverage tốt
- Modern C++17 với smart pointers và RAII
- Documentation chi tiết
- Build/test pipeline với Docker

### Điểm Yếu So Với NMDB2 (Rust)
- **Thiếu Transaction layer** - khoảng cách quan trọng nhất
- **Không có crash recovery** - WAL tồn tại nhưng chưa wire vào recovery
- **Thread safety không hoàn chỉnh** - chỉ FileMgr được bảo vệ
- **Error handling yếu** - silent failures, thiếu error types
- **Thiếu ~55 files** so với NMDB2 (metadata, parse, query, index, materialize)

### Đánh Giá Tổng Thể

MudopDB_v1 là một **nền tảng vững chắc** cho các tầng thấp (storage engine core), nhưng cần thêm nhiều công việc để đạt được feature parity với NMDB2. Khoảng cách lớn nhất không nằm ở chất lượng code (đã tốt) mà ở **phạm vi chức năng** - đặc biệt là Transaction layer, là backbone cho mọi operation ở tầng cao hơn.

**Tiến độ migration:** ~30% theo code volume, ~57% theo số phases.

---

**Phiên bản tài liệu:** 1.0
**Ngày tạo:** 7 tháng 2, 2026

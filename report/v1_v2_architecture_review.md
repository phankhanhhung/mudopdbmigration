# Review Kien Truc: MudopDB_v1 (C++) vs NMDB2 (Rust) - Toan Bo Cac Layer

**Ngay:** 8 thang 2, 2026
**Pham vi:** So sanh chi tiet toan bo 8 layers: File, Log, Buffer, Record, Transaction, Query/Optimization, Index/Metadata/Multibuffer, API/Network

---

## Muc Luc

1. [Layer 1: File Layer](#1-layer-1-file-layer)
2. [Layer 2: Log Layer](#2-layer-2-log-layer)
3. [Layer 3: Buffer Layer](#3-layer-3-buffer-layer)
4. [Layer 4: Record Layer](#4-layer-4-record-layer)
5. [Layer 5: Transaction Layer](#5-layer-5-transaction-layer)
6. [Layer 6: Query Execution & Optimization](#6-layer-6-query-execution--optimization)
7. [Layer 7: Index, Metadata & Multi-buffer](#7-layer-7-index-metadata--multi-buffer)
8. [Layer 8: API & Network](#8-layer-8-api--network)
9. [Tong Ket](#9-tong-ket)

---

## 1. Layer 1: File Layer

### 1.1 BlockId

**Tuong duong ve chuc nang.** Ca hai deu la value type immutable.

| Tieu chi | C++ (`blockid.hpp`) | Rust (`block_id.rs`) |
|----------|---------------------|----------------------|
| Hash support | Manual `std::hash` specialization | `#[derive(Hash)]` - tu sinh |
| Equality | Manual `operator==`, `operator!=` | `#[derive(Eq, PartialEq)]` |
| Ordering | Manual `operator<` | Khong co (chua can) |
| Display | `to_string()` method | `impl Display` trait |
| Clone | Implicit (copy constructor) | `#[derive(Clone)]` |

**Nhan xet:** Rust ngan gon hon nho derive macros. C++ phai viet manual hon nhung cung cap `operator<` (cho `std::map`) ma Rust khong can.

### 1.2 Page

**Tuong duong ve chuc nang.** Ca hai dung big-endian encoding, format `[4-byte length][data]` cho bytes/strings.

| Tieu chi | C++ (`page.cpp`) | Rust (`page.rs`) |
|----------|-------------------|-------------------|
| Bounds checking | Explicit `check_bounds()` throw `out_of_range` | Implicit qua Rust slice panics |
| get_string return | `std::string` (luon thanh cong) | `Result<String, FromUtf8Error>` |
| get_bytes return | `const uint8_t*` (raw pointer) | `&[u8]` (safe slice) |
| Negative length check | `if (length < 0)` throw | Khong can (dung `usize`) |
| contents() visibility | `public` | `pub(in crate)` - chi noi bo |

**Van de C++:** `get_bytes` tra raw pointer ma khong kem length. Caller phai goi `get_bytes_length()` rieng. Neu quen goi hoac dung sai length -> buffer overflow. Rust tra `&[u8]` slice da bao gom length.

**Van de Rust:** `get_string` tra `Result` nhung caller phai handle UTF-8 error. C++ bo qua validation UTF-8 (don gian hon nhung khong safe).

### 1.3 FileMgr

| Tieu chi | C++ (`filemgr.cpp`) | Rust (`filemgr.rs`) |
|----------|---------------------|---------------------|
| Constructor | Throw-on-error (implicit) | `Result<FileMgr, Error>` |
| Thread safety | `std::mutex` cho moi method | Khong co mutex |
| File caching | `open_files_: HashMap<name, block_count>` | Khong cache - doc metadata moi lan |
| read() khi file chua co | **Silent return** - page giu zeros | Tra `Ok(())` - page giu zeros neu file ngan hon |

**C++ thang o:** Thread safety (mutex) va performance (file size cache).
**Rust thang o:** Error handling va API safety.

---

## 2. Layer 2: Log Layer

### 2.1 LogMgr

| Tieu chi | C++ (`logmgr.cpp`) | Rust (`logmgr.rs`) |
|----------|---------------------|---------------------|
| Constructor fail | Khong the fail (nhung nen) | `Result<LogMgr, Error>` |
| append() return | `size_t` (LSN) | `Result<usize, Error>` |
| flush() return | `void` | `Result<(), Error>` |
| Shared ownership | `std::shared_ptr<FileMgr>` | `Arc<FileMgr>` |
| Thread safety | Khong (comment ghi ro) | Khong (wrapper ben ngoai) |

**Logic gan nhu giong het nhau:** Backward-growing log, flush khi het page, append new block.

**Khac biet nho:**
- C++ phai dung dummy `BlockId("", 0)` roi gan lai trong constructor. Rust dung `if/else` expression tra thang value.
- Rust explicit voi `?` cho error propagation, C++ dung implicit exception.

### 2.2 LogIterator

| Tieu chi | C++ (`logiterator.cpp`) | Rust (`logiterator.rs`) |
|----------|-------------------------|-------------------------|
| Interface | Custom `has_next()` / `next()` | `impl Iterator` (standard trait) |
| next() khi het | Throw `runtime_error` | Return `None` |
| Error trong iteration | Khong co | `.ok()` nuot loi (design limitation) |

Rust `LogIterator` implement `Iterator` trait nen duoc dung voi standard library iterator combinators (map, filter, take, collect...). C++ phai viet while loop thu cong.

**Ket luan Layer Log:** Logic giong nhau. Rust sach hon o initialization va iterator design.

---

## 3. Layer 3: Buffer Layer

### 3.1 Buffer

| Tieu chi | C++ (`buffer.cpp`) | Rust (`buffer.rs`) |
|----------|---------------------|---------------------|
| LogMgr access | `shared_ptr<LogMgr>` (shared, unguarded) | `Arc<Mutex<LogMgr>>` (shared, guarded) |
| assign_to_block return | `void` (exception on error) | `Result<(), Error>` |
| flush() return | `void` | `Result<(), Error>` |
| Visibility control | Comment "Package-private" | `pub(in crate::buffer)` compiler-enforced |

**Van de C++ quan trong:** `Buffer::flush()` goi `lm_->flush()` tren `shared_ptr<LogMgr>`. LogMgr khong thread-safe. Neu 2 buffers cung flush song song -> data race tren LogMgr. Rust bat buoc `lm.lock().unwrap()` nen khong the quen.

### 3.2 BufferMgr

| Tieu chi | C++ (`buffermgr.cpp`) | Rust (`buffermgr.rs`) |
|----------|------------------------|------------------------|
| pin() return | `size_t` (throw on timeout) | `Result<usize, AbortError>` |
| Wait mechanism | `sleep_for(100ms)` polling | `park_timeout(max_time)` |
| Error type | `BufferAbortException` (1 loai) | `AbortError` enum (Time/IO/General) |

C++ polling 100ms gay overhead. Rust `park_timeout` hieu qua hon. Tuy nhien ca hai chua optimal - ly tuong nen dung condition variable.

---

## 4. Layer 4: Record Layer

### 4.1 Schema & Layout

**Gan nhu tuong duong hoan toan.** Cung data structures, cung logic.

### 4.2 RecordPage - ✅ DA FIX (Phase 5)

| He qua | C++ (Via Transaction) | Rust (Via Transaction) |
|---------|------------------------|------------------------|
| WAL logging khi write | ✅ Co (Transaction write log truoc data) | Co (Transaction write log truoc data) |
| Concurrency control | ✅ Co (Transaction acquire S/X lock) | Co (Transaction acquire S/X lock) |
| Crash recovery | ✅ Co (Transaction rollback undo changes) | Co (Transaction rollback undo changes) |
| set_modified marker | ✅ Actual txnum + LSN tu log | Actual txnum + LSN tu log |

**Da fix tu Phase 5.** C++ RecordPage goi qua Transaction cho moi read/write. Verified voi 12 WAL compliance tests (test_wal.cpp).

### 4.3 TableScan

C++ `next()` doc ro rang, logic flow de hieu hon. Rust verbose vi borrow checker constraints va `Option<RecordPage>` phai `if let Some` moi lan truy cap.

---

## 5. Layer 5: Transaction Layer

### 5.1 Transaction Main Class

| Tieu chi | C++ (`transaction.hpp/cpp`) | Rust (`transaction.rs`) |
|----------|------------------------------|--------------------------|
| Constructor | Khong the fail | `Result<Transaction, Error>` |
| Transaction ID | `static atomic<size_t>` seq_cst | `static AtomicUsize` SeqCst |
| Error handling | Exceptions (`std::runtime_error`) | `Result<T, TransactionError>` enum |
| Resource cleanup | RAII via destructors | Explicit via Drop trait |

**Error type:** C++ gop moi loi vao `std::runtime_error` voi string message. Rust phan biet ro:

```rust
pub enum TransactionError {
    Abort(AbortError),
    Recovery(RecoveryError),
    Utf8(FromUtf8Error),
    IO(Error),
    General,
}
```

**Commit/Rollback:** Logic giong nhau - flush dirty buffers, write commit/rollback record, flush log, release locks, unpin all.

### 5.2 Recovery System

Ca hai ho tro 6 log record types giong nhau: CHECKPOINT(0), START(1), COMMIT(2), ROLLBACK(3), SETINT(4), SETSTRING(5).

| Tieu chi | C++ | Rust |
|----------|-----|------|
| LogRecord interface | `virtual` base class | `trait LogRecord` + `Box<dyn LogRecord>` |
| Factory function | `unique_ptr<LogRecord>` | `Result<Box<dyn LogRecord>, TransactionError>` |
| Undo signature | `void undo(Transaction& tx)` | `Result<(), TransactionError>` |

**WAL Protocol (ca hai):**
1. Acquire exclusive lock (`x_lock`)
2. Write log record TRUOC khi modify data (neu `ok_to_log`)
3. Modify buffer contents
4. Mark buffer as modified voi LSN

```cpp
// C++ WAL - giong Rust nhung dung exception
void Transaction::set_int(const file::BlockId& blk, size_t offset,
                           int32_t val, bool ok_to_log) {
    concur_mgr_.x_lock(blk);
    // ... recovery_mgr_.set_int -> log TRUOC
    buff.contents().set_int(offset, val);  // modify SAU
    buff.set_modified(txnum_, lsn);
}
```

### 5.3 Concurrency / Locking

| Tieu chi | C++ | Rust |
|----------|-----|------|
| Global Lock Table | Meyer's singleton + static mutex | `Lazy<Mutex<LockTable>>` |
| Lock representation | `unordered_map<BlockId, string>` ("S"/"X") | `HashMap<BlockId, String>` ("S"/"X") |
| Deadlock detection | Timeout 10s | Timeout 10s |
| Wait mechanism | `sleep_for(max_time_)` | `park_timeout(max_time)` |

**Bug chung:** Ca hai track lock type bang `String` ("S"/"X") - nen dung enum. Lock upgrade tu S -> X chi ghi nhan o local map, khong goi `x_lock()` tren global lock table.

### 5.4 BufferList (Per-Transaction Buffer Tracking)

Logic tuong duong: `buffers_` map (BlockId -> index) cho fast lookup, `pins_` vector cho pin count tracking.

| Tieu chi | C++ | Rust |
|----------|-----|------|
| Error handling | Throws exception | Returns `Result` |
| Visibility | Public | `pub(in crate::tx)` |
| Buffer access | Direct via shared_ptr | Explicit lock acquisition |

### Danh Gia Layer Transaction

| Component | C++ | Rust | Winner |
|-----------|-----|------|--------|
| Error handling | Exceptions | Result<T, TransactionError> | **Rust** |
| Recovery protocol | Correct WAL | Correct WAL | **Hoa** |
| Concurrency | Manual mutex | Type-enforced Mutex | **Rust** |
| Lock table | Global singleton | Lazy static | **Hoa** |
| Code clarity | Familiar OOP | More verbose | **C++** |
| Thread safety | Manual discipline | Compiler-enforced | **Rust** |

---

## 6. Layer 6: Query Execution & Optimization

### 6.1 Scan Interface

**C++ - Abstract Base Class:**
```cpp
class Scan {
public:
    virtual ~Scan() = default;
    virtual void before_first() = 0;
    virtual bool next() = 0;
    virtual int get_int(const std::string& fldname) = 0;
    virtual std::string get_string(const std::string& fldname) = 0;
    virtual Constant get_val(const std::string& fldname) = 0;
    virtual bool has_field(const std::string& fldname) const = 0;
    virtual void close() = 0;
};
```

**Rust - Trait + Enum Dispatch:**
```rust
#[enum_dispatch(Scan)]
pub trait ScanControl {
    fn before_first(&mut self) -> Result<(), TransactionError>;
    fn next(&mut self) -> Result<bool, TransactionError>;
    fn get_int(&mut self, fldname: &str) -> Result<i32, TransactionError>;
    // ...
}

#[enum_dispatch]
pub enum Scan {
    Product(ProductScan), Project(ProjectScan), Select(SelectScan),
    Table(TableScan), IndexSelect(IndexSelectScan), IndexJoin(IndexJoinScan),
    Chunk(ChunkScan), MultibufferProduct(MultibufferProductScan),
    Sort(SortScan), GroupBy(GroupByScan), MergeJoin(MergeJoinScan),
}
```

| Tieu chi | C++ | Rust |
|----------|-----|------|
| Dispatch | Virtual (runtime, vtable) | enum_dispatch (compile-time, zero-cost) |
| Error handling | Exceptions | Result<T, Error> |
| Extensibility | Open (any class can inherit) | Closed (enum variants fixed) |
| UpdateScan cast | `dynamic_cast<UpdateScan*>` (co the fail) | Pattern matching (exhaustive) |

### 6.2 Constant / Expression / Term / Predicate

| Tieu chi | C++ | Rust |
|----------|-----|------|
| Constant storage | `std::variant<int, string>` (type-safe union) | Dual `Option<i32>` + `Option<String>` |
| Expression::evaluate | Takes `Scan&` (polymorphic) | Generic `T: ScanControl` |
| Predicate operations | Identical logic | Identical logic |
| Derive traits | Manual operators | `#[derive(Clone, Eq, Hash, ...)]` |

**C++ Constant tot hon:** `std::variant` la union thuc su, chi 1 value tai 1 thoi diem. Rust dung 2 Option fields -> lang phi bo nho.

### 6.3 SQL Parser

Ca hai dung recursive descent parser voi Lexer tuong tu.

| Tieu chi | C++ | Rust |
|----------|-----|------|
| Error handling | Exceptions | `Result<T, BadSyntaxError>` |
| Update command type | `std::variant<InsertData, DeleteData, ...>` | `enum Object` voi enum_dispatch |
| Token type | `enum class TokenType` | `enum TokenType` |

Logic parsing gan nhu giong nhau. Rust dung `?` operator cho error propagation, C++ dung implicit exceptions.

### 6.4 Plan Interface

**C++ - Abstract Base Class:**
```cpp
class Plan {
public:
    virtual std::unique_ptr<Scan> open() = 0;
    virtual size_t blocks_accessed() const = 0;
    virtual size_t records_output() const = 0;
    virtual size_t distinct_values(const std::string& fldname) const = 0;
    virtual std::shared_ptr<record::Schema> schema() const = 0;
};
```

**Rust - Trait + Enum:**
```rust
#[derive(Clone)]
#[enum_dispatch]
pub enum Plan {
    Table(TablePlan), Select(SelectPlan), Project(ProjectPlan),
    Product(ProductPlan), IndexSelect(IndexSelectPlan), IndexJoin(IndexJoinPlan),
    Materialize(MaterializePlan), MultibufferProduct(MultibufferProductPlan),
    Sort(SortPlan), GroupBy(GroupByPlan), MergeJoin(MergeJoinPlan),
    OptimizedProduct(OptimizedProductPlan),
}
```

**Khac biet quan trong:**
- C++ `ProductScan` owns scans via `unique_ptr` (exclusive, don gian)
- Rust `ProductScan` dung `Arc<Mutex<Scan>>` (shared, thread-safe nhung overhead)
- Rust `Plan` enum la `Clone` -> co the try multiple plan orderings

### 6.5 Query Planner / Optimizer

| Component | C++ | Rust |
|-----------|-----|------|
| BasicQueryPlanner | Co | Co |
| BetterQueryPlanner | **Khong** | Co (chon product order toi uu) |
| HeuristicQueryPlanner | Co | Co |
| OptimizedProductPlan | **Khong** | Co (wrapper chon optimal order) |

Rust co them 2 optimizer components ma C++ khong co: `BetterQueryPlanner` va `OptimizedProductPlan`.

**HeuristicQueryPlanner:** Logic giong nhau - greedy algorithm chon lowest-cost join/product.

### 6.6 Update Commands (Data Structures)

Tuong duong. Ca hai co: InsertData, DeleteData, ModifyData, CreateTableData, CreateViewData, CreateIndexData. Ca hai return by value.

### Danh Gia Layer Query

| Component | C++ | Rust | Winner |
|-----------|-----|------|--------|
| Scan dispatch | Virtual (runtime) | enum_dispatch (zero-cost) | **Rust** |
| Scan ownership | unique_ptr (simple) | Arc<Mutex<>> (safe) | **C++** (simplicity) |
| Constant type | variant (efficient) | Dual Option (wasteful) | **C++** |
| Parser | Identical logic | Result-based | **Rust** (error handling) |
| Plan cloneability | Khong | Clone derive | **Rust** |
| Optimizer completeness | 2 planners | 4 planners | **Rust** |
| Code readability | Cleaner | More verbose | **C++** |

---

## 7. Layer 7: Index, Metadata & Multi-buffer

### 7.1 Index Interface

| Tieu chi | C++ | Rust |
|----------|-----|------|
| Interface | Abstract base class `Index` | Trait `IndexControl` + enum_dispatch |
| Error handling | Exceptions | Result<T, TransactionError> |
| Implementations | HashIndex, BTreeIndex | HashIndex, BTreeIndex |
| Namespace issue | Phai dung `::Index` de tranh conflict | Khong co van de |

### 7.2 Hash Index

Tuong duong. Ca hai dung `NUM_BUCKETS = 100`, hash searchkey de chon bucket, tao TableScan de scan bucket.

| Tieu chi | C++ | Rust |
|----------|-----|------|
| State storage | `std::optional<TableScan>` voi `emplace` | `Option<TableScan>` |
| Hashing | `std::hash<Constant>{}` | `DefaultHasher` manual |
| search_cost() | `size / NUM_BUCKETS` | `size / NUM_BUCKETS` |

### 7.3 B-Tree Index

Ca hai implement B-Tree voi:
- `BTPage` - low-level page management (get/set records, split, format)
- `BTreeDir` - directory/internal node navigation
- `BTreeLeaf` - leaf node operations (insert, delete, overflow chaining)
- `BTreeIndex` - top-level coordinator

| Tieu chi | C++ | Rust |
|----------|-----|------|
| Split logic | Identical | Identical |
| Overflow chaining | Same (flag points to next block) | Same |
| DirEntry/LeafEntry | Value types | Value types |
| Error handling | Exceptions | Result propagation |

### 7.4 Index Query Scans

**IndexSelectScan** (equality lookup qua index):
```cpp
// C++ - clean, direct
bool IndexSelectScan::next() {
    bool ok = idx_->next();
    if (ok) ts_->move_to_rid(idx_->get_data_rid());
    return ok;
}
```

**IndexJoinScan** (nested-loop join qua index): Logic giong nhau - iterate LHS, for each LHS row search index, fetch matching RHS row.

### 7.5 Metadata Management

| Manager | C++ | Rust | Khac biet |
|---------|-----|------|-----------|
| TableMgr | `shared_ptr<TableMgr>` | `Arc<TableMgr>` | Tuong duong |
| ViewMgr | Direct access | Direct access | Tuong duong |
| StatMgr | `shared_ptr<StatMgr>` (no mutex) | `Arc<Mutex<StatMgr>>` | **Rust safe hon** |
| IndexMgr | Direct access | Direct access | Tuong duong |
| MetadataMgr | Facade pattern | Facade pattern | Tuong duong |

**Khac biet quan trong:** Rust wrap StatMgr trong `Arc<Mutex<>>` vi stat cache co the thay doi (refresh). C++ dung `shared_ptr` khong co mutex -> potential data race khi concurrent stat refresh.

### 7.6 Multi-buffer Operations

| Component | C++ | Rust |
|-----------|-----|------|
| BufferNeeds::best_factor | Integer ceiling division | Float `ceil()` |
| ChunkScan | Vector of RecordPages | Vec of RecordPages |
| MultibufferProductScan | **Direct nested loop** | **Delegates to ProductScan** |
| MultibufferProductPlan | TempTable materialization | TempTable materialization |

**Khac biet kien truc MultibufferProductScan:**

```cpp
// C++ - implements nested loop TRUC TIEP
bool MultibufferProductScan::next() {
    while (rhsscan_) {
        if (rhsscan_->next()) return true;
        if (!lhsscan_->next()) {
            if (!use_next_chunk()) return false;
            if (!lhsscan_->next()) return false;
        }
        rhsscan_->before_first();
    }
    return false;
}
```

```rust
// Rust - DELEGATE cho ProductScan
fn next(&mut self) -> Result<bool, TransactionError> {
    while let Some(prodscan) = &mut self.prodscan {
        if prodscan.next()? { return Ok(true); }
        if !self.use_next_chunk()? { return Ok(false); }
    }
    Err(TransactionError::General)
}
```

C++ don gian hon va hieu qua hon (khong co layer ProductScan trung gian). Rust phai tao moi `ProductScan` cho moi chunk do `Arc<Mutex<Scan>>` ownership requirement.

### Danh Gia Layer Index/Metadata/Multibuffer

| Component | C++ | Rust | Winner |
|-----------|-----|------|--------|
| Index interface | Virtual base | enum_dispatch | **Rust** (zero-cost) |
| HashIndex | Tuong duong | Tuong duong | **Hoa** |
| BTreeIndex | Exceptions | Result propagation | **Rust** (explicit) |
| IndexSelectScan | unique_ptr | Owned structs | **Hoa** |
| IndexJoinScan | unique_ptr | Box<Scan> | **Hoa** |
| StatMgr thread safety | No mutex | Arc<Mutex<>> | **Rust** |
| MultibufferProductScan | Direct loop | ProductScan delegate | **C++** (simpler) |
| BufferNeeds | Integer math | Float math | **C++** (efficient) |

---

## 8. Layer 8: API & Network

### 8.1 Connection Abstraction

**C++ - Virtual Inheritance:**
```cpp
class Connection {
public:
    virtual std::unique_ptr<Statement> create_statement() = 0;
    virtual void close() = 0;
    virtual void commit() = 0;
    virtual void rollback() = 0;
};

class EmbeddedConnection : public Connection { /* ... */ };
class NetworkConnection : public Connection { /* STUB */ };
```

**Rust - Trait + Enum:**
```rust
pub trait ConnectionControl {
    fn close(&mut self) -> Result<(), SQLError>;
    fn commit(&mut self) -> Result<(), SQLError>;
    fn rollback(&mut self) -> Result<(), SQLError>;
}

pub enum Connection {
    Embedded(Arc<Mutex<EmbeddedConnection>>),
    Network(Arc<Mutex<NetworkConnection>>),   // FULL IMPLEMENTATION
}
```

### 8.2 Statement & ResultSet

| Tieu chi | C++ | Rust |
|----------|-----|------|
| execute_query return | `unique_ptr<ResultSet>` | `Result<ResultSet, SQLError>` |
| execute_update return | `size_t` (throw on error) | `Result<usize, SQLError>` |
| ResultSet::next | `bool` (try-catch internally) | `Result<bool, SQLError>` |
| Field name handling | Lowercase conversion | Lowercase conversion |
| Auto-rollback | On exception | On error |

### 8.3 Metadata Interface

| Tieu chi | C++ | Rust |
|----------|-----|------|
| Const correctness | All methods `const` | Methods take `&mut self` (khong can thiet) |
| Null handling | Checks for null schema | Assumes valid |
| Column indexing | 1-indexed (JDBC-style) | 1-indexed (same) |

**C++ tot hon** ve const correctness cho Metadata - read-only operations nen la `const`.

### 8.4 Network Server - **KHAC BIET LON NHAT**

| Tieu chi | C++ | Rust |
|----------|-----|------|
| Protocol | **KHONG CO** (chi co facade) | gRPC voi Protobuf |
| Server | `SimpleDBServer` wrapper class | 4 gRPC services (Connection, Statement, ResultSet, MetaData) |
| Client | `NetworkConnection` **STUB** | Full gRPC client voi Tokio |
| Async model | Synchronous | Async/await voi Tokio runtime |
| Transport | Khong co | HTTP/2 qua Tonic |
| State management | HashMap<id, ResultSet> | Per-connection state |

**C++ SimpleDBServer Architecture:**
```cpp
class SimpleDBServer {
    SimpleDB db_;
    std::shared_ptr<EmbeddedConnection> conn_;
    std::mutex mutex_;
    std::unordered_map<uint64_t, std::unique_ptr<ResultSetHolder>> result_sets_;
    // ID-based API: execute_query returns uint64_t id
    // Client calls get_int(id, fldname), next(id), etc.
};
```

C++ co thiet ke tot cho network API (ID-based result set management), nhung chua implement transport layer nao.

**Rust gRPC Protocol:**
```protobuf
service Connection {
    rpc Close (CloseConnectionRequest) returns (CloseConnectionReply) {}
    rpc Commit (CommitRequest) returns (CommitReply) {}
    rpc Rollback (RollbackRequest) returns (RollbackReply) {}
}
service Statement {
    rpc ExecuteQuery (ExecuteQueryRequest) returns (ExecuteQueryReply) {}
    rpc ExecuteUpdate (ExecuteUpdateRequest) returns (ExecuteUpdateReply) {}
}
service ResultSet { /* Next, GetInt, GetString, Close */ }
service MetaData { /* GetColumnCount, GetColumnName, GetColumnType, GetColumnDisplaySize */ }
```

Rust co full client/server implementation voi async I/O.

### Danh Gia Layer API/Network

| Component | C++ | Rust | Winner |
|-----------|-----|------|--------|
| Connection design | Virtual inheritance | Tagged enum | **Rust** |
| Error handling | Exceptions | Result<T, SQLError> | **Rust** |
| Thread safety | No protection | Arc<Mutex<>> | **Rust** |
| Network protocol | **Khong co** | gRPC/Protobuf | **Rust** (by far) |
| Async support | Synchronous | Tokio async | **Rust** |
| Metadata const | `const` methods | `&mut self` | **C++** |
| Server architecture | Good facade design | Service-oriented | **Rust** |

---

## 9. Tong Ket

### 9.1 Bang Tong Ket Toan Bo

| Layer | Component | C++ | Rust | Winner | Ly do chinh |
|-------|-----------|-----|------|--------|-------------|
| **File** | BlockId | ---- | ---- | Hoa | Cung chuc nang |
| | Page | ---+ | ----+ | Rust | Slice > raw pointer |
| | FileMgr | ----+ | ---- | **C++** | Mutex + file size cache |
| **Log** | LogMgr | ---- | ----+ | Rust | Constructor init + error |
| | LogIterator | ---+ | ---- | Rust | Standard Iterator trait |
| **Buffer** | Buffer | ---+ | ---- | Rust | Mutex LogMgr, visibility |
| | BufferMgr | ---- | ---- | Hoa | Ca hai wait chua optimal |
| **Record** | Schema/Layout | ---- | ---- | Hoa | Gan giong nhau |
| | RecordPage | --- | ----+ | Rust | WAL-compliant qua Transaction |
| | TableScan | ---- | ---+ | **C++** | Readability, clean flow |
| **Transaction** | Transaction | ---+ | ---- | Rust | Error types, thread safety |
| | Recovery | ---- | ---- | Hoa | WAL protocol identical |
| | Concurrency | ---- | ---- | Hoa | Same algorithm, same bug |
| | BufferList | ---- | ----+ | Rust | Visibility, error handling |
| **Query** | Scan interface | ---+ | ----+ | Rust | Zero-cost enum dispatch |
| | Constant | ----+ | --- | **C++** | variant > dual Option |
| | Parser | ---- | ----+ | Rust | Result-based errors |
| | Plan interface | ----+ | ---+ | **C++** | Simpler scan ownership |
| | Optimizer | --- | ----+ | **Rust** | 4 vs 2 planners |
| **Index** | Hash/BTree | ---- | ----+ | Rust | Explicit error propagation |
| | Index Scans | ---- | ---- | Hoa | Same logic |
| | Metadata Mgrs | ---- | ----+ | Rust | StatMgr thread safety |
| | MultibufferProduct | ----+ | --- | **C++** | Direct loop, no wrapper |
| **API** | Connection/Statement | ---+ | ----+ | Rust | Type safety, error handling |
| | Network | - | ----+ | **Rust** | gRPC vs khong co |
| | Metadata | ----+ | --- | **C++** | Const correctness |

### 9.2 Thong Ke

| | C++ thang | Rust thang | Hoa |
|---|----------|-----------|-----|
| So luong | **6** | **13** | **8** |

### 9.3 C++ Tot Hon O Dau

1. **FileMgr** - mutex + file size cache (Rust khong co ca hai)
2. **TableScan readability** - code ngan gon, logic flow truc quan
3. **Constant type** - `std::variant` hieu qua hon dual `Option`
4. **Plan ownership** - `unique_ptr<Scan>` don gian hon `Arc<Mutex<Scan>>`
5. **MultibufferProductScan** - nested loop truc tiep, khong can wrapper
6. **Metadata const correctness** - read-only methods dung `const`

### 9.4 Rust Tot Hon O Dau

1. ~~**Error handling nhat quan**~~ ✅ DA FIX: DbResult<T> error handling (commit b7aaa20)
2. **Thread safety compiler-enforced** - `Arc<Mutex<>>` bat buoc lock. C++ co the quen
3. ~~**Network implementation**~~ ✅ DA FIX: TCP transport voi binary protocol (commit 99ed9d6)
4. **Zero-cost enum dispatch** - Scan/Plan/Index deu dung enum_dispatch, khong co vtable overhead (inherent C++ vtable)
5. **Optimizer completeness** - 4 query planners vs 2
6. **Visibility enforcement** - `pub(in crate::module)` vs comment "package-private"
7. ~~**WAL compliance**~~ ✅ DA FIX TU Phase 5: RecordPage da goi qua Transaction. Verified voi 12 WAL tests

### 9.5 Tuong Duong

1. **Core algorithms** - WAL protocol, B-Tree split/merge, hash bucketing, lock protocol, eviction strategy
2. **Data encoding** - Big-endian integers, `[4-byte length][data]` strings
3. **Schema design** - Same catalog tables (tblcat, fldcat, viewcat, idxcat)
4. **Recovery** - Same 6 log record types, same undo-only recovery

### 9.6 Bugs Chung

| Bug | C++ | Rust |
|-----|-----|------|
| Lock upgrade khong ghi len global lock table | Co | Co |
| ~~Lock type dung String thay vi enum~~ | ✅ C++ fixed (LockType enum) | Co |
| ~~Khong co lock fairness / starvation prevention~~ | ✅ C++ fixed (condition_variable) | Co |
| LogIterator nuot I/O error | N/A (throw) | Co (`.ok()`) |

### 9.7 Ket Luan Tong The

**Rust (NMDB2) thang tong the** voi 13/27 components, nhung C++ da thu hep khoang cach:
- ~~Error handling~~ ✅ C++ da co DbResult<T>
- ~~Network layer~~ ✅ C++ da co TCP transport
- ~~WAL compliance~~ ✅ C++ da fix tu Phase 5
- ~~Lock fairness~~ ✅ C++ da co condition_variable
- **Con lai Rust hon**: Thread safety (compiler-enforced), enum dispatch (zero-cost), optimizer completeness (4 vs 2), visibility enforcement

**C++ (MudopDB_v1) thang o pragmatics** voi 6/27 components:
- **Simplicity**: Ownership model don gian hon (unique_ptr vs Arc<Mutex<>>)
- **Performance**: File cache, integer math, direct nested loop
- **Readability**: Less verbose, familiar OOP patterns

**Trade-off co ban:** Rust doi code verbose hon (Result wrapping, borrow checker gymnastics) de lay compiler-enforced safety. C++ doi manual discipline de lay simplicity va familiarity. Cho database system can correctness va concurrency, Rust la lua chon tot hon cho production.

---

**Phien ban tai lieu:** 3.0 - Full review toan bo 8 layers
**Ngay tao:** 8 thang 2, 2026

# Review Kiến Trúc: MudopDB_v1 (C++) vs NMDB2 (Rust) - Chỉ Các Phần Đã Hoàn Thành

**Ngày:** 7 tháng 2, 2026
**Phạm vi:** So sánh chi tiết 4 layers đã implement: File, Log, Buffer, Record

---

## 1. Layer 1: File Layer

### 1.1 BlockId

**Tương đương về chức năng.** Cả hai đều là value type immutable.

| Tiêu chí | C++ (`blockid.hpp`) | Rust (`block_id.rs`) |
|----------|---------------------|----------------------|
| Hash support | Manual `std::hash` specialization | `#[derive(Hash)]` - tự sinh |
| Equality | Manual `operator==`, `operator!=` | `#[derive(Eq, PartialEq)]` |
| Ordering | Manual `operator<` | Không có (chưa cần) |
| Display | `to_string()` method | `impl Display` trait |
| Clone | Implicit (copy constructor) | `#[derive(Clone)]` |

**Nhận xét:** Rust ngắn gọn hơn nhờ derive macros. C++ phải viết manual hơn nhưng cung cấp `operator<` (cho `std::map`) mà Rust không cần.

### 1.2 Page

**Tương đương về chức năng.** Cả hai dùng big-endian encoding, format `[4-byte length][data]` cho bytes/strings.

| Tiêu chí | C++ (`page.cpp`) | Rust (`page.rs`) |
|----------|-------------------|-------------------|
| Bounds checking | Explicit `check_bounds()` throw `out_of_range` | Implicit qua Rust slice panics |
| get_string return | `std::string` (luôn thành công) | `Result<String, FromUtf8Error>` |
| get_bytes return | `const uint8_t*` (raw pointer) | `&[u8]` (safe slice) |
| Negative length check | `if (length < 0)` throw | Không cần (dùng `usize`) |
| contents() visibility | `public` | `pub(in crate)` - chỉ nội bộ |

**Khác biệt đáng chú ý:**

```cpp
// C++: get_bytes trả raw pointer - caller phải tự biết length
const uint8_t* Page::get_bytes(size_t offset) const {
    check_bounds(offset, 4);
    int32_t length = get_int(offset);
    if (length < 0) throw std::runtime_error("Invalid byte array length");
    check_bounds(offset + 4, static_cast<size_t>(length));
    return &bb_[offset + 4];  // Raw pointer, không kèm length
}
```

```rust
// Rust: get_bytes trả safe slice - length gắn liền với data
pub fn get_bytes(&self, offset: usize) -> &[u8] {
    let len = self.get_int(offset) as usize;
    &self.bb[offset + 4..offset + 4 + len]  // Slice = pointer + length
}
```

**Vấn đề C++:** `get_bytes` trả raw pointer mà không kèm length. Caller phải gọi `get_bytes_length()` riêng. Nếu quên gọi hoặc dùng sai length -> buffer overflow. Rust trả `&[u8]` slice đã bao gồm length.

**Vấn đề Rust:** `get_string` trả `Result` nhưng caller phải handle UTF-8 error. C++ bỏ qua validation UTF-8 (đơn giản hơn nhưng không safe).

### 1.3 FileMgr

| Tiêu chí | C++ (`filemgr.cpp`) | Rust (`filemgr.rs`) |
|----------|---------------------|---------------------|
| Constructor | Throw-on-error (implicit) | `Result<FileMgr, Error>` |
| Thread safety | `std::mutex` cho mọi method | Không có mutex |
| File caching | `open_files_: HashMap<name, block_count>` | Không cache - đọc metadata mỗi lần |
| read() khi file chưa có | **Silent return** - page giữ zeros | Trả `Ok(())` - page giữ zeros nếu file ngắn hơn |

**Khác biệt kiến trúc quan trọng:**

```cpp
// C++ FileMgr: Có mutex, cache file size
class FileMgr {
    mutable std::mutex mutex_;
    std::unordered_map<std::string, size_t> open_files_;  // Cache
};

void FileMgr::read(const BlockId& blk, Page& page) {
    std::lock_guard<std::mutex> lock(mutex_);   // Thread-safe
    if (!fs::exists(filepath)) return;           // Silent - page zeroed
    // ...
}

size_t FileMgr::length(const std::string& filename) {
    auto it = open_files_.find(filename);
    if (it != open_files_.end()) return it->second;  // Cache hit
    // ...
}
```

```rust
// Rust FileMgr: Không mutex, không cache
pub struct FileMgr {
    db_directory: String,
    blocksize: usize,
    is_new: bool,
    // Không có mutex, không có cache
}

pub fn read(&self, blk: &BlockId, p: &mut Page) -> Result<(), Error> {
    let mut f = self.get_file(blk.file_name())?;  // Mở file mỗi lần
    // ...
    if f.metadata()?.len() >= pos + p.contents().len() as u64 {
        f.read_exact(p.contents())?;               // Chỉ đọc nếu đủ size
    }
    Ok(())
}

pub fn length(&self, filename: &str) -> Result<usize, Error> {
    let file = self.get_file(filename)?;            // Mở file mỗi lần
    Ok(file.metadata()?.len() as usize / self.blocksize)
}
```

**Đánh giá Layer File:**

| | C++ tốt hơn | Rust tốt hơn | Tương đương |
|---|------------|-------------|------------|
| Thread safety | ✅ Có mutex | | |
| Performance (cache) | ✅ Cache file size | | |
| Error handling | | ✅ Result-based | |
| API safety (get_bytes) | | ✅ Slice thay raw ptr | |
| Temp file cleanup | | | ✅ |
| Constructor error | | ✅ Trả Result | |

**C++ thắng ở:** Thread safety (mutex) và performance (file size cache).
**Rust thắng ở:** Error handling và API safety.

---

## 2. Layer 2: Log Layer

### 2.1 LogMgr

| Tiêu chí | C++ (`logmgr.cpp`) | Rust (`logmgr.rs`) |
|----------|---------------------|---------------------|
| Constructor fail | Không thể fail (nhưng nên) | `Result<LogMgr, Error>` |
| append() return | `size_t` (LSN) | `Result<usize, Error>` |
| flush() return | `void` | `Result<(), Error>` |
| Shared ownership | `std::shared_ptr<FileMgr>` | `Arc<FileMgr>` |
| Thread safety | Không (comment ghi rõ) | Không (wrapper bên ngoài) |

**Logic gần như giống hệt nhau:**

```cpp
// C++ LogMgr::append
size_t LogMgr::append(const std::vector<uint8_t>& logrec) {
    int32_t boundary = logpage_.get_int(0);
    int32_t recsize = static_cast<int32_t>(logrec.size());
    int32_t bytesneeded = recsize + 4;
    if (boundary - bytesneeded < 4) {
        flush_impl();
        currentblk_ = append_new_block();
        boundary = logpage_.get_int(0);
    }
    int32_t recpos = boundary - bytesneeded;
    logpage_.set_bytes(static_cast<size_t>(recpos), logrec.data(), logrec.size());
    logpage_.set_int(0, recpos);
    latest_lsn_++;
    return latest_lsn_;
}
```

```rust
// Rust LogMgr::append
pub fn append(&mut self, logrec: &Vec<u8>) -> Result<usize, Error> {
    let mut boundary = self.logpage.get_int(0);
    let recsize = logrec.len() as i32;
    let bytesneeded = recsize + 4;
    if boundary - bytesneeded < 4 {
        self.flush_impl()?;
        self.currentblk = self.append_new_block()?;
        boundary = self.logpage.get_int(0);
    }
    let recpos = boundary - bytesneeded;
    self.logpage.set_bytes(recpos as usize, logrec);
    self.logpage.set_int(0, recpos);
    self.lastest_lsn += 1;
    Ok(self.lastest_lsn)
}
```

**Khác biệt nhỏ nhưng đáng chú ý:**

1. **Constructor initialization:** C++ phải khởi tạo `currentblk_("", 0)` dummy rồi gán lại - hơi hacky. Rust dùng `if/else` expression trả thẳng value.

```cpp
// C++: Phải dùng dummy BlockId("", 0) rồi gán lại
LogMgr::LogMgr(...)
    : currentblk_("", 0),  // Dummy - sẽ bị ghi đè
      latest_lsn_(0), last_saved_lsn_(0) {
    if (logsize == 0) {
        currentblk_ = append_new_block_helper(...);
    } else {
        currentblk_ = file::BlockId(logfile_, logsize - 1);
        fm_->read(currentblk_, logpage_);
    }
}
```

```rust
// Rust: Conditional expression, không cần dummy
let currentblk = if logsize == 0 {
    append_new_block(&fm, &mut logpage, logfile)?
} else {
    let currentblk = BlockId::new(logfile, logsize as i32 - 1);
    fm.read(&currentblk, &mut logpage)?;
    currentblk
};
```

2. **Error propagation trong flush:** C++ `flush_impl()` gọi `fm_->write()` mà write có thể throw - exception sẽ propagate ngầm. Rust explicit với `?`.

### 2.2 LogIterator

| Tiêu chí | C++ (`logiterator.cpp`) | Rust (`logiterator.rs`) |
|----------|-------------------------|-------------------------|
| Interface | Custom `has_next()` / `next()` | `impl Iterator` (standard trait) |
| next() khi hết | Throw `runtime_error` | Return `None` |
| Construction | Constructor trực tiếp | `Result<LogIterator, Error>` |

**Khác biệt quan trọng:**

```cpp
// C++: Custom iteration protocol
while (iter->has_next()) {
    auto rec = iter->next();  // Throw nếu gọi sai
    // ...
}
```

```rust
// Rust: Standard Iterator trait - dùng được for loop, collect, map...
for rec in log_mgr.iterator()? {
    // rec là Vec<u8>
}
// Hoặc: iter.map(...).filter(...).collect()
```

Rust `LogIterator` implement `Iterator` trait nên được dùng với toàn bộ standard library iterator combinators (map, filter, take, collect...). C++ phải viết while loop thủ công.

**Đánh giá Layer Log:**

| | C++ tốt hơn | Rust tốt hơn | Tương đương |
|---|------------|-------------|------------|
| Core logic | | | ✅ Giống nhau |
| Constructor init | | ✅ Không cần dummy | |
| Iterator integration | | ✅ Standard trait | |
| Error propagation | | ✅ Explicit Result | |

**Kết luận:** Logic giống nhau. Rust sạch hơn ở initialization và iterator design.

---

## 3. Layer 3: Buffer Layer

### 3.1 Buffer

| Tiêu chí | C++ (`buffer.cpp`) | Rust (`buffer.rs`) |
|----------|---------------------|---------------------|
| LogMgr access | `shared_ptr<LogMgr>` (shared, unguarded) | `Arc<Mutex<LogMgr>>` (shared, guarded) |
| assign_to_block return | `void` (exception on error) | `Result<(), Error>` |
| flush() return | `void` | `Result<(), Error>` |
| Visibility control | Comment "Package-private" | `pub(in crate::buffer)` compiler-enforced |

**Khác biệt thiết kế quan trọng:**

```cpp
// C++: assign_to_block và flush không trả error
void Buffer::assign_to_block(const file::BlockId& blk) {
    flush();              // Có thể throw - nhưng caller không biết
    blk_ = blk;
    fm_->read(blk, contents_);  // Có thể throw
    pins_ = 0;
}

void Buffer::flush() {
    if (txnum_.has_value()) {
        if (lsn_.has_value()) {
            lm_->flush(lsn_.value());   // LogMgr không thread-safe!
        }
        if (blk_.has_value()) {
            fm_->write(blk_.value(), contents_);
        }
        txnum_ = std::nullopt;
    }
}
```

```rust
// Rust: Mọi I/O đều trả Result, LogMgr phải lock
pub(in crate::buffer) fn assign_to_block(&mut self, b: BlockId) -> Result<(), Error> {
    self.flush()?;        // Error propagated
    self.blk = Some(b.clone());
    self.fm.read(&b, &mut self.contents)?;
    self.pins = 0;
    Ok(())
}

pub(in crate::buffer) fn flush(&mut self) -> Result<(), Error> {
    if self.txnum.is_some() {
        if let Some(lsn) = self.lsn {
            self.lm.lock().unwrap().flush(lsn)?;  // Phải lock LogMgr
        }
        if let Some(ref blk) = self.blk {
            self.fm.write(blk, &mut self.contents)?;
        }
        self.txnum = None;
    }
    Ok(())
}
```

**Vấn đề C++ quan trọng:** `Buffer::flush()` gọi `lm_->flush()` trên `shared_ptr<LogMgr>`. LogMgr không thread-safe (ghi rõ trong comment). Nếu 2 buffers cùng flush song song -> data race trên LogMgr. Rust bắt buộc `lm.lock().unwrap()` nên không thể quên.

**Visibility:** C++ chỉ ghi comment "Package-private" - compiler không enforce. Rust dùng `pub(in crate::buffer)` - compile error nếu gọi từ ngoài module buffer.

### 3.2 BufferMgr

| Tiêu chí | C++ (`buffermgr.cpp`) | Rust (`buffermgr.rs`) |
|----------|------------------------|------------------------|
| pin() return | `size_t` (throw on timeout) | `Result<usize, AbortError>` |
| Wait mechanism | `sleep_for(100ms)` polling | `park_timeout(max_time)` |
| Error type | `BufferAbortException` (1 loại) | `AbortError` enum (Time/IO/General) |
| flush_all return | `void` | `Result<(), Error>` |

**Khác biệt wait mechanism:**

```cpp
// C++: Busy-polling mỗi 100ms
size_t BufferMgr::pin(const file::BlockId& blk) {
    auto start_time = std::chrono::steady_clock::now();
    std::optional<size_t> idx = try_to_pin(blk);
    while (!idx.has_value() && !waiting_too_long(start_time)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Busy poll
        idx = try_to_pin(blk);
    }
    if (!idx.has_value()) throw BufferAbortException();
    return idx.value();
}
```

```rust
// Rust: Thread parking - ngủ đến khi được đánh thức hoặc timeout
pub fn pin(&mut self, blk: &BlockId) -> Result<usize, AbortError> {
    let timestamp = SystemTime::now();
    let mut idx = self.try_to_pin(blk)?;
    while idx.is_none() && !waiting_too_long(timestamp, self.max_time)? {
        park_timeout(Duration::from_millis(self.max_time as u64));  // Park
        idx = self.try_to_pin(blk)?;
    }
    match idx {
        Some(idx) => Ok(idx),
        None => Err(AbortError::General),
    }
}
```

**Nhận xét:** C++ polling 100ms gây overhead không cần thiết. Tuy nhiên, cả hai đều chưa optimal - lý tưởng nên dùng condition variable (C++) hoặc `Condvar` (Rust).

**Error type richness:**

```cpp
// C++: Chỉ 1 loại exception, chỉ 1 message
class BufferAbortException : public std::runtime_error {
    BufferAbortException()
        : std::runtime_error("Buffer abort: pool exhausted after timeout") {}
};
```

```rust
// Rust: Phân biệt nguyên nhân lỗi
pub enum AbortError {
    Time(SystemTimeError),  // Lỗi system time
    IO(Error),              // Lỗi I/O khi assign_to_block
    General,                // Timeout hết pool
}
```

C++ gộp mọi lỗi vào 1 message string. Rust phân biệt rõ: timeout? I/O error? system time error? - giúp caller xử lý chính xác hơn.

**Đánh giá Layer Buffer:**

| | C++ tốt hơn | Rust tốt hơn | Tương đương |
|---|------------|-------------|------------|
| Core pin/unpin logic | | | ✅ |
| Error granularity | | ✅ AbortError enum | |
| LogMgr access safety | | ✅ Mutex-guarded | |
| Visibility enforcement | | ✅ `pub(in crate)` | |
| Wait mechanism | | ≈ (cả hai chưa optimal) | |
| flush_all error handling | | ✅ Result | |

---

## 4. Layer 4: Record Layer

### 4.1 Schema & Layout

**Gần như tương đương hoàn toàn.** Cùng data structures, cùng logic.

| Tiêu chí | C++ | Rust |
|----------|-----|------|
| Type enum | `Type::INTEGER = 4, VARCHAR = 12` | `Type::Integer = 4, Varchar = 12` |
| Field storage | `vector<string>` + `unordered_map<string, FieldInfo>` | `Vec<String>` + `HashMap<String, FieldInfo>` |
| Layout ownership | `shared_ptr<Schema>` | `Arc<Schema>` |
| Layout cloneable | Không (manual copy) | `#[derive(Clone)]` |

Khác biệt nhỏ: Rust `Layout` implement `Clone` cho phép cheap copy (vì `Arc` clone chỉ tăng refcount). C++ không implement copy nên phải pass by reference hoặc dùng `shared_ptr<Layout>`.

### 4.2 RecordPage - **Khác biệt kiến trúc lớn nhất**

Đây là nơi hai version phân tách rõ nhất:

```cpp
// C++ RecordPage: Truy cập TRỰC TIẾP Buffer
class RecordPage {
    buffer::Buffer& buff_;   // Reference trực tiếp vào buffer
    Layout layout_;
};

// Đọc/ghi data trực tiếp lên buffer page
int32_t RecordPage::get_int(size_t slot, const std::string& fldname) {
    size_t fldpos = offset(slot) + layout_.offset(fldname);
    return buff_.contents().get_int(fldpos);  // Trực tiếp
}

void RecordPage::set_int(size_t slot, const std::string& fldname, int32_t val) {
    size_t fldpos = offset(slot) + layout_.offset(fldname);
    buff_.contents().set_int(fldpos, val);     // Trực tiếp
    buff_.set_modified(0, std::nullopt);       // Hardcode txnum=0, no LSN
}
```

```rust
// Rust RecordPage: Truy cập QUA Transaction
pub struct RecordPage {
    tx: Arc<Mutex<Transaction>>,  // Qua transaction layer
    blk: BlockId,
    layout: Layout,
}

// Đọc/ghi qua Transaction -> Transaction acquire lock -> rồi mới đọc buffer
pub fn get_int(&mut self, slot: usize, fldname: &str) -> Result<i32, TransactionError> {
    let fldpos = self.offset(slot) + self.layout.offset(fldname);
    self.tx.lock().unwrap().get_int(&self.blk, fldpos)  // Qua Transaction
}

pub fn set_int(&mut self, slot: usize, fldname: &str, val: i32) -> Result<(), TransactionError> {
    let fldpos = self.offset(slot) + self.layout.offset(fldname);
    self.tx.lock().unwrap().set_int(&self.blk, fldpos, val, true)  // Qua Transaction, có logging
}
```

**Hệ quả của khác biệt này:**

| Hệ quả | C++ (Direct Buffer) | Rust (Via Transaction) |
|---------|---------------------|------------------------|
| WAL logging khi write | Không (hardcode `txnum=0, lsn=nullopt`) | Có (Transaction write log trước data) |
| Concurrency control | Không | Transaction acquire S/X lock |
| Crash recovery | Không thể rollback | Transaction rollback undo changes |
| set_modified marker | Hardcode `(0, nullopt)` | Actual txnum + LSN từ log |

C++ ghi `set_modified(0, std::nullopt)` cho MỌI write operation - tức là:
- `txnum = 0` luôn -> không phân biệt transaction nào sửa
- `lsn = nullopt` luôn -> không link tới log record nào
- Khi flush, Buffer sẽ gọi `lm_->flush(lsn)` nhưng lsn là nullopt -> skip log flush -> **vi phạm WAL protocol**

### 4.3 TableScan

| Tiêu chí | C++ (`tablescan.cpp`) | Rust (`tablescan.rs`) |
|----------|------------------------|------------------------|
| Dependency | `shared_ptr<BufferMgr>` | `Arc<Mutex<Transaction>>` |
| Interface | Inherits `Scan` (virtual) | Implements `ScanControl` + `UpdateScanControl` |
| Error handling | Void/throw | `Result<T, TransactionError>` |
| Current record page | `unique_ptr<RecordPage>` | `Option<RecordPage>` |
| Pin/unpin | Manual via BufferMgr | Automatic via Transaction |

**Khác biệt close():**

```cpp
// C++: Phải tự quản lý buffer index
void TableScan::close() {
    if (current_buffer_idx_.has_value()) {
        bm_->unpin(current_buffer_idx_.value());  // Manual unpin
        current_buffer_idx_ = std::nullopt;
    }
}
```

```rust
// Rust: Unpin qua Transaction
fn close(&mut self) -> Result<(), AbortError> {
    if let Some(rp) = &self.rp {
        self.tx.lock().unwrap().unpin(rp.block())?;  // Transaction quản lý
    }
    Ok(())
}
```

C++ phải track `current_buffer_idx_` riêng (thêm 1 field state). Rust delegate cho Transaction.

**Khác biệt next():**

```cpp
// C++: Clean, concise, nhưng không handle errors
bool TableScan::next() {
    currentslot_ = rp_->next_after(currentslot_);
    while (!currentslot_.has_value()) {
        if (at_last_block()) return false;
        move_to_block(rp_->block().number() + 1);
        currentslot_ = rp_->next_after(currentslot_);
    }
    return true;
}
```

```rust
// Rust: Verbose hơn vì phải handle Option<RecordPage> + Result
fn next(&mut self) -> Result<bool, TransactionError> {
    if let Some(rp) = &mut self.rp {
        self.currentslot = rp.next_after(self.currentslot)?;
    } else {
        return Err(TransactionError::General);
    }
    while self.currentslot.is_none() {
        if self.at_last_block()? {
            return Ok(false);
        }
        let mut blknum = None;
        if let Some(rp) = &self.rp {
            blknum = Some(rp.block().number() + 1);
        }
        // ... more pattern matching
    }
    Ok(true)
}
```

**Nhận xét:** C++ `next()` đọc rõ ràng, logic flow dễ hiểu hơn. Rust verbose vì:
1. `rp` là `Option<RecordPage>` nên phải `if let Some` mỗi lần truy cập
2. Borrow checker không cho mượn `self.rp` mutable rồi lại gọi `self.move_to_block()` -> phải extract `blknum` ra biến tạm trước

Đây là trade-off thực tế: C++ code sạch hơn ở chỗ này, nhưng Rust buộc handle null case.

**Đánh giá Layer Record:**

| | C++ tốt hơn | Rust tốt hơn | Tương đương |
|---|------------|-------------|------------|
| Schema/Layout | | | ✅ |
| RecordPage correctness | | ✅ WAL đúng qua Transaction | |
| TableScan readability | ✅ Clean hơn | | |
| Error safety | | ✅ Result everywhere | |
| Null state handling | | ✅ Compiler-checked | |

---

## 5. Tổng Kết So Sánh Ngang Hàng

### 5.1 C++ Tốt Hơn Ở Đâu

1. **FileMgr có mutex + file size cache** - Rust FileMgr không có cả hai. Mỗi lần gọi `length()`, Rust phải mở file rồi đọc metadata. C++ cache trong `open_files_` map.

2. **Code readability ở TableScan** - Logic flow trong `next()`, `insert()` của C++ ngắn gọn, dễ đọc hơn. Rust bị verbose do borrow checker constraints.

3. **Consistent style** - C++ code style đồng nhất, documentation comment chi tiết cho mọi method.

### 5.2 Rust Tốt Hơn Ở Đâu

1. **RecordPage qua Transaction vs trực tiếp Buffer** - Đây là khác biệt lớn nhất. C++ RecordPage ghi `set_modified(0, nullopt)` cho mọi write -> **vi phạm WAL protocol**. Rust RecordPage qua Transaction nên WAL, locking, recovery đều đúng.

2. **Error handling nhất quán** - Mọi I/O operation trong Rust trả `Result`. C++:
   - `FileMgr::read()` silent return khi file chưa có
   - `Buffer::flush()` trả void nhưng bên trong có thể throw
   - `BufferMgr::flush_all()` trả void nhưng buffer.flush() có thể throw

3. **API safety** - `Page::get_bytes()` trả raw pointer (C++) vs slice (Rust). `pub(in crate::buffer)` vs comment "package-private".

4. **LogIterator implement Iterator trait** - Rust LogIterator dùng được với standard combinators. C++ phải viết while loop.

5. **Error type hierarchy** - `AbortError{Time, IO, General}` phân biệt nguyên nhân. C++ chỉ có 1 string message.

### 5.3 Tương Đương

1. **Core algorithms** - Backward-growing WAL, pin/unpin protocol, slot-based record storage, sequential table scan - logic giống hệt nhau.

2. **Data encoding** - Big-endian integers, `[4-byte length][data]` format cho strings.

3. **Eviction strategy** - Cả hai dùng naive "first unpinned" scan.

---

## 6. Các Vấn Đề Cụ Thể Cần Lưu Ý

### 6.1 WAL Violation trong C++ RecordPage

```cpp
// recordpage.cpp:21 - HARDCODED, không có actual logging
buff_.set_modified(0, std::nullopt);
```

Khi Phase 5 implement Transaction, mọi call site `set_modified(0, nullopt)` phải đổi thành actual txnum + LSN từ log record. Hiện có **6 chỗ** trong `recordpage.cpp` cần sửa.

### 6.2 Silent Failure trong C++ FileMgr::read()

```cpp
// filemgr.cpp:66-69
if (!fs::exists(filepath)) {
    return;  // Page giữ zeros, caller không biết file chưa tồn tại
}
```

Trong Rust, nếu file chưa tồn tại, `get_file()` sẽ tạo file mới (do flag `create(true)`), rồi `read_exact` sẽ đọc zeros. Khác biệt nhỏ nhưng behavior rõ ràng hơn.

### 6.3 Rust RecordPage - Excessive Locking

```rust
// Mỗi get/set đều lock Transaction
pub fn get_int(&mut self, slot: usize, fldname: &str) -> Result<i32, TransactionError> {
    self.tx.lock().unwrap().get_int(&self.blk, fldpos)  // Lock mỗi field access
}
```

Với bảng có 5 fields, đọc 1 row = 5 lần `lock().unwrap()`. Đây là overhead. C++ không có overhead này (truy cập trực tiếp buffer).

### 6.4 Rust LogIterator - Swallowed Error

```rust
// logiterator.rs - .ok() bỏ qua lỗi
self.move_to_block(&self.blk.clone()).ok();  // Error silently ignored
```

`Iterator::next()` trả `Option<T>`, không có chỗ cho `Error`. Khi `move_to_block` fail (I/O error), lỗi bị `.ok()` nuốt mất. Đây là design limitation của standard Iterator trait.

---

## 7. Bảng Tổng Kết

| Layer | Component | C++ Quality | Rust Quality | Winner | Lý do |
|-------|-----------|-------------|-------------|--------|-------|
| File | BlockId | ★★★★ | ★★★★ | Tie | Cùng chức năng |
| File | Page | ★★★★ | ★★★★½ | Rust | Slice > raw pointer |
| File | FileMgr | ★★★★½ | ★★★★ | **C++** | Mutex + cache |
| Log | LogMgr | ★★★★ | ★★★★½ | Rust | Constructor init + error |
| Log | LogIterator | ★★★½ | ★★★★ | Rust | Standard Iterator trait |
| Buffer | Buffer | ★★★½ | ★★★★ | Rust | Mutex LogMgr, visibility |
| Buffer | BufferMgr | ★★★★ | ★★★★ | Tie | Cùng logic, error type Rust tốt hơn nhưng cả hai wait chưa optimal |
| Record | Schema/Layout | ★★★★ | ★★★★ | Tie | Gần giống nhau |
| Record | RecordPage | ★★★ | ★★★★½ | **Rust** | WAL-compliant qua Transaction |
| Record | TableScan | ★★★★ | ★★★½ | **C++** | Readability, clean flow |

**Tổng:** C++ thắng 2, Rust thắng 4, Hòa 4.

Rust thắng chủ yếu ở **correctness** (error handling, WAL compliance). C++ thắng ở **pragmatics** (thread safety cho FileMgr, readability cho TableScan).

---

**Phiên bản tài liệu:** 2.0 - Focused on completed layers only
**Ngày tạo:** 7 tháng 2, 2026

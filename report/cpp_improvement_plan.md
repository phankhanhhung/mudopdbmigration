# MudopDB_v1 (C++) - Improvement Plan

**Ngay:** 8 thang 2, 2026
**Trang thai:** Da implement P1-P4, build thanh cong, 179/179 tests pass

---

## Tong Quan

Dua tren review kien truc v1 (C++) vs v2 (Rust), tai lieu nay tong hop cac van de cua C++ codebase va phuong an khac phuc. Tat ca cac fix da duoc implement va test.

---

## P1: Thread Safety (CRITICAL)

### P1a: FileMgr::length() - Khong dung mutex

**Van de:** `FileMgr` co `mutex_` nhung `length()` KHONG dung no. Doc/ghi `open_files_` cache khong dong bo.

**File:** `src/file/filemgr.cpp:148-168`

**Phuong an:**
- Tach `length()` thanh 2 methods: `length()` (public, co lock) va `length_impl()` (private, khong lock)
- `append()` da giu lock nen goi `length_impl()` de tranh deadlock
- Them `length_impl` vao header

**Trang thai:** DA IMPLEMENT

### P1b: LogMgr - Hoan toan khong co mutex

**Van de:** LogMgr duoc share qua `shared_ptr` giua nhieu Buffer. Header ghi "NOT thread-safe" nhung khong ai synchronize. 2 buffers flush song song = data race tren `latest_lsn_`, `last_saved_lsn_`, `logpage_`.

**Files:** `include/log/logmgr.hpp`, `src/log/logmgr.cpp`

**Phuong an:**
- Them `mutable std::mutex mutex_` vao LogMgr
- Them `std::lock_guard<std::mutex>` vao `append()`, `flush()`, `iterator()`
- Cap nhat documentation: "Thread-safe. All public methods protected by internal mutex."

**Trang thai:** DA IMPLEMENT

### P1c: BufferMgr - Khong co mutex, busy-polling 100ms

**Van de:**
1. `num_available_` counter doc/ghi khong co lock
2. `pin()` dung `sleep_for(100ms)` polling - ton CPU, khong wake up khi co buffer available

**Files:** `include/buffer/buffermgr.hpp`, `src/buffer/buffermgr.cpp`

**Phuong an:**
- Them `mutable std::mutex mutex_` va `std::condition_variable cv_`
- `available()`, `flush_all()`, `pin()`, `unpin()` deu co lock
- `pin()` dung `cv_.wait_for()` thay vi `sleep_for(100ms)`
- `unpin()` goi `cv_.notify_all()` khi buffer tro thanh available

**Trang thai:** DA IMPLEMENT

### P1d: ConcurrencyMgr - TOCTOU race + x_lock khong goi global lock table

**Van de:**
1. `s_lock()`: Check `locks_.find(blk)` NGOAI lock, action TRONG lock = TOCTOU race
2. `x_lock()`: Chi ghi nhan "X" o local map, KHONG goi `get_lock_table().x_lock()` tren global table

**Files:** `src/tx/concurrency/concurrencymgr.cpp`, `include/tx/concurrency/locktable.hpp`, `src/tx/concurrency/locktable.cpp`

**Phuong an:**
- Di chuyen check VAO trong lock_guard scope (atomic check-then-act)
- Them `LockTable::x_lock()` method: wait cho other S locks release, set value = -1
- `ConcurrencyMgr::x_lock()` goi `get_lock_table().x_lock()` truoc khi ghi nhan "X"

**Trang thai:** DA IMPLEMENT

### P1e: StatMgr - Khong co mutex

**Van de:** `tablestats_` cache va `numcalls_` counter truy cap khong dong bo. Concurrent `get_stat_info()` calls co the gay data race.

**Files:** `include/metadata/statmgr.hpp`, `src/metadata/statmgr.cpp`

**Phuong an:**
- Them `mutable std::mutex mutex_` vao StatMgr
- `get_stat_info()` co `lock_guard` bao ve `numcalls_++` va `tablestats_` access

**Trang thai:** DA IMPLEMENT

---

## P2: Error Handling

### P2a: API layer - catch(...) nuot loi

**Van de:** 5 catch blocks trong `statement.cpp` va `result_set.cpp` dung `catch (...)` nuot moi exception:
- `execute_query()`: catch -> rollback -> throw generic error (mat root cause)
- `execute_update()`: catch -> rollback -> throw generic error
- `next()`: catch -> rollback -> return false (caller khong biet co loi)
- `get_int()`: catch -> rollback -> return 0 (caller nhan gia tri sai)
- `get_string()`: catch -> rollback -> return "" (caller nhan gia tri sai)

**Files:** `src/api/statement.cpp`, `src/api/result_set.cpp`

**Phuong an:**
- Doi `catch (...)` thanh `catch (const std::exception& e)` de bao toan error message
- Statement: rollback + re-throw voi context message
- ResultSet `get_int`/`get_string`: throw khi khong co scan (thay vi tra default)
- ResultSet `next()`: throw khi scan error (thay vi tra false)
- Cap nhat 5 test cases de expect throw thay vi expect default values

**Trang thai:** DA IMPLEMENT (5 tests updated)

---

## P3: Missing Optimizer Components

### P3a: BetterQueryPlanner

**Van de:** Rust co `BetterQueryPlanner` danh gia CA HAI join orders va chon order re hon. C++ chi co `BasicQueryPlanner` (luon left-deep).

**Files moi:**
- `include/plan/betterqueryplanner.hpp`
- `src/plan/betterqueryplanner.cpp`

**Phuong an:**
- Clone tu `BasicQueryPlanner`, chi doi vong loop join:
  ```cpp
  // BasicQueryPlanner (cu):
  p = std::make_shared<ProductPlan>(p, plans[i]);

  // BetterQueryPlanner (moi):
  auto choice1 = std::make_shared<ProductPlan>(plans[i], p);
  auto choice2 = std::make_shared<ProductPlan>(p, plans[i]);
  p = (choice1->blocks_accessed() < choice2->blocks_accessed())
      ? choice1 : choice2;
  ```

**Trang thai:** DA IMPLEMENT

### P3b: OptimizedProductPlan

**Van de:** Rust co `OptimizedProductPlan` - wrapper tao ca 2 product orders va chon order tot nhat tai construction time.

**Files moi:**
- `include/plan/optimizedproductplan.hpp`
- `src/plan/optimizedproductplan.cpp`

**Phuong an:**
- Wrapper class implement `Plan` interface
- Constructor: tao 2 `ProductPlan`, giu `bestplan_` la plan co `blocks_accessed()` nho hon
- Delegate moi method sang `bestplan_`

**Trang thai:** DA IMPLEMENT

---

## P4: Visibility Enforcement

### P4: Buffer private methods + friend class

**Van de:** `Buffer::assign_to_block()`, `flush()`, `pin()`, `unpin()` duoc ghi comment "Package-private" nhung compiler khong enforce. Bat ky code nao cung co the goi truc tiep.

**File:** `include/buffer/buffer.hpp`

**Phuong an:**
- Them `friend class BufferMgr` vao Buffer class
- Giu methods public vi tests can truy cap truc tiep, nhung them comment "Internal: only BufferMgr should call these in production code"
- Luu y: Neu muon strict enforcement, can them `friend class BufferTest` cho test fixture

**Trang thai:** DA IMPLEMENT (partial - comment-based vi test compatibility)

---

## Tong Ket Thay Doi

| File | Thay doi |
|------|---------|
| `include/file/filemgr.hpp` | Them `length_impl()` declaration |
| `src/file/filemgr.cpp` | Tach `length()` / `length_impl()`, lock `length()` |
| `include/log/logmgr.hpp` | Them `mutex_`, cap nhat doc |
| `src/log/logmgr.cpp` | Lock `append()`, `flush()`, `iterator()` |
| `include/buffer/buffermgr.hpp` | Them `mutex_`, `cv_`, cap nhat doc |
| `src/buffer/buffermgr.cpp` | Lock tat ca public methods, `cv_.wait_for`, `cv_.notify_all` |
| `include/buffer/buffer.hpp` | Them `friend class BufferMgr`, re-organize visibility |
| `src/tx/concurrency/concurrencymgr.cpp` | Fix TOCTOU, goi `x_lock()` tren global table |
| `include/tx/concurrency/locktable.hpp` | Them `x_lock()` declaration |
| `src/tx/concurrency/locktable.cpp` | Implement `x_lock()` (wait + set -1) |
| `include/metadata/statmgr.hpp` | Them `mutex_` |
| `src/metadata/statmgr.cpp` | Lock `get_stat_info()` |
| `src/api/statement.cpp` | catch(exception) thay catch(...), propagate error msg |
| `src/api/result_set.cpp` | Throw thay vi return defaults |
| `include/plan/betterqueryplanner.hpp` | **MOI** - BetterQueryPlanner header |
| `src/plan/betterqueryplanner.cpp` | **MOI** - BetterQueryPlanner implementation |
| `include/plan/optimizedproductplan.hpp` | **MOI** - OptimizedProductPlan header |
| `src/plan/optimizedproductplan.cpp` | **MOI** - OptimizedProductPlan implementation |
| `tests/test_resultset.cpp` | 5 tests updated: expect throw thay vi expect defaults |

**Ket qua build:** 179/179 tests PASSED

---

## Van De Chua Xu Ly (Tuong Lai)

| Van de | Do uu tien | Ly do chua lam |
|--------|-----------|----------------|
| Scan interface doi sang Result-based | Thap | Thay doi 35+ methods, can migration plan |
| std::variant dispatch thay vi vtable | Rat thap | Khong dang - mat extensibility, loi nho |
| Lock type dung enum thay vi String | Trung binh | Both C++ va Rust co bug nay |
| Lock fairness / starvation prevention | Trung binh | Can condition_variable + queue |
| Full network transport (gRPC) | Cao | Lon, can separate project |
| Scan interface: Result return types | Trung binh | Breaking change |

---

**Phien ban tai lieu:** 1.0
**Ngay tao:** 8 thang 2, 2026

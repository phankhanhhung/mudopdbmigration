# Phase 4: Record Management - Detailed Component Specification

**Date:** October 12, 2025
**Status:** 🔲 Planned (Phase 1-3 Complete)
**Estimated Effort:** 4-5 days, ~2000 lines of code, ~30 tests

---

## Overview

The record management layer provides table storage and access mechanisms. It manages the physical layout of records on pages and provides scan interfaces for reading and writing table data.

### Key Responsibilities
1. **Schema Definition:** Define table structure (field names, types, lengths)
2. **Physical Layout:** Calculate field offsets and record sizes
3. **Record Storage:** Manage records within pages (slot-based)
4. **Table Scanning:** Sequential access to table records
5. **Record Identification:** Unique record IDs (RID) for direct access

---

## Component Summary

Phase 4 consists of **5 components**:

| Component | Purpose | Dependencies | Est. Lines |
|-----------|---------|--------------|------------|
| **Schema** | Table schema definition | None | ~150 |
| **Layout** | Physical record layout | Schema, Page | ~100 |
| **RID** | Record identifier | None | ~50 |
| **RecordPage** | Record management in page | Layout, Buffer (via Transaction) | ~400 |
| **TableScan** | Table scan operations | RecordPage, RID, Layout | ~600 |

**Total:** ~1300 lines implementation + ~700 lines tests = **~2000 lines**

---

## Component 1: Schema

**Files to Create:**
- `include/record/schema.hpp`
- `src/record/schema.cpp`

### Purpose
Defines the logical structure of a table: field names, types, and lengths.

### API Specification

```cpp
#ifndef SCHEMA_HPP
#define SCHEMA_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace record {

/**
 * Type enumeration for field types.
 */
enum class Type {
    INTEGER = 4,   // 4 bytes
    VARCHAR = 12   // Variable length (max specified in schema)
};

/**
 * Schema defines the structure of a table.
 *
 * A schema consists of:
 * - Ordered list of field names
 * - Type and length for each field
 *
 * Corresponds to Schema in Rust (NMDB2/src/record/schema.rs)
 */
class Schema {
public:
    Schema();

    /**
     * Adds a field to the schema.
     *
     * @param fldname the field name
     * @param type the field type
     * @param length the field length (for VARCHAR, 0 for INTEGER)
     */
    void add_field(const std::string& fldname, Type type, size_t length);

    /**
     * Adds an integer field.
     *
     * @param fldname the field name
     */
    void add_int_field(const std::string& fldname);

    /**
     * Adds a string field.
     *
     * @param fldname the field name
     * @param length the maximum string length
     */
    void add_string_field(const std::string& fldname, size_t length);

    /**
     * Adds a field from another schema.
     *
     * @param fldname the field name
     * @param sch the source schema
     */
    void add(const std::string& fldname, const Schema& sch);

    /**
     * Adds all fields from another schema.
     *
     * @param sch the source schema
     */
    void add_all(const Schema& sch);

    /**
     * Returns the ordered list of field names.
     *
     * @return vector of field names
     */
    const std::vector<std::string>& fields() const;

    /**
     * Checks if schema has a field.
     *
     * @param fldname the field name
     * @return true if field exists
     */
    bool has_field(const std::string& fldname) const;

    /**
     * Returns the type of a field.
     *
     * @param fldname the field name
     * @return the field type
     */
    Type type(const std::string& fldname) const;

    /**
     * Returns the length of a field.
     *
     * @param fldname the field name
     * @return the field length (for VARCHAR, 0 for INTEGER)
     */
    size_t length(const std::string& fldname) const;

private:
    struct FieldInfo {
        Type type;
        size_t length;
    };

    std::vector<std::string> fields_;
    std::unordered_map<std::string, FieldInfo> info_;
};

} // namespace record

#endif // SCHEMA_HPP
```

### Test Coverage (6 tests)

```cpp
TEST(SchemaTest, CreateEmpty)
TEST(SchemaTest, AddIntField)
TEST(SchemaTest, AddStringField)
TEST(SchemaTest, AddFromOtherSchema)
TEST(SchemaTest, AddAllFields)
TEST(SchemaTest, FieldOrder)  // Verify fields() returns insertion order
```

---

## Component 2: Layout

**Files to Create:**
- `include/record/layout.hpp`
- `src/record/layout.cpp`

### Purpose
Calculates the physical layout of records: field offsets and total record size (slot size).

### Record Format

```
Slot Layout:
[4-byte flag][field1][field2][...]

- Flag: 0 = EMPTY, 1 = USED
- Fields stored at calculated offsets
- Integer: 4 bytes
- Varchar: 4-byte length + data (max N bytes)
```

### API Specification

```cpp
#ifndef LAYOUT_HPP
#define LAYOUT_HPP

#include "record/schema.hpp"
#include <memory>
#include <unordered_map>

namespace record {

/**
 * Layout describes the physical layout of a record.
 *
 * Calculates:
 * - Offset of each field within a record slot
 * - Total slot size (4-byte flag + all fields)
 *
 * Corresponds to Layout in Rust (NMDB2/src/record/layout.rs)
 */
class Layout {
public:
    /**
     * Creates a layout from a schema.
     * Automatically calculates field offsets.
     *
     * @param schema the table schema
     */
    explicit Layout(std::shared_ptr<Schema> schema);

    /**
     * Creates a layout with explicit metadata.
     * (Used for deserialization from metadata catalog)
     *
     * @param schema the table schema
     * @param offsets field name to offset map
     * @param slotsize the total slot size
     */
    Layout(std::shared_ptr<Schema> schema,
           std::unordered_map<std::string, size_t> offsets,
           size_t slotsize);

    /**
     * Returns the schema.
     *
     * @return shared pointer to schema
     */
    std::shared_ptr<Schema> schema() const;

    /**
     * Returns the offset of a field within a slot.
     *
     * @param fldname the field name
     * @return the byte offset
     */
    size_t offset(const std::string& fldname) const;

    /**
     * Returns the total size of a record slot.
     *
     * @return slot size in bytes
     */
    size_t slot_size() const;

private:
    /**
     * Calculates the storage size of a field.
     *
     * @param fldname the field name
     * @return bytes needed to store the field
     */
    size_t length_in_bytes(const std::string& fldname) const;

private:
    std::shared_ptr<Schema> schema_;
    std::unordered_map<std::string, size_t> offsets_;
    size_t slotsize_;
};

} // namespace record

#endif // LAYOUT_HPP
```

### Implementation Notes

```cpp
Layout::Layout(std::shared_ptr<Schema> schema)
    : schema_(schema), slotsize_(4) {  // Start with 4-byte flag

    for (const auto& fldname : schema_->fields()) {
        offsets_[fldname] = slotsize_;
        slotsize_ += length_in_bytes(fldname);
    }
}

size_t Layout::length_in_bytes(const std::string& fldname) const {
    Type fldtype = schema_->type(fldname);

    switch (fldtype) {
        case Type::INTEGER:
            return 4;
        case Type::VARCHAR:
            return file::Page::max_length(schema_->length(fldname));
    }
}
```

### Test Coverage (5 tests)

```cpp
TEST(LayoutTest, CreateFromSchema)
TEST(LayoutTest, FieldOffsets)
TEST(LayoutTest, SlotSize)
TEST(LayoutTest, IntegerFieldSize)
TEST(LayoutTest, VarcharFieldSize)  // Verify max_length calculation
```

---

## Component 3: RID (Record ID)

**Files to Create:**
- `include/record/rid.hpp`
- `src/record/rid.cpp`

### Purpose
Uniquely identifies a record by its block number and slot within that block.

### API Specification

```cpp
#ifndef RID_HPP
#define RID_HPP

#include <string>
#include <cstdint>

namespace record {

/**
 * RID (Record ID) uniquely identifies a record.
 *
 * Format: [block_number, slot]
 *
 * Corresponds to Rid in Rust (NMDB2/src/record/rid.rs)
 */
class RID {
public:
    /**
     * Creates a record ID.
     *
     * @param blknum the block number
     * @param slot the slot number within the block
     */
    RID(int32_t blknum, size_t slot);

    /**
     * Returns the block number.
     *
     * @return block number
     */
    int32_t block_number() const;

    /**
     * Returns the slot number.
     *
     * @return slot number
     */
    size_t slot() const;

    /**
     * Returns string representation.
     *
     * @return format: "[blknum, slot]"
     */
    std::string to_string() const;

    // Equality operators
    bool operator==(const RID& other) const;
    bool operator!=(const RID& other) const;

private:
    int32_t blknum_;
    size_t slot_;
};

} // namespace record

#endif // RID_HPP
```

### Test Coverage (3 tests)

```cpp
TEST(RIDTest, Construction)
TEST(RIDTest, Equality)
TEST(RIDTest, ToString)
```

---

## Component 4: RecordPage

**Files to Create:**
- `include/record/recordpage.hpp`
- `src/record/recordpage.cpp`

### Purpose
Manages records within a single page using slot-based storage.

### Key Features
- Slot-based record storage (fixed-size slots)
- Empty/Used flag per slot (4 bytes at start of each slot)
- Insert/delete operations
- Field get/set operations
- Format page (initialize all slots to EMPTY)

### API Specification

```cpp
#ifndef RECORDPAGE_HPP
#define RECORDPAGE_HPP

#include "record/layout.hpp"
#include "buffer/buffer.hpp"
#include "file/blockid.hpp"
#include <memory>
#include <optional>

namespace record {

/**
 * RecordPage manages records within a single page.
 *
 * Page Format:
 * [Slot 0: flag + fields][Slot 1: flag + fields][...]
 *
 * Flag: 0 = EMPTY, 1 = USED
 *
 * NOTE: Phase 4 version does NOT use Transaction layer (Phase 5).
 * Instead, directly uses Buffer for simplicity.
 *
 * Corresponds to RecordPage in Rust (NMDB2/src/record/recordpage.rs)
 */
class RecordPage {
public:
    /**
     * Creates a record page for a block.
     *
     * @param buff the buffer containing the page
     * @param layout the record layout
     */
    RecordPage(buffer::Buffer& buff, const Layout& layout);

    /**
     * Gets an integer field value.
     *
     * @param slot the slot number
     * @param fldname the field name
     * @return the integer value
     */
    int32_t get_int(size_t slot, const std::string& fldname);

    /**
     * Gets a string field value.
     *
     * @param slot the slot number
     * @param fldname the field name
     * @return the string value
     */
    std::string get_string(size_t slot, const std::string& fldname);

    /**
     * Sets an integer field value.
     *
     * @param slot the slot number
     * @param fldname the field name
     * @param val the integer value
     */
    void set_int(size_t slot, const std::string& fldname, int32_t val);

    /**
     * Sets a string field value.
     *
     * @param slot the slot number
     * @param fldname the field name
     * @param val the string value
     */
    void set_string(size_t slot, const std::string& fldname, const std::string& val);

    /**
     * Deletes a record (sets flag to EMPTY).
     *
     * @param slot the slot number
     */
    void delete_record(size_t slot);

    /**
     * Formats the page (sets all slots to EMPTY).
     */
    void format();

    /**
     * Finds the next used slot after the given slot.
     *
     * @param slot starting slot (or std::nullopt for start of page)
     * @return next used slot, or std::nullopt if none
     */
    std::optional<size_t> next_after(std::optional<size_t> slot);

    /**
     * Finds the next empty slot and marks it as USED.
     *
     * @param slot starting slot (or std::nullopt for start of page)
     * @return newly allocated slot, or std::nullopt if page full
     */
    std::optional<size_t> insert_after(std::optional<size_t> slot);

    /**
     * Returns the block ID of this page.
     *
     * @return block ID
     */
    const file::BlockId& block() const;

private:
    enum class Flag {
        EMPTY = 0,
        USED = 1
    };

    /**
     * Sets the flag for a slot.
     */
    void set_flag(size_t slot, Flag flag);

    /**
     * Searches for a slot with the given flag.
     */
    std::optional<size_t> search_after(std::optional<size_t> slot, Flag flag);

    /**
     * Checks if a slot number is valid (fits in page).
     */
    bool is_valid_slot(size_t slot) const;

    /**
     * Calculates the byte offset of a slot.
     */
    size_t offset(size_t slot) const;

private:
    buffer::Buffer& buff_;
    Layout layout_;
};

} // namespace record

#endif // RECORDPAGE_HPP
```

### Implementation Notes

```cpp
void RecordPage::format() {
    size_t slot = 0;
    while (is_valid_slot(slot)) {
        // Set flag to EMPTY
        buff_.contents().set_int(offset(slot), static_cast<int32_t>(Flag::EMPTY));

        // Initialize fields to zero/empty
        for (const auto& fldname : layout_.schema()->fields()) {
            size_t fldpos = offset(slot) + layout_.offset(fldname);

            if (layout_.schema()->type(fldname) == Type::INTEGER) {
                buff_.contents().set_int(fldpos, 0);
            } else {
                buff_.contents().set_string(fldpos, "");
            }
        }
        slot++;
    }
}

std::optional<size_t> RecordPage::next_after(std::optional<size_t> slot) {
    return search_after(slot, Flag::USED);
}

std::optional<size_t> RecordPage::insert_after(std::optional<size_t> slot) {
    std::optional<size_t> newslot = search_after(slot, Flag::EMPTY);
    if (newslot.has_value()) {
        set_flag(newslot.value(), Flag::USED);
    }
    return newslot;
}
```

### Test Coverage (10 tests)

```cpp
TEST(RecordPageTest, Format)
TEST(RecordPageTest, SetGetInt)
TEST(RecordPageTest, SetGetString)
TEST(RecordPageTest, InsertAfter)
TEST(RecordPageTest, NextAfter)
TEST(RecordPageTest, DeleteRecord)
TEST(RecordPageTest, PageFull)
TEST(RecordPageTest, MultipleRecords)
TEST(RecordPageTest, SlotReuse)  // Delete then insert reuses slot
TEST(RecordPageTest, FieldBoundaries)  // Verify fields don't overlap
```

---

## Component 5: TableScan

**Files to Create:**
- `include/record/tablescan.hpp`
- `src/record/tablescan.cpp`

### Purpose
Provides sequential access to table records across multiple blocks.

### Key Features
- Implements Scan interface (from Phase 0)
- Sequential scan (before_first, next, get/set fields)
- Insert/delete operations
- Move to specific RID
- Automatic block management (creates new blocks as needed)

### API Specification

```cpp
#ifndef TABLESCAN_HPP
#define TABLESCAN_HPP

#include "record/recordpage.hpp"
#include "record/layout.hpp"
#include "record/rid.hpp"
#include "buffer/buffermgr.hpp"
#include "query/scan.hpp"
#include <memory>
#include <optional>
#include <string>

namespace record {

/**
 * TableScan provides sequential access to table records.
 *
 * Implements the Scan interface for reading records.
 * Provides additional methods for updates (insert, delete, set).
 *
 * NOTE: Phase 4 version simplified - no Transaction layer yet.
 * Directly uses BufferMgr.
 *
 * Corresponds to TableScan in Rust (NMDB2/src/record/tablescan.rs)
 */
class TableScan : public query::Scan {
public:
    /**
     * Creates a table scan.
     *
     * @param bm the buffer manager
     * @param tablename the table name
     * @param layout the table layout
     */
    TableScan(std::shared_ptr<buffer::BufferMgr> bm,
              const std::string& tablename,
              const Layout& layout);

    // Scan interface implementation
    void before_first() override;
    bool next() override;
    int32_t get_int(const std::string& fldname) override;
    std::string get_string(const std::string& fldname) override;
    query::Constant get_val(const std::string& fldname) override;
    bool has_field(const std::string& fldname) const override;
    void close() override;

    // Update operations (not in Scan interface)

    /**
     * Sets a field value.
     */
    void set_val(const std::string& fldname, const query::Constant& val);

    /**
     * Sets an integer field.
     */
    void set_int(const std::string& fldname, int32_t val);

    /**
     * Sets a string field.
     */
    void set_string(const std::string& fldname, const std::string& val);

    /**
     * Inserts a new record.
     * Finds an empty slot, possibly creating a new block.
     */
    void insert();

    /**
     * Deletes the current record.
     */
    void delete_record();

    /**
     * Returns the RID of the current record.
     *
     * @return the RID, or std::nullopt if before first or after last
     */
    std::optional<RID> get_rid() const;

    /**
     * Moves to a specific record.
     *
     * @param rid the record ID
     */
    void move_to_rid(const RID& rid);

private:
    /**
     * Moves to a specific block.
     */
    void move_to_block(int32_t blknum);

    /**
     * Creates and moves to a new block.
     */
    void move_to_new_block();

    /**
     * Checks if at the last block.
     */
    bool at_last_block() const;

private:
    std::shared_ptr<buffer::BufferMgr> bm_;
    Layout layout_;
    std::unique_ptr<RecordPage> rp_;
    std::string filename_;
    std::optional<size_t> currentslot_;
    std::optional<size_t> current_buffer_idx_;
};

} // namespace record

#endif // TABLESCAN_HPP
```

### Implementation Notes

```cpp
void TableScan::before_first() {
    move_to_block(0);
}

bool TableScan::next() {
    currentslot_ = rp_->next_after(currentslot_);

    while (!currentslot_.has_value()) {
        if (at_last_block()) {
            return false;
        }
        move_to_block(rp_->block().number() + 1);
        currentslot_ = rp_->next_after(currentslot_);
    }
    return true;
}

void TableScan::insert() {
    currentslot_ = rp_->insert_after(currentslot_);

    while (!currentslot_.has_value()) {
        if (at_last_block()) {
            move_to_new_block();
        } else {
            move_to_block(rp_->block().number() + 1);
        }
        currentslot_ = rp_->insert_after(currentslot_);
    }
}

void TableScan::move_to_block(int32_t blknum) {
    close();  // Unpin current block

    file::BlockId blk(filename_, blknum);
    current_buffer_idx_ = bm_->pin(blk);
    rp_ = std::make_unique<RecordPage>(
        bm_->buffer(current_buffer_idx_.value()), layout_);
    currentslot_ = std::nullopt;
}
```

### Test Coverage (10 tests)

```cpp
TEST(TableScanTest, CreateEmptyTable)
TEST(TableScanTest, InsertSingleRecord)
TEST(TableScanTest, InsertMultipleRecords)
TEST(TableScanTest, ScanAllRecords)
TEST(TableScanTest, DeleteRecord)
TEST(TableScanTest, UpdateRecord)
TEST(TableScanTest, GetRID)
TEST(TableScanTest, MoveToRID)
TEST(TableScanTest, MultipleBlocks)  // Table spanning multiple blocks
TEST(TableScanTest, InsertIntoFullPage)  // Creates new block
```

---

## Integration with Existing Layers

### Dependencies

**Phase 1 (File Layer):**
```cpp
#include "file/page.hpp"
#include "file/blockid.hpp"
```

**Phase 3 (Buffer Layer):**
```cpp
#include "buffer/buffer.hpp"
#include "buffer/buffermgr.hpp"
```

**Phase 0 (Query Layer):**
```cpp
#include "query/scan.hpp"
#include "query/constant.hpp"
```

### Usage Pattern

```cpp
// Create schema
auto schema = std::make_shared<Schema>();
schema->add_int_field("id");
schema->add_string_field("name", 20);
schema->add_int_field("age");

// Create layout
Layout layout(schema);

// Create table scan
auto bm = std::make_shared<BufferMgr>(fm, lm, 8);
TableScan scan(bm, "students", layout);

// Insert records
scan.insert();
scan.set_int("id", 1);
scan.set_string("name", "Alice");
scan.set_int("age", 25);

scan.insert();
scan.set_int("id", 2);
scan.set_string("name", "Bob");
scan.set_int("age", 30);

// Scan records
scan.before_first();
while (scan.next()) {
    int id = scan.get_int("id");
    std::string name = scan.get_string("name");
    int age = scan.get_int("age");
    std::cout << id << ", " << name << ", " << age << std::endl;
}

scan.close();
```

---

## CMakeLists.txt Updates

### Add Record Sources

```cmake
# In src/CMakeLists.txt (no changes needed, GLOB_RECURSE picks up new files)

# Files will be automatically included:
# src/record/schema.cpp
# src/record/layout.cpp
# src/record/rid.cpp
# src/record/recordpage.cpp
# src/record/tablescan.cpp
```

### Add Tests

```cmake
# In tests/CMakeLists.txt
add_executable(tests
    # ... existing tests ...
    test_schema.cpp
    test_layout.cpp
    test_rid.cpp
    test_recordpage.cpp
    test_tablescan.cpp
)
```

---

## Key Design Decisions

### 1. Simplified Phase 4 (No Transaction Layer Yet)

**Decision:** RecordPage and TableScan directly use Buffer/BufferMgr

**Rationale:**
- Transaction layer (concurrency, recovery) is Phase 5-6
- Simpler to implement and test record management first
- Can add Transaction wrapper later

**Trade-off:** Will need to refactor in Phase 5

### 2. Slot-Based Storage

**Decision:** Fixed-size slots with empty/used flag

**Rationale:**
- Simple and efficient
- Easy to find empty slots
- No fragmentation issues

**Alternative:** Variable-size records with free space tracking (more complex)

### 3. VARCHAR Max Length

**Decision:** VARCHAR fields have maximum length specified in schema

**Rationale:**
- Allows fixed-size slot calculation
- Simpler implementation

**Trade-off:** Wastes space if strings are shorter than max

### 4. Flag at Start of Slot

**Decision:** 4-byte flag (0=EMPTY, 1=USED) at offset 0 of each slot

**Rationale:**
- Matches Rust reference
- Easy to check if slot is in use

**Alternative:** Bitmap at page start (more compact but more complex)

---

## Testing Strategy

### Unit Tests
- **Schema:** Field management, type checking
- **Layout:** Offset calculation, slot size
- **RID:** Construction, equality
- **RecordPage:** Insert, delete, format, get/set fields
- **TableScan:** Scan, insert, delete, multi-block

### Integration Tests
- **End-to-End:** Create table, insert, scan, delete, verify
- **Multi-Block:** Table spanning multiple pages
- **RID Navigation:** Insert, get RID, move to RID

### Edge Cases
- Full page (insert creates new block)
- Empty table (no records)
- Slot reuse (delete then insert)
- Field boundaries (verify no overlap)

---

## Estimated Implementation Time

| Task | Estimated Time |
|------|----------------|
| Schema implementation + tests | 0.5 day |
| Layout implementation + tests | 0.5 day |
| RID implementation + tests | 0.25 day |
| RecordPage implementation + tests | 1.5 days |
| TableScan implementation + tests | 1.5 days |
| Integration testing | 0.5 day |
| Build, debug, documentation | 0.25 day |
| **Total** | **4.5-5 days** |

---

## Success Criteria

### Phase 4 Complete When:
- ✅ All 30+ record tests passing (100%)
- ✅ Integration with Phases 1-3 verified
- ✅ Docker build successful
- ✅ Code reviewed and documented
- ✅ Multi-block tables work correctly
- ✅ Insert/delete/update operations correct

---

## Files Summary

### Headers (5 files)
1. `include/record/schema.hpp` (~90 lines)
2. `include/record/layout.hpp` (~60 lines)
3. `include/record/rid.hpp` (~50 lines)
4. `include/record/recordpage.hpp` (~150 lines)
5. `include/record/tablescan.hpp` (~120 lines)

### Implementation (5 files)
6. `src/record/schema.cpp` (~60 lines)
7. `src/record/layout.cpp` (~40 lines)
8. `src/record/rid.cpp` (~20 lines)
9. `src/record/recordpage.cpp` (~250 lines)
10. `src/record/tablescan.cpp` (~350 lines)

### Tests (5 files)
11. `tests/test_schema.cpp` (~150 lines)
12. `tests/test_layout.cpp` (~120 lines)
13. `tests/test_rid.cpp` (~80 lines)
14. `tests/test_recordpage.cpp` (~300 lines)
15. `tests/test_tablescan.cpp` (~350 lines)

**Total New Code:** ~2190 lines

---

## Next Phase Preview

After Phase 4 completes, Phase 5 (Metadata Management) will add:

- **TableMgr:** Create/drop tables, store schemas in catalog
- **ViewMgr:** Create/drop views
- **StatMgr:** Table statistics
- **IndexMgr:** Index metadata

Phase 5 will also introduce the **Transaction layer** to wrap BufferMgr with logging and concurrency control.

---

**Document Version:** 1.0
**Last Updated:** October 12, 2025
**Status:** Ready for Implementation (after Phase 3 complete)
**Estimated Start Date:** October 13, 2025
**Estimated Completion:** October 17-18, 2025

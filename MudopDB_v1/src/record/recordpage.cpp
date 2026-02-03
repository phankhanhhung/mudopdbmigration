#include "record/recordpage.hpp"

namespace record {

RecordPage::RecordPage(buffer::Buffer& buff, const Layout& layout)
    : buff_(buff), layout_(layout) {}

int32_t RecordPage::get_int(size_t slot, const std::string& fldname) {
    size_t fldpos = offset(slot) + layout_.offset(fldname);
    return buff_.contents().get_int(fldpos);
}

std::string RecordPage::get_string(size_t slot, const std::string& fldname) {
    size_t fldpos = offset(slot) + layout_.offset(fldname);
    return buff_.contents().get_string(fldpos);
}

void RecordPage::set_int(size_t slot, const std::string& fldname, int32_t val) {
    size_t fldpos = offset(slot) + layout_.offset(fldname);
    buff_.contents().set_int(fldpos, val);
    buff_.set_modified(0, std::nullopt);  // Mark buffer as dirty (txnum=0 for Phase 4)
}

void RecordPage::set_string(size_t slot, const std::string& fldname, const std::string& val) {
    size_t fldpos = offset(slot) + layout_.offset(fldname);
    buff_.contents().set_string(fldpos, val);
    buff_.set_modified(0, std::nullopt);  // Mark buffer as dirty (txnum=0 for Phase 4)
}

void RecordPage::delete_record(size_t slot) {
    set_flag(slot, Flag::EMPTY);
    buff_.set_modified(0, std::nullopt);  // Mark buffer as dirty (txnum=0 for Phase 4)
}

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
    buff_.set_modified(0, std::nullopt);  // Mark buffer as dirty (txnum=0 for Phase 4)
}

std::optional<size_t> RecordPage::next_after(std::optional<size_t> slot) {
    return search_after(slot, Flag::USED);
}

std::optional<size_t> RecordPage::insert_after(std::optional<size_t> slot) {
    std::optional<size_t> newslot = search_after(slot, Flag::EMPTY);
    if (newslot.has_value()) {
        set_flag(newslot.value(), Flag::USED);
        buff_.set_modified(0, std::nullopt);  // Mark buffer as dirty (txnum=0 for Phase 4)
    }
    return newslot;
}

const file::BlockId& RecordPage::block() const {
    return buff_.block().value();
}

void RecordPage::set_flag(size_t slot, Flag flag) {
    buff_.contents().set_int(offset(slot), static_cast<int32_t>(flag));
}

RecordPage::Flag RecordPage::get_flag(size_t slot) {
    int32_t flag_val = buff_.contents().get_int(offset(slot));
    return static_cast<Flag>(flag_val);
}

std::optional<size_t> RecordPage::search_after(std::optional<size_t> slot, Flag flag) {
    size_t current = slot.has_value() ? slot.value() + 1 : 0;

    while (is_valid_slot(current)) {
        if (get_flag(current) == flag) {
            return current;
        }
        current++;
    }

    return std::nullopt;
}

bool RecordPage::is_valid_slot(size_t slot) const {
    return offset(slot + 1) <= buff_.contents().size();
}

size_t RecordPage::offset(size_t slot) const {
    return slot * layout_.slot_size();
}

} // namespace record

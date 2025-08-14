#include "record/layout.hpp"

namespace record {

Layout::Layout(std::shared_ptr<Schema> schema)
    : schema_(schema), slotsize_(4) {  // Start with 4-byte flag

    for (const auto& fldname : schema_->fields()) {
        offsets_[fldname] = slotsize_;
        slotsize_ += length_in_bytes(fldname);
    }
}

Layout::Layout(std::shared_ptr<Schema> schema,
               std::unordered_map<std::string, size_t> offsets,
               size_t slotsize)
    : schema_(schema), offsets_(offsets), slotsize_(slotsize) {}

std::shared_ptr<Schema> Layout::schema() const {
    return schema_;
}

size_t Layout::offset(const std::string& fldname) const {
    return offsets_.at(fldname);
}

size_t Layout::slot_size() const {
    return slotsize_;
}

size_t Layout::length_in_bytes(const std::string& fldname) const {
    Type fldtype = schema_->type(fldname);

    switch (fldtype) {
        case Type::INTEGER:
            return 4;
        case Type::VARCHAR:
            return file::Page::max_length(schema_->length(fldname));
    }

    // Should never reach here
    return 0;
}

} // namespace record

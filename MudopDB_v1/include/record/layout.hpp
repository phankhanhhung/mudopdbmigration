#ifndef LAYOUT_HPP
#define LAYOUT_HPP

#include "record/schema.hpp"
#include "file/page.hpp"
#include <memory>
#include <unordered_map>

namespace record {

class Layout {
public:
    explicit Layout(std::shared_ptr<Schema> schema);

    Layout(std::shared_ptr<Schema> schema,
           std::unordered_map<std::string, size_t> offsets,
           size_t slotsize);

    std::shared_ptr<Schema> schema() const;

    size_t offset(const std::string& fldname) const;

    size_t slot_size() const;

private:
    size_t length_in_bytes(const std::string& fldname) const;

private:
    std::shared_ptr<Schema> schema_;
    std::unordered_map<std::string, size_t> offsets_;
    size_t slotsize_;
};

} // namespace record

#endif // LAYOUT_HPP

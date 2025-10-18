/**
 * TableMgr implementation.
 * Manages the tblcat/fldcat system catalog tables that store table schemas.
 */

#include "metadata/tablemgr.hpp"
#include "record/tablescan.hpp"
#include "tx/transaction.hpp"
#include <stdexcept>

namespace metadata {

static record::Layout build_tcat_layout() {
    auto sch = std::make_shared<record::Schema>();
    sch->add_string_field("tblname", TableMgr::MAX_NAME);
    sch->add_int_field("slotsize");
    return record::Layout(sch);
}

static record::Layout build_fcat_layout() {
    auto sch = std::make_shared<record::Schema>();
    sch->add_string_field("tblname", TableMgr::MAX_NAME);
    sch->add_string_field("fldname", TableMgr::MAX_NAME);
    sch->add_int_field("type");
    sch->add_int_field("length");
    sch->add_int_field("offset");
    return record::Layout(sch);
}

TableMgr::TableMgr(bool is_new, std::shared_ptr<tx::Transaction> tx)
    : tcat_layout_(build_tcat_layout()), fcat_layout_(build_fcat_layout()) {
    if (is_new) {
        create_table("tblcat", tcat_layout_.schema(), tx);
        create_table("fldcat", fcat_layout_.schema(), tx);
    }
}

void TableMgr::create_table(const std::string& tblname,
                             std::shared_ptr<record::Schema> sch,
                             std::shared_ptr<tx::Transaction> tx) {
    record::Layout layout(sch);

    // Insert into tblcat
    record::TableScan tcat(tx, "tblcat", tcat_layout_);
    tcat.insert();
    tcat.set_string("tblname", tblname);
    tcat.set_int("slotsize", static_cast<int32_t>(layout.slot_size()));
    tcat.close();

    // Insert fields into fldcat
    record::TableScan fcat(tx, "fldcat", fcat_layout_);
    for (const auto& fldname : sch->fields()) {
        fcat.insert();
        fcat.set_string("tblname", tblname);
        fcat.set_string("fldname", fldname);
        fcat.set_int("type", static_cast<int32_t>(sch->type(fldname)));
        fcat.set_int("length", static_cast<int32_t>(sch->length(fldname)));
        fcat.set_int("offset", static_cast<int32_t>(layout.offset(fldname)));
    }
    fcat.close();
}

record::Layout TableMgr::get_layout(const std::string& tblname,
                                     std::shared_ptr<tx::Transaction> tx) {
    std::optional<size_t> size;

    // Read slot size from tblcat
    record::TableScan tcat(tx, "tblcat", tcat_layout_);
    while (tcat.next()) {
        if (tcat.get_string("tblname") == tblname) {
            size = static_cast<size_t>(tcat.get_int("slotsize"));
            break;
        }
    }
    tcat.close();

    // Read field info from fldcat
    auto sch = std::make_shared<record::Schema>();
    std::unordered_map<std::string, size_t> offsets;

    record::TableScan fcat(tx, "fldcat", fcat_layout_);
    while (fcat.next()) {
        if (fcat.get_string("tblname") == tblname) {
            std::string fldname = fcat.get_string("fldname");
            int32_t fldtype = fcat.get_int("type");
            int32_t fldlen = fcat.get_int("length");
            int32_t fldoffset = fcat.get_int("offset");
            offsets[fldname] = static_cast<size_t>(fldoffset);

            record::Type t;
            if (fldtype == static_cast<int32_t>(record::Type::INTEGER)) {
                t = record::Type::INTEGER;
            } else if (fldtype == static_cast<int32_t>(record::Type::VARCHAR)) {
                t = record::Type::VARCHAR;
            } else {
                throw std::runtime_error("Unknown field type");
            }
            sch->add_field(fldname, t, static_cast<size_t>(fldlen));
        }
    }
    fcat.close();

    if (size.has_value()) {
        return record::Layout(sch, offsets, size.value());
    }
    throw std::runtime_error("TableMgr::get_layout: table not found: " + tblname);
}

} // namespace metadata

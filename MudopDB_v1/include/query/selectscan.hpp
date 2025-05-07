#ifndef SELECTSCAN_HPP
#define SELECTSCAN_HPP

#include "query/updatescan.hpp"
#include "query/predicate.hpp"
#include <memory>

class SelectScan : public UpdateScan {
public:
    SelectScan(std::unique_ptr<Scan> s, const Predicate& pred);

    void before_first() override;
    bool next() override;
    int get_int(const std::string& fldname) override;
    std::string get_string(const std::string& fldname) override;
    Constant get_val(const std::string& fldname) override;
    bool has_field(const std::string& fldname) const override;
    void close() override;

    // UpdateScan delegation
    void set_val(const std::string& fldname, const Constant& val) override;
    void set_int(const std::string& fldname, int32_t val) override;
    void set_string(const std::string& fldname, const std::string& val) override;
    void insert() override;
    void delete_record() override;
    std::optional<record::RID> get_rid() const override;
    void move_to_rid(const record::RID& rid) override;

private:
    UpdateScan* get_update_scan() const;

    std::unique_ptr<Scan> s_;
    Predicate pred_;
};

#endif // SELECTSCAN_HPP

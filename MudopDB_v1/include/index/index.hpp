#ifndef INDEX_HPP
#define INDEX_HPP

#include "query/constant.hpp"
#include "record/rid.hpp"
#include <memory>

/**
 * Abstract base class for index implementations.
 *
 * Corresponds to IndexControl trait in Rust (NMDB2/src/index/index.rs)
 */
class Index {
public:
    virtual ~Index() = default;
    virtual void before_first(const Constant& searchkey) = 0;
    virtual bool next() = 0;
    virtual record::RID get_data_rid() = 0;
    virtual void insert(const Constant& val, const record::RID& rid) = 0;
    virtual void delete_entry(const Constant& val, const record::RID& rid) = 0;
    virtual void close() = 0;
};

#endif // INDEX_HPP

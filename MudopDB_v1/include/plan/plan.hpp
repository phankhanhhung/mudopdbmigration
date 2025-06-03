#ifndef PLAN_HPP
#define PLAN_HPP

#include <memory>
#include <string>

class Scan;

namespace record {
class Schema;
}

class Plan {
public:
    virtual ~Plan() = default;

    virtual std::unique_ptr<Scan> open() = 0;
    virtual size_t blocks_accessed() const = 0;
    virtual size_t records_output() const = 0;
    virtual std::shared_ptr<record::Schema> schema() const = 0;
};

#endif // PLAN_HPP

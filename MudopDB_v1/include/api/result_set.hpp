#ifndef RESULT_SET_HPP
#define RESULT_SET_HPP

#include <cstdint>
#include <memory>
#include <string>

class Scan;
class Plan;
class Metadata;
class EmbeddedConnection;
class NetworkConnection;

namespace record { class Schema; }

/**
 * Abstract base class for result sets.
 * Corresponds to ResultSetControl trait in Rust (NMDB2/src/api/resultset.rs)
 */
class ResultSet {
public:
    virtual ~ResultSet() = default;
    virtual bool next() = 0;
    virtual int32_t get_int(const std::string& fldname) = 0;
    virtual std::string get_string(const std::string& fldname) = 0;
    virtual std::unique_ptr<Metadata> get_meta_data() const = 0;
    virtual void close() = 0;
};

/**
 * Embedded result set backed by a Scan.
 * Corresponds to EmbeddedResultSet in Rust (NMDB2/src/api/embedded/embeddedresultset.rs)
 */
class EmbeddedResultSet : public ResultSet {
public:
    EmbeddedResultSet(std::shared_ptr<Plan> plan,
                      std::shared_ptr<EmbeddedConnection> conn);

    bool next() override;
    int32_t get_int(const std::string& fldname) override;
    std::string get_string(const std::string& fldname) override;
    std::unique_ptr<Metadata> get_meta_data() const override;
    void close() override;

private:
    std::unique_ptr<Scan> s_;
    std::shared_ptr<record::Schema> sch_;
    std::shared_ptr<EmbeddedConnection> conn_;
};

/**
 * Network result set (stub).
 */
class NetworkResultSet : public ResultSet {
public:
    NetworkResultSet(std::shared_ptr<NetworkConnection> conn, int64_t id);

    bool next() override;
    int32_t get_int(const std::string& fldname) override;
    std::string get_string(const std::string& fldname) override;
    std::unique_ptr<Metadata> get_meta_data() const override;
    void close() override;
};

#endif // RESULT_SET_HPP

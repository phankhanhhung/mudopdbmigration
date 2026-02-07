#ifndef DRIVER_HPP
#define DRIVER_HPP

#include <memory>
#include <string>

class Connection;

/**
 * Abstract base class for database drivers.
 * Corresponds to DriverControl trait in Rust (NMDB2/src/api/driver.rs)
 */
class Driver {
public:
    virtual ~Driver() = default;
    virtual std::unique_ptr<Connection> connect(const std::string& url) = 0;
};

/**
 * Embedded driver creates an in-process database connection.
 * Corresponds to EmbeddedDriver in Rust (NMDB2/src/api/embedded/embeddeddriver.rs)
 */
class EmbeddedDriver : public Driver {
public:
    std::unique_ptr<Connection> connect(const std::string& url) override;
};

/**
 * Network driver (stub).
 */
class NetworkDriver : public Driver {
public:
    std::unique_ptr<Connection> connect(const std::string& url) override;
};

#endif // DRIVER_HPP

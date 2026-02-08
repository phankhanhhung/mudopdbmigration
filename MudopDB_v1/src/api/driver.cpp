#include "api/driver.hpp"
#include "api/connection.hpp"
#include "server/simpledb.hpp"
#include <memory>
#include <string>
#include <stdexcept>

static std::string extract_dbname(const std::string& url) {
    size_t pos = url.rfind(':');
    if (pos != std::string::npos) {
        return url.substr(pos + 1);
    }
    return url;
}

std::unique_ptr<Connection> EmbeddedDriver::connect(const std::string& url) {
    std::string dbname = extract_dbname(url);
    auto db = std::make_shared<server::SimpleDB>(dbname);
    return std::make_unique<EmbeddedConnection>(db);
}

// URL format: "host:port" or "//host:port" or "jdbc:mudopdb://host:port"
std::unique_ptr<Connection> NetworkDriver::connect(const std::string& url) {
    std::string addr = url;
    // Strip "jdbc:mudopdb://" or "//" prefix
    auto pos = addr.find("//");
    if (pos != std::string::npos) {
        addr = addr.substr(pos + 2);
    }
    // Parse host:port
    std::string host = "localhost";
    uint16_t port = 1099;
    auto colon = addr.rfind(':');
    if (colon != std::string::npos) {
        host = addr.substr(0, colon);
        port = static_cast<uint16_t>(std::stoi(addr.substr(colon + 1)));
    } else if (!addr.empty()) {
        host = addr;
    }
    return std::make_unique<NetworkConnection>(host, port);
}

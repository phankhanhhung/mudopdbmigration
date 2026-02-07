#include "api/driver.hpp"
#include "api/connection.hpp"
#include "server/simpledb.hpp"
#include <memory>
#include <string>

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

std::unique_ptr<Connection> NetworkDriver::connect(const std::string& url) {
    (void)url;
    return std::make_unique<NetworkConnection>();
}

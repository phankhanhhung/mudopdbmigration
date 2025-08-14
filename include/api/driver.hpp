#ifndef DRIVER_HPP
#define DRIVER_HPP

#include <memory>
#include <string>

//Forward declaration of class Connection
class Connection;

class Driver {
public:
  virtual ~Driver() = default;
  virtual std::unique_ptr<Connection> connect(const std::string& c) = 0;
};

class EmbeddedDriver : public Driver {
public:
  std::unique_ptr<Connection> connect(const std::string& c) override;
};
class NetworkDriver : public Driver {
public:
  std::unique_ptr<Connection> connect(const std::string& c) override;
};

#endif // DRIVER_HPP




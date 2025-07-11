#include <iostream>
#include "app/app.hpp"

int main() {
  std::string conn;
  std::cout << "Connect> ";
  std::getline(std::cin, conn);
  return run_app(conn);
}

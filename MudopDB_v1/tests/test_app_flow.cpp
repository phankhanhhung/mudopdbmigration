#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <functional>
#include "app/app.hpp"

TEST(AppFlow, HandlesQuit) {
  AppConfig cfg{.connection_string = "mem://test"};
  std::istringstream in("quit\n");
  std::ostringstream out;
  int rc = run_app(cfg, in, out);
  EXPECT_EQ(rc, 0);
  EXPECT_NE(out.str().find("SQL>"), std::string::npos);
}

static std::string capture(std::function<int()> fn) {
  std::ostringstream oss;
  auto* old = std::cout.rdbuf(oss.rdbuf());
  int rc = fn();
  std::cout.rdbuf(old);
  return oss.str();
}

TEST(AppFlow, RoutesQueryAndUpdate) {
  AppConfig cfg{.connection_string = "mem://test"};
  std::istringstream in("select x\nupdate y\nquit\n");
  std::ostringstream out;
  auto s = capture([&]{ return run_app(cfg, in, out); });
  // run_app writes prompts to 'out' and messages to std::cout, so we assert on captured stdout here.
  EXPECT_NE(s.find("Executing query: select x"), std::string::npos);
  EXPECT_NE(s.find("Executing update: update y"), std::string::npos);
}

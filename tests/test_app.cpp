#include <gtest/gtest.h>
#include <sstream>
#include "app/app.hpp"

TEST(App, QuitImmediately) {
  AppConfig cfg{.connection_string = "mem://test"};
  std::istringstream in("quit\n");
  std::ostringstream out;
  int rc = run_app(cfg, in, out);
  EXPECT_EQ(rc, 0);
  // Basic smoke: prompt appears once
  EXPECT_NE(out.str().find("SQL>"), std::string::npos);
}

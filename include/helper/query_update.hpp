#ifndef QUERYUPDATE_HPP
#define QUERYUPDATE_HPP

#include <string>

// Forward declaration of Statement.
class Statement;

void do_query(Statement *stmt, const std::string &cmd);
void do_update(Statement *stmt, const std::string &cmd);

#endif // QUERYUPDATE_HPP
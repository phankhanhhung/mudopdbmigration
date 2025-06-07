#ifndef UPDATEPLANNER_HPP
#define UPDATEPLANNER_HPP

#include <memory>
#include <cstddef>

namespace parse {
class InsertData;
class DeleteData;
class ModifyData;
class CreateTableData;
class CreateViewData;
class CreateIndexData;
}
namespace tx { class Transaction; }

class UpdatePlanner {
public:
    virtual ~UpdatePlanner() = default;
    virtual size_t execute_insert(const parse::InsertData& data, std::shared_ptr<tx::Transaction> tx) = 0;
    virtual size_t execute_delete(const parse::DeleteData& data, std::shared_ptr<tx::Transaction> tx) = 0;
    virtual size_t execute_modify(const parse::ModifyData& data, std::shared_ptr<tx::Transaction> tx) = 0;
    virtual size_t execute_create_table(const parse::CreateTableData& data, std::shared_ptr<tx::Transaction> tx) = 0;
    virtual size_t execute_create_view(const parse::CreateViewData& data, std::shared_ptr<tx::Transaction> tx) = 0;
    virtual size_t execute_create_index(const parse::CreateIndexData& data, std::shared_ptr<tx::Transaction> tx) = 0;
};

#endif // UPDATEPLANNER_HPP

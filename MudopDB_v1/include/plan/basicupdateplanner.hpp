#ifndef BASICUPDATEPLANNER_HPP
#define BASICUPDATEPLANNER_HPP

#include "plan/updateplanner.hpp"
#include <memory>

namespace metadata { class MetadataMgr; }

class BasicUpdatePlanner : public UpdatePlanner {
public:
    explicit BasicUpdatePlanner(std::shared_ptr<metadata::MetadataMgr> mdm);

    size_t execute_insert(const parse::InsertData& data, std::shared_ptr<tx::Transaction> tx) override;
    size_t execute_delete(const parse::DeleteData& data, std::shared_ptr<tx::Transaction> tx) override;
    size_t execute_modify(const parse::ModifyData& data, std::shared_ptr<tx::Transaction> tx) override;
    size_t execute_create_table(const parse::CreateTableData& data, std::shared_ptr<tx::Transaction> tx) override;
    size_t execute_create_view(const parse::CreateViewData& data, std::shared_ptr<tx::Transaction> tx) override;
    size_t execute_create_index(const parse::CreateIndexData& data, std::shared_ptr<tx::Transaction> tx) override;

private:
    std::shared_ptr<metadata::MetadataMgr> mdm_;
};

#endif // BASICUPDATEPLANNER_HPP

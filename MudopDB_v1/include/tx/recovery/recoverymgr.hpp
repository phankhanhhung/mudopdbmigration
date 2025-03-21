#ifndef RECOVERYMGR_HPP
#define RECOVERYMGR_HPP

#include "log/logmgr.hpp"
#include "buffer/buffermgr.hpp"
#include <memory>

namespace tx {

class RecoveryMgr {
public:
    RecoveryMgr(size_t txnum,
                std::shared_ptr<log::LogMgr> lm,
                std::shared_ptr<buffer::BufferMgr> bm);

    void commit();
    size_t set_int(buffer::Buffer& buff, size_t offset, int32_t newval);
    size_t set_string(buffer::Buffer& buff, size_t offset, const std::string& newval);

private:
    std::shared_ptr<log::LogMgr> lm_;
    std::shared_ptr<buffer::BufferMgr> bm_;
    size_t txnum_;
};

} // namespace tx

#endif // RECOVERYMGR_HPP

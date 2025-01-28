#ifndef LOGMGR_HPP
#define LOGMGR_HPP

#include "file/blockid.hpp"
#include "file/page.hpp"
#include "file/filemgr.hpp"
#include "log/logiterator.hpp"
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <mutex>

namespace log {

class LogMgr {
public:
    LogMgr(std::shared_ptr<file::FileMgr> fm, const std::string& logfile);

    size_t append(const std::vector<uint8_t>& logrec);

    void flush(size_t lsn);

    std::unique_ptr<LogIterator> iterator();

private:
    std::shared_ptr<file::FileMgr> fm_;
    std::string logfile_;
    file::Page logpage_;
    file::BlockId currentblk_;
    size_t latest_lsn_;
    size_t last_saved_lsn_;

    file::BlockId append_new_block();

    void flush_impl();

    static file::BlockId append_new_block_helper(
        file::FileMgr* fm,
        file::Page& logpage,
        const std::string& logfile
    );
};

} // namespace log

#endif // LOGMGR_HPP

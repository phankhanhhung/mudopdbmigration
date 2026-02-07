#include "api/result_set.hpp"
#include "api/metadata.hpp"
#include "api/connection.hpp"
#include "plan/plan.hpp"
#include "query/scan.hpp"
#include "record/schema.hpp"
#include <algorithm>
#include <cctype>
#include <memory>
#include <string>

static std::string to_lowercase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return result;
}

// ============================================================================
// EmbeddedResultSet
// ============================================================================

EmbeddedResultSet::EmbeddedResultSet(std::shared_ptr<Plan> plan,
                                     std::shared_ptr<EmbeddedConnection> c)
    : s_(nullptr), sch_(nullptr), conn_(std::move(c)) {
    if (plan) {
        s_ = plan->open();
        sch_ = plan->schema();
    }
}

bool EmbeddedResultSet::next() {
    if (s_) {
        try {
            return s_->next();
        } catch (...) {
            if (conn_) conn_->rollback();
            return false;
        }
    }
    return false;
}

int32_t EmbeddedResultSet::get_int(const std::string& fldname) {
    std::string fld = to_lowercase(fldname);
    if (s_) {
        try {
            return s_->get_int(fld);
        } catch (...) {
            if (conn_) conn_->rollback();
            return 0;
        }
    }
    return 0;
}

std::string EmbeddedResultSet::get_string(const std::string& fldname) {
    std::string fld = to_lowercase(fldname);
    if (s_) {
        try {
            return s_->get_string(fld);
        } catch (...) {
            if (conn_) conn_->rollback();
            return "";
        }
    }
    return "";
}

std::unique_ptr<Metadata> EmbeddedResultSet::get_meta_data() const {
    return std::make_unique<EmbeddedMetadata>(sch_);
}

void EmbeddedResultSet::close() {
    if (s_) s_->close();
    if (conn_) conn_->close();
}

// ============================================================================
// NetworkResultSet (stub)
// ============================================================================

NetworkResultSet::NetworkResultSet(std::shared_ptr<NetworkConnection>, int64_t) {}

bool NetworkResultSet::next() { return false; }

int32_t NetworkResultSet::get_int(const std::string&) { return 0; }

std::string NetworkResultSet::get_string(const std::string&) { return {}; }

std::unique_ptr<Metadata> NetworkResultSet::get_meta_data() const {
    return std::make_unique<NetworkMetadata>(std::shared_ptr<NetworkConnection>{}, 0);
}

void NetworkResultSet::close() {}

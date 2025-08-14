#include "api/result_set.hpp"
#include "api/metadata.hpp"
#include "api/connection.hpp"
#include <memory>
#include <string>
#include <algorithm>
#include <cctype>

// Helper function to convert string to lowercase
// Corresponds to fldname.to_lowercase() in Rust
static std::string to_lowercase(const std::string& str) {
	std::string result = str;
	std::transform(result.begin(), result.end(), result.begin(),
		[](unsigned char c) { return std::tolower(c); });
	return result;
}

// ============================================================================
// EmbeddedResultSet implementation
// ============================================================================

EmbeddedResultSet::EmbeddedResultSet(std::shared_ptr<Plan> plan, std::shared_ptr<EmbeddedConnection> c)
	: s(nullptr), sch(nullptr), conn(std::move(c)) {
	// Corresponds to EmbeddedResultSet::new in embeddedresultset.rs:23-32
	if (plan) {
		// TODO: Once Plan::open() and Plan::schema() are migrated, uncomment:
		// s = plan->open();
		// sch = plan->schema();
		(void)plan; // Suppress unused parameter warning until Plan is migrated
	}
	// For now, initialize with defaults
	sch = std::make_shared<record::Schema>();
}

bool EmbeddedResultSet::next() {
	// Corresponds to embeddedresultset.rs:36-42
	if (s) {
		try {
			return s->next();
		} catch (const std::exception&) {
			// On error, rollback and return false
			if (conn) {
				conn->rollback();
			}
			return false;
		}
	}
	return false; // No scan available
}

std::int32_t EmbeddedResultSet::get_int(std::string fldname) {
	// Corresponds to embeddedresultset.rs:44-51
	std::string fldname_lower = to_lowercase(fldname);

	if (s) {
		try {
			return s->get_int(fldname_lower);
		} catch (const std::exception&) {
			// On error, rollback and return 0
			if (conn) {
				conn->rollback();
			}
			return 0;
		}
	}
	return 0; // No scan available
}

std::string EmbeddedResultSet::get_string(std::string fldname) {
	// Corresponds to embeddedresultset.rs:53-60
	std::string fldname_lower = to_lowercase(fldname);

	if (s) {
		try {
			return s->get_string(fldname_lower);
		} catch (const std::exception&) {
			// On error, rollback and return empty string
			if (conn) {
				conn->rollback();
			}
			return "";
		}
	}
	return ""; // No scan available
}

const Metadata* EmbeddedResultSet::get_meta_data() const noexcept {
	// Corresponds to embeddedresultset.rs:62-64
	static EmbeddedMetadata md{std::make_shared<record::Schema>()};
	// TODO: Once migration is complete, return proper metadata:
	// static EmbeddedMetadata md{sch};
	// return &md;
	return &md;
}

void EmbeddedResultSet::close() {
	// Corresponds to embeddedresultset.rs:66-69
	if (s) {
		s->close();
	}
	if (conn) {
		conn->close();
	}
}

// NetworkResultSet
NetworkResultSet::NetworkResultSet(std::shared_ptr<NetworkConnection> /*conn*/, int64_t /*id*/) {}

bool NetworkResultSet::next() { return false; }

std::int32_t NetworkResultSet::get_int(std::string /*fldname*/) { return 0; }

std::string NetworkResultSet::get_string(std::string /*fldname*/) { return {}; }

const Metadata* NetworkResultSet::get_meta_data() const noexcept {
	static NetworkMetadata md{std::shared_ptr<NetworkConnection>{}, 0};
	return &md;
}

void NetworkResultSet::close() { /* no-op for mock */ }

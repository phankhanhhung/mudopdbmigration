#ifndef BUFFERNEEDS_HPP
#define BUFFERNEEDS_HPP

#include <cstddef>

namespace multibuffer {

/**
 * Computes optimal chunk sizes for multi-buffer operations.
 *
 * Corresponds to BufferNeeds in Rust (NMDB2/src/multibuffer/bufferneeds.rs)
 */
class BufferNeeds {
public:
    static size_t best_factor(size_t available, size_t size);
};

} // namespace multibuffer

#endif // BUFFERNEEDS_HPP

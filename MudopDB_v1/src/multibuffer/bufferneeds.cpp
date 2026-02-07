#include "multibuffer/bufferneeds.hpp"

namespace multibuffer {

size_t BufferNeeds::best_factor(size_t available, size_t size) {
    size_t avail = (available > 2) ? available - 2 : 0;
    if (avail <= 1) {
        return 1;
    }
    size_t k = size;
    size_t i = 1;
    while (k > avail) {
        i++;
        k = (size + i - 1) / i; // ceiling division
    }
    return k;
}

} // namespace multibuffer

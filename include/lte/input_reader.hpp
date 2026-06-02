#pragma once

namespace lte {

class InputReader {
public:
    bool has_input(int timeout_ms);
    int read_some(unsigned char* buffer, int capacity);
};

} // namespace lte

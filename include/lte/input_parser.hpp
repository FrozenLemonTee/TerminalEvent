#pragma once

#include "lte/native_event.hpp"

#include <span>
#include <vector>

namespace lte {

enum class ParseStatus {
    Emit,
    NeedMore,
    Invalid,
    Empty,
};

class InputParser {
public:
    void feed(std::span<const unsigned char> bytes);
    ParseStatus next_event(NativeEvent& out);
    ParseStatus flush_timeout(NativeEvent& out);
    bool empty() const;

private:
    std::vector<unsigned char> pending_;

    ParseStatus parse_escape(NativeEvent& out);
    ParseStatus parse_csi(NativeEvent& out);
    ParseStatus parse_ss3(NativeEvent& out);
    ParseStatus parse_utf8_or_ascii(NativeEvent& out);
    void consume(std::size_t count);
};

} // namespace lte

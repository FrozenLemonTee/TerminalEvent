#pragma once

#include "lte/native_event.hpp"

namespace lte {

class TerminalSession {
public:
    TerminalSession();
    TerminalSession(const TerminalSession&) = delete;
    TerminalSession& operator=(const TerminalSession&) = delete;
    ~TerminalSession();

    int enter_raw_mode();
    int restore();
    TerminalSize size() const;
    bool consume_resize_pending() const;

private:
    bool raw_enabled_ = false;

#ifndef _WIN32
    struct TermiosStorage;
    TermiosStorage* termios_ = nullptr;
#endif
};

} // namespace lte

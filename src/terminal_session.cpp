#include "lte/terminal_session.hpp"

#ifndef _WIN32
#include <atomic>
#include <csignal>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace lte {

#ifndef _WIN32
namespace {

std::atomic_bool resize_pending {false};
struct sigaction previous_winch {};
bool previous_winch_set = false;

void handle_winch(int signal)
{
    resize_pending.store(true);
    if (previous_winch_set) {
        if (previous_winch.sa_handler == SIG_DFL || previous_winch.sa_handler == SIG_IGN) {
            return;
        }
        previous_winch.sa_handler(signal);
    }
}

} // namespace

struct TerminalSession::TermiosStorage {
    termios original {};
};
#endif

TerminalSession::TerminalSession()
{
#ifndef _WIN32
    termios_ = new TermiosStorage();
#endif
}

TerminalSession::~TerminalSession()
{
    restore();
#ifndef _WIN32
    delete termios_;
    termios_ = nullptr;
#endif
}

int TerminalSession::enter_raw_mode()
{
#ifdef _WIN32
    return -1;
#else
    if (raw_enabled_) {
        return 0;
    }

    if (tcgetattr(STDIN_FILENO, &termios_->original) != 0) {
        return -1;
    }

    termios raw = termios_->original;
    raw.c_iflag &= static_cast<unsigned int>(~(BRKINT | ICRNL | INPCK | ISTRIP | IXON));
    raw.c_oflag &= static_cast<unsigned int>(~(OPOST));
    raw.c_cflag |= CS8;
    raw.c_lflag &= static_cast<unsigned int>(~(ECHO | ICANON | IEXTEN | ISIG));
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
        return -1;
    }

    struct sigaction action {};
    action.sa_handler = handle_winch;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    previous_winch_set = sigaction(SIGWINCH, &action, &previous_winch) == 0;

    raw_enabled_ = true;
    return 0;
#endif
}

int TerminalSession::restore()
{
#ifdef _WIN32
    return 0;
#else
    if (!raw_enabled_) {
        return 0;
    }

    int result = tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios_->original);
    if (previous_winch_set) {
        sigaction(SIGWINCH, &previous_winch, nullptr);
        previous_winch_set = false;
    }
    raw_enabled_ = false;
    return result == 0 ? 0 : -1;
#endif
}

TerminalSize TerminalSession::size() const
{
#ifdef _WIN32
    return {};
#else
    winsize ws {};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0) {
        return {};
    }
    return TerminalSize {static_cast<int>(ws.ws_col), static_cast<int>(ws.ws_row)};
#endif
}

bool TerminalSession::consume_resize_pending() const
{
#ifdef _WIN32
    return false;
#else
    return resize_pending.exchange(false);
#endif
}

} // namespace lte

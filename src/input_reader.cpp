#include "lte/input_reader.hpp"

#ifndef _WIN32
#include <cerrno>
#include <sys/select.h>
#include <unistd.h>
#else
#include <io.h>
#endif

namespace lte {

bool InputReader::has_input(int timeout_ms)
{
#ifdef _WIN32
    (void)timeout_ms;
    return false;
#else
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);

    timeval timeout {};
    timeval* timeout_ptr = nullptr;
    if (timeout_ms >= 0) {
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;
        timeout_ptr = &timeout;
    }

    int result = select(STDIN_FILENO + 1, &read_fds, nullptr, nullptr, timeout_ptr);
    return result > 0 && FD_ISSET(STDIN_FILENO, &read_fds);
#endif
}

int InputReader::read_some(unsigned char* buffer, int capacity)
{
    if (buffer == nullptr || capacity <= 0) {
        return -1;
    }

#ifdef _WIN32
    return -1;
#else
    int result = static_cast<int>(read(STDIN_FILENO, buffer, static_cast<std::size_t>(capacity)));
    if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
        return 0;
    }
    return result;
#endif
}

} // namespace lte

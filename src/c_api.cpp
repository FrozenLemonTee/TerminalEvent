#include "lte/c_api.h"

#include "lte/input_parser.hpp"
#include "lte/input_reader.hpp"
#include "lte/terminal_session.hpp"

#include <memory>
#include <span>
#include <string>

namespace {

struct BackendState {
    lte::TerminalSession session;
    lte::InputReader reader;
    lte::InputParser parser;
    lte::NativeEvent last_event;
    int last_error = 0;
};

std::unique_ptr<BackendState> state;

BackendState& backend()
{
    if (!state) {
        state = std::make_unique<BackendState>();
    }
    return *state;
}

int byte_at(const std::string& value, int index)
{
    if (index < 0 || index >= static_cast<int>(value.size())) {
        return -1;
    }
    return static_cast<unsigned char>(value[static_cast<std::size_t>(index)]);
}

int emit_event(const lte::NativeEvent& event)
{
    backend().last_event = event;
    return static_cast<int>(event.kind);
}

} // namespace

extern "C" {

int lte_init(void)
{
    backend();
    return 0;
}

int lte_shutdown(void)
{
    if (state) {
        state->session.restore();
    }
    state.reset();
    return 0;
}

int lte_enter_raw_mode(void)
{
    auto& b = backend();
    int result = b.session.enter_raw_mode();
    b.last_error = result == 0 ? 0 : -1;
    return result;
}

int lte_restore_terminal(void)
{
    auto& b = backend();
    int result = b.session.restore();
    b.last_error = result == 0 ? 0 : -1;
    return result;
}

int lte_terminal_width(void)
{
    return backend().session.size().width;
}

int lte_terminal_height(void)
{
    return backend().session.size().height;
}

int lte_poll_event(int timeout_ms)
{
    auto& b = backend();

    if (b.session.consume_resize_pending()) {
        return emit_event(lte::make_resize(b.session.size()));
    }

    lte::NativeEvent event;
    auto status = b.parser.next_event(event);
    if (status == lte::ParseStatus::Emit || status == lte::ParseStatus::Invalid) {
        return emit_event(event);
    }

    if (!b.reader.has_input(timeout_ms)) {
        status = b.parser.flush_timeout(event);
        if (status == lte::ParseStatus::Emit || status == lte::ParseStatus::Invalid) {
            return emit_event(event);
        }
        b.last_event = {};
        return static_cast<int>(lte::EventKind::None);
    }

    unsigned char buffer[128] {};
    int read = b.reader.read_some(buffer, static_cast<int>(sizeof(buffer)));
    if (read < 0) {
        b.last_error = -2;
        b.last_event.kind = lte::EventKind::Error;
        return static_cast<int>(lte::EventKind::Error);
    }
    if (read == 0) {
        b.last_event = {};
        return static_cast<int>(lte::EventKind::None);
    }

    b.parser.feed(std::span<const unsigned char>(buffer, static_cast<std::size_t>(read)));
    status = b.parser.next_event(event);
    if (status == lte::ParseStatus::Emit || status == lte::ParseStatus::Invalid) {
        return emit_event(event);
    }

    b.last_event = {};
    return static_cast<int>(lte::EventKind::None);
}

int lte_event_kind(void)
{
    return static_cast<int>(backend().last_event.kind);
}

int lte_event_key_code(void)
{
    return static_cast<int>(backend().last_event.key_code);
}

int lte_event_key_number(void)
{
    return backend().last_event.key_number;
}

int lte_event_modifiers(void)
{
    return backend().last_event.modifiers;
}

int lte_event_text_len(void)
{
    return static_cast<int>(backend().last_event.text.size());
}

int lte_event_text_char_at(int index)
{
    return byte_at(backend().last_event.text, index);
}

int lte_event_resize_width(void)
{
    return backend().last_event.width;
}

int lte_event_resize_height(void)
{
    return backend().last_event.height;
}

int lte_event_mouse_button(void)
{
    return backend().last_event.mouse_button;
}

int lte_event_mouse_action(void)
{
    return backend().last_event.mouse_action;
}

int lte_event_mouse_x(void)
{
    return backend().last_event.mouse_x;
}

int lte_event_mouse_y(void)
{
    return backend().last_event.mouse_y;
}

int lte_event_focus_state(void)
{
    return backend().last_event.focus_state;
}

int lte_event_unknown_len(void)
{
    return static_cast<int>(backend().last_event.unknown.size());
}

int lte_event_unknown_char_at(int index)
{
    return byte_at(backend().last_event.unknown, index);
}

int lte_last_error_code(void)
{
    return backend().last_error;
}

} // extern "C"

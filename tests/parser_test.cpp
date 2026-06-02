#include "lte/input_parser.hpp"

#include <cassert>
#include <cstring>
#include <string>

namespace {

lte::NativeEvent parse_one(lte::InputParser& parser)
{
    lte::NativeEvent event;
    auto status = parser.next_event(event);
    assert(status == lte::ParseStatus::Emit || status == lte::ParseStatus::Invalid);
    return event;
}

lte::NativeEvent parse_bytes(const char* bytes)
{
    lte::InputParser parser;
    parser.feed(std::span(
        reinterpret_cast<const unsigned char*>(bytes),
        std::strlen(bytes)));
    return parse_one(parser);
}

void assert_key(const lte::NativeEvent& event, lte::KeyCode code)
{
    assert(event.kind == lte::EventKind::Key);
    assert(event.key_code == code);
}

} // namespace

int main()
{
    {
        auto event = parse_bytes("a");
        assert_key(event, lte::KeyCode::Character);
        assert(event.text == "a");
    }

    {
        const char bytes[] = {'\x03', '\0'};
        auto event = parse_bytes(bytes);
        assert_key(event, lte::KeyCode::Character);
        assert(event.text == "c");
        assert((event.modifiers & lte::Modifier::Ctrl) != 0);
    }

    assert_key(parse_bytes("\r"), lte::KeyCode::Enter);
    assert_key(parse_bytes("\x7f"), lte::KeyCode::Backspace);
    assert_key(parse_bytes("\x1b[A"), lte::KeyCode::Up);
    assert_key(parse_bytes("\x1b[B"), lte::KeyCode::Down);
    assert_key(parse_bytes("\x1b[C"), lte::KeyCode::Right);
    assert_key(parse_bytes("\x1b[D"), lte::KeyCode::Left);
    assert_key(parse_bytes("\x1b[3~"), lte::KeyCode::Delete);
    assert_key(parse_bytes("\x1b[5~"), lte::KeyCode::PageUp);
    assert_key(parse_bytes("\x1b[6~"), lte::KeyCode::PageDown);

    {
        lte::InputParser parser;
        const unsigned char esc[] = {0x1b};
        parser.feed(std::span(esc));
        lte::NativeEvent event;
        assert(parser.next_event(event) == lte::ParseStatus::NeedMore);
        assert(parser.flush_timeout(event) == lte::ParseStatus::Emit);
        assert_key(event, lte::KeyCode::Escape);
    }

    {
        auto event = parse_bytes("\x1b[1;5A");
        assert_key(event, lte::KeyCode::Up);
        assert((event.modifiers & lte::Modifier::Ctrl) != 0);
    }

    {
        auto event = parse_bytes("\x1b[200~hello\x1b[201~");
        assert(event.kind == lte::EventKind::Paste);
        assert(event.text == "hello");
    }

    return 0;
}

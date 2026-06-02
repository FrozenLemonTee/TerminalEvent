#include "lte/native_event.hpp"

#include <utility>

namespace lte {

NativeEvent make_key(const KeyCode code, const std::string_view text, const int modifiers, const int key_number)
{
    NativeEvent event;
    event.kind = EventKind::Key;
    event.key_code = code;
    event.key_number = key_number;
    event.modifiers = modifiers;
    event.text = text;
    return event;
}

NativeEvent make_resize(const TerminalSize size)
{
    NativeEvent event;
    event.kind = EventKind::Resize;
    event.width = size.width;
    event.height = size.height;
    return event;
}

NativeEvent make_paste(std::string text)
{
    NativeEvent event;
    event.kind = EventKind::Paste;
    event.text = std::move(text);
    return event;
}

NativeEvent make_unknown(std::string bytes)
{
    NativeEvent event;
    event.kind = EventKind::Unknown;
    event.unknown = std::move(bytes);
    return event;
}

} // namespace lte

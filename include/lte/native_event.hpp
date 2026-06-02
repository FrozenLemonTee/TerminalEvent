#pragma once

#include <string>

namespace lte {

enum class EventKind : int {
    Error = -1,
    None = 0,
    Key = 1,
    Resize = 2,
    Paste = 3,
    Mouse = 4,
    Focus = 5,
    Unknown = 6,
};

enum class KeyCode : int {
    Character = 0,
    Enter = 1,
    Escape = 2,
    Backspace = 3,
    Tab = 4,
    Left = 5,
    Right = 6,
    Up = 7,
    Down = 8,
    Home = 9,
    End = 10,
    PageUp = 11,
    PageDown = 12,
    Insert = 13,
    Delete = 14,
    Function = 15,
};

enum Modifier : int {
    Shift = 1 << 0,
    Ctrl = 1 << 1,
    Alt = 1 << 2,
    Meta = 1 << 3,
};

struct TerminalSize {
    int width = 0;
    int height = 0;
};

struct NativeEvent {
    EventKind kind = EventKind::None;

    KeyCode key_code = KeyCode::Character;
    int key_number = 0;
    int modifiers = 0;
    std::string text;

    int width = 0;
    int height = 0;

    int mouse_button = 0;
    int mouse_action = 0;
    int mouse_x = 0;
    int mouse_y = 0;

    int focus_state = 0;
    std::string unknown;
};

NativeEvent make_key(KeyCode code, std::string_view text = {}, int modifiers = 0, int key_number = 0);
NativeEvent make_resize(TerminalSize size);
NativeEvent make_paste(std::string text);
NativeEvent make_unknown(std::string bytes);

} // namespace lte

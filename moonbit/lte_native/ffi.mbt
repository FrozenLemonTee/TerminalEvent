///|
/// Raw C ABI bindings for the C++ TerminalEvent backend.
///
/// Higher-level MoonBit event mapping belongs in LunarEvent. This package only
/// mirrors the stable `lte_*` C ABI exposed by TerminalEvent.
pub const EVENT_KIND_ERROR : Int = -1

///|
pub const EVENT_KIND_NONE : Int = 0

///|
pub const EVENT_KIND_KEY : Int = 1

///|
pub const EVENT_KIND_RESIZE : Int = 2

///|
pub const EVENT_KIND_PASTE : Int = 3

///|
pub const EVENT_KIND_MOUSE : Int = 4

///|
pub const EVENT_KIND_FOCUS : Int = 5

///|
pub const EVENT_KIND_UNKNOWN : Int = 6

///|
pub const KEY_CODE_CHARACTER : Int = 0

///|
pub const KEY_CODE_ENTER : Int = 1

///|
pub const KEY_CODE_ESCAPE : Int = 2

///|
pub const KEY_CODE_BACKSPACE : Int = 3

///|
pub const KEY_CODE_TAB : Int = 4

///|
pub const KEY_CODE_LEFT : Int = 5

///|
pub const KEY_CODE_RIGHT : Int = 6

///|
pub const KEY_CODE_UP : Int = 7

///|
pub const KEY_CODE_DOWN : Int = 8

///|
pub const KEY_CODE_HOME : Int = 9

///|
pub const KEY_CODE_END : Int = 10

///|
pub const KEY_CODE_PAGE_UP : Int = 11

///|
pub const KEY_CODE_PAGE_DOWN : Int = 12

///|
pub const KEY_CODE_INSERT : Int = 13

///|
pub const KEY_CODE_DELETE : Int = 14

///|
pub const KEY_CODE_FUNCTION : Int = 15

///|
pub const MODIFIER_SHIFT : Int = 1

///|
pub const MODIFIER_CTRL : Int = 2

///|
pub const MODIFIER_ALT : Int = 4

///|
pub const MODIFIER_META : Int = 8

///|
pub const MOUSE_BUTTON_UNKNOWN : Int = 0

///|
pub const MOUSE_BUTTON_LEFT : Int = 1

///|
pub const MOUSE_BUTTON_RIGHT : Int = 2

///|
pub const MOUSE_BUTTON_MIDDLE : Int = 3

///|
pub const MOUSE_BUTTON_BACK : Int = 4

///|
pub const MOUSE_BUTTON_FORWARD : Int = 5

///|
pub const MOUSE_ACTION_UNKNOWN : Int = 0

///|
pub const MOUSE_ACTION_MOVE : Int = 1

///|
pub const MOUSE_ACTION_DOWN : Int = 2

///|
pub const MOUSE_ACTION_UP : Int = 3

///|
pub const MOUSE_ACTION_DRAG : Int = 4

///|
pub const MOUSE_ACTION_SCROLL : Int = 5

///|
pub const FOCUS_STATE_LOST : Int = 0

///|
pub const FOCUS_STATE_GAINED : Int = 1

///|
extern "c" fn c_init() -> Int = "lte_init"

///|
extern "c" fn c_shutdown() -> Int = "lte_shutdown"

///|
extern "c" fn c_enter_raw_mode() -> Int = "lte_enter_raw_mode"

///|
extern "c" fn c_restore_terminal() -> Int = "lte_restore_terminal"

///|
extern "c" fn c_terminal_width() -> Int = "lte_terminal_width"

///|
extern "c" fn c_terminal_height() -> Int = "lte_terminal_height"

///|
extern "c" fn c_poll_event(timeout_ms : Int) -> Int = "lte_poll_event"

///|
extern "c" fn c_event_kind() -> Int = "lte_event_kind"

///|
extern "c" fn c_event_key_code() -> Int = "lte_event_key_code"

///|
extern "c" fn c_event_key_number() -> Int = "lte_event_key_number"

///|
extern "c" fn c_event_modifiers() -> Int = "lte_event_modifiers"

///|
extern "c" fn c_event_text_len() -> Int = "lte_event_text_len"

///|
extern "c" fn c_event_text_char_at(index : Int) -> Int = "lte_event_text_char_at"

///|
extern "c" fn c_event_resize_width() -> Int = "lte_event_resize_width"

///|
extern "c" fn c_event_resize_height() -> Int = "lte_event_resize_height"

///|
extern "c" fn c_event_mouse_button() -> Int = "lte_event_mouse_button"

///|
extern "c" fn c_event_mouse_action() -> Int = "lte_event_mouse_action"

///|
extern "c" fn c_event_mouse_x() -> Int = "lte_event_mouse_x"

///|
extern "c" fn c_event_mouse_y() -> Int = "lte_event_mouse_y"

///|
extern "c" fn c_event_focus_state() -> Int = "lte_event_focus_state"

///|
extern "c" fn c_event_unknown_len() -> Int = "lte_event_unknown_len"

///|
extern "c" fn c_event_unknown_char_at(index : Int) -> Int = "lte_event_unknown_char_at"

///|
extern "c" fn c_last_error_code() -> Int = "lte_last_error_code"

///|
pub fn init_backend() -> Int {
  c_init()
}

///|
pub fn shutdown() -> Int {
  c_shutdown()
}

///|
pub fn enter_raw_mode() -> Int {
  c_enter_raw_mode()
}

///|
pub fn restore_terminal() -> Int {
  c_restore_terminal()
}

///|
pub fn terminal_width() -> Int {
  c_terminal_width()
}

///|
pub fn terminal_height() -> Int {
  c_terminal_height()
}

///|
pub fn poll_event(timeout_ms : Int) -> Int {
  c_poll_event(timeout_ms)
}

///|
pub fn event_kind() -> Int {
  c_event_kind()
}

///|
pub fn event_key_code() -> Int {
  c_event_key_code()
}

///|
pub fn event_key_number() -> Int {
  c_event_key_number()
}

///|
pub fn event_modifiers() -> Int {
  c_event_modifiers()
}

///|
pub fn event_text_len() -> Int {
  c_event_text_len()
}

///|
pub fn event_text_char_at(index : Int) -> Int {
  c_event_text_char_at(index)
}

///|
pub fn event_resize_width() -> Int {
  c_event_resize_width()
}

///|
pub fn event_resize_height() -> Int {
  c_event_resize_height()
}

///|
pub fn event_mouse_button() -> Int {
  c_event_mouse_button()
}

///|
pub fn event_mouse_action() -> Int {
  c_event_mouse_action()
}

///|
pub fn event_mouse_x() -> Int {
  c_event_mouse_x()
}

///|
pub fn event_mouse_y() -> Int {
  c_event_mouse_y()
}

///|
pub fn event_focus_state() -> Int {
  c_event_focus_state()
}

///|
pub fn event_unknown_len() -> Int {
  c_event_unknown_len()
}

///|
pub fn event_unknown_char_at(index : Int) -> Int {
  c_event_unknown_char_at(index)
}

///|
pub fn last_error_code() -> Int {
  c_last_error_code()
}

const KIND_ERROR : Int = -1
const KIND_NONE : Int = 0
const KIND_KEY : Int = 1
const KIND_RESIZE : Int = 2
const KIND_PASTE : Int = 3
const KIND_MOUSE : Int = 4
const KIND_FOCUS : Int = 5
const KIND_UNKNOWN : Int = 6

const KEY_CHARACTER : Int = 0
const KEY_ENTER : Int = 1
const KEY_ESCAPE : Int = 2
const KEY_BACKSPACE : Int = 3
const KEY_TAB : Int = 4
const KEY_LEFT : Int = 5
const KEY_RIGHT : Int = 6
const KEY_UP : Int = 7
const KEY_DOWN : Int = 8
const KEY_HOME : Int = 9
const KEY_END : Int = 10
const KEY_PAGE_UP : Int = 11
const KEY_PAGE_DOWN : Int = 12
const KEY_INSERT : Int = 13
const KEY_DELETE : Int = 14
const KEY_FUNCTION : Int = 15

pub struct EventBackend {}

pub fn EventBackend::new() -> EventBackend {
  @lte_native.init_backend() |> ignore
  EventBackend::{}
}

pub fn EventBackend::shutdown(self : EventBackend) -> Unit {
  ignore(self)
  @lte_native.shutdown() |> ignore
}

pub fn EventBackend::enter_raw_mode(
  self : EventBackend,
) -> Result[Unit, TerminalError] {
  ignore(self)
  match @lte_native.enter_raw_mode() {
    0 => Ok(())
    _ => Err(TerminalError::NativeError(@lte_native.last_error_code()))
  }
}

pub fn EventBackend::restore_terminal(self : EventBackend) -> Unit {
  ignore(self)
  @lte_native.restore_terminal() |> ignore
}

pub fn EventBackend::terminal_size(
  self : EventBackend,
) -> Result[Size, TerminalError] {
  ignore(self)
  let width = @lte_native.terminal_width()
  let height = @lte_native.terminal_height()
  if width > 0 && height > 0 {
    Ok(Size::{ width: width, height: height })
  } else {
    Err(TerminalError::NativeError(@lte_native.last_error_code()))
  }
}

pub fn EventBackend::poll_event(
  self : EventBackend,
  timeout_ms : Int,
) -> TerminalEvent? {
  ignore(self)
  match @lte_native.poll_event(timeout_ms) {
    KIND_NONE => None
    KIND_KEY => Some(TerminalEvent::Key(build_key_event()))
    KIND_RESIZE => Some(TerminalEvent::Resize(build_resize_event()))
    KIND_PASTE => Some(TerminalEvent::Paste(read_text()))
    KIND_MOUSE => Some(TerminalEvent::Mouse(build_mouse_event()))
    KIND_FOCUS => Some(TerminalEvent::Focus(build_focus_event()))
    KIND_UNKNOWN => Some(TerminalEvent::Unknown(read_unknown()))
    KIND_ERROR => None
    _ => None
  }
}

fn build_key_event() -> KeyEvent {
  let text = read_text()
  KeyEvent::{
    code: read_key_code(),
    text: if text.length() > 0 { Some(text) } else { None },
    modifiers: read_modifiers(),
  }
}

fn build_resize_event() -> ResizeEvent {
  ResizeEvent::{
    width: @lte_native.event_resize_width(),
    height: @lte_native.event_resize_height(),
  }
}

fn build_mouse_event() -> MouseEvent {
  MouseEvent::{
    button: read_mouse_button(),
    action: read_mouse_action(),
    x: @lte_native.event_mouse_x(),
    y: @lte_native.event_mouse_y(),
    modifiers: read_modifiers(),
  }
}

fn build_focus_event() -> FocusEvent {
  FocusEvent::{
    state: if @lte_native.event_focus_state() == 1 {
      FocusState::Gained
    } else {
      FocusState::Lost
    },
  }
}

fn read_key_code() -> KeyCode {
  match @lte_native.event_key_code() {
    KEY_CHARACTER => KeyCode::Character
    KEY_ENTER => KeyCode::Enter
    KEY_ESCAPE => KeyCode::Escape
    KEY_BACKSPACE => KeyCode::Backspace
    KEY_TAB => KeyCode::Tab
    KEY_LEFT => KeyCode::Left
    KEY_RIGHT => KeyCode::Right
    KEY_UP => KeyCode::Up
    KEY_DOWN => KeyCode::Down
    KEY_HOME => KeyCode::Home
    KEY_END => KeyCode::End
    KEY_PAGE_UP => KeyCode::PageUp
    KEY_PAGE_DOWN => KeyCode::PageDown
    KEY_INSERT => KeyCode::Insert
    KEY_DELETE => KeyCode::Delete
    KEY_FUNCTION => KeyCode::Function(@lte_native.event_key_number())
    _ => KeyCode::Unknown
  }
}

fn read_modifiers() -> Modifiers {
  let mask = @lte_native.event_modifiers()
  Modifiers::{
    shift: (mask & 1) != 0,
    ctrl: (mask & 2) != 0,
    alt: (mask & 4) != 0,
    meta: (mask & 8) != 0,
  }
}

fn read_mouse_button() -> MouseButton {
  match @lte_native.event_mouse_button() {
    1 => MouseButton::Left
    2 => MouseButton::Right
    3 => MouseButton::Middle
    4 => MouseButton::Back
    5 => MouseButton::Forward
    _ => MouseButton::Unknown
  }
}

fn read_mouse_action() -> MouseAction {
  match @lte_native.event_mouse_action() {
    1 => MouseAction::Move
    2 => MouseAction::Down
    3 => MouseAction::Up
    4 => MouseAction::Drag
    5 => MouseAction::Scroll
    _ => MouseAction::Unknown
  }
}

fn read_text() -> String {
  let mut text = ""
  for i in 0..<@lte_native.event_text_len() {
    let code = @lte_native.event_text_char_at(i)
    if code >= 0 {
      text = text + String::make(1, code.unsafe_to_char())
    }
  }
  text
}

fn read_unknown() -> String {
  let mut text = ""
  for i in 0..<@lte_native.event_unknown_len() {
    let code = @lte_native.event_unknown_char_at(i)
    if code >= 0 {
      text = text + String::make(1, code.unsafe_to_char())
    }
  }
  text
}

fn to_lunartui_event(event : @lte.TerminalEvent) -> @base.Event? {
  match event {
    @lte.TerminalEvent::Key(key) => Some(@base.Event::key(convert_key(key)))
    @lte.TerminalEvent::Resize(size) => {
      Some(@base.Event::resize(@base.ResizeEvent::{
        width: size.width,
        height: size.height,
      }))
    }
    @lte.TerminalEvent::Paste(text) => {
      Some(@base.Event::text_input(@base.TextInputEvent::{ text }))
    }
    @lte.TerminalEvent::Mouse(mouse) => convert_mouse(mouse)
    @lte.TerminalEvent::Focus(focus) => {
      Some(@base.Event::focus(@base.FocusEvent::{
        state: match focus.state {
          @lte.FocusState::Gained => @base.FocusState::Gained
          @lte.FocusState::Lost => @base.FocusState::Lost
        },
      }))
    }
    @lte.TerminalEvent::Unknown(_) => None
  }
}

fn convert_key(key : @lte.KeyEvent) -> @base.KeyEvent {
  @base.KeyEvent::{
    state: @base.KeyState::Down,
    code: convert_key_code(key),
    text: key.text,
    modifiers: convert_modifiers(key.modifiers),
    repeat_count: 1,
  }
}

fn convert_key_code(key : @lte.KeyEvent) -> @base.KeyCode {
  match key.code {
    @lte.KeyCode::Character => {
      match key.text {
        Some(" ") => @base.KeyCode::Space
        _ => @base.KeyCode::Character
      }
    }
    @lte.KeyCode::Enter => @base.KeyCode::Enter
    @lte.KeyCode::Escape => @base.KeyCode::Esc
    @lte.KeyCode::Backspace => @base.KeyCode::Backspace
    @lte.KeyCode::Tab => @base.KeyCode::Tab
    @lte.KeyCode::Left => @base.KeyCode::Left
    @lte.KeyCode::Right => @base.KeyCode::Right
    @lte.KeyCode::Up => @base.KeyCode::Up
    @lte.KeyCode::Down => @base.KeyCode::Down
    @lte.KeyCode::Home => @base.KeyCode::Home
    @lte.KeyCode::End => @base.KeyCode::End
    @lte.KeyCode::PageUp => @base.KeyCode::PageUp
    @lte.KeyCode::PageDown => @base.KeyCode::PageDown
    @lte.KeyCode::Insert => @base.KeyCode::Insert
    @lte.KeyCode::Delete => @base.KeyCode::Delete
    @lte.KeyCode::Function(n) => convert_function_key(n)
    @lte.KeyCode::Unknown => @base.KeyCode::Character
  }
}

fn convert_function_key(n : Int) -> @base.KeyCode {
  match n {
    1 => @base.KeyCode::F1
    2 => @base.KeyCode::F2
    3 => @base.KeyCode::F3
    4 => @base.KeyCode::F4
    5 => @base.KeyCode::F5
    6 => @base.KeyCode::F6
    7 => @base.KeyCode::F7
    8 => @base.KeyCode::F8
    9 => @base.KeyCode::F9
    10 => @base.KeyCode::F10
    11 => @base.KeyCode::F11
    12 => @base.KeyCode::F12
    _ => @base.KeyCode::Character
  }
}

fn convert_modifiers(mods : @lte.Modifiers) -> @base.Modifiers {
  @base.Modifiers::{
    shift: mods.shift,
    ctrl: mods.ctrl,
    alt: mods.alt,
    meta: mods.meta,
  }
}

fn convert_mouse(mouse : @lte.MouseEvent) -> @base.Event? {
  match (convert_mouse_action(mouse.action), convert_mouse_button(mouse.button)) {
    (Some(state), button) => {
      Some(@base.Event::mouse(@base.MouseEvent::{
        state,
        button,
        scroll: None,
        x: mouse.x,
        y: mouse.y,
        modifiers: convert_modifiers(mouse.modifiers),
      }))
    }
    _ => None
  }
}

fn convert_mouse_button(button : @lte.MouseButton) -> @base.MouseButton? {
  match button {
    @lte.MouseButton::Left => Some(@base.MouseButton::Left)
    @lte.MouseButton::Right => Some(@base.MouseButton::Right)
    @lte.MouseButton::Middle => Some(@base.MouseButton::Middle)
    @lte.MouseButton::Back => Some(@base.MouseButton::Back)
    @lte.MouseButton::Forward => Some(@base.MouseButton::Forward)
    @lte.MouseButton::Unknown => None
  }
}

fn convert_mouse_action(action : @lte.MouseAction) -> @base.MouseState? {
  match action {
    @lte.MouseAction::Move => Some(@base.MouseState::Move)
    @lte.MouseAction::Down => Some(@base.MouseState::Down)
    @lte.MouseAction::Up => Some(@base.MouseState::Up)
    @lte.MouseAction::Drag => Some(@base.MouseState::Drag)
    @lte.MouseAction::Scroll => Some(@base.MouseState::Scroll)
    @lte.MouseAction::Unknown => None
  }
}

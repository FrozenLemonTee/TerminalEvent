# C++ Terminal Event Backend MVP 草案（独立后端 + MoonBit 适配层｜C++ 求职简历特供）

> 本版修正一个关键边界：**MoonBit 应用层不应该手写 `NativeEvent -> LunarTUI @base.Event` 的转换**。  
> 正确设计是：C++ 终端事件后端项目提供稳定的 MoonBit 语言层 API，并额外提供一个可选的 LunarTUI adapter。MoonBit 应用只依赖“适配后的事件源”，不接触 C ABI、native event cache、getter、底层事件字段等实现细节。

---

## 1. 项目定位

本项目定位为一个独立的 **C++ Terminal Event Backend / Native Terminal Event Runtime**。

它不是 LunarTUI 的附属模块，也不是 LunarTUI 的内部 backend。它的核心价值是：

- 用 C++ 实现 POSIX 终端原生能力；
- 解析终端输入字节流和 ANSI / CSI / SS3 转义序列；
- 将键盘、resize、paste、mouse、focus 等输入归一化为稳定事件模型；
- 通过 C ABI 暴露跨语言边界；
- 提供 MoonBit binding 和可选 LunarTUI adapter，让 MoonBit TUI 应用可以直接消费高层事件。

关键定位：

```text
错误定位 1：LunarTUI 的 C++ backend
错误定位 2：C++ 项目内置完整 LunarTUI 应用运行时
错误定位 3：MoonBit 应用层手动完成 NativeEvent -> LunarTUI Event 转换

正确定位：
独立 C++ 终端事件后端
  + MoonBit binding
  + 可选 LunarTUI adapter
  + 示例 MoonBit 应用验证集成闭环
```

这样既能保持 C++ 项目的独立性，又不会把底层实现细节泄漏到应用层。

---

## 2. 修正后的依赖关系

### 2.1 推荐依赖图

```text
┌───────────────────────────────────────────────────────────────┐
│                  MoonBit Application / Demo                   │
│                                                               │
│  例如：tiny-editor / counter / list demo                       │
│                                                               │
│  只负责：                                                     │
│  - 创建 LunarTUI 组件树                                       │
│  - 调用适配后的 EventSource.poll_event()                      │
│  - root.handle_event(event)                                   │
│  - 根据 EventResult 决定是否重绘                              │
│                                                               │
│  不负责：                                                     │
│  - C ABI 调用                                                 │
│  - native event getter                                        │
│  - NativeEvent -> LunarTUI Event 字段转换                     │
└───────────────────────▲───────────────────────────────────────┘
                        │ @lte_lunartui.EventSource
                        │ returns @base.Event?
┌───────────────────────┴───────────────────────────────────────┐
│          MoonBit LunarTUI Adapter（可选桥接层）                 │
│                                                               │
│  - 依赖 MoonBit binding                                       │
│  - 依赖 LunarTUI                                              │
│  - 完成 TerminalEvent -> @base.Event 的转换                   │
│  - 对应用暴露 LunarTUI 友好的事件源                           │
└───────────────▲──────────────────────────────▲────────────────┘
                │ TerminalEvent                │ LunarTUI Event API
┌───────────────┴──────────────┐   ┌───────────┴────────────────┐
│ MoonBit Binding / Public API  │   │ LunarTUI                    │
│                               │   │                             │
│ - EventBackend                │   │ - Widget / Layout / Frame   │
│ - TerminalEvent               │   │ - @base.Event               │
│ - KeyEvent / ResizeEvent      │   │ - Widget::handle_event      │
│ - poll_event(timeout)         │   │ - EventResult               │
└───────────────▲──────────────┘   └────────────────────────────┘
                │ hidden raw extern "C"
┌───────────────┴───────────────────────────────────────────────┐
│             C ABI Layer（稳定、扁平、隐藏实现）                 │
│                                                               │
│ - ltui_enter_raw_mode                                         │
│ - ltui_restore_terminal                                       │
│ - ltui_poll_event                                             │
│ - ltui_event_* getters                                        │
└───────────────▲───────────────────────────────────────────────┘
                │ C++ function boundary
┌───────────────┴───────────────────────────────────────────────┐
│        Independent C++ Terminal Event Backend Core             │
│                                                               │
│ - termios / select or poll / ioctl / SIGWINCH                 │
│ - pending buffer                                               │
│ - ANSI / CSI / UTF-8 parser                                    │
│ - NativeEvent cache                                            │
│ - RAII terminal session                                        │
└───────────────────────────────────────────────────────────────┘
```

### 2.2 关键结论

MoonBit 应用层应该看到的是：

```moonbit
let source = @lte_lunartui.EventSource::new()
source.enter_raw_mode()

defer source.restore_terminal()

while running {
  match source.poll_event(16) {
    Some(event) => {
      let result = root.handle_event(event)
      if result == @base.EventResult::Redraw {
        terminal.draw(root)
      }
    }
    None => ()
  }
}
```

而不是：

```moonbit
// 不推荐：应用层不应该知道这些细节
let native = @lte.poll_event(16)
let kind = native.kind
let key_code = native.key_code
let lunar_event = manually_convert_native_event_to_lunartui_event(native)
```

应用层只需要使用“已经适配到 LunarTUI 的事件源”。

---

## 3. 三层 MoonBit API 设计

为了既保持 C++ 后端独立，又避免应用层泄漏底层细节，建议将 MoonBit 侧分成三层。

### 3.1 Raw FFI 层：私有或半私有

建议包名：

```text
moonbit/lte_native
```

职责：

- 声明 `extern "C"` 函数；
- 调用 C ABI；
- 读取 C++ event cache 中的字段；
- 不对普通应用用户开放；
- 不依赖 LunarTUI。

示例：

```moonbit
extern "C" fn c_enter_raw_mode() -> Int = "lte_enter_raw_mode"
extern "C" fn c_restore_terminal() -> Int = "lte_restore_terminal"
extern "C" fn c_poll_event(timeout_ms : Int) -> Int = "lte_poll_event"
extern "C" fn c_event_kind() -> Int = "lte_event_kind"
extern "C" fn c_event_key_code() -> Int = "lte_event_key_code"
extern "C" fn c_event_modifiers() -> Int = "lte_event_modifiers"
```

这一层的存在只是为了隔离 C ABI，不应该成为 demo 应用的直接依赖。

### 3.2 Public Binding 层：稳定终端事件 API

建议包名：

```text
moonbit/lte
```

职责：

- 对外暴露稳定的 MoonBit 高层事件模型；
- 隐藏 C ABI getter；
- 不依赖 LunarTUI；
- 可以被任何 MoonBit 项目使用，不限于 LunarTUI。

建议公开类型：

```moonbit
pub(all) enum TerminalEvent {
  Key(KeyEvent)
  Resize(ResizeEvent)
  Paste(String)
  Mouse(MouseEvent)
  Focus(FocusEvent)
  Unknown(String)
}

pub(all) struct KeyEvent {
  code : KeyCode
  text : String?
  modifiers : Modifiers
}

pub(all) enum KeyCode {
  Character
  Enter
  Escape
  Backspace
  Tab
  Left
  Right
  Up
  Down
  Home
  End
  PageUp
  PageDown
  Insert
  Delete
  Function(Int)
  Unknown(String)
}

pub(all) struct ResizeEvent {
  width : Int
  height : Int
}

pub(all) struct EventBackend {
  // 内部可以为空，也可以保存 backend id / session id
}

pub fn EventBackend::new() -> EventBackend
pub fn EventBackend::enter_raw_mode(self : EventBackend) -> Result[Unit, TerminalError]
pub fn EventBackend::restore_terminal(self : EventBackend) -> Unit
pub fn EventBackend::poll_event(self : EventBackend, timeout_ms : Int) -> TerminalEvent?
pub fn EventBackend::terminal_size(self : EventBackend) -> Result[Size, TerminalError]
```

这一层是 C++ 后端项目的主要 MoonBit API。它输出的是**后端无关、高层、稳定的终端事件**，不是 C ABI 结构，也不是 LunarTUI 类型。

### 3.3 LunarTUI Adapter 层：可选桥接层

建议包名：

```text
moonbit/lte_lunartui
```

职责：

- 依赖 `moonbit/lte`；
- 依赖 LunarTUI；
- 完成 `TerminalEvent -> @base.Event`；
- 对应用层暴露 LunarTUI 友好的 `EventSource`；
- 不让应用层手动接触转换细节。

建议 API：

```moonbit
pub struct EventSource {
  backend : @lte.EventBackend
}

pub fn EventSource::new() -> EventSource
pub fn EventSource::enter_raw_mode(self : EventSource) -> Result[Unit, @lte.TerminalError]
pub fn EventSource::restore_terminal(self : EventSource) -> Unit
pub fn EventSource::poll_event(self : EventSource, timeout_ms : Int) -> @base.Event?
```

内部才有转换函数：

```moonbit
fn to_lunartui_event(event : @lte.TerminalEvent) -> @base.Event? {
  match event {
    Key(key) => Some(convert_key_event(key))
    Resize(size) => Some(convert_resize_event(size))
    Paste(text) => Some(convert_paste_event(text))
    Mouse(mouse) => Some(convert_mouse_event(mouse))
    Focus(focus) => Some(convert_focus_event(focus))
    Unknown(_) => None
  }
}
```

这个函数可以存在，但不应该由应用层手写。它属于 **adapter 层**，不是 demo 业务逻辑。

---

## 4. 为什么 adapter 可以依赖 LunarTUI，但 core 不能依赖 LunarTUI

C++ 后端核心必须保持独立：

```text
C++ core 不依赖 LunarTUI
C ABI 不依赖 LunarTUI
moonbit/lte 不依赖 LunarTUI
```

这样它仍然是一个通用终端事件后端项目。

但是可选 adapter 可以依赖 LunarTUI：

```text
moonbit/lte_lunartui 依赖 moonbit/lte + LunarTUI
```

这不会破坏 C++ 后端项目的独立性，因为 adapter 只是一个“下游集成包”或“官方适配层”。它的作用是把底层实现细节封装起来，避免每个应用都重复写转换逻辑。

这类似于：

```text
核心库：只提供通用能力
官方 adapter：负责对接某个具体框架
应用层：只消费 adapter 暴露的高层接口
```

这比“应用层手写转换”更合理，也比“C++ core 直接绑定 LunarTUI”更健康。

---

## 5. 与 rabbita_tui 的关系

`rabbita_tui` 是 MoonBit 包，所以它采用：

```text
C FFI 很薄
MoonBit parser 很厚
MoonBit runtime 很厚
```

本项目为了服务 C++ 简历，应采用：

```text
C++ terminal backend 很厚
C++ parser 很厚
C ABI 中等
MoonBit binding 中等
LunarTUI adapter 很薄
MoonBit demo app 很薄
```

需要学习 rabbita_tui 的不是“完整运行时照搬”，而是：

1. 事件模型覆盖面；
2. pending buffer / incomplete input 的处理思想；
3. raw mode / restore / terminal size / resize handler 的 FFI 边界；
4. `Key / Mouse / Resize / Focus / Paste / Unknown` 的归一化事件设计。

但不要照搬：

- 完整 Program runtime；
- Cmd / Sub 系统；
- async task group；
- render ticker；
- subscriptions；
- headless runtime。

这些对 MVP 来说太重。

---

## 6. C++ Backend MVP 范围

第一阶段只做最小闭环。

### 6.1 必做能力

```text
1. raw mode enter / restore
2. RAII TerminalSession
3. select / poll based stdin wait
4. non-blocking or timeout read
5. pending buffer
6. ASCII / UTF-8 char parser
7. Ctrl key parser
8. Alt key parser
9. Enter / Backspace / Tab / Escape
10. Arrow / Home / End / Delete / PageUp / PageDown
11. terminal size query
12. SIGWINCH resize pending flag
13. C ABI event cache + getters
14. MoonBit public binding
15. LunarTUI adapter
16. counter/list demo 闭环
```

### 6.2 第二阶段加分

```text
1. bracketed paste
2. SGR mouse
3. focus gained / focus lost
4. F1-F12
5. modified CSI key
6. tiny editor demo
7. parser fuzz / property tests
```

### 6.3 暂时不做

```text
1. Windows Console API
2. 完整 Kitty keyboard protocol
3. 完整应用运行时
4. C++ 调 MoonBit trait object
5. 组件树调度
6. LunarTUI focus manager
7. 鼠标拖拽捕获
8. 多后端抽象
```

---

## 7. C++ 模块划分

推荐目录：

```text
cpp-terminal-event-backend/
├── include/
│   └── lte/
│       ├── terminal_session.hpp
│       ├── input_reader.hpp
│       ├── input_parser.hpp
│       ├── native_event.hpp
│       └── c_api.h
├── src/
│   ├── terminal_session.cpp
│   ├── input_reader.cpp
│   ├── input_parser.cpp
│   ├── native_event.cpp
│   └── c_api.cpp
├── moonbit/
│   ├── lte_native/
│   │   ├── ffi.mbt
│   │   └── moon.pkg.json
│   ├── lte/
│   │   ├── event.mbt
│   │   ├── backend.mbt
│   │   └── moon.pkg.json
│   └── lte_lunartui/
│       ├── adapter.mbt
│       ├── event_source.mbt
│       └── moon.pkg.json
├── examples/
│   ├── counter/
│   ├── list/
│   └── tiny_editor/      # 第二阶段
├── tests/
│   ├── parser_test.cpp
│   ├── c_api_smoke_test.cpp
│   └── adapter_test.mbt
├── CMakeLists.txt
└── README.md
```

### 7.1 `TerminalSession`

职责：

- 保存原始 termios；
- 进入 raw mode；
- 恢复终端；
- 查询终端尺寸；
- 安装 resize handler；
- 提供异常/退出安全的恢复逻辑。

建议：

```cpp
class TerminalSession {
public:
    int enter_raw_mode();
    int restore();
    TerminalSize size() const;
    ~TerminalSession();

private:
    bool raw_enabled_ = false;
    termios original_{};
};
```

### 7.2 `InputReader`

职责：

- `select` / `poll` 等待 stdin；
- 支持 timeout；
- 读取可用 bytes；
- 不解析事件，只负责字节读取。

建议：

```cpp
class InputReader {
public:
    bool has_input(int timeout_ms);
    int read_some(unsigned char* buffer, int capacity);
};
```

### 7.3 `InputParser`

职责：

- 保存 pending buffer；
- 解析 UTF-8；
- 解析 ESC / CSI / SS3；
- 识别 incomplete / invalid / emit；
- 输出 `NativeEvent`。

建议：

```cpp
enum class ParseStatus {
    Emit,
    NeedMore,
    Invalid,
};

class InputParser {
public:
    void feed(std::span<const unsigned char> bytes);
    ParseStatus next_event(NativeEvent& out);
    ParseStatus flush_timeout(NativeEvent& out);

private:
    std::vector<unsigned char> pending_;
};
```

### 7.4 `NativeEvent`

C++ 内部事件结构：

```cpp
enum class EventKind : int {
    None = 0,
    Key = 1,
    Resize = 2,
    Paste = 3,
    Mouse = 4,
    Focus = 5,
    Unknown = 6,
    Error = -1,
};

struct NativeEvent {
    EventKind kind = EventKind::None;

    int key_code = 0;
    int modifiers = 0;
    std::string text;

    int width = 0;
    int height = 0;

    int mouse_button = 0;
    int mouse_action = 0;
    int mouse_x = 0;
    int mouse_y = 0;

    std::string unknown;
};
```

---

## 8. C ABI 设计

C ABI 只暴露扁平函数，不暴露 C++ 类型，不暴露 `std::string`，不要求 MoonBit 管理 C++ 内存。

建议第一版：

```c
int lte_init(void);
int lte_shutdown(void);

int lte_enter_raw_mode(void);
int lte_restore_terminal(void);

int lte_terminal_width(void);
int lte_terminal_height(void);

int lte_poll_event(int timeout_ms);

int lte_event_kind(void);

int lte_event_key_code(void);
int lte_event_modifiers(void);
int lte_event_text_len(void);
int lte_event_text_char_at(int index);

int lte_event_resize_width(void);
int lte_event_resize_height(void);

int lte_event_mouse_button(void);
int lte_event_mouse_action(void);
int lte_event_mouse_x(void);
int lte_event_mouse_y(void);

int lte_event_unknown_len(void);
int lte_event_unknown_char_at(int index);

int lte_last_error_code(void);
```

`lte_poll_event(timeout_ms)` 的返回值：

```text
-1 = error
 0 = none
 1 = key
 2 = resize
 3 = paste
 4 = mouse
 5 = focus
 6 = unknown
```

MoonBit public binding 将这些 getter 封装成 `TerminalEvent`，应用层永远不直接使用这些 getter。

---

## 9. MoonBit 适配层设计

### 9.1 Public binding 示例

```moonbit
pub fn EventBackend::poll_event(
  self : EventBackend,
  timeout_ms : Int,
) -> TerminalEvent? {
  ignore(self)
  match @lte_native.poll_event(timeout_ms) {
    0 => None
    1 => Some(build_key_event())
    2 => Some(build_resize_event())
    3 => Some(build_paste_event())
    4 => Some(build_mouse_event())
    5 => Some(build_focus_event())
    6 => Some(build_unknown_event())
    _ => None
  }
}
```

### 9.2 LunarTUI adapter 示例

```moonbit
pub fn EventSource::poll_event(
  self : EventSource,
  timeout_ms : Int,
) -> @base.Event? {
  match self.backend.poll_event(timeout_ms) {
    Some(event) => to_lunartui_event(event)
    None => None
  }
}
```

应用层最终只使用 `EventSource`，不使用 `TerminalEvent` 也可以。

如果应用需要做非 LunarTUI 场景，也可以直接依赖 `moonbit/lte` 使用 `TerminalEvent`。

---

## 10. Demo 闭环

### 10.1 Counter Demo

目标：验证 key + redraw。

行为：

```text
↑：counter + 1
↓：counter - 1
q：退出
resize：重绘
```

应用层只写：

```moonbit
let source = @lte_lunartui.EventSource::new()
let root = CounterWidget::new()

while running {
  match source.poll_event(16) {
    Some(event) => {
      let result = root.handle_event(event)
      if result == @base.EventResult::Redraw {
        terminal.draw(root)
      }
    }
    None => ()
  }
}
```

### 10.2 List Demo

目标：验证方向键、Enter、组件状态。

行为：

```text
↑ / ↓：移动选中项
Enter：确认
q：退出
```

### 10.3 Tiny Editor Demo（第二阶段）

目标：验证完整简历叙事。

行为：

```text
普通字符输入
Backspace 删除
方向键移动光标
Ctrl+S 保存
resize 后重绘
```

它的价值是证明：

```text
C++ backend -> MoonBit binding -> LunarTUI adapter -> Widget event -> redraw -> file I/O
```

整条链路跑通。

---

## 11. 测试计划

### 11.1 C++ parser tests

必须做，简历价值很高。

```text
"a"                         -> Key(Character, "a")
"\x03"                      -> Ctrl+C
"\r"                        -> Enter
"\x7f"                      -> Backspace
"\x1b" + timeout            -> Escape
"\x1b[A"                    -> Up
"\x1b[B"                    -> Down
"\x1b[C"                    -> Right
"\x1b[D"                    -> Left
"\x1b[3~"                   -> Delete
"\x1b[5~"                   -> PageUp
"\x1b[6~"                   -> PageDown
"\x1b[200~hello\x1b[201~"   -> Paste("hello")
```

### 11.2 C ABI smoke tests

验证：

```text
init / shutdown
enter raw / restore
poll none
mock event getter
terminal size > 0
```

### 11.3 MoonBit binding tests

验证：

```text
C ABI kind -> TerminalEvent
modifier bitmask -> Modifiers
text getter -> String
unknown getter -> Unknown
```

### 11.4 LunarTUI adapter tests

验证：

```text
TerminalEvent::Key -> @base.Event::key
TerminalEvent::Resize -> @base.Event::resize
TerminalEvent::Paste -> @base.Event::text_input 或 paste 语义
Unknown -> None 或 Unknown
```

---

## 12. 简历表述

### 12.1 一行版

> 独立实现 C++ POSIX 终端事件后端，基于 `termios/select/ioctl/SIGWINCH` 与 ANSI/CSI 解析完成 raw mode、键盘输入、窗口 resize 等事件归一化，并通过 C ABI + MoonBit binding + LunarTUI adapter 打通跨语言 TUI 交互闭环。

### 12.2 项目经历版

> **C++ Terminal Event Backend：跨语言终端事件后端**  
> 独立设计并实现 POSIX 终端事件后端，使用 C++ 封装 raw mode、非阻塞输入读取、终端尺寸查询与 SIGWINCH resize 监听；实现 pending buffer 与 ANSI/CSI/UTF-8 输入解析器，将字符、Ctrl/Alt、方向键、特殊键和 resize 归一化为稳定事件模型；通过 C ABI 暴露跨语言边界，并提供 MoonBit binding 与 LunarTUI adapter，使 MoonBit TUI 应用可直接消费高层事件，完成 Counter/List/TinyEditor 等交互式示例闭环。

### 12.3 面试讲法

这个项目不是 LunarTUI 的附属，而是一个独立 C++ 后端。LunarTUI 只负责组件和渲染，C++ 后端只负责终端输入和事件归一化，中间通过 MoonBit binding 和 adapter 对接。这样做的好处是：

- C++ 后端可以被其他语言或其他 TUI 框架复用；
- LunarTUI 不绑定某个具体终端实现；
- 应用层不接触 C ABI 和 native event getter；
- 事件模型和 UI 框架之间通过 adapter 解耦；
- MVP 边界清晰，能快速做出可演示结果。

---

## 13. 最终结论

本项目最合理的 MVP 定位是：

```text
独立 C++ Terminal Event Backend
  + 稳定 C ABI
  + MoonBit public binding
  + 可选 LunarTUI adapter
  + MoonBit demo application
```

其中：

```text
C++ core：负责底层终端事件 runtime 和 parser
MoonBit binding：负责隐藏 C ABI，输出稳定 TerminalEvent
LunarTUI adapter：负责 TerminalEvent -> @base.Event
MoonBit app：只负责业务状态、组件树、事件循环和重绘
```

这条路线同时满足三个目标：

1. **C++ 简历有效**：核心技术量集中在 C++；
2. **工程边界合理**：不把 LunarTUI 绑死到 C++ 后端；
3. **MVP 可快速完成**：应用层不接触底层转换，demo 能很快跑通。

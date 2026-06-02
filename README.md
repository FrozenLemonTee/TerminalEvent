# TerminalEvent

Independent C++ terminal event backend for TUI applications.

The project follows the MVP boundary in `cpp_terminal_event_backend_mvp_plan_v3.md`:

- C++ core owns terminal raw mode, input reading, resize detection, and ANSI/CSI/UTF-8 parsing.
- C ABI exposes a flat stable surface named `lte_*`.
- MoonBit `lte` wraps the C ABI into a high-level `TerminalEvent` model.
- MoonBit `lte_lunartui` adapts `TerminalEvent` into LunarTUI `@base.Event`.

## Layout

```text
include/lte/        Public C++ and C ABI headers
src/                C++ implementation
moonbit/lte_native  Raw MoonBit FFI declarations
moonbit/lte         Public MoonBit terminal event API
moonbit/lte_lunartui LunarTUI adapter package
tests/              C++ parser and C ABI smoke tests
```

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

The terminal runtime is POSIX-first. On Windows, terminal raw-mode operations currently return unsupported values while parser tests remain platform-independent.

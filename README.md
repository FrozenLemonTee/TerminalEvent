# TerminalEvent

Independent C++ terminal event backend for TUI applications.

The project owns the native boundary only:

- C++ core owns terminal raw mode, input reading, resize detection, and ANSI/CSI/UTF-8 parsing.
- C ABI exposes a flat stable surface named `lte_*`.
- MoonBit `moonbit/lte_native` mirrors that C ABI for downstream MoonBit packages.

High-level MoonBit event modeling and LunarTUI adaptation live in `FrozenLemonTee/LunarEvent`.

## Layout

```text
include/lte/         Public C ABI and internal C++ headers
src/                 C++ implementation
moonbit/lte_native   Raw MoonBit FFI declarations for the `lte_*` C ABI
tests/               C++ parser and C ABI smoke tests
```

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

The terminal runtime is POSIX-first. On Windows, terminal raw-mode operations currently return unsupported values while parser tests remain platform-independent.

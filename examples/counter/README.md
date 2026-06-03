# Counter Demo

This example is now hosted at the LunarEvent integration layer.

Recommended input path:

1. Application imports `FrozenLemonTee/LunarEvent/lunartui`.
2. Create a `@lunartui.EventSource`.
3. Poll LunarTUI `@base.Event` values from that adapter.
4. Dispatch events to widgets and redraw on `EventResult::Redraw`.

TerminalEvent itself only exposes the raw `lte_*` C FFI surface.

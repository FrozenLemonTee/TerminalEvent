# Counter Demo

Planned MVP demo:

1. Create an `@lte_lunartui.EventSource`.
2. Enter raw mode.
3. Poll LunarTUI `@base.Event` values from the adapter.
4. Dispatch events to a counter widget.
5. Redraw on `EventResult::Redraw`.

Behavior:

- Up: increment
- Down: decrement
- q: quit
- resize: redraw

#pragma once

#ifdef _WIN32
#  ifdef LTE_BUILD_DLL
#    define LTE_API __declspec(dllexport)
#  elif defined(LTE_USE_DLL)
#    define LTE_API __declspec(dllimport)
#  else
#    define LTE_API
#  endif
#else
#  define LTE_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LTE_API int lte_init(void);
LTE_API int lte_shutdown(void);

LTE_API int lte_enter_raw_mode(void);
LTE_API int lte_restore_terminal(void);

LTE_API int lte_terminal_width(void);
LTE_API int lte_terminal_height(void);

LTE_API int lte_poll_event(int timeout_ms);

LTE_API int lte_event_kind(void);
LTE_API int lte_event_key_code(void);
LTE_API int lte_event_key_number(void);
LTE_API int lte_event_modifiers(void);
LTE_API int lte_event_text_len(void);
LTE_API int lte_event_text_char_at(int index);

LTE_API int lte_event_resize_width(void);
LTE_API int lte_event_resize_height(void);

LTE_API int lte_event_mouse_button(void);
LTE_API int lte_event_mouse_action(void);
LTE_API int lte_event_mouse_x(void);
LTE_API int lte_event_mouse_y(void);

LTE_API int lte_event_focus_state(void);

LTE_API int lte_event_unknown_len(void);
LTE_API int lte_event_unknown_char_at(int index);

LTE_API int lte_last_error_code(void);

#ifdef __cplusplus
}
#endif

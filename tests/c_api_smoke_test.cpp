#include "lte/c_api.h"

#include <cassert>

int main()
{
    assert(lte_init() == 0);
    assert(lte_event_kind() == 0);
    assert(lte_terminal_width() >= 0);
    assert(lte_terminal_height() >= 0);
    assert(lte_poll_event(0) >= -1);
    assert(lte_restore_terminal() == 0);
    assert(lte_shutdown() == 0);
    return 0;
}

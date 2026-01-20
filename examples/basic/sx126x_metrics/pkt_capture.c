#include "pkt_capture.h"
#include "thread.h"

typedef void (*capture_callback)(void *);

// Threads that need to run whenever availabile
#define THREAD_PRIORITY_MED (THREAD_PRIORITY_MAIN - 2)
// Stack for the stats thread
static char _stack[THREAD_STACKSIZE_MEDIUM];

static void *_pkt_capture_thread(void *ctx)
{
    capture_callback callback = ctx;
    (void)callback;

    while (1) {
    }
    return NULL;
}

void init_pkt_capture_thread(capture_callback callback)
{
    thread_create(_stack, sizeof(_stack), THREAD_PRIORITY_MED, 0, _pkt_capture_thread, callback, "packet capture");
}

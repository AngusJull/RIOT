#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "tsrb.h"

// Provide an ID for this board
#ifndef IDENTIFIER
#  define IDENTIFIER 0
#endif

struct display_thread_args {
    tsrb_t *power_ringbuffer;
    tsrb_t *netstat_ringbuffer;
    tsrb_t *capture_ringbuffer;
};

int init_display_thread(struct display_thread_args *args);

#endif // _DISPLAY_H_

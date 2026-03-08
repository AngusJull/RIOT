#ifndef _PKT_CAPTURE_H_
#define _PKT_CAPTURE_H_

#include "tsrb.h"

struct pkt_capture_thread_args {
    tsrb_t *capture_tsrb;
};

void init_pkt_capture_thread(struct pkt_capture_thread_args *args);

#endif // _PKT_CAPTURE_H_

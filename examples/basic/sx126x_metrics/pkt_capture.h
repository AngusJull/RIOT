#ifndef _PKT_CAPTURE_H_
#define _PKT_CAPTURE_H_

enum callback_type {
    CAPTURE_CALLBACK_RECIEVED,
    CAPTURE_CALLBACK_SENDING,
};

typedef void (*capture_callback)(void *, enum callback_type);

void init_pkt_capture_thread(capture_callback callback);

#endif // _PKT_CAPTURE_H_

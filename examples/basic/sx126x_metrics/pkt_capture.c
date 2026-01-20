#include "sys/errno.h"
#include "net/gnrc/netapi.h"
#include "net/gnrc/netreg.h"
#include "net/gnrc/pktbuf.h"
#include "thread.h"

#include "pkt_capture.h"

// Threads that need to run whenever availabile
#define THREAD_PRIORITY_MED (THREAD_PRIORITY_MAIN - 2)
// Stack for the stats thread
static char _stack[THREAD_STACKSIZE_MEDIUM];
// Size of the message queue for recieving packets
#define QUEUE_SIZE 5

static void *_pkt_capture_loop(void *ctx)
{
    capture_callback callback = ctx;
    (void)callback;
    static msg_t _msg_q[QUEUE_SIZE];
    msg_t msg;

    msg_t reply;
    reply.type = GNRC_NETAPI_MSG_TYPE_ACK;
    reply.content.value = -ENOTSUP;

    msg_init_queue(_msg_q, QUEUE_SIZE);
    gnrc_netreg_entry_t me_reg = GNRC_NETREG_ENTRY_INIT_PID(GNRC_NETREG_DEMUX_CTX_ALL,
                                                            thread_getpid());

    gnrc_netreg_register(GNRC_NETTYPE_IPV6, &me_reg);

    while (1) {
        msg_receive(&msg);

        switch (msg.type) {
        case GNRC_NETAPI_MSG_TYPE_RCV:
            // Recieving Data
            (void)msg.content.ptr;
            callback(msg.content.ptr, CAPTURE_CALLBACK_RECIEVED);
            gnrc_pktbuf_release(msg.content.ptr);
            break;
        case GNRC_NETAPI_MSG_TYPE_SND:
            // Sending data
            callback(msg.content.ptr, CAPTURE_CALLBACK_SENDING);
            (void)msg.content.ptr;
            gnrc_pktbuf_release(msg.content.ptr);
            break;
        // These case statements aren't needed for just watching for send/recv
        case GNRC_NETAPI_MSG_TYPE_GET:
        case GNRC_NETAPI_MSG_TYPE_SET:
            msg_reply(&msg, &reply);
            break;
        default:
            break;
        }
    }
    return NULL;
}

void init_pkt_capture_thread(capture_callback callback)
{
    thread_create(_stack, sizeof(_stack), THREAD_PRIORITY_MED, 0, _pkt_capture_loop, callback, "packet capture");
}

#include "net/gnrc/pktbuf.h"
#include "sys/errno.h"
#include "net/gnrc/netapi.h"
#include "net/gnrc/netreg.h"
#include "ztimer.h"

#define ENABLE_DEBUG 1
#include "debug.h"

#include "stats.h"
#include "pkt_capture.h"

// Threads that need to run whenever availabile
#define THREAD_PRIORITY_MED (THREAD_PRIORITY_MAIN - 2)
// Stack for the stats thread
static char _stack[THREAD_STACKSIZE_MEDIUM];
// Size of the message queue for recieving packets
#define QUEUE_SIZE 8

void add_capture_record(tsrb_t *capture_tsrb, void *pkt, enum capture_type type)
{
    (void)pkt;
    struct capture_record record = { .type = type, .time = ztimer_now(ZTIMER_MSEC) };
    add_record(capture_tsrb, (uint8_t *)&record, sizeof(record));
}

static void *_pkt_capture_loop(void *ctx)
{
    struct pkt_capture_thread_args *args = ctx;

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
            add_capture_record(args->capture_tsrb, msg.content.ptr, CAPTURE_RECORD_TYPE_RECV);
            gnrc_pktbuf_release(msg.content.ptr);
            break;
        case GNRC_NETAPI_MSG_TYPE_SND:
            // Sending data
            add_capture_record(args->capture_tsrb, msg.content.ptr, CAPTURE_RECORD_TYPE_SEND);
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

void init_pkt_capture_thread(struct pkt_capture_thread_args *args)
{
    thread_create(_stack, sizeof(_stack), THREAD_PRIORITY_MED, 0, _pkt_capture_loop, args, "packet capture");
}

#include "shell.h"
#include "tsrb.h"
#include "ztimer.h"

#include <stdio.h>

#include "display.h"
#include "stats.h"
#include "pkt_capture.h"

// Must be powers of two (limitation of tsrb)
#define MAX_BYTES_NETSTAT_RECORDS (2 << 12)
#define MAX_BYTES_POWER_RECORDS   (2 << 12)
#define MAX_BYTES_CAPTURE_RECORDS (2 << 10)

// Allocate statically, so that we don't need to use a memory allocator/linked list
static uint8_t netstat_buffer[MAX_BYTES_NETSTAT_RECORDS];
static uint8_t power_buffer[MAX_BYTES_POWER_RECORDS];
static uint8_t capture_buffer[MAX_BYTES_CAPTURE_RECORDS];

// use thread safe buffers for inter-thread communication and data storage
static tsrb_t netstat_ringbuffer;
static tsrb_t power_ringbuffer;
static tsrb_t capture_ringbuffer;

static void *_shell_loop(void *ctx)
{
    (void)ctx;
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);
    return NULL;
}

int main(void)
{
    tsrb_init(&netstat_ringbuffer, (unsigned char *)netstat_buffer, sizeof(netstat_buffer));
    tsrb_init(&power_ringbuffer, (unsigned char *)power_buffer, sizeof(power_buffer));
    tsrb_init(&capture_ringbuffer, (unsigned char *)capture_buffer, sizeof(capture_buffer));

    init_stats_thread(&(struct stats_thread_args){ .power_tsrb = &power_ringbuffer, .netstat_tsrb = &netstat_ringbuffer });
    init_display_thread(&(struct display_thread_args){ .power_ringbuffer = &power_ringbuffer, .netstat_ringbuffer = &netstat_ringbuffer, .capture_ringbuffer = &capture_ringbuffer });
    init_pkt_capture_thread(&(struct pkt_capture_thread_args){ .capture_tsrb = &capture_ringbuffer });

    (void)puts("Threads started, running shell\n");
    _shell_loop(NULL);

    return 0;
}

static int _dump_buffers(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    unsigned power_count = tsrb_avail(&power_ringbuffer);
    unsigned netstat_count = tsrb_avail(&netstat_ringbuffer);
    unsigned capture_count = tsrb_avail(&capture_ringbuffer);

    printf("Netstat bytes/count: %u/%u, Power bytes/count: %u/%u, Capture bytes/count: %u/%u\n",
           netstat_count, netstat_count / sizeof(struct netstat_record),
           power_count, power_count / sizeof(struct power_record),
           capture_count, capture_count / sizeof(struct capture_record));

    unsigned int current_time = ztimer_now(ZTIMER_MSEC);
    printf("Current time is %u\n", current_time);
    return 0;
}

SHELL_COMMAND(dump_buf, "Dump the stats collection buffers or associated information", _dump_buffers);

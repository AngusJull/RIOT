#include "shell.h"
#include "tsrb.h"

#include <stdio.h>

#include "display.h"
#include "stats.h"
#include "pkt_capture.h"

// Must be powers of two (limitation of tsrb)
#define MAX_NUM_NETSTAT_RECORDS (2 << 9)
#define MAX_NUM_POWER_RECORDS   (2 << 9)
#define MAX_NUM_CAPTURE_RECORDS (2 << 3)

// Allocate statically, so that we don't need to use a memory allocator/linked list
static struct netstat_record netstat_buffer[MAX_NUM_NETSTAT_RECORDS];
static struct power_record power_buffer[MAX_NUM_POWER_RECORDS];
static struct capture_record capture_buffer[MAX_NUM_CAPTURE_RECORDS];

// use Thread
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

    init_display_thread(NULL);
    init_pkt_capture_thread(NULL);
    init_stats_thread(&(struct stats_thread_args){ .power_tsrb = &power_ringbuffer, .netstat_tsrb = &netstat_ringbuffer });
    (void)puts("Threads started, running shell\n");
    _shell_loop(NULL);

    return 0;
}

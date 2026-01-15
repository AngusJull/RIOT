#include "net/gnrc.h"
#include "net/gnrc/pktdump.h"
#include "periph_conf_common.h"
#include "shell.h"
#include "thread.h"

#include "thread_config.h"
#include <stdio.h>

#include "display.h"
#include "battery.h"

// Threads that need to run whenever availabile
#define THREAD_PRIORITY_MED THREAD_PRIORITY_MAIN - 1

// Stack for the stats thread
static char _stack[THREAD_STACKSIZE_MEDIUM];

static void *_stats_loop(void *ctx)
{
    (void)ctx;

    init_display();
    init_battery_adc();
    while (1) {
        netstats_t test_stats = { 0 };
        unsigned int battery_mv = read_battery_mv();
        draw_display(battery_mv, 1, 1, test_stats);
        ztimer_sleep(ZTIMER_SEC, 1);
    }
    return NULL;
}

static void *_shell_loop(void *ctx)
{
    (void)ctx;
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);
    return NULL;
}

int main(void)
{
    // If this doesn't go first, gnrc may fail an assertion
    gnrc_netreg_entry_t dump = GNRC_NETREG_ENTRY_INIT_PID(GNRC_NETREG_DEMUX_CTX_ALL, gnrc_pktdump_pid);
    gnrc_netreg_register(GNRC_NETTYPE_UNDEF, &dump);

    // Start thread for stats and display, seperate from shell thread to allow both to run
    gnrc_pktdump_pid = thread_create(_stack, sizeof(_stack), THREAD_PRIORITY_MED, 0, &_shell_loop, NULL, "stats");

    (void)puts("Welcome to RIOT!");
    _stats_loop(NULL);

    return 0;
}

#include "net/gnrc.h"
#include "net/gnrc/pktdump.h"
#include "periph_conf_common.h"
#include "shell.h"
#include "thread.h"

#include "thread_config.h"
#include <stdio.h>

#include "display.h"
#include "stats.h"
#include "netstats.h"
#include "battery.h"

// Provide an ID for this board
#ifndef IDENTIFIER
#  define IDENTIFIER 0
#endif

#define ENABLE_DEBUG 1
#include "debug.h"

// Threads that need to run whenever availabile
#define THREAD_PRIORITY_MED THREAD_PRIORITY_MAIN - 1

// Stack for the stats thread
static char _stack[THREAD_STACKSIZE_MEDIUM];

void copy_to_records(uint32_t millivolts, enum netstat_type type, netstats_t *stats)
{
    struct power_record power = { .time = 0, .millivolts = millivolts };
    add_power_record(&power);
    struct netstat_record netstat;
    copy_netstat_to_record(stats, type, &netstat);
    add_netstat_record(&netstat);
}

static void *_stats_loop(void *ctx)
{
    (void)ctx;
    netif_t *radio_netif = get_lora_netif();

    init_display();
    init_battery_adc();
    while (1) {
        // Get new information
        unsigned int battery_mv = read_battery_mv();
        netstats_t stats;
        if (get_stats(radio_netif, NETSTATS_ALL, &stats) < 0) {
            DEBUG("Could not read stats\n");
        }
        // Copy for reading out later
        copy_to_records(battery_mv, NETSTATS_ALL, &stats);

        // Draw information
        draw_display(battery_mv, 1, IDENTIFIER, &stats);
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

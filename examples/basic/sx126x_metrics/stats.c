#include "stats.h"
#include "net/netif.h"
#include "netstats.h"
#include "battery.h"
#include "tsrb.h"
#include "ztimer.h"

#define ENABLE_DEBUG 0
#include "debug.h"

// Threads that need to run whenever availabile
#define THREAD_PRIORITY_MED (THREAD_PRIORITY_MAIN - 2)
// Stack for the stats thread
static char _stack[THREAD_STACKSIZE_MEDIUM];

int add_record(tsrb_t *tsrb, uint8_t *record, size_t size)
{
    // Overwrite oldest value
    if (tsrb_avail(tsrb) < size) {
        // Must drop whole records so we don't corrupt data
        tsrb_drop(tsrb, size);
        // Assume this completed successfully, but if it doesn't should just be unable to add
    }
    tsrb_add(tsrb, record, size);
    return 0;
}

static void copy_netstat_to_record(netstats_t *from, enum netstat_type type, struct netstat_record *dest)
{
    dest->rx_bytes = from->rx_bytes;
    dest->tx_bytes = from->tx_bytes;
    dest->tx_failed = from->tx_failed;
    dest->tx_success = from->tx_success;
    dest->tx_mcast_count = from->tx_mcast_count;
    dest->tx_unicast_count = from->tx_unicast_count;
    dest->rx_count = from->rx_count;
    dest->type = type;
    // Must use the same clock as other uses of ztimer_now
    dest->time = ztimer_now(ZTIMER_MSEC);
}

// Stats collection and recording loop
static void *_stats_loop(void *ctx)
{
    struct stats_thread_args *args = ctx;
    netif_t *radio_netif = get_lora_netif();
    DEBUG("Radio netif identified as %p\n", radio_netif);
    print_netif_name(radio_netif);

    init_battery_adc();
    while (1) {
        // Get new information
        struct power_record power = { .time = 0, .millivolts = read_battery_mv() };
        add_record(args->power_tsrb, (uint8_t *)&power, sizeof(power));

        netstats_t stats;
        if (radio_netif) {
            if (get_stats(radio_netif, NETSTATS_LAYER2, &stats) < 0) {
                DEBUG("Could not read stats\n");
            }
            else {
                struct netstat_record netstats;
                copy_netstat_to_record(&stats, NETSTATS_RECORD_TYPE_L2, &netstats);
                add_record(args->power_tsrb, (uint8_t *)&netstats, sizeof(netstats));
            }
        }

        // Draw information
        ztimer_sleep(ZTIMER_SEC, 1);
    }
    return NULL;
}

// Start collecting stats
int init_stats_thread(struct stats_thread_args *args)
{
    thread_create(_stack, sizeof(_stack), THREAD_PRIORITY_MED, 0, _stats_loop, args, "stats collection");
    return 0;
}

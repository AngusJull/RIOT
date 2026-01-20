#include "stats.h"
#include "net/netif.h"
#include "netstats.h"
#include "battery.h"
#include "tsrb.h"
#include "ztimer.h"

#define ENABLE_DEBUG 0
#include "debug.h"

// Must be powers of two
#define MAX_NUM_NETSTAT_RECORDS (2 << 9)
#define MAX_NUM_POWER_RECORDS   (2 << 9)

// Threads that need to run whenever availabile
#define THREAD_PRIORITY_MED     (THREAD_PRIORITY_MAIN - 2)
// Stack for the stats thread
static char _stack[THREAD_STACKSIZE_MEDIUM];

// Allocate statically, so that we don't need to use a memory allocator/linked list
static struct netstat_record netstat_buffer[MAX_NUM_NETSTAT_RECORDS];
static struct power_record power_buffer[MAX_NUM_POWER_RECORDS];

// Use thread same ringbuffer so that display can get same stats
static tsrb_t netstat_ringbuffer;
static tsrb_t power_ringbuffer;

static int add_power_record(struct power_record *record)
{
    // Overwrite oldest value
    if (tsrb_avail(&power_ringbuffer) < sizeof(*record)) {
        // Must drop whole records so we don't corrupt data
        tsrb_drop(&power_ringbuffer, sizeof(*record));
        // Assume this completed successfully, but if it doesn't should just be unable to add
    }
    tsrb_add(&power_ringbuffer, (uint8_t *)record, sizeof(*record));
    return 0;
}

static int add_netstat_record(struct netstat_record *record)
{
    // Overwrite oldest value
    if (tsrb_avail(&netstat_ringbuffer) < sizeof(*record)) {
        // Must drop whole records so we don't corrupt data
        tsrb_drop(&netstat_ringbuffer, sizeof(*record));
    }
    tsrb_add(&netstat_ringbuffer, (uint8_t *)record, sizeof(*record));
    return 0;
}

static void copy_netstat_to_record(netstats_t *from, enum netstat_type type, struct netstat_record *to)
{
    to->rx_bytes = from->rx_bytes;
    to->tx_bytes = from->tx_bytes;
    to->tx_failed = from->tx_failed;
    to->tx_success = from->tx_success;
    to->tx_mcast_count = from->tx_mcast_count;
    to->tx_unicast_count = from->tx_unicast_count;
    to->rx_count = from->rx_count;
    to->type = type;
    // TODO: Get time source
    to->time = 0;
}

static void copy_to_records(uint32_t millivolts, enum netstat_type type, netstats_t *stats)
{
    struct power_record power = { .time = 0, .millivolts = millivolts };
    add_power_record(&power);
    struct netstat_record netstat;
    copy_netstat_to_record(stats, type, &netstat);
    add_netstat_record(&netstat);
}

// Stats collection and recording loop
static void *_stats_loop(void *ctx)
{
    (void)ctx;
    netif_t *radio_netif = get_lora_netif();
    DEBUG("Radio netif identified as %p\n", radio_netif);
    print_netif_name(radio_netif);

    init_battery_adc();
    while (1) {
        // Get new information
        unsigned int battery_mv = read_battery_mv();
        netstats_t stats;

        if (radio_netif) {
            if (get_stats(radio_netif, NETSTATS_LAYER2, &stats) < 0) {
                DEBUG("Could not read stats\n");
            }
            else {
                copy_to_records(battery_mv, NETSTATS_ALL, &stats);
            }
        }

        // Draw information
        ztimer_sleep(ZTIMER_SEC, 1);
    }
    return NULL;
}

// Start collecting stats
int init_stats_thread(void *ctx)
{
    tsrb_init(&netstat_ringbuffer, (unsigned char *)netstat_buffer, sizeof(netstat_buffer));
    tsrb_init(&power_ringbuffer, (unsigned char *)power_buffer, sizeof(power_buffer));

    thread_create(_stack, sizeof(_stack), THREAD_PRIORITY_MED, 0, _stats_loop, ctx, "stats collection");
    return 0;
}

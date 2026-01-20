#include "stats.h"
#include "ringbuffer.h"

#define MAX_NUM_NETSTAT_RECORDS 1000
#define MAX_NUM_POWER_RECORDS   1000

// Allocate statically, so that we don't need to use a memory allocator/linked list
static struct netstat_record netstat_buffer[MAX_NUM_NETSTAT_RECORDS];
static struct power_record power_buffer[MAX_NUM_POWER_RECORDS];

static ringbuffer_t netstat_ringbuffer;
static ringbuffer_t power_ringbuffer;

int init_stats(void)
{
    ringbuffer_init(&netstat_ringbuffer, (char *)netstat_buffer, sizeof(netstat_buffer));
    ringbuffer_init(&power_ringbuffer, (char *)power_buffer, sizeof(power_buffer));
    return 0;
}

int add_power_record(struct power_record *record)
{
    // Overwrite oldest value
    uint8_t *data = (uint8_t *)record;
    for (unsigned i = 0; i < sizeof(*record); i++) {
        ringbuffer_add_one(&power_ringbuffer, data[i]);
    }
    return 0;
}

int add_netstat_record(struct netstat_record *record)
{
    // Overwrite oldest value
    uint8_t *data = (uint8_t *)record;
    for (unsigned i = 0; i < sizeof(*record); i++) {
        ringbuffer_add_one(&power_ringbuffer, data[i]);
    }
    return 0;
}

void copy_netstat_to_record(netstats_t *from, enum netstat_type type, struct netstat_record *to)
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

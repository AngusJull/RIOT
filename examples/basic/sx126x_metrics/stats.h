#ifndef _STATS_H_
#define _STATS_H_

#include "stdint.h"

typedef uint32_t stat_time_t;

struct power_record {
    stat_time_t time;
    uint32_t millivolts;
};

enum netstat_type {
    NETSTATS_TYPE_ALL,
    NETSTATS_TYPE_IPV6,
    NETSTATS_TYPE_L2,
};

// Stats for L2 or L3
struct netstat_record {
    stat_time_t time;
    enum netstat_type type;

    // Copied from netstats_t, so that we can remove or add fields later
    uint32_t tx_unicast_count; /**< packets sent via unicast */
    uint32_t tx_mcast_count;   /**< packets sent via multicast
                                     (including broadcast) */
    uint32_t tx_success;       /**< successful sending operations
                                     (either acknowledged or unconfirmed
                                     sending operation, e.g. multicast) */
    uint32_t tx_failed;        /**< failed sending operations */
    uint32_t tx_bytes;         /**< sent bytes */
    uint32_t rx_count;         /**< received (data) packets */
    uint32_t rx_bytes;         /**< received bytes */
};

int init_stats_thread(void *ctx);

#endif // _STATS_H_

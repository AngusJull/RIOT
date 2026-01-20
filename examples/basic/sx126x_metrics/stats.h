#ifndef _STATS_H_
#define _STATS_H_

#include "stdint.h"
#include "tsrb.h"

typedef uint32_t stat_time_t;

struct power_record {
    stat_time_t time;
    uint32_t millivolts;
};

enum netstat_type {
    NETSTATS_RECORD_TYPE_ALL,
    NETSTATS_RECORD_TYPE_IPV6,
    NETSTATS_RECORD_TYPE_L2,
};

// Stats for L2 or L3
struct netstat_record {
    enum netstat_type type;
    stat_time_t time;

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

enum capture_type {
    CAPTURE_RECORD_TYPE_RECV,
    CAPTURE_RECORD_TYPE_SEND,
};

// Stats for received and sent packets
struct capture_record {
    enum capture_type type;
    stat_time_t time;
};

struct stats_thread_args {
    tsrb_t *power_tsrb;
    tsrb_t *netstat_tsrb;
};

int init_stats_thread(struct stats_thread_args *args);

int add_record(tsrb_t *tsrb, uint8_t *record, size_t size);

#endif // _STATS_H_

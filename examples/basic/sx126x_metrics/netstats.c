#include "net/netdev.h"
#include "net/netopt.h"
#include "netstats.h"

#define ENABLE_DEBUG 1
#include "debug.h"

netif_t *get_lora_netif(void)
{
    netif_t *next = netif_iter(NULL);
    while (next) {
        uint16_t type;
        int res = netif_get_opt(next, NETOPT_DEVICE_TYPE, 0, &type, sizeof(type));
        if (res < 0) {
            DEBUG("Failed to get device type!\n");
        }

        if (type == NETDEV_TYPE_IEEE802154) {
            // Assume there's probably only one IEEE802.15.4 radio
            return next;
        }
    }
    return NULL;
}

int get_stats(netif_t *netif, unsigned int type, netstats_t *stats)
{
    return netif_get_opt(netif, NETOPT_STATS, type, stats, sizeof(*stats));
}

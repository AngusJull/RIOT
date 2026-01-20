#ifndef _NETSTATS_H_
#define _NETSTATS_H_

#include "net/netif.h"
#include "net/netstats.h"

netif_t *get_lora_netif(void);
int get_stats(netif_t *netif, unsigned int type, netstats_t *stats);
void print_netif_name(netif_t *netif);

#endif // _NETSTATS_H_

#include <string.h>
#include "net/gnrc/ipv6/ext/rh.h"
#include "net/ipv6/addr.h"
#include "net/gnrc/srv6/srh.h"

#define ENABLE_DEBUG 1
#include "debug.h"

static char addr_str[IPV6_ADDR_MAX_STR_LEN];

int gnrc_srv6_srh_process(ipv6_hdr_t *ipv6, gnrc_srv6_srh_t *rh, void **err_ptr)
{
    // Validate segments left must not exceed the actual list size
    // Excluding the first 8 bytes
    uint8_t total_segments = (rh->len >> 1);

    DEBUG("SRv6 SRH: processing header. Total segments: %u, Segments Left: %u\n",
          (unsigned)total_segments, (unsigned)rh->seg_left);

    if (rh->seg_left > total_segments) {
        DEBUG("SRv6 SRH: error - segments left (%u) > total segments (%u)\n",
              (unsigned)rh->seg_left, (unsigned)total_segments);

        *err_ptr = &rh->seg_left;
        return GNRC_IPV6_EXT_RH_ERROR;
    }

    // If segments left is 0, we have reached the final destination
    if (rh->seg_left == 0) {
        DEBUG("SRv6 SRH: segments left is 0, packet is at final destination\n");
        return GNRC_IPV6_EXT_RH_AT_DST;
    }

    // Extract the next segment from the list
    ipv6_addr_t *segment_list = (ipv6_addr_t *)(rh + 1);
    uint8_t next_seg_idx = rh->seg_left - 1;
    ipv6_addr_t *next_hop = &segment_list[next_seg_idx];

    // Update the Segments left counter
    rh->seg_left = next_seg_idx;

    DEBUG("SRv6 SRH: Swapping destination to Next Hop: %s\n",
          ipv6_addr_to_str(addr_str, next_hop, sizeof(addr_str)));

    // Swap the IPv6 Destination with the Next Segment
    memcpy(&ipv6->dst, next_hop, sizeof(ipv6_addr_t));

    // Tell the dispatcher to forward this packet to the new destination
    return GNRC_IPV6_EXT_RH_FORWARDED;
}

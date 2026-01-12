// For debug
#include "net/gnrc.h"
#include "net/gnrc/pktdump.h"
#include "net/gnrc/pktbuf.h"
#include "net/gnrc/netif.h"
#include "net/ipv6/addr.h"
#include "net/ipv6/hdr.h"
#include "net/gnrc/ipv6/hdr.h"
#include "net/gnrc/srv6/srh.h"
#include "net/gnrc/icmpv6/echo.h"
#include "net/protnum.h"

#include <stdio.h>
#include <string.h>

#include "shell.h"
#include "msg.h"

#define MAIN_QUEUE_SIZE (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

static int _srv6_ping(int argc, char **argv)
{
    if (argc < 3) {
        printf("Usage: srv6_ping <destination> <segment1> [segment2 ...]\n");
        return 1;
    }

    ipv6_addr_t dest;
    if (ipv6_addr_from_str(&dest, argv[1]) == NULL) {
        printf("Invalid destination address\n");
        return 1;
    }

    int num_segments = argc - 1;  // includes destination as last segment
    if (num_segments < 1 || num_segments > 255) {
        printf("Invalid number of segments (1-255)\n");
        return 1;
    }

    // allocate packet buffer for SRH
    size_t srh_size = sizeof(gnrc_srv6_srh_t) + num_segments * sizeof(ipv6_addr_t);
    gnrc_pktsnip_t *srh_snip = gnrc_pktbuf_add(NULL, NULL, srh_size, GNRC_NETTYPE_IPV6_EXT);
    if (srh_snip == NULL) {
        printf("Failed to allocate SRH\n");
        return 1;
    }

    gnrc_srv6_srh_t *srh = srh_snip->data;
    srh->nh = PROTNUM_ICMPV6;
    srh->len = (srh_size - 8) / 8;  // length in 8-octet units, excluding first 8
    srh->type = IPV6_EXT_RH_TYPE_SRV6;
    srh->seg_left = num_segments - 1;  // segments left should not include current destination
    srh->last_entry = num_segments - 1;
    srh->flags = 0;
    srh->tag = 0;

    ipv6_addr_t *segments = (ipv6_addr_t *)(srh + 1);
    for (int i = 0; i < num_segments; i++) {
        if (i < num_segments - 1) {
            // Parse intermediate segments
            if (ipv6_addr_from_str(&segments[i], argv[2 + i]) == NULL) {
                printf("Invalid segment address %s\n", argv[2 + i]);
                gnrc_pktbuf_release(srh_snip);
                return 1;
            }
        } else {
            // Last segment is the final destination
            memcpy(&segments[i], &dest, sizeof(ipv6_addr_t));
        }
    }

    // set the next destination to the first segment
    ipv6_addr_t ipv6_dest = segments[0];

    // allocate ICMPv6 echo request
    gnrc_pktsnip_t *pkt = gnrc_icmpv6_echo_build(ICMPV6_ECHO_REQ, 0, 1, NULL, 0); // we can replace this with whatever packet
    if (pkt == NULL) {
        printf("Failed to allocate ICMPv6 echo\n");
        gnrc_pktbuf_release(srh_snip);
        return 1;
    }

    // chain packets: SRH -> ICMP (SRH encapsulates the ICMP payload)
    srh_snip->next = pkt;

    // add IPv6 header with destination set to first segment
    // gnrc_ipv6_hdr_build will automatically set nh field based on next snip type
    gnrc_pktsnip_t *ipv6_snip = gnrc_ipv6_hdr_build(srh_snip, NULL, &ipv6_dest);
    if (ipv6_snip == NULL) {
        printf("Failed to allocate IPv6 header\n");
        gnrc_pktbuf_release(srh_snip);
        return 1;
    }
    pkt = ipv6_snip;

    // send packet to IPv6 layer for transmission
    if (!gnrc_netapi_dispatch_send(GNRC_NETTYPE_IPV6, GNRC_NETREG_DEMUX_CTX_ALL, pkt)) {
        printf("Failed to send packet\n");
        gnrc_pktbuf_release(pkt);
        return 1;
    }

    printf("SRv6 packet sent to %s with %d segments\n", argv[1], num_segments);
    return 0;
}

static const shell_command_t shell_commands[] = {
    { "srv6_ping", "Send ICMPv6 echo with SRv6 SRH", _srv6_ping },
    { NULL, NULL, NULL }
};

int main(void)
{
    // initialize message queue for the main thread
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);

    
    // only show icmpv6 echo requests (gets result of srv6_ping) 
    gnrc_netreg_entry_t dump_echo = GNRC_NETREG_ENTRY_INIT_PID(ICMPV6_ECHO_REQ,
                                                                gnrc_pktdump_pid);
    gnrc_netreg_register(GNRC_NETTYPE_ICMPV6, &dump_echo);

    (void)puts("Welcome to RIOT!");

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}

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
#include "net/gnrc/udp.h"
#include "net/udp.h"
#include "net/gnrc/netreg.h"
#include "thread.h"

#include <stdio.h>
#include <string.h>

#include "shell.h"
#include "msg.h"

#include "../sx126x_metrics/display.h"
#include "../sx126x_metrics/stats.h"
#include "../sx126x_metrics/pkt_capture.h"


#define SERVER_PORT     54321
#define BUF_SIZE        128
static char gnrc_udp_server_stack[THREAD_STACKSIZE_DEFAULT];

#define MAIN_QUEUE_SIZE (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

// Must be powers of two (limitation of tsrb)
#define MAX_BYTES_NETSTAT_RECORDS (2 << 12)
#define MAX_BYTES_POWER_RECORDS   (2 << 12)
#define MAX_BYTES_CAPTURE_RECORDS (2 << 10)

// Allocate statically, so that we don't need to use a memory allocator/linked list
static uint8_t netstat_buffer[MAX_BYTES_NETSTAT_RECORDS];
static uint8_t power_buffer[MAX_BYTES_POWER_RECORDS];
static uint8_t capture_buffer[MAX_BYTES_CAPTURE_RECORDS];

// use thread safe buffers for inter-thread communication and data storage
static tsrb_t netstat_ringbuffer;
static tsrb_t power_ringbuffer;
static tsrb_t capture_ringbuffer;


static void debug_print_snip_chain(const char *msg, gnrc_pktsnip_t *pkt) {
    printf("[SRv6 example] %s: snip chain: ", msg);
    while (pkt) {
        printf("[%d:%u]->", pkt->type, (unsigned)pkt->size);
        pkt = pkt->next;
    }
    puts("NULL");
}

static void *gnrc_udp_server_thread(void *arg)
{
    (void)arg;
    msg_t msg;
    gnrc_netreg_entry_t entry = GNRC_NETREG_ENTRY_INIT_PID(SERVER_PORT, thread_getpid());

    static msg_t server_msg_queue[MAIN_QUEUE_SIZE];
    msg_init_queue(server_msg_queue, MAIN_QUEUE_SIZE);

    // register to recieve UDP packets from server port
    gnrc_netreg_register(GNRC_NETTYPE_UDP, &entry);

    puts("GNRC UDP server started.");

    while (1) {
        msg_receive(&msg);
        if (msg.type == GNRC_NETAPI_MSG_TYPE_RCV) {
            gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t *)msg.content.ptr;
            // extract udp payload
            gnrc_pktsnip_t *payload = gnrc_pktsnip_search_type(pkt, GNRC_NETTYPE_UNDEF);
            if (payload) {
                size_t len = payload->size;
                char buf[BUF_SIZE];
                if (len > BUF_SIZE) len = BUF_SIZE;
                memcpy(buf, payload->data, len);
                printf("Received payload of %u bytes: <\t%.*s\t>\n", (unsigned)len, (int)len, buf);
            } else {
                printf("No payload found in recieved packet.\n");
            }
            gnrc_pktbuf_release(pkt);
        }
    }
    return NULL;
}

void gnrc_udp_server_start(void)
{
    thread_create(gnrc_udp_server_stack, sizeof(gnrc_udp_server_stack),
                  THREAD_PRIORITY_MAIN - 1, 0, gnrc_udp_server_thread, NULL, "gnrc_udp_srv");
}

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

    // allocate udp request
    uint8_t udp_payload[] = "hello world";
    gnrc_pktsnip_t *payload_snip = gnrc_pktbuf_add(NULL, udp_payload, sizeof(udp_payload) - 1, GNRC_NETTYPE_UNDEF);
    if (payload_snip == NULL) {
        printf("Failed to allocate UDP payload\n");
        return 1;
    }
    // build UDP header
    uint16_t src_port = 12345;
    uint16_t dst_port = SERVER_PORT;
    gnrc_pktsnip_t *udp_snip = gnrc_udp_hdr_build(payload_snip, src_port, dst_port);
    if (udp_snip == NULL) {
        printf("Failed to allocate UDP packet\n");
        gnrc_pktbuf_release(payload_snip);
        return 1;
    }
    udp_hdr_t *udp = udp_snip->data;
    udp->length = byteorder_htons(gnrc_pkt_len(udp_snip));

    // allocate packet buffer for SRH
    size_t srh_size = sizeof(gnrc_srv6_srh_t) + num_segments * sizeof(ipv6_addr_t);
    gnrc_pktsnip_t *srh_snip = gnrc_pktbuf_add(udp_snip, NULL, srh_size, GNRC_NETTYPE_IPV6_EXT);
    if (srh_snip == NULL) {
        printf("Failed to allocate SRH\n");
        gnrc_pktbuf_release(udp_snip);
        return 1;
    }

    gnrc_srv6_srh_t *srh = srh_snip->data;
    srh->nh = PROTNUM_UDP;
    srh->len = (srh_size - 8) / 8;  // length in 8-octet units, excluding first 8
    srh->type = IPV6_EXT_RH_TYPE_SRV6;
    srh->seg_left = num_segments - 1;  // segments left should not include current destination
    srh->last_entry = num_segments - 1;
    srh->flags = 0;
    srh->tag = 0;


    ipv6_addr_t *segments = (ipv6_addr_t *)(srh + 1);
    // segment_list[0] = final destination; segment_list[last_entry] = first hop
    memcpy(&segments[0], &dest, sizeof(ipv6_addr_t));

    for (int i = 0; i < num_segments-1; i++) {
        int seg_idx = num_segments-1 - i;      // fill from the end backwards
        if (ipv6_addr_from_str(&segments[seg_idx], argv[2 + i]) == NULL) {
            printf("Invalid segment address %s\n", argv[2 + i]);
            return 1;
        }
    }

    // initial IPv6 destination is the first hop
    ipv6_addr_t ipv6_dest = segments[srh->last_entry];

    // get current network config for source address
    gnrc_netif_t *netif = gnrc_netif_iter(NULL);
    ipv6_addr_t addrs[CONFIG_GNRC_NETIF_IPV6_ADDRS_NUMOF];
    int num_addrs = gnrc_netif_ipv6_addrs_get(netif, addrs, sizeof(addrs));
    ipv6_addr_t ipv6_src;
    if (num_addrs > 0) {
        ipv6_src = addrs[0];
        for (int i = 0; i < (int)(num_addrs / sizeof(ipv6_addr_t)); i++) {
            if (ipv6_addr_is_global(&addrs[i])) {
                ipv6_src = addrs[i];
                break;
            }
        }
    } else { printf("No source address found. Allowing automatic source population, meaning checksum will be invalid.\n"); }

    // add IPv6 header w/ first segment as dest and myself as source
    // gnrc_ipv6_hdr_build will auto-set nh field from next snip type
    gnrc_pktsnip_t *ipv6_snip = gnrc_ipv6_hdr_build(srh_snip, &ipv6_src, &ipv6_dest);
    if (ipv6_snip == NULL) {
        printf("Failed to allocate IPv6 header\n");
        gnrc_pktbuf_release(srh_snip);
        return 1;
    } 

    gnrc_pktsnip_t *pkt = ipv6_snip;

    // create pseudo IPv6 header with final dest for checksum
    gnrc_pktsnip_t *pseudo_ipv6_snip = gnrc_pktbuf_add(NULL, ipv6_snip->data,
                                                        ipv6_snip->size,
                                                        GNRC_NETTYPE_IPV6);
    if (pseudo_ipv6_snip == NULL) {
        printf("Failed to allocate pseudo IPv6 header\n");
        gnrc_pktbuf_release(pkt);
        return 1;
    }
    ipv6_hdr_t *pseudo_hdr = pseudo_ipv6_snip->data;
    memcpy(&pseudo_hdr->dst, &dest, sizeof(ipv6_addr_t));

    // calculate udp checksum
    if (gnrc_udp_calc_csum(udp_snip, pseudo_ipv6_snip) < 0) {
        printf("Failed to calculate UDP checksum\n");
    }
    gnrc_pktbuf_release(pseudo_ipv6_snip);

    // send packet to IPv6 layer for transmission 
    debug_print_snip_chain("before sending packet", pkt);
    if (!gnrc_netapi_dispatch_send(GNRC_NETTYPE_IPV6, GNRC_NETREG_DEMUX_CTX_ALL, pkt)) {
        printf("Failed to send packet\n");
        gnrc_pktbuf_release(pkt);
        return 1;
    }

    printf("SRv6 packet sent to %s with %d segments\n", argv[1], num_segments);
    return 0;
}

static const shell_command_t shell_commands[] = {
    { "srv6_ping", "Send UDP message with SRv6 SRH", _srv6_ping },
    { NULL, NULL, NULL }
};

int main(void)
{
    // initialize message queue for the main thread
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    gnrc_udp_init();
    
    (void)puts("Welcome to RIOT!");

    // init udp server for dest
    gnrc_udp_server_start();

    tsrb_init(&netstat_ringbuffer, (unsigned char *)netstat_buffer, sizeof(netstat_buffer));
    tsrb_init(&power_ringbuffer, (unsigned char *)power_buffer, sizeof(power_buffer));
    tsrb_init(&capture_ringbuffer, (unsigned char *)capture_buffer, sizeof(capture_buffer));

    init_stats_thread(&(struct stats_thread_args){ .power_tsrb = &power_ringbuffer, .netstat_tsrb = &netstat_ringbuffer });
    init_display_thread(&(struct display_thread_args){ .power_ringbuffer = &power_ringbuffer, .netstat_ringbuffer = &netstat_ringbuffer, .capture_ringbuffer = &capture_ringbuffer });
    init_pkt_capture_thread(&(struct pkt_capture_thread_args){ .capture_tsrb = &capture_ringbuffer });

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}

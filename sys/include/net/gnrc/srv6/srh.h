/*
 * Copyright (C) 2024
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#ifndef NET_GNRC_SRV6_H
#  define NET_GNRC_SRV6_H

/**
 * @defgroup    net_gnrc_srv6 SRv6 (srv6) Extension Header Processing
 * @ingroup     net_gnrc_ipv6
 * @brief       Support for IPv6 Segment Routing Headers (SRH)
 * @{
 *
 * @file
 * @brief       Definitions for SRv6 Segment Routing Headers
 * @author      
 */

#  include "net/ipv6/hdr.h"
#  include "net/ipv6/addr.h"

#  ifdef __cplusplus
extern "C" {
#  endif

/**
 * @brief IPv6 Routing Type for SRv6 (Type 4)
 * @see RFC 8754, Section 2
 */
#  define IPV6_EXT_RH_TYPE_SRV6 (4U)

/**
 * @brief   The SRv6 Segment Routing Header.
 * * This structure represents the fixed 8-byte portion of an SRH.
 * The Segment List (array of ipv6_addr_t) follows immediately after.
 *
 * @see <a href="https://tools.ietf.org/html/rfc8754">RFC 8754</a>
 * @extends gnrc_ipv6_ext_rh_t
 */
typedef struct __attribute__((packed)) {
    uint8_t nh;         /**< Next Header */
    uint8_t len;        /**< Hdr Ext Len: Length in 8-octet units, not including first 8 octets */
    uint8_t type;       /**< Routing Type: 4 for SRv6 */
    uint8_t seg_left;   /**< Segments Left: Number of route segments remaining */
    uint8_t last_entry; /**< Index of the last element in the Segment List */
    uint8_t flags;      /**< Flags (unused in this implementation) */
    uint16_t tag;       /**< Tag: Identifies a packet as part of a class or group */
} gnrc_srv6_srh_t;

/**
 * @brief   Process the SRv6 Segment Routing Header.
 *
 * This function handles the "Address Swap" logic: promoting the next 
 * segment from the list to the IPv6 Destination Address field.
 *
 * @param[in, out] ipv6 The IPv6 header of the incoming packet.
 * @param[in] rh        An SRv6 segment routing header.
 * @param[out] err_ptr  A pointer to an erroneous field within @p rh on error.
 *
 * @return  @ref GNRC_IPV6_EXT_RH_AT_DST, if the final destination is reached
 * @return  @ref GNRC_IPV6_EXT_RH_FORWARDED, when the packet was updated and should be forwarded
 * @return  @ref GNRC_IPV6_EXT_RH_ERROR, on validation error
 */
int gnrc_srv6_srh_process(ipv6_hdr_t *ipv6, gnrc_srv6_srh_t *rh, void **err_ptr);

#  ifdef __cplusplus
}
#  endif

#endif /* NET_GNRC_SRV6_H */
       /** @} */

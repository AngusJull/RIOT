/*
 * Copyright (C) 2026
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     net_gnrc_sixlowpan
 * @{
 *
 * @file
 * @brief       GHC (Generic Header Compression) definitions
 *
 * @author      Brian Tran
 */

#ifndef NET_GNRC_SIXLOWPAN_GHC_H
#define NET_GNRC_SIXLOWPAN_GHC_H

#include <stdint.h>
#include <sys/types.h>
#include "net/ipv6/hdr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Decodes a GHC-compressed SRH into the destination buffer.
 *
 * @param[out] dst      Destination buffer for the uncompressed SRH
 * @param[in]  dst_max  Maximum size of the destination buffer
 * @param[in]  src      Source buffer containing the GHC compressed data
 * @param[in]  src_len  Length of the source buffer
 * @param[in]  ipv6     The IPv6 header (used for dictionary initialization)
 *
 * @return  The size of the uncompressed SRH on success
 * @return  -ENOSPC if the destination buffer is too small
 * @return  -1 on malformed GHC data
 */
ssize_t gnrc_sixlowpan_ghc_decode_srh(uint8_t *dst, size_t dst_max,
                                      const uint8_t *src, size_t src_len,
                                      const ipv6_hdr_t *ipv6);

/**
 * @brief   Encodes an SRH using GHC compression.
 *
 * @param[out] out      Destination buffer for the compressed data
 * @param[in]  out_max  Maximum size of the destination buffer
 * @param[in]  in       Source buffer containing the raw SRH
 * @param[in]  in_len   Length of the source buffer
 * @param[in]  ipv6     The IPv6 header (used for dictionary initialization)
 *
 * @return  The size of the compressed data on success
 * @return  -ENOSPC if the destination buffer is too small
 */
ssize_t gnrc_sixlowpan_ghc_encode_srh(uint8_t *out, size_t out_max,
                                      const uint8_t *in, size_t in_len,
                                      const ipv6_hdr_t *ipv6);

#ifdef __cplusplus
}
#endif

#endif /* NET_GNRC_SIXLOWPAN_GHC_H */
/** @} */

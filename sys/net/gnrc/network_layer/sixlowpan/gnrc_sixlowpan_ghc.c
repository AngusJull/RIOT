/*
 * Copyright (C) 2026
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 *
 * @file
 * @brief       6LoWPAN Generic Header Compression (GHC) implementation defined in RFC 7400
 *
 * @author      Brian Tran
 */

#include <string.h>
#include <errno.h>
#include "sys/include/net/gnrc/sixlowan/ghc.h"

#define ENABLE_DEBUG 0
#include "debug.h"

// RFC 7400 Bytecodes

// k literal bytes to copy
// 0kkkkkkk
#define GHC_OP_LITERAL(k)      ((k) & 0x7F)
#define GHC_IS_LITERAL(c)      (((c) & 0x80) == 0x00) 

// nnnn bytes of zero sequence
// 1000 nnnn
#define GHC_OP_ZERO(n)         (0x80 | ((n) & 0x0F))
#define GHC_IS_ZERO(c)         (((c) & 0xF0) == 0x80)

// End of the compressed stream
// 10010000
#define GHC_OP_STOP            (0x90)                
#define GHC_IS_STOP(c)         ((c) == 0x90)

/// ssss and n are multiplied by 8 and added to sa and na respectively
// 101nssss
#define GHC_OP_SETUP(n, s)     (0xA0 | (((n) & 0x01) << 4) | ((s) & 0x0F))
#define GHC_IS_SETUP(c)        (((c) & 0xE0) == 0xA0)

// length to copy, n = na + nnn + 2
// distance to look back = kkk + sa + n
// 11nnnkkk
#define GHC_OP_BACKREF(n, k)   (0xC0 | (((n) & 0x07) << 3) | ((k) & 0x07))
#define GHC_IS_BACKREF(c)      (((c) & 0xC0) == 0xC0)

// The dictionary size is fixed at 48 bytes per the RFC
#define GHC_DICT_SIZE        (48U)
static const uint8_t ghc_static_dict[16] = {
    0x16, 0xfe, 0xfd, 0x17, 0xfe, 0xfd, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00
};

ssize_t gnrc_sixlowpan_ghc_decode_srh(uint8_t *dst, size_t dst_max,
                                      const uint8_t *src, size_t src_len,
                                      const ipv6_hdr_t *ipv6)
{
    // Destination pointer (writing to)
    size_t d_idx = 0;
    // Source pointer (reading from)
    size_t s_idx = 0;

    // Accumulators
    uint8_t sa = 0; // Offset
    uint8_t na = 0; // Length

    // Build the dictionary
    uint8_t dict[GHC_DICT_SIZE];
    memcpy(dict, &ipv6->src, 16); // Source Address 
    memcpy(dict + 16, &ipv6->dst, 16); // Destination Address
    memcpy(dict + 32, ghc_static_dict, sizeof(ghc_static_dict)); // ghc static dictionary

    // Process the compressed stream
    while (s_idx < src_len) {
        // Read the byte code
        uint8_t code = src[s_idx++];

        if (GHC_IS_LITERAL(code)) {
            // Copy k literal bytes from source to destination
            uint8_t k = code & 0x7F;

            // Ensure we have enough space in the destination buffer
            if (d_idx + k > dst_max){
                DEBUG("GHC: Literal overflow\n");
                return -ENOSPC;
            }

            // Copy the literal bytes and advance pointers
            memcpy(dst + d_idx, src + s_idx, k);
            d_idx += k;
            s_idx += k;
        }

        else if (GHC_IS_ZERO(code)){
            // The +2 offset is for optimization. Replacing 0 or 1 zero with a 1-byte opcode provides no benefit, 
            // so the minimum encoded run is 2.
            uint8_t n = (code & 0x0F) + 2;
            
            // Ensure we have enough space in the destination buffer
            if (d_idx + n > dst_max){
                DEBUG("GHC: Zero run overflow\n");
                return -ENOSPC;
            }

            // Insert zeroes into the destination buffer
            memset(dst + d_idx, 0, n);
            d_idx += n;
        }

        // Terminiation, return the total uncompressed size
        else if (GHC_IS_STOP(code)){
            return (int)d_idx;
        }

        else if (GHC_IS_SETUP(code)){
            // Update extended arguments
            na += ((code & 0x10) >> 1);
            sa += ((code & 0x0F) << 3);
        }

        else if (GHC_IS_BACKREF(code)){
            // Calculate the total length to copy, including any accumulated 'na'
            uint8_t n = na + ((code >> 3) & 0x07) + 2;
            uint8_t s = sa + (code & 0x07) + n;

            if (d_idx + n > dst_max){
                DEBUG("GHC: Backref overflow\n");
                return -ENOSPC;
            }

            for (uint8_t i = 0; i < n; i++){
                int loopback_idx = (int)d_idx - s;
                
                if (loopback_idx < 0){
                    // Read from dictionary
                    // Ensure we dont underflow dictionary
                    if (loopback_idx < -((int)GHC_DICT_SIZE)){
                        DEBUG("GHC Dict underflow");
                        return -1;
                    }
                    dst[d_idx++] = dict[GHC_DICT_SIZE + loopback_idx];
                } 
                else {
                    // Read from reconstructed output
                    dst[d_idx++] = dst[loopback_idx];
                }
            }
            // Reset accumulators
            na = 0; sa = 0;
        }
        else{
            DEBUG("GHC: Invalid code 0x%02x\n", code);
            return -1;
        }
    }
    // No stop code found
    DEBUG("GHC: Warning - source ended without stop code\n");
    return -1;
}

ssize_t gnrc_sixlowpan_ghc_encode_srh(uint8_t *out, size_t out_max,
                                      const uint8_t *src, size_t input_len,
                                      const ipv6_hdr_t *ipv6)
{
    // Hold location for input and output
    size_t input_ptr = 0;
    size_t output_ptr = 0;

    // Track our literal groups
    size_t literal_head_ptr = 0; // Remembers where we reserved the instruction byte
    uint8_t literal_count = 0;   // How many raw bytes we have grouped so far

    // Build the dictionary
    uint8_t dict[GHC_DICT_SIZE];
    memcpy(dict, &ipv6->src, 16); // Source Address
    memcpy(dict + 16, &ipv6->dst, 16); // Destination Address
    memcpy(dict + 32, ghc_static_dict, sizeof(ghc_static_dict));

    // Main compression loop
    while (input_ptr < input_len){
        // Make sure output buffer has enough space
        if (output_ptr >= out_max){
            DEBUG("GHC: Output buffer overflow\n");
            return -ENOSPC;
        }

        // Check for runs of zeros
        size_t zero_run_len = 0;
        while ((input_ptr + zero_run_len < input_len) && (src[input_ptr + zero_run_len] == 0)) {
            zero_run_len++;
        }

        int best_len = 0;
        int best_offset = 0;

        // If we found a good zero run, cap it at 17 (15 + 2)
        if (zero_run_len >= 2) {
            if (zero_run_len > 17) {
                zero_run_len = 17;
            }
        }

        else {
            // Look for Backreferences
            for (int candidate = -((int)GHC_DICT_SIZE); candidate < (int)input_ptr; candidate++){
                int len = 0;
                
                // Check how many bytes match from this candidate position
                while(input_ptr + len < input_len){
                    uint8_t byte_from_the_past;
                    int past_index = candidate + len;

                    // Negative index means reading from dictionary
                    if (past_index < 0) {
                        // Add the GHC_DICT_SIZE to convert to positive index
                        byte_from_the_past = dict[48 + past_index];
                    }
                    else{
                        // Look into the history of the source packet
                        byte_from_the_past = src[past_index];
                    }

                    // Match
                    if (byte_from_the_past == src[input_ptr + len]) {
                        len++;
                    } 
                    // No match, stop
                    else {
                        break;
                    }
                }

                // Optimization ensure +2 here because the minimum backreference length
                if (len >= 2) {
                    int dist = (int)input_ptr - candidate;
                    if (dist >= len && len > best_len){
                        best_len = len;
                        best_offset = dist;
                    }
                }
            }
        }

        // Found a good compression opportunity
        if (zero_run_len >= 2 || best_len >= 2) {
            
            // Pending literals, need to close them before next byte code
            if (literal_count > 0) {
                out[literal_head_ptr] = GHC_OP_LITERAL(literal_count);
                literal_count = 0; // Reset for the next time
            }

            // Now emit the actual compression instruction we found
            if (zero_run_len >= 2) {
                if (output_ptr >= out_max) return -ENOSPC;
                out[output_ptr++] = GHC_OP_ZERO(zero_run_len - 2);
                input_ptr += zero_run_len;
                continue; 
            } 
            else {
                // Encode Backreference
                uint8_t n_val = best_len - 2; // -2 here so that we ensure a minimum length of 2 is encoded
                uint8_t k_val = best_offset - best_len;
                uint8_t sa_total = (k_val >> 3);
                uint8_t na_total = (n_val >> 3);

                while (sa_total > 0 || na_total > 0) {
                    if (output_ptr >= out_max){
                        return -ENOSPC;
                    }
                    uint8_t n_chunk = (na_total > 1) ? 1 : na_total;
                    uint8_t s_chunk = (sa_total > 15) ? 15 : sa_total;

                    // Write the Setup opcode and update the remaining totals
                    out[output_ptr++] = GHC_OP_SETUP(n_chunk, s_chunk);

                    // Update the remaining totals based on what we just encoded
                    na_total -= n_chunk;
                    sa_total -= s_chunk;
                }

                if (output_ptr >= out_max){
                    return -ENOSPC;
                }

                // Write the actual BACKREF opcode containing the final 3 bits of the length and offset
                out[output_ptr++] = GHC_OP_BACKREF(n_val, k_val);
                input_ptr += best_len;
            }
        }
        else {
            // Literal Byte
            
            // If this is the very first literal byte in a new group, 
            // reserve a spot for the instruction byte
            if (literal_count == 0) {
                if (output_ptr >= out_max) return -ENOSPC;
                literal_head_ptr = output_ptr++; // Save the index and move forward
            }

            // Safety check
            if (output_ptr >= out_max) {
                return -ENOSPC;
            }
            
            // Write the raw literal data into the output buffer
            out[output_ptr++] = src[input_ptr++];
            literal_count++;

            // RFC 7400 limits literal groups to < 96 bytes.
            // If we hit 95, we finalize this group right now. The next 
            // uncompressible byte will automatically start a brand new group.
            if (literal_count == 95) {
                out[literal_head_ptr] = GHC_OP_LITERAL(literal_count);
                literal_count = 0; 
            }
        }
    }

    // If we have a literal group
    if (literal_count > 0) {
        out[literal_head_ptr] = GHC_OP_LITERAL(literal_count);
    }

    // End with stop op code
    if (output_ptr >= out_max) return -ENOSPC;
    out[output_ptr++] = GHC_OP_STOP;

    return (ssize_t)output_ptr;
}

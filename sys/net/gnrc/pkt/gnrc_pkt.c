/*
 * Copyright (C) 2016 Freie Universität Berlin
 *               2017 HAW Hamburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 *
 * @file
 * @author  Martine Lenders <mlenders@inf.fu-berlin.de>
 * @author  Sebastian Meiling <s@mlng.net>
 */

#include "net/gnrc/pkt.h"
#include <stdio.h>

gnrc_pktsnip_t *gnrc_pktsnip_search_type(gnrc_pktsnip_t *pkt,
                                         gnrc_nettype_t type)
{
    // printf("\nlooking for packet type %d\n", type);
    while ((pkt != NULL) && (pkt->type != type)) {
        // printf("\t%d\n", pkt->type);
        pkt = pkt->next;
    }
    // printf("found packet of type %d\n", pkt->type);
    return pkt;
}

/** @} */

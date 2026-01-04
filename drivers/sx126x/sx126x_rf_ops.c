#include "net/ieee802154/radio.h"

#include "sx126x.h"
#include <sx126x.h>

#define ENABLE_DEBUG 0
#include "debug.h"

// Forward declaration
static const ieee802154_radio_ops_t sx126x_ops;

void sx126x_hal_setup(sx126x_t *dev, ieee802154_dev_t *hal)
{
    hal->driver = &sx126x_ops;
    // Use the sx126x information, but will need different _send, etc. ops
    hal->priv = dev;
}

static int _write(ieee802154_dev_t *dev, const iolist_t *psdu)
{
    sx126x_t *sx_dev = dev->priv;

    // Check we're not already transmitting
    netopt_state_t state;
    sx_dev->netdev->driver->get(netdev, NETOPT_STATE, &state, sizeof(uint8_t));
    if (state == NETOPT_STATE_TX) {
        DEBUG("[sx126x] ieee hal: cannot send packet, radio is already transmitting.\n");
        return -ENOTSUP;
    }

    size_t pos = 0;

    /* Write payload buffer */
    for (const iolist_t *iol = psdu; iol; iol = iol->iol_next) {
        if (iol->iol_len > 0) {
            sx126x_write_buffer(sx_dev, pos, iol->iol_base, iol->iol_len);
            DEBUG("[sx126x] ieee hal: send: wrote data to payload buffer.\n");
            pos += iol->iol_len;
        }
    }

    // Leave actually setting to TX mode for a different operation
    return 0;
}

static int _len(ieee802154_dev_t *dev)
{
    DEBUG("[sx126x] ieee hal: checking length of recieved pkt");
    sx126x_t *sx_dev = dev->priv;

    sx126x_rx_buffer_status_t rx_buffer_status;
    sx126x_get_rx_buffer_status(sx_dev, &rx_buffer_status);
    // TODO - Might want to check status in the future to make sure completed properly
    uint8_t size = rx_buffer_status.pld_len_in_bytes;

    return size;
}

static int _read(ieee802154_dev_t *dev, void *buf, size_t size, ieee802154_rx_info_t *info)
{
    DEBUG("[sx126x ieee hal: reading recieved packet");
    sx126x_t *sx_dev = dev->priv;

    if (buf == NULL) {
        // As required by docs
        return 0;
    }

    if (info) {
        sx126x_pkt_status_lora_t pkt_status;
        sx126x_get_lora_pkt_status(sx_dev, &pkt_status);
        // Since SNR and RSSI are both quality indicators, for now just add to get overall link quality
        info->lqi = pkt_status.snr_pkt_in_db + pkt_status.rssi_pkt_in_dbm;
        info->rssi = pkt_status.rssi_pkt_in_dbm;
    }

    // Duplicate of _len, but need the start pointer for later
    sx126x_rx_buffer_status_t rx_buffer_status;
    sx126x_get_rx_buffer_status(sx_dev, &rx_buffer_status);
    uint8_t read_size = rx_buffer_status.pld_len_in_bytes;

    if (read_size > size) {
        return -ENOBUFS;
    }

    sx126x_read_buffer(sx_dev, rx_buffer_status.buffer_start_pointer, buf, size);

    return read_size;
}

static int _off(ieee802154_dev_t *dev)
{
    // Ignore turning off for now
    DEBUG("[sx126x] ieee hal:  would turn off, but ignoring\n");
    (void)dev;
    return 0;
}

static int _request_on(ieee802154_dev_t *dev)
{
    // Assume that the sx126x driver will turn it on
    DEBUG("[sx126x] ieee hal: would turn on, but ignoring\n");
    (void)dev;
    return 0;
}

static int _confirm_on(ieee802154_dev_t *dev)
{
    // Assume that the device is always on
    DEBUG("[sx126x] ieee hal: would confirm on, but ignoring\n");
    (void)dev;
    return 0;
}

static int _request_op(ieee802154_dev_t *dev, ieee802154_hal_op_t op, void *ctx)
{
    (void)ctx;

    sx126x_t *sx_dev = dev->priv;
    int res = -EBUSY;
    switch (op) {
    case IEEE802154_HAL_OP_TRANSMIT:
        DEBUG("[sx126x] ieee hal: starting transmit\n");
        // May need to consider retransmission here
        // No timeout
        sx126x_set_tx(sx_dev, 0);
        break;
    case IEEE802154_HAL_OP_SET_IDLE:
        DEBUG("[sx126x] ieee hal: going to idle\n");
        // Might just want to do nothing here. Check if we need to stop RX or TX?
        // Assuming this mode just means to cancel tx and rx operations
        sx126x_set_standby(sx_dev, SX126X_CHIP_MODE_STBY_XOSC);
        // TODO - check ctx as boolean for "forced" where we only go to standby if not doing nothing
        break;
    case IEEE802154_HAL_OP_SET_RX:
        DEBUG("[sx126x] ieee hal: starting reception\n");
        // Go to idle when we're done, maybe should just leave this to setup
        sx126x_set_rx_tx_fallback_mode(sx_dev, SX126X_FALLBACK_STDBY_XOSC);
        // Might want to check if we want SINGLE or COTINUOUS mode
        sx126x_set_rx(sx_dev, SX126X_RX_SINGLE_MODE);
        break;
    case IEEE802154_HAL_OP_CCA:
        DEBUG("[sx126x] ieee hal: would start CCA, doing nothing\n");
        /* Do channel detection operation. Likely, just need to do set_cad_params,
         * then do set_cad, and configure an IRQ handler for when its done.
         *
         * For now, doing nothing should work, as long as we pretend there's a clear channel
         * Might need to disable RX here if we're in it (though CAD has fallback mode that can be configured)
         */
        break;
    }

    // If there's an error, maybe jump past this?
    res = 0;
    return res;
}

static int _confirm_op(ieee802154_dev_t *dev, ieee802154_hal_op_t op, void *ctx)
{
    sx126x_t *sx_dev = dev->priv;

    int res = -EAGAIN;
    switch (op) {
    case IEEE802154_HAL_OP_TRANSMIT: {
        // Provie an error if not completed
        sx126x_chip_status_t status;
        sx126x_get_status(sx_dev, &status);

        if (status.chip_mode != SX126X_CHIP_MODE_TX) {
            goto error;
        }

        if (ctx) {
            ieee802154_tx_info_t *info = ctx;

            // If retrying, return status TX_STATUS_MEDIUM_BUSY
            // TODO - need to handle other statuses, like if we fail to retransmit or wait for ACK
            info->status = TX_STATUS_SUCCESS;
        }
    } break;
    case IEEE802154_HAL_OP_SET_IDLE: {
        sx126x_chip_status_t status;
        sx126x_get_status(sx_dev, &status);

        // Could also just do nothing?
        if (status.chip_mode != SX126X_CHIP_MODE_STBY_XOSC) {
            goto error;
        }
    } break;
    case IEEE802154_HAL_OP_SET_RX:
        // Might have received since turning on RX, which would put us back in idle. Just do nothing
    case IEEE802154_HAL_OP_CCA:
        // Provide boolean status for cca completion
        break;
    }

    res = 0;

error:
    return res;
}

static int _set_cca_threshold(ieee802154_dev_t *dev, int8_t threshold)
{
    // TODO
    (void)dev;
    (void)threshold;
    return 0;
}

static int _set_cca_mode(ieee802154_dev_t *dev, ieee802154_cca_mode_t mode)
{
    // TODO
    (void)dev;
    (void)mode;
    return 0;
}

static int _config_phy(ieee802154_dev_t *dev, const ieee802154_phy_conf_t *conf)
{
    // For now, only use default frequency bands and powers
    (void)dev;
    (void)conf;
    return 0;
}

static int _set_csma_params(ieee802154_dev_t *dev, const ieee802154_csma_be_t *bd, int8_t retries)
{
    // TODO
    (void)dev;
    (void)bd;
    (void)retries;
    return 0;
}

static int _set_frame_filter_mode(ieee802154_dev_t *dev, ieee802154_filter_mode_t mode)
{
    // Only need to implement one of these cases
    switch (mode) {
    case IEEE802154_FILTER_ACCEPT:
        // TODO
        // Likely need to build in frame filter ourselves
    case IEEE802154_FILTER_ACK_ONLY:
        // Probably need to do some check when recieving a frame to discard if not an ACK
        // Might be able to get away with not implementing for now
        // TODO
        break;
    default:
        return -ENOTSUP;
    }

    return 0;
}

static int _get_frame_filter_mode(ieee802154_dev_t *dev, ieee802154_filter_mode_t *mode)
{
    *mode = IEEE802154_FILTER_ACCEPT;
    return -ENOTSUP;
}

static int _config_addr_filter(ieee802154_dev_t *dev, ieee802154_af_cmd_t cmd, const void *value)
{
    DEBUG("[sx126x] ieee hal: Configuring address filter.\n");
    switch (cmd) {
    case IEEE802154_AF_SHORT_ADDR:
        // Will be 2 bytes with the short address
    case IEEE802154_AF_EXT_ADDR:
        // Will be 8 bytes with the full address
    case IEEE802154_AF_PANID:
        // Will be 2 bytes with the pan id
    case IEEE802154_AF_PAN_COORD:
        // Won't implement the pan coordinator role
        return -ENOTSUP;
    }
    return 0;
}

static int _config_src_addr_match(ieee802154_dev_t *dev, ieee802154_src_match_t cmd, const void *value)
{
    // This is only needed for turning on the frame pending bit for all recieved packets, needed when
    // a data request MAC command is made. Since this isn't needed right away, we can just leave this out
    (void)dev;
    (void)cmd;
    (void)value;
    return -ENOTSUP;
}

static const ieee802154_radio_ops_t sx126x_ops = {
    // Some of these capabilities might not be entirely accurate, but should provide needed functionality
    .caps = IEEE802154_CAP_SUB_GHZ | IEEE802154_CAP_PHY_MR_FSK | IEEE802154_CAP_IRQ_TX_DONE | IEEE802154_CAP_IRQ_CCA_DONE,
    .write = _write,
    .len = _len,
    .read = _read,
    .off = _off,
    .request_on = _request_on,
    .confirm_on = _confirm_on,
    .request_op = _request_op,
    .confirm_op = _confirm_op,
    .set_cca_threshold = _set_cca_threshold,
    .set_cca_mode = _set_cca_mode,
    .config_phy = _config_phy,
    .set_frame_retrans = NULL, // Not supported, so left as NULL
    .set_csma_params = _set_csma_params,
    .set_frame_filter_mode = _set_frame_filter_mode,
    .get_frame_filter_mode = _get_frame_filter_mode,
    .config_addr_filter = _config_addr_filter,
    .config_src_addr_match = _config_src_addr_match,
};

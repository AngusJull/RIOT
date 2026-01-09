#include "sx126x.h"
#include <sx126x.h>
#include "sx126x_rf_ops.h"

#define ENABLE_DEBUG 1
#include "debug.h"

#if IS_USED(MODULE_SX126X_IEEE802154)

// Helper for getting device driver from HAL structure
#  define SX_DEV(hal_dev)   (((sx126x_hal_priv_t *)(hal_dev)->priv)->sx_dev)

// Helper for getting the private data pointer from HAL structure
#  define HAL_PRIV(hal_dev) ((sx126x_hal_priv_t *)(hal_dev)->priv)

// Forward declaration of HAL operations
static const ieee802154_radio_ops_t sx126x_ops;

// From PR for sx126x
static bool _l2filter(ieee802154_dev_t *hal, uint8_t *mhr)
{
    sx126x_hal_priv_t *priv = hal->priv;
    uint8_t dst_addr[IEEE802154_LONG_ADDRESS_LEN];
    le_uint16_t dst_pan;
    le_uint16_t pan_bcast = { .u8 = IEEE802154_PANID_BCAST };

    int addr_len = ieee802154_get_dst(mhr, dst_addr, &dst_pan);

    if ((mhr[0] & IEEE802154_FCF_TYPE_MASK) == IEEE802154_FCF_TYPE_BEACON) {
        if (priv->pan_id == pan_bcast.u16) {
            DEBUG("[sx126x hal] beacon address checked\n");
            return true;
        }
    }

    /* filter PAN ID */
    /* Will only work on little endian platform (all?) */
    if (pan_bcast.u16 != byteorder_ltohs(dst_pan) &&
        priv->pan_id != byteorder_ltohs(dst_pan)) {
        DEBUG("[sx126x hal] PAN ID mismatch\n");
        return false;
    }

    /* check destination address */
    if (addr_len == IEEE802154_SHORT_ADDRESS_LEN) {
        if (memcmp(priv->short_addr, dst_addr, addr_len) == 0 ||
            memcmp(ieee802154_addr_bcast, dst_addr, addr_len) == 0) {
            return true;
        }
        else {
            DEBUG("[sx126x hal] short address mismatch\n");
            return false;
        }
    }
    else if (addr_len == IEEE802154_LONG_ADDRESS_LEN) {
        if (memcmp(priv->long_addr, dst_addr, addr_len) == 0) {
            return true;
        }
        else {
            return false;
        }
    }

    return false;
}

static void _dio1_isr(void *arg)
{
    // Pass along arg, which should be a pointer to the bhp
    DEBUG("[sx126x hal] isr triggered, adding event\n");
    bhp_event_isr_cb(arg);
}

static void _event_isr_cb(void *arg)
{
    ieee802154_dev_t *hal = arg;
    sx126x_hal_priv_t *priv = hal->priv;
    sx126x_t *sx_dev = SX_DEV(hal);

    // Handling of ISR adapted from existing PR
    sx126x_irq_mask_t mask;
    sx126x_get_and_clear_irq_status(sx_dev, &mask);

    if (mask & SX126X_IRQ_TX_DONE) {
        DEBUG("[sx126x hal] SX126X_IRQ_TX_DONE\n");
        hal->cb(hal, IEEE802154_RADIO_CONFIRM_TX_DONE);
    }
    else if (mask & SX126X_IRQ_RX_DONE) {
        DEBUG("[sx126x hal] SX126X_IRQ_RX_DONE\n");
        uint8_t mhdr[IEEE802154_MAX_HDR_LEN];
        sx126x_rx_buffer_status_t rx_buffer_status;
        sx126x_get_rx_buffer_status(sx_dev, &rx_buffer_status);
        sx126x_read_buffer(sx_dev, rx_buffer_status.buffer_start_pointer, mhdr, sizeof(mhdr));
        bool is_ack = (mhdr[0] & IEEE802154_FCF_TYPE_MASK) == IEEE802154_FCF_TYPE_ACK;
        bool is_data = (mhdr[0] & IEEE802154_FCF_TYPE_MASK) == IEEE802154_FCF_TYPE_DATA;
        bool ackf = priv->ack_filter;
        bool match = _l2filter(hal, mhdr);

        /* If the L2 filter passes, device if the frame is indicated
         * directly or if the driver should send an ACK frame before the indication */
        if (is_ack || (!ackf && match)) {
            if (is_data && (mhdr[0] & IEEE802154_FCF_ACK_REQ)) {
                DEBUG("[sx126x hal] Received valid frame, need to send ack\n");
            }
            DEBUG("[sx126x hal] RX done\n");
            hal->cb(hal, IEEE802154_RADIO_INDICATION_RX_DONE);
        }
        /* If radio is in promiscuos mode, indicate packet right away. Need to selectively not ACK here */
        else if (priv->promisc) {
            DEBUG("[sx126x hal] Promiscuous mode is enabled.\n");
            hal->cb(hal, IEEE802154_RADIO_INDICATION_RX_DONE);
        }
        /* If all failed, simply drop the frame and continue listening to incoming frames */
        else {
            // Maybe should add back in set state function in case we change rx modes eventually
            sx126x_set_rx(sx_dev, SX126X_RX_SINGLE_MODE);
        }
    }
    else if (mask & SX126X_IRQ_PREAMBLE_DETECTED) {
        DEBUG("[sx126x hal] SX126X_IRQ_PREAMBLE_DETECTED\n");
    }
    else if (mask & SX126X_IRQ_SYNC_WORD_VALID) {
        DEBUG("[sx126x hal] SX126X_IRQ_SYNC_WORD_VALID\n");
    }
    else if (mask & SX126X_IRQ_HEADER_VALID) {
        DEBUG("[sx126x hal] SX126X_IRQ_HEADER_VALID\n");
        hal->cb(hal, IEEE802154_RADIO_INDICATION_RX_START);
    }
    else if (mask & SX126X_IRQ_HEADER_ERROR) {
        DEBUG("[sx126x hal] SX126X_IRQ_HEADER_ERROR\n");
    }
    else if (mask & SX126X_IRQ_CRC_ERROR) {
        DEBUG("[sx126x hal] SX126X_IRQ_CRC_ERROR\n");
        hal->cb(hal, IEEE802154_RADIO_INDICATION_CRC_ERROR);
    }
    else if (mask & SX126X_IRQ_CAD_DONE) {
        if (mask & SX126X_IRQ_CAD_DETECTED) {
            DEBUG("[sx126x hal] SX126X_IRQ_CAD_DETECTED \n");
            priv->cad_detected = true;
        }
        DEBUG("[sx126x hal] SX126X_IRQ_CAD_DONE\n");
        priv->cad_done = true;
        hal->cb(hal, IEEE802154_RADIO_CONFIRM_CCA);
    }
    else if (mask & SX126X_IRQ_TIMEOUT) {
        DEBUG("[sx126x hal] SX126X_IRQ_TIMEOUT\n");
    }
    else {
        DEBUG("[sx126x hal] SX126X_IRQ_NONE\n");
    }
}

void sx126x_print_status(sx126x_t *dev)
{
    sx126x_errors_mask_t errors;
    if (sx126x_get_device_errors(dev, &errors) != SX126X_STATUS_OK) {
        DEBUG("[sx126x hal] get device errors failed\n");
    }
    else {
        DEBUG("[sx126x hal] get errors: %x\n", errors);
    }

    sx126x_chip_status_t status;
    if (sx126x_get_status(dev, &status) != SX126X_STATUS_OK) {
        DEBUG("[sx126x hal] get status failed\n");
    }
    else {
        DEBUG("[sx126x hal] chip mode: %d\n", status.chip_mode);
        DEBUG("[sx126x hal] cmd status: %d\n", status.cmd_status);
    }
    DEBUG("[sx126x hal] get channel: %" PRIu32 "\n", sx126x_get_channel(dev));
    sx126x_pkt_type_t pkt_type = 0;
    if (sx126x_get_pkt_type(dev, &pkt_type) != SX126X_STATUS_OK) {
        DEBUG("[sx126x hal] get pkt type failed\n");
    }
    else {
        DEBUG("[sx126x hal] get pkt type: %u\n", pkt_type);
    }
}

void sx126x_hal_setup(sx126x_hal_priv_t *dev, sx126x_t *sx_dev, const sx126x_params_t *params, event_queue_t *evq, ieee802154_dev_t *hal)
{
    hal->driver = &sx126x_ops;
    hal->priv = dev;

    // Don't use sx126x_setup to set params and such because it registers with netdev
    sx_dev->params = (sx126x_params_t *)params;
    dev->sx_dev = sx_dev;

    // Use the sx126x setup, but don't set up the netdev by avoiding the sx126x_setup function. This configures the
    // interrupt handler which we should also change
    if (sx126x_init(sx_dev) < 0) {
        DEBUG("[sx126x hal] init failed\n");
        return;
    }

    bhp_event_init(&dev->bhp, evq, _event_isr_cb, hal);

    // Override the pin isr set by sx126x_init
    int res = gpio_init_int(sx_dev->params->dio1_pin, GPIO_IN, GPIO_RISING, _dio1_isr, &dev->bhp);
    if (res < 0) {
        DEBUG("[sx126x hal] interrupt setup failed\n");
        return;
    }

    // Debug info in case there's errors during setup
    sx126x_clear_device_errors(sx_dev);
    sx126x_print_status(sx_dev);
}

static int _write(ieee802154_dev_t *dev, const iolist_t *psdu)
{
    sx126x_t *sx_dev = SX_DEV(dev);

    sx126x_chip_status_t status;
    sx126x_get_status(sx_dev, &status);
    if (status.chip_mode == SX126X_CHIP_MODE_TX) {
        DEBUG("[sx126x hal] cannot send packet, radio is already transmitting.\n");
        return -EBUSY;
    }

    // Should not need to change the buffer base addresses, since the defaults should be fine
    size_t pos = 0;

    /* Write payload buffer */
    for (const iolist_t *iol = psdu; iol; iol = iol->iol_next) {
        if (iol->iol_len > 0) {
            sx126x_write_buffer(sx_dev, pos, iol->iol_base, iol->iol_len);
            DEBUG("[sx126x hal]  wrote data to payload buffer.\n");
            pos += iol->iol_len;
        }
    }

    if (!pos) {
        return 0;
    }

    sx126x_set_lora_payload_length(sx_dev, pos);
    // Leave actually setting to TX mode for a different operation
    return 0;
}

static int _len(ieee802154_dev_t *dev)
{
    DEBUG("[sx126x hal]  checking length of recieved pkt");
    sx126x_t *sx_dev = SX_DEV(dev);

    sx126x_rx_buffer_status_t rx_buffer_status;
    sx126x_get_rx_buffer_status(sx_dev, &rx_buffer_status);

    // Might want to check status in the future to make sure completed properly
    return rx_buffer_status.pld_len_in_bytes - (uint8_t)IEEE802154_FCS_LEN;
}

static int _read(ieee802154_dev_t *dev, void *buf, size_t size, ieee802154_rx_info_t *info)
{
    DEBUG("[sx126x hal] reading recieved packet");
    sx126x_t *sx_dev = SX_DEV(dev);

    if (buf == NULL) {
        // As required by docs
        return 0;
    }

    if (info) {
        sx126x_pkt_status_lora_t pkt_status;
        sx126x_get_lora_pkt_status(sx_dev, &pkt_status);
        // Just use SNR as LQI, but map from signed to unsigned range
        info->lqi = (-INT8_MIN) + pkt_status.snr_pkt_in_db;
        info->rssi = ieee802154_dbm_to_rssi(pkt_status.rssi_pkt_in_dbm);
    }

    // Duplicate of _len, but need the start pointer for later
    sx126x_rx_buffer_status_t rx_buffer_status;
    sx126x_get_rx_buffer_status(sx_dev, &rx_buffer_status);
    // Don't want to copy out FCS, or include in size calculation
    uint8_t read_size = rx_buffer_status.pld_len_in_bytes - IEEE802154_FCS_LEN;

    if (read_size > size) {
        return -ENOBUFS;
    }
    sx126x_read_buffer(sx_dev, rx_buffer_status.buffer_start_pointer, buf, read_size);

    return read_size;
}

static int _off(ieee802154_dev_t *dev)
{
    // Ignore turning off for now
    DEBUG("[sx126x hal] would turn off, but ignoring\n");
    (void)dev;
    return 0;
}

static int _request_on(ieee802154_dev_t *dev)
{
    // Assume that the sx126x driver will turn it on
    DEBUG("[sx126x hal]  would turn on, but ignoring\n");
    (void)dev;
    return 0;
}

static int _confirm_on(ieee802154_dev_t *dev)
{
    // Assume that the device is always on
    DEBUG("[sx126x hal] would confirm on, but ignoring\n");
    (void)dev;
    return 0;
}

static int _request_op(ieee802154_dev_t *dev, ieee802154_hal_op_t op, void *ctx)
{
    (void)ctx;

    sx126x_hal_priv_t *priv = HAL_PRIV(dev);
    sx126x_t *sx_dev = SX_DEV(dev);

    int res = -EBUSY;
    switch (op) {
    case IEEE802154_HAL_OP_TRANSMIT:
        DEBUG("[sx126x hal] starting transmit\n");
        sx126x_set_tx(sx_dev, 0);
        break;
    case IEEE802154_HAL_OP_SET_IDLE:
        DEBUG("[sx126x hal] going to idle\n");
        sx126x_set_standby(sx_dev, SX126X_CHIP_MODE_STBY_XOSC);
        break;
    case IEEE802154_HAL_OP_SET_RX:
        DEBUG("[sx126x hal] starting reception\n");
        // Go to idle when we're done, maybe should just leave this to setup
        // sx126x_set_rx_tx_fallback_mode(sx_dev, SX126X_FALLBACK_STDBY_XOSC);
        sx126x_set_rx(sx_dev, SX126X_RX_SINGLE_MODE);
        break;
    case IEEE802154_HAL_OP_CCA:
        DEBUG("[sx126x hal] starting CCA\n");
        priv->cad_detected = false;
        priv->cad_done = false;
        sx126x_set_cad(sx_dev);
        break;
    }

    // If there's an error, maybe jump past this?
    res = 0;
    return res;
}

static int _confirm_op(ieee802154_dev_t *dev, ieee802154_hal_op_t op, void *ctx)
{
    sx126x_hal_priv_t *priv = HAL_PRIV(dev);
    sx126x_t *sx_dev = SX_DEV(dev);

    int res = -EAGAIN;
    switch (op) {
    case IEEE802154_HAL_OP_TRANSMIT: {
        DEBUG("[sx126x hal] confirming transmit\n");
        // Provie an error if not completed
        sx126x_chip_status_t status;
        sx126x_get_status(sx_dev, &status);
        if (status.chip_mode == SX126X_CHIP_MODE_TX) {
            goto error;
        }

        if (ctx) {
            ieee802154_tx_info_t *info = ctx;
            // Note - without cap for AUTO_CSMA, no guarantee is made that the chip does any CSMA before transmitting
            // Just assume that the upper layer will take care of retransmit and other errors
            info->status = TX_STATUS_SUCCESS;
        }
    } break;
    case IEEE802154_HAL_OP_SET_IDLE: {
        DEBUG("[sx126x hal] confirming set idle\n");
        sx126x_chip_status_t status;
        sx126x_get_status(sx_dev, &status);
        if (status.chip_mode != SX126X_CHIP_MODE_STBY_XOSC && status.chip_mode != SX126X_CHIP_MODE_STBY_RC) {
            goto error;
        }
    } break;
    case IEEE802154_HAL_OP_SET_RX:
        DEBUG("[sx126x hal] confirming recv\n");
        // Might have received since turning on RX, which would put us back in idle. Just do nothing
        break;
    case IEEE802154_HAL_OP_CCA:
        DEBUG("[sx126x hal] checking cca done\n");
        if (!priv->cad_done) {
            goto error;
        }
        else {
            *((bool *)ctx) = priv->cad_detected;
            // Leave cad_done as true in this case so that we need to start the operation again to get stuck polling
        }
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
    (void)mode;
    sx126x_t *sx_dev = SX_DEV(dev);
    DEBUG("[sx126x hal] set_cca_mode \n");
    sx126x_cad_params_t cad_params = {
        .cad_exit_mode = SX126X_CAD_ONLY,
        .cad_detect_min = 10,
        .cad_detect_peak = 22,
        .cad_symb_nb = SX126X_CAD_02_SYMB,
        /* Rx timeout = cad_timeout * 15.625us */
        /* Rx timeout = 60ms */
        .cad_timeout = 0x000F00,
    };
    sx126x_set_cad_params(sx_dev, &cad_params);
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
    sx126x_hal_priv_t *priv = HAL_PRIV(dev);

    bool ackf = false;
    bool promisc = false;

    switch (mode) {
    case IEEE802154_FILTER_ACCEPT:
        DEBUG("[sx126x hal] Filter accept all\n");
        break;
    case IEEE802154_FILTER_PROMISC:
        promisc = true;
        break;
    case IEEE802154_FILTER_ACK_ONLY:
        ackf = true;
        DEBUG("[sx126x hal] Filter ACK only\n");
        break;
    default:
        return -ENOTSUP;
    }

    priv->ack_filter = ackf;
    priv->promisc = promisc;

    return 0;
}

static int _get_frame_filter_mode(ieee802154_dev_t *dev, ieee802154_filter_mode_t *mode)
{
    sx126x_hal_priv_t *priv = HAL_PRIV(dev);

    if (priv->ack_filter) {
        *mode = IEEE802154_FILTER_ACK_ONLY;
    }
    else if (priv->promisc) {
        *mode = IEEE802154_FILTER_PROMISC;
    }
    else {
        *mode = IEEE802154_FILTER_ACCEPT;
    }

    return 0;
}

static int _config_addr_filter(ieee802154_dev_t *dev, ieee802154_af_cmd_t cmd, const void *value)
{
    DEBUG("[sx126x hal] Configuring address filter.\n");
    sx126x_hal_priv_t *priv = HAL_PRIV(dev);
    switch (cmd) {
    case IEEE802154_AF_SHORT_ADDR:
        memcpy(priv->short_addr, value, IEEE802154_SHORT_ADDRESS_LEN);
        break;
    case IEEE802154_AF_EXT_ADDR:
        memcpy(priv->long_addr, value, IEEE802154_LONG_ADDRESS_LEN);
        break;
    case IEEE802154_AF_PANID:
        priv->pan_id = *(uint16_t *)value;
        break;
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
    // Add the FSK capability, which is only half wrong because although the radio has FSK, we won't use it
    .caps = IEEE802154_CAP_SUB_GHZ |
            IEEE802154_CAP_IRQ_TX_DONE |
            IEEE802154_CAP_IRQ_CCA_DONE |
            IEEE802154_CAP_IRQ_RX_START |
            IEEE802154_CAP_IRQ_CRC_ERROR |
            IEEE802154_CAP_PHY_MR_FSK,
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

#endif // IS_USED(MODULE_SX126X_IEEE802154)

#include "net/ieee802154/radio.h"

#include "sx126x.h"

#define ENABLE_DEBUG 0
#include "debug.h"

// Forward declaration
static const ieee802154_radio_ops_t sx126x_ops;

void sx126x_hal_setup(sx126x_t *dev, ieee802154_dev_t *hal)
{
    hal->driver = &sx126x_ops;
    hal->priv = dev;
}

static int _write(ieee802154_dev_t *dev, const iolist_t *psdu)
{
    sx126x_t *sx_dev = dev->priv;

    return 0;
}

static int _len(ieee802154_dev_t *dev)
{
    return 0;
}

static int _read(ieee802154_dev_t *dev, void *buf, size_t size, ieee802154_rx_info_t *info)
{
    return 0;
}

static int _off(ieee802154_dev_t *dev)
{
    return 0;
}

static int _request_on(ieee802154_dev_t *dev)
{
    return 0;
}

static int _confirm_on(ieee802154_dev_t *dev)
{
    return 0;
}

static int _request_op(ieee802154_dev_t *dev, ieee802154_hal_op_t op, void *ctx)
{
    return 0;
}

static int _confirm_op(ieee802154_dev_t *dev, ieee802154_hal_op_t op, void *ctx)
{
    return 0;
}

static int _set_cca_threshold(ieee802154_dev_t *dev, int8_t threshold)
{
    return 0;
}

static int _set_cca_mode(ieee802154_dev_t *dev, ieee802154_cca_mode_t mode)
{
    return 0;
}

static int _config_phy(ieee802154_dev_t *dev, const ieee802154_phy_conf_t *conf)
{
    return 0;
}

static int _set_frame_retrans(ieee802154_dev_t *dev, const uint8_t retrans)
{
    return 0;
}

static int _set_csma_params(ieee802154_dev_t *dev, const ieee802154_csma_be_t *bd, int8_t retries)
{
    return 0;
}

static int _set_frame_filter_mode(ieee802154_dev_t *dev, ieee802154_filter_mode_t mode)
{
    return 0;
}

static int _get_frame_filter_mode(ieee802154_dev_t *dev, ieee802154_af_cmd_t cmd, const void *value)
{
    return 0;
}

static int _config_addr_filter(ieee802154_dev_t *dev, ieee802154_af_cmd_t cmd, const void *value)
{
    return 0;
}

static int _config_src_addr_match(ieee802154_dev_t *dev, ieee802154_src_match_t cmd, const void *value)
{
    return 0;
}

static const ieee802154_radio_ops_t sx126x_ops = {
    .caps = IEEE802154_CAP_SUB_GHZ | IEEE802154_CAP_PHY_MR_FSK | IEEE802154_CAP_IRQ_TX_DONE
    /*
           * No hardware acceleration. Check what caps mean
           * - IEEE802154_CAP_FRAME_RETRANS
           * - IEEE802154_CAP_AUTO_CSMA
           * - IEEE802154_CAP_AUTO_ACK
           * - IEEE802154_CAP_IRQ_ACK_TIMEOUT
           * - IEEE802154_CAP_SRC_ADDR_MATCH
           */
    ,
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
    .set_frame_retrans = _set_frame_retrans,
    .set_csma_params = _set_csma_params,
    .set_frame_filter_mode = _set_frame_filter_mode,
    .get_frame_filter_mode = _get_frame_filter_mode,
    .config_addr_filter = _config_addr_filter,
    .config_src_addr_match = _config_src_addr_match,
};

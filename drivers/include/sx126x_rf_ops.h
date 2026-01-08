#include "sx126x.h"
#include "net/ieee802154/radio.h"
#include "bhp/event.h"

typedef struct {
    sx126x_t *sx_dev; /** Device driver, used without netdev */
    bhp_event_t bhp;  /** Bottom half processor for IRQ events, since sx126x works over SPI */

    bool cad_detected;                                /** Channel Activity Detected Flag */
    bool cad_done;                                    /** Channel Activity Detection Done Flag */
    bool ack_filter;                                  /** Whether the ACK filter is activated or not */
    bool promisc;                                     /** Whether the device is in promiscuous mode or not */
    uint8_t short_addr[IEEE802154_SHORT_ADDRESS_LEN]; /** Short (2 bytes) device address */
    uint8_t long_addr[IEEE802154_LONG_ADDRESS_LEN];   /** Long (8 bytes) device address */
    uint16_t pan_id;                                  /** PAN ID */
} sx126x_hal_priv_t;

// Hal definition
void sx126x_hal_setup(sx126x_hal_priv_t *dev, sx126x_t *sx_dev, const sx126x_params_t *params, event_queue_t *evq, ieee802154_dev_t *hal);

// For radio device
#include "sx126x.h"
#include "sx126x_params.h"
// Need for setting up submac, and for setting up lwip (look into what that is)
#include "sx126x_rf_ops.h" // TODO - check if real

// For debug
#include "net/gnrc/pktdump.h"

// For networking stack
#include "log.h"
#include "net/gnrc.h"
#include "net/gnrc/netif/ieee802154.h"
#include "net/netdev/ieee802154_submac.h"
#include <stdio.h>
#include <string.h>

#include "shell.h"

/**
 * @brief   Calculate the number of configured SX126X devices
 */
#define SX126X_NUMOF     ARRAY_SIZE(sx126x_params)

/**
 * @brief   Define stack parameters for the MAC layer thread
 */
// Define the stacksize manually because include is not available outside of auto-init section?
#define SX126X_STACKSIZE (2048 - 128)
#ifndef SX126X_PRIO
#  define SX126X_PRIO (GNRC_NETIF_PRIO)
#endif

/**
 * @brief   Allocate memory for device descriptors, stacks, and GNRC adaption
 */
static sx126x_t sx126x_devs[SX126X_NUMOF];
static char sx126x_stacks[SX126X_NUMOF][SX126X_STACKSIZE];
static gnrc_netif_t netifs[SX126X_NUMOF];
static netdev_ieee802154_submac_t submac_netdevs[SX126X_NUMOF];

static void register_sx126x(void)
{
    for (unsigned i = 0; i < SX126X_NUMOF; ++i) {
        LOG_DEBUG("[auto_init_netif] initializing sx126x #%u\n", i);

        // Register the submac driver
        netdev_register(&submac_netdevs[i].dev.netdev, NETDEV_SX126X, i);
        // Init the submac driver
        netdev_ieee802154_submac_init(&submac_netdevs[i]);

        // Provide the submac with the radio operations it can perform and the device instance
        sx126x_hal_setup(&sx126x_devs[i], &submac_netdevs[i].submac.dev);

        // Set up the radio device
        sx126x_setup(&sx126x_devs[i], &sx126x_params[i], i);

        // Create the ieee802154 driver using the submac that was set up
        gnrc_netif_ieee802154_create(&netifs[i], sx126x_stacks[i], SX126X_STACKSIZE, SX126X_PRIO, "sx126x_dev", &submac_netdevs[i].dev.netdev);
    }
}

// END OF AUTO-INIT

int main(void)
{
    register_sx126x();

    gnrc_netreg_entry_t dump = GNRC_NETREG_ENTRY_INIT_PID(GNRC_NETREG_DEMUX_CTX_ALL,
                                                          gnrc_pktdump_pid);
    gnrc_netreg_register(GNRC_NETTYPE_UNDEF, &dump);

    (void)puts("Welcome to RIOT!");

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}

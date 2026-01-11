# name of your application
APPLICATION = sx126x_example

# If no BOARD is found in the environment, use this default:
BOARD ?= esp32-heltec-lora32-v3

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../..

# Uncomment these lines if you want to use platform support from external
# repositories:
#RIOTCPU ?= $(CURDIR)/../../RIOT/thirdparty_cpu
#EXTERNAL_BOARD_DIRS ?= $(CURDIR)/../../RIOT/thirdparty_boards

# Uncomment this to enable scheduler statistics for ps:
#USEMODULE += schedstatistics

# If you want to use native with valgrind, you should recompile native
# with the target all-valgrind instead of all:
# make -B clean all-valgrind

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# Modules to include:
USEMODULE += shell
USEMODULE += shell_cmds_default
USEMODULE += ps
# include and auto-initialize all available sensors
USEMODULE += saul_default

# netif is required for this application (though should still work if optional)
FEATURES_REQUIRED += netif
FEATURES_OPTIONAL += periph_rtc

# use modules for networking

# gnrc is a meta module including all required, basic gnrc networking modules
USEMODULE += gnrc
# use the default network interface for the board
USEMODULE += netdev_default
# auto init networking modules
USEMODULE += auto_init_gnrc_netif
# enable ipv6 networking stack, which automatically includes sixlowpan if a IEEE 802.15.4 device present
USEMODULE += gnrc_ipv6_router_default
# enable RPL routing protocol
USEMODULE += gnrc_rpl
USEMODULE += auto_init_gnrc_rpl
# enable RPL source routing header support
USEMODULE += gnrc_rpl_srh
# shell command to send L2 packets with a simple string
USEMODULE += gnrc_txtsnd
# the application dumps received packets to stdout
USEMODULE += gnrc_pktdump
# include udp
USEMODULE += gnrc_udp
#include test program for udp send/recv
USEMODULE += shell_cmd_gnrc_udp
# include shell program for inspecting and modifying forwarding information base
USEMODULE += fib
# include RPL shell commands
USEMODULE += shell_cmd_gnrc_rpl
# include ICMPv6 echo functionality and command
USEMODULE += gnrc_icmpv6_echo
USEMODULE += shell_cmd_gnrc_icmpv6_echo
# icmpv6 error message support
USEMODULE += gnrc_icmpv6_error

# enable stats
USEMODULE += netstats_l2
USEMODULE += netstats_ipv6

# use IEEE802154 for sx126x instead of LoRa or RAW modes
USEMODULE += sx126x_ieee802154

# We use only the lower layers of the GNRC network stack, hence, we can
# reduce the size of the packet buffer a bit
# Set GNRC_PKTBUF_SIZE via CFLAGS if not being set via Kconfig.
ifndef CONFIG_GNRC_PKTBUF_SIZE
	CFLAGS += -DCONFIG_GNRC_PKTBUF_SIZE=1024
endif

include $(RIOTBASE)/Makefile.include

# Set a custom channel if needed
include $(RIOTMAKE)/default-radio-settings.inc.mk

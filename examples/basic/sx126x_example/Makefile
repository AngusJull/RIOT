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

include $(RIOTBASE)/Makefile.include

# Set a custom channel if needed
include $(RIOTMAKE)/default-radio-settings.inc.mk

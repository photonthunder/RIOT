APPLICATION = adc_poller
RIOTBASE ?= $(CURDIR)/../..
BOARD ?= samd21-xpro
USEMODULE += xtimer
USEMODULE += ps
QUIET ?= 1

FEATURES_REQUIRED += periph_timer

CFLAGS += -DDEVELHELP
#CFLAGS += -DNDEBUG
CFLAGS += -DDEBUG_ASSERT_VERBOSE

include $(RIOTBASE)/Makefile.include


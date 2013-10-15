#
#
# --------------------------------------------------------
#         ClockTamer Board Replicator Makefile.
# --------------------------------------------------------

GRAPH_LCD    = 1
OLD_LED      = 1
MCU          = at90usb1287
ARCH         = AVR8
BOARD        = USER
F_CPU        = 8000000
F_USB        = $(F_CPU)
OPTIMIZATION = s
TARGET       = BoardReplicator
SRC          = $(TARGET).c freqmes.c lmk0x0xx.c ctcontrol.c ui_text.c datast_usb.c VirtualSerialHost.c flashspm.c Descriptors.c control.c spiio.c datast.c $(LUFA_SRC_USB) $(LUFA_SRC_USBCLASS)

LUFA_PATH    = LUFA-120730/LUFA
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER -IConfig/
# LD_FLAGS     = -Wl,--section-start=.bootspm=0x1FC00

ifeq ($(GRAPH_LCD),1)
SRC += lcd_graph.c
CC_FLAGS += -DGRAPH_LCD
else
SRC += lcd_text.c
endif

ifeq ($(OLD_LED),1)
CC_FLAGS += -DOLD_LED
endif

# Default target
all:

# Include LUFA build script makefiles
include $(LUFA_PATH)/Build/lufa_core.mk
include $(LUFA_PATH)/Build/lufa_sources.mk
include $(LUFA_PATH)/Build/lufa_build.mk
include $(LUFA_PATH)/Build/lufa_cppcheck.mk
#include $(LUFA_PATH)/Build/lufa_doxygen.mk
include $(LUFA_PATH)/Build/lufa_dfu.mk
include $(LUFA_PATH)/Build/lufa_hid.mk
include $(LUFA_PATH)/Build/lufa_avrdude.mk
include $(LUFA_PATH)/Build/lufa_atprogram.mk

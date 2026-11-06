EXECUTABLE = $(R2R_PD)/$(PRODUCT_BASE).abs
LIBRARY = $(R2R_PD)/$(PRODUCT_BASE).$(PLATFORM).lib

MWD := $(realpath $(dir $(lastword $(MAKEFILE_LIST)))..)
include $(MWD)/common.mk
include $(MWD)/toolchains/z88dk.mk

HDOS_FLAGS = +hdos -create-app -startup=3 -Cz--abs -pragma-define:CLIB_DISABLE_FGETS_CURSOR=1
CFLAGS += $(HDOS_FLAGS) -D__H89__
LDFLAGS += $(HDOS_FLAGS)

r2r:: $(BUILD_EXEC) $(BUILD_LIB) $(R2R_EXTRA_DEPS)
	make -f $(PLATFORM_MK) $(PLATFORM)/r2r-post

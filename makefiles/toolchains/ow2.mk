CC_DEFAULT ?= wcc
AS_DEFAULT ?= wasm
LD_DEFAULT ?= wlink OPTION quiet

include $(MWD)/tc-common.mk

CFLAGS += -zq -0 -bt=dos -fr=$(basename $@).err
ASFLAGS +=
LDFLAGS += SYSTEM dos LIBPATH $(FUJINET_LIB_DIR)

# Was causing issues when building with wine/wcc on macOS - can add back later
# CFLAGS += -DGIT_VERSION="$(GIT_VERSION)"

define include-dir-flag
  -I$1
endef

define asm-include-dir-flag
  -I$1
endef

define library-dir-flag
endef

define library-flag
  $1
endef

define link-lib
  $(LIB) -n $1 $2
endef

define link-bin
  $(LD) $(LDFLAGS) \
    disable 1014 \
    name $1 \
    file {$2} \
    library {$(LIBS)}
endef

define compile
  $(CC) $(CFLAGS) -ad=$(OBJ_DIR)/$(basename $(notdir $2)).d -fo=$1 $2
endef

define assemble
  $(AS) -c $(ASFLAGS) -o $1 $2 2>&1
endef


# 
# Z:\Users\eric\Documents\projects\fujinet-battleship>wcc -DPLATFORM_VARS=\"../msdos/vars.h\" -DUSING_FUJINET_LIB -I_cache/fujinet-lib/4.9.0-msdos -zq -0 -bt=dos -fr=build/msdos/gamelogic.err -DGIT_VERSION=\"15f1b22*\" -D__MSDOS__ -ad=build/msdos/gamelogic.d -fo=build/msdos/gamelogic.o src/gamelogic.c
# Z:\Users\eric\Documents\watcom\wine.cmd wcc -DPLATFORM_VARS="../msdos/vars.h" -DUSING_FUJINET_LIB -I_cache/fujinet-lib/4.9.0-msdos -zq -0 -bt=dos -fr=build/msdos/gamelogic.err -DGIT_VERSION="15f1b22*" -D__MSDOS__ -ad=build/msdos/gamelogic.d -fo=build/msdos/gamelogic.o src/gamelogic.c
# wine /Users/eric/Documents/WATCOM/wine.cmd wcc -DPLATFORM_VARS=\"\"../msdos/vars.h\"\" -DUSING_FUJINET_LIB -I_cache/fujinet-lib/4.9.0-msdos -zq -0 -bt=dos -fr=build/msdos/gamelogic.err -DGIT_VERSION="15f1b22*" -D__MSDOS__ -ad=build/msdos/gamelogic.d -fo=build/msdos/gamelogic.o src/gamelogic.c
# wine /Users/eric/Documents/watcom/wine.cmd wcc -DPLATFORM_VARS="../msdos/vars.h" -DUSING_FUJINET_LIB -I_cache/fujinet-lib/4.9.0-msdos -zq -0 -bt=dos -fr=build/msdos/gamelogic.err -D__MSDOS__ -ad=build/msdos/gamelogic.d -fo=build/msdos/gamelogic.o src/gamelogic.c
# Z:\Users\eric\Documents\watcom\wine.cmd wcc -DPLATFORM_VARS="../msdos/vars.h" -DUSING_FUJINET_LIB -I_cache/fujinet-lib/4.9.0-msdos -zq -0 -bt=dos -fr=build/msdos/gamelogic.err -DGIT_VERSION="15f1b22*" -D__MSDOS__ -ad=build/msdos/gamelogic.d -fo=build/msdos/gamelogic.o src/gamelogic.c
# Z:\Users\eric\Documents\watcom\wine.cmd wcc -DPLATFORM_VARS=\"../msdos/vars.h\" -DUSING_FUJINET_LIB -I_cache/fujinet-lib/4.9.0-msdos -zq -0 -bt=dos -fr=build/msdos/gamelogic.err -D__MSDOS__ -ad=build/msdos/gamelogic.d -fo=build/msdos/gamelogic.o src/gamelogic.c
# Z:\Users\eric\Documents\watcom\wine.cmd wcc -DPLATFORM_VARS="../msdos/vars.h" -DUSING_FUJINET_LIB -I_cache/fujinet-lib/4.9.0-msdos -zq -0 -bt dos -fr build/msdos/gamelogic.err -D__MSDOS__ -ad build/msdos/gamelogic.d -fo build/msdos/gamelogic.o src/gamelogic.c
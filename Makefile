#################################################################
# THIS is the ONLY Makefile that should be modified.            #
#                                                               #
# Do NOT customize files in /mekkogx                           #
#                                                               #
# Read mekkogx/README.md for more information                   #
#################################################################

# TO BUILD: 		make <platform>
# PLATFORMS: 		apple2 atari coco coco3 msdos
# PLATFORMS TODO:   c64 adam msxrom

# C64 SPECIFIC:
# To test in VICE:  make c64 VICE=1
# You must run support/c64/fuji_mock_network.py as a bridge

# COCO SPECIFIC:
# 	Coco 1/2: 		make coco
# 	Coco 3: 		make coco3
#   Combined Dist:  make coco-dist
#   Test Dist:      make coco-dist test-coco-dist

define HEADER
  ______     _ _ _   _      _     ____        _   _   _           _     _       
 │  ____│   (_│_) ╲ │ │    │ │   │  _ ╲      │ │ │ │ │ │         │ │   (_)      
 │ │__ _   _ _ _│  ╲│ │ ___│ │_  │ │_) │ __ _│ │_│ │_│ │ ___  ___│ │__  _ _ __  
 │  __│ │ │ │ │ │ . ` │╱ _ ╲ __│ │  _ < ╱ _` │ __│ __│ │╱ _ ╲╱ __│ '_ ╲│ │ '_ ╲ 
 │ │  │ │_│ │ │ │ │╲  │  __╱ │_  │ │_) │ (_│ │ │_│ │_│ │  __╱╲__ ╲ │ │ │ │ │_) │
 │_│   ╲__,_│ │_│_│ ╲_│╲___│╲__│ │____╱ ╲__,_│╲__│╲__│_│╲___││___╱_│ │_│_│ .__╱ 
           _╱ │                                                          │ │    
          │__╱                                                           │_│          
  
endef

PRODUCT = fbs
PRODUCT_UPPER = FBS
PLATFORMS = coco atari apple2 c64 adam

# Use "make-exp msdos" to build msdos.
# That version uses fujinet-lib-experimental.

# SRC_DIRS may use the literal %PLATFORM% token.
# It expands to the chosen PLATFORM plus any of its combos.
SRC_DIRS = src src/%PLATFORM%

# FUJINET_LIB - specify version such as 4.7.6, or leave empty for latest
FUJINET_LIB = 4.8.2

## Compiler / Linker flags                                     ##
#################################################################

## Include platform specific vars.h
CFLAGS += -DPLATFORM_VARS="\"../$(PLATFORM)/vars.h\""

## MS-DOS hack - combination of wine/wcc doesn't like \" above, so resorting to manual include in vars.h
ifeq ($(PLATFORM),msdos)
  CFLAGS =
endif

## Adam (z88dk) - same vars.h hack; build against the local fujinet-lib tree
ifeq ($(PLATFORM),adam)
  CFLAGS =
  FUJINET_LIB = ../fujinet-lib/build
endif
LDFLAGS_EXTRA_ADAM = -m

## ColecoVision (z88dk +coleco, FujiNet mailbox cartridge). Not in PLATFORMS:
## it needs fujinet-lib-experimental (the only lib with the coleco bus), so
## build with either
##   ./make-exp coleco                                      (clones + builds the lib)
##   make coleco FUJINET_LIB=$(HOME)/Workspace/fujinet-lib-experimental
## z88dk puts -D__COLECO__ in the target-wide OPTIONS line, so the Adam
## subtype defines it too -- __COLECO__ alone does not mean ColecoVision.
## src/coleco is therefore guarded by BUILD_COLECO.
ifeq ($(PLATFORM),coleco)
  CFLAGS =
endif
CFLAGS_EXTRA_COLECO = -DBUILD_COLECO -DCUSTOM_FUJINET_CALLS -O3
# The console has 1K of RAM and the BIOS owns both ends of it: $7000-$702B is
# the cartridge header's own tables, $73B9-$73FF is BIOS scratch, and what is
# left -- 908 bytes -- holds BSS, DATA and the C stack. -m leaves a map next to
# the image so that total can actually be read off. No generic console: the
# game drives the VDP itself (src/coleco/graphics.c), so the crt0 is told to
# leave the screen mode alone.
LDFLAGS_EXTRA_COLECO += -m \
  -pragma-define:CRT_ORG_BSS=0x702C \
  -pragma-define:REGISTER_SP=0x73B8 \
  -pragma-define:CRT_ENABLE_STDIO=0 \
  -pragma-define:CLIB_FOPEN_MAX=0 \
  -pragma-define:CLIB_EXIT_STACK_SIZE=0 \
  -pragma-define:CLIB_DEFAULT_SCREEN_MODE=-1

## Coco specific flags (cmoc)
CFLAGS_EXTRA_COCO = \
	-Wno-assign-in-condition \
	--no-relocate \
	--intermediate

ifeq ($(MAKE_COCO3),COCO3)
# 	Coco 3
	CFLAGS_EXTRA_COCO += -DCOCO3
	LDFLAGS_EXTRA_COCO = --limit=7800 --org=1000
else
# 	Coco 1/2	
	LDFLAGS_EXTRA_COCO = --limit=5ff0 --org=1000
endif

ifeq ($(VICE),1)
# VICE C64 emulator specific flags
	CFLAGS_EXTRA_C64 += -DUSE_EMULATOR=1
	LDFLAGS_EXTRA_C64 += -DUSE_EMULATOR=1
endif

# Variables for coco-dist
R2R_PRODUCT = r2r/coco/$(PRODUCT)
COCO_DISK = $(R2R_PRODUCT).dsk

# Support 'make coco3'
coco3:
	make coco MAKE_COCO3=COCO3


# Apple II specific flags (cc65)
CFLAGS_EXTRA_APPLE2 += -Os
LDFLAGS_EXTRA_APPLE2 += --start-addr 0x4000 --ld-args -D,__HIMEM__=0xBF00


# C64 specific flags (cc65)
# Use custom linker configuration to move stack from $CFFF to $BFFF
LDFLAGS_EXTRA_C64 += -C support/c64/c64-custom.cfg

#################################################################
## PRE BUILD STEPS                                             ##
#################################################################


$(PLATFORM)/r2r::
	$(info $(HEADER) )	

#   TEMP USE - Uncomment to clean entire obj dir
#	rm -rf $(OBJ_DIR)

#	Delete charset objects so every build gets the latest charset
#	from /support/[platform] without needing to clean.
	rm -f build/$(PLATFORM)/charset.o
	rm -f build/$(PLATFORM)/hires.o

#   COLECO ONLY - regenerate the charset from the msdos art
ifeq ($(PLATFORM),coleco)
	python3 support/coleco/make_charset.py
endif

#   COCO ONLY - copy proper file for Coco1/2 vs Coco3	
ifeq ($(MAKE_COCO3),COCO3)
	cp support/coco/charset-16.image support/coco/charset.bin
else
# 	The 2bpp charset source file is 1024 bytes (up to 128 characters).
#   CoCo 1/2 has limited space, so copy just the bytes we need
#   Currently: 105 characters -  105*8=840 bytes
	head -c 840 support/coco/charset.fnt > support/coco/charset.bin
endif

#################################################################
# Include MekkoGX makefile system (Make Gen-X)
include mekkogx/toplevel-rules.mk
#################################################################


#################################################################
## POST BUILD STEPS                                            ##
#################################################################

## Show executable size
$(PLATFORM)/disk-post::
	@echo ........................................................................ ;ls -l $(EXECUTABLE);echo ........................................................................

coco/disk-post::
ifneq ($(SKIP_EMU),1)
#	Copy to fujinet-pc SD drive. On first run, mount that drive for future runs
	cp $(DISK) ~/Documents/fujinetpc-coco/SD

#   Mount the disk in FujiNet-PC (assumes host 1 is SD)
	curl -s "http://localhost:8000/browse/host/1/$(PRODUCT).dsk?action=newmount&slot=1&mode=r" >/dev/null
	curl -s "http://localhost:8000/mount?mountall=1&redirect=1" >/dev/null
#
# 	Fast speed: -ui_active and -nothrottle starts the emulator in fast mode to quickly load the app. I then throttle it to 100% speed with a hotkey.

#	cd ~/mame_coco;mame coco3 -ui_active -nothrottle -window -nomaximize -resolution 1300x1024 -autoboot_delay 2 -nounevenstretch  -autoboot_command "runm\"$(PRODUCT)\n"

ifeq ($(MAKE_COCO3),COCO3)
	cd ~/mame_coco;mame coco3 -ui_active -nothrottle -window -nomaximize -resolution 1300x1024 -autoboot_delay 2 -nounevenstretch  -autoboot_command "runm\"$(PRODUCT)\n"
else
	cd ~/mame_coco;mame coco -ui_active -nothrottle -window -nomaximize -resolution 1200x1024 -autoboot_delay 2 -nounevenstretch  -autoboot_command "runm\"$(PRODUCT)\n"
endif
endif
# Start normal speed
#	cd ~/mame_coco;mame coco -ui_active -throttle -window -nomaximize -resolution 1200x1024 -autoboot_delay 2 -nounevenstretch  -autoboot_command "runm\"fbs\n"

c64/disk-post::
	x64sc $(CURDIR)/$(EXECUTABLE)
  
atari/disk-post::
	wine /Users/eric/Documents/Altirra/Altirra64.exe /singleinstance /run $(EXECUTABLE) >/dev/null 2>&1
#	Copy to fujinet-pc SD drive. On first run, mount that drive for future runs
#	cp $(EXECUTABLE) ~/Documents/fujinetpc-atari/SD

msdos/disk-post::
	mcopy -t -i $(DISK) src/msdos/AUTOEXEC.BAT "::AUTOEXEC.BAT"
#	cp $(DISK) ~/tnfs/
#	Copy to fujinet-pc SD drive.
	cp $(DISK) ~/Documents/fujinetpc-rs232/SD
#	Mount the disk in FujiNet-PC (assumes host 1 is SD)
	curl -s "http://localhost:8005/browse/host/1/$(PRODUCT).img?action=newmount&slot=1&mode=r" >/dev/null 2>&1
	curl -s "http://localhost:8005/mount?mountall=1&redirect=1" >/dev/null 2>&1

apple2/disk-post::
# HACK - MekkoGX Currently doesn't use the preferred bootable PO for Apple II, so manually creating
	cp support/apple2/bootable.po $(DISK)
	ac -p "$(DISK)" $(PRODUCT_UPPER).SYSTEM SYS < $(CC65_UTILS_DIR)/$(LOADER_SYSTEM)
	ac -as "$(DISK)" $(PRODUCT_UPPER) bin <$(EXECUTABLE)

adam/r2r-post::
#	Stage the DDP on the local TNFS root for loading via the TMA-3 host slot.
#	Skipped silently inside the defoogi container, where ~/tnfs is not mounted.
	-@[ -d ~/tnfs ] && cp $(EXECUTABLE) ~/tnfs/ && echo "Copied $(EXECUTABLE) to ~/tnfs/" || true
	

# ColecoVision: headless smoke test in MAME's coleco driver, against a live
# fujinet-pc (the cartridge device dials its BoIP listener on 127.0.0.1:9995).
# MAME resolves rompath, pluginspath and its Lua search path against its OWN
# working directory, so it is run from the MAME tree and everything handed to
# it is absolute -- run it from anywhere else and -autoboot_script is ignored
# silently. That tree needs fujinet-firmware/pico/coleco/emu/apply.sh run
# against it once for -cartslot fujinet to exist.
#
#   make coleco-smoke                          print the screen
#   make coleco-smoke EXPECT="FUJI BATTLESHIP" and assert on it
#   make coleco-smoke SCRIPT="fire,fire"       drive the controller first
#   make coleco-smoke AT=20                    settle longer before sampling
MAME_DIR    ?= $(HOME)/Workspace/mame
COLECO_ROM  := $(CURDIR)/r2r/coleco/$(PRODUCT).rom
AT          ?= 8
EXPECT      ?=
SCRIPT      ?=
# Each scripted press costs a hold plus a gap; 3s of settle before the first.
SETTLE      ?= 8
comma       := ,
SECS        ?= $(shell echo $$(( $(AT) + $(SETTLE) + 4 + 2 * $(words $(subst $(comma), ,$(SCRIPT))) )))

.PHONY: coleco-smoke

coleco-smoke:
	cd $(MAME_DIR) && \
	FBS_FONT=$(CURDIR)/src/coleco/font.bin FBS_AT=$(AT) FBS_EXPECT="$(EXPECT)" \
	FBS_SCRIPT="$(SCRIPT)" FBS_SETTLE=$(SETTLE) FBS_SNAP="$(SNAP)" \
	./mame coleco -cartslot fujinet -cart $(COLECO_ROM) \
	    -video none -sound none -nothrottle -seconds_to_run $(SECS) \
	    -autoboot_script $(CURDIR)/support/coleco/smoke.lua

# Reset FujiNet-PC
reset-fn:
	curl http://localhost:8000/restart >/dev/null


#################################################################
## CUSTOM DISTRIBUTION RECIPES                                 ##
#################################################################

coco-dist:
	make clean	
# Build both versions of the program, clearing build dir between builds
	rm -rf $(BUILD_DIR)
	make coco SKIP_EMU=1
	mv r2r/coco/$(PRODUCT).bin $(R2R_PRODUCT)12.bin 

	rm -rf $(BUILD_DIR)
	make coco3 SKIP_EMU=1
	mv r2r/coco/$(PRODUCT).bin $(R2R_PRODUCT)3.bin 

# Build the loader
	cmoc -DPRODUCT=\"$(PRODUCT_UPPER)\" -o $(R2R_PRODUCT).bin support/coco/loader.c

# Create the disk with the loader and both versions of the program
	$(RM) $(COCO_DISK)
	decb dskini $(COCO_DISK)
	echo RUNM\"$(PRODUCT_UPPER)\" > build/coco/autoexec.bas
	decb copy -t -0 build/coco/autoexec.bas $(COCO_DISK),AUTOEXEC.BAS
	writecocofile $(COCO_DISK) $(R2R_PRODUCT).bin
	writecocofile $(COCO_DISK) $(R2R_PRODUCT)12.bin
	writecocofile $(COCO_DISK) $(R2R_PRODUCT)3.bin

test-coco-dist:
#   Launch dist disk in emulator

#	Copy to fujinet-pc SD drive. On first run, mount that drive for future runs
	cp $(COCO_DISK) ~/Documents/fujinetpc-coco/SD
#	Mount the disk in FujiNet-PC (assumes host 1 is SD)
	curl -s "http://localhost:8000/browse/host/1/$(PRODUCT).dsk?action=newmount&slot=1&mode=r" >/dev/null
	curl -s "http://localhost:8000/mount?mountall=1&redirect=1" >/dev/null
	cd ~/mame_coco;mame coco -ui_active -throttle -window -nomaximize -resolution 1300x1024 -autoboot_delay 2 -nounevenstretch  -autoboot_command ""
#	cd ~/mame_coco;mame coco3 -ui_active -throttle -window -nomaximize -resolution 1300x1024 -autoboot_delay 2 -nounevenstretch  -autoboot_command ""

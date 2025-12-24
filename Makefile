#################################################################
# THIS is the ONLY Makefile that should be modified.            #
#                                                               #
# Do NOT customize files in /makefiles                          #
#                                                               #
# Read makefiles/README.md for more information                 #
#################################################################

# TO BUILD: 		make <platform>
# COCO PLATFORMS:
# 	Coco 1/2: 		make coco
# 	Coco 3: 		make coco3
#   Combined Dist:  make coco-dist
#   Test Dist:      make coco-dist test

PRODUCT = fbs
PRODUCT_UPPER = FBS
PLATFORMS = coco msdos atari apple2

#PLATFORMS = coco apple2 atari c64 adam msdos msxrom # TODO

# SRC_DIRS may use the literal %PLATFORM% token.
# It expands to the chosen PLATFORM plus any of its combos.
SRC_DIRS = src src/%PLATFORM%

# FUJINET_LIB - specify version such as 4.7.6, or leave empty for latest
FUJINET_LIB = 


## Compiler / Linker flags                                     ##
#################################################################

## Include platform specific vars.h
CFLAGS += -DPLATFORM_VARS="\"../$(PLATFORM)/vars.h\""

CFLAGS_EXTRA_COCO = \
	-Wno-assign-in-condition \
	-I src/include \
	--no-relocate \
	--intermediate

## COCO 1/2 or COCO 3 specific flags
ifeq ($(MAKE_COCO3),COCO3)
	CFLAGS_EXTRA_COCO += -DCOCO3
	LDFLAGS_EXTRA_COCO = --limit=7800 --org=1000 # Coco3
else
	LDFLAGS_EXTRA_COCO = --limit=5fff --org=1000 # Coco1/2
endif

# Variables for coco-dist
R2R_PRODUCT = r2r/coco/$(PRODUCT)
COCO_DISK = $(R2R_PRODUCT).dsk

coco3:
	make coco MAKE_COCO3=COCO3

# Apple II specific flags (cc65)
CFLAGS_EXTRA_APPLE2 += -Os -D__APPLE2__
LDFLAGS_EXTRA_APPLE2 += --start-addr 0x4000 --ld-args -D,__HIMEM__=0xBF00
#################################################################
## PRE BUILD STEPS                                             ##
#################################################################

# Delete charset objects so every build gets the latest charset
# from /support/[platform]/charset.fnt without needing to clean.
# COCO ONLY - copy proper file for Coco1/2 vs Coco3
$(PLATFORM)/r2r::
	rm -rf $(OBJ_DIR)
	rm -f build/$(PLATFORM)/charset.o
ifeq ($(MAKE_COCO3),COCO3)
	cp support/coco/charset-16.img.bin support/coco/charset.bin
else
	cp support/coco/charset.fnt support/coco/charset.bin
endif

#################################################################
# Include MekkoGX makefile system (Make Gen-X)
include makefiles/toplevel-rules.mk
#################################################################


#################################################################
## POST BUILD STEPS                                            ##
#################################################################

## Show executable size
$(PLATFORM)/disk-post::
	@echo ........................................................................ ;ls -l $(EXECUTABLE);echo ........................................................................

coco/disk-post::
#	Copy to fujinet-pc SD drive. On first run, mount that drive for future runs
	cp $(DISK) ~/Documents/fujinetpc-coco/SD

#   Mount the disk in FujiNet-PC (assumes host 1 is SD)
	curl -s "http://localhost:8000/browse/host/1/$(PRODUCT).dsk?action=newmount&slot=1&mode=r" >/dev/null
	curl -s "http://localhost:8000/mount?mountall=1&redirect=1" >/dev/null
#
# 	Fast speed: -ui_active and -nothrottle starts the emulator in fast mode to quickly load the app. I then throttle it to 100% speed with a hotkey.

#	cd ~/mame_coco;mame coco3 -ui_active -nothrottle -window -nomaximize -resolution 1300x1024 -autoboot_delay 2 -nounevenstretch  -autoboot_command "runm\"$(PRODUCT)\n"
ifneq ($(SKIP_EMU),1)
ifeq ($(MAKE_COCO3),COCO3)
	cd ~/mame_coco;mame coco3 -ui_active -nothrottle -window -nomaximize -resolution 1300x1024 -autoboot_delay 2 -nounevenstretch  -autoboot_command "runm\"$(PRODUCT)\n"
else
	cd ~/mame_coco;mame coco -ui_active -nothrottle -window -nomaximize -resolution 1200x1024 -autoboot_delay 2 -nounevenstretch  -autoboot_command "runm\"$(PRODUCT)\n"
endif
endif
# Start normal speed
#	cd ~/mame_coco;mame coco -ui_active -throttle -window -nomaximize -resolution 1200x1024 -autoboot_delay 2 -nounevenstretch  -autoboot_command "runm\"fbs\n"


atari/disk-post::
	wine /Users/eric/Documents/Altirra/Altirra64.exe /singleinstance /run $(EXECUTABLE) >/dev/null 2>&1
#	Copy to fujinet-pc SD drive. On first run, mount that drive for future runs
#	cp $(EXECUTABLE) ~/Documents/fujinetpc-atari/SD

msdos/disk-post::
	cp $(DISK) ~/tnfs/

# Reset FujiNet-PC
reset-fn:
	curl http://localhost:8000/restart >/dev/null


#################################################################
## CUSTOM DISTRIBUTION RECIPES                                 ##
#################################################################

coco-dist:
	make clean	
# Build both versions of the program
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

test: coco-dist
#   Launch dist disk in emulator

#	Copy to fujinet-pc SD drive. On first run, mount that drive for future runs
	cp $(COCO_DISK) ~/Documents/fujinetpc-coco/SD
#	Mount the disk in FujiNet-PC (assumes host 1 is SD)
	curl -s "http://localhost:8000/browse/host/1/$(PRODUCT).dsk?action=newmount&slot=1&mode=r" >/dev/null
	curl -s "http://localhost:8000/mount?mountall=1&redirect=1" >/dev/null
#	cd ~/mame_coco;mame coco -ui_active -throttle -window -nomaximize -resolution 1300x1024 -autoboot_delay 2 -nounevenstretch  -autoboot_command ""
	cd ~/mame_coco;mame coco3 -ui_active -throttle -window -nomaximize -resolution 1300x1024 -autoboot_delay 2 -nounevenstretch  -autoboot_command ""

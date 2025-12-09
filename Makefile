#################################################################
# THIS is the ONLY Makefile that should be modified.            #
#                                                               #
# Do NOT customize files in /makefiles                          #
#                                                               #
# Read makefiles/README.md for more information                 #
#################################################################

# TO BUILD: Run 'make <platform>'

PRODUCT = fbs
PLATFORMS = coco msdos atari

#PLATFORMS = coco apple2 atari c64 adam msdos msxrom # TODO



# SRC_DIRS may use the literal %PLATFORM% token.
# It expands to the chosen PLATFORM plus any of its combos.
SRC_DIRS = src src/%PLATFORM%

# FUJINET_LIB - specify version such as 4.7.6, or leave empty for latest
FUJINET_LIB =

#################################################################
## Compiler / Linker flags                                     ##
#################################################################

## Include platform specific vars.h
CFLAGS += -DPLATFORM_VARS="\"../$(PLATFORM)/vars.h\""

## COCO (CMOC)
CFLAGS_EXTRA_COCO = \
	-Wno-assign-in-condition \
	-I src/include \
	--no-relocate \
	--intermediate

LDFLAGS_EXTRA_COCO = --limit=5fff --org=1000


#################################################################
## PRE BUILD STEPS                                             ##
#################################################################

# Delete charset objects so every build gets the latest charset
# from /support/[platform]/charset.fnt without needing to clean.
$(PLATFORM)/r2r::
	rm -f build/$(PLATFORM)/charset.o


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
#	cd ~/mame_coco;mame coco -ui_active -nothrottle -window -nomaximize -resolution 1200x1024 -autoboot_delay 2 -nounevenstretch  -autoboot_command "runm\"$(PRODUCT)\n"
#	cd ~/mame_coco;mame coco3 -ui_active -nothrottle -window -nomaximize -resolution 1300x1024 -autoboot_delay 2 -nounevenstretch  -autoboot_command "runm\"$(PRODUCT)\n"
	cd ~/mame_coco;mame coco3 -ui_active -nothrottle -window -nomaximize -resolution 800x600 -autoboot_delay 2 -nounevenstretch  -autoboot_command "runm\"$(PRODUCT)\n"
# Start normal speed
#	cd ~/mame_coco;mame coco -ui_active -throttle -window -nomaximize -resolution 1200x1024 -autoboot_delay 2 -nounevenstretch  -autoboot_command "runm\"fbs\n"


atari/disk-post::
	wine /Users/eric/Documents/Altirra/Altirra64.exe /singleinstance /run $(EXECUTABLE) >/dev/null 2>&1
#	Copy to fujinet-pc SD drive. On first run, mount that drive for future runs
#	cp $(EXECUTABLE) ~/Documents/fujinetpc-atari/SD


# Reset FujiNet-PC
reset-fn:
	curl http://localhost:8000/restart >/dev/null

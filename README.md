# Fuji Battleships
This is a WIP cross platform game client for the Fuji Battleships server.



### Supported Platforms
* **Apple II**
* **Atari**
* **CoCo**
* **C64** (WIP)
* **MS-Dos**
* **NES** (FujiNet NES cartridge)
* **Sega Master System** (FujiNet SMS cartridge)
* **Heathkit H89** (H89 FujiNet card; HDOS and CP/M)
* *Please contribute to add more!*

### To Build
1. Review/edit the Post Build and the emulator start commands in `Makefile`
2. To build and test: `make [platform]`
3. Platforms: **apple2** **atari** **c64*** **coco*** **msdos**

### C64
To test in VICE, run support/c64/fuji_mock_network.py and make as follows:
* `make c64 VICE=1`

### CoCo
The distribution disk includes two binaries and a small loader to detect Coco 1/2 or 3 and run the appropriate binary. You may also build just one binary for testing.
* 	CoCo 1/2: 		`make coco`
* 	CoCo 3: 		`make coco3`
*   Combined Disk:  `make coco-dist`
*   Test Disk:      `make coco-dist test-coco-dist`

### NES
Built with cc65 against the `add-nes` branch of fujinet-lib-experimental (the only lib with the NES cartridge bus):
* `make PLATFORMS=nes nes FUJINET_LIB=$HOME/Workspace/fujinet-lib-experimental`

The output is `r2r/nes/fbs.nes` (NROM, 32K PRG + 8K CHR), stamped with the "FUJI" claim the cartridge needs. The art is the MS-DOS sheet converted by `src/nes/mkchr.py`, which runs before every build. Test in MAME with the FujiNet NES slot against fujinet-pc:
* `make nes-smoke EXPECT="FUJI BATTLESHIP" [SCRIPT="a,wait5,select+start"] [SNAP=/path/shot.png]`
* `make nes-play`

Controls: d-pad moves, A selects/fires, B refreshes/rotates, START opens the in-game menu, SELECT changes name, SELECT+A help, SELECT+B sound, SELECT+START quit.

### Sega Master System
Built with z88dk (`zcc +sms`) against the `add-sms` branch of fujinet-lib-experimental (a worktree of it at `~/Workspace/fnlib-sms`), the only lib with the SMS cartridge bus:
* `make PLATFORMS=sms sms FUJINET_LIB=$HOME/Workspace/fnlib-sms`

The output is `r2r/sms/fbs.sms`, a flat 32K image stamped with the "FUJI" claim the cartridge needs. The art is the CoCo 3's 16-colour sheet, which Mode 4 shows without losing a colour; `src/sms/mktiles.py` converts it, with the MS-DOS font for text, before every build. Test in MAME's `sms1` driver with the FujiNet slot against fujinet-pc (BoIP on 127.0.0.1:9995):
* `make sms-smoke EXPECT="FUJI BATTLESHIP" [SCRIPT="b1,wait5,b2+up,snap"] [SNAP=/path/shot.png]`
* `make sms-smoke NOTHROTTLE= ...` for a live game: the server's lobby countdown runs on wall-clock time
* `make sms-play`

Controls: d-pad moves, 1 selects/fires/readies, 2 refreshes/rotates, Pause opens the in-game menu. Holding 2 shifts: 2+Up help, 2+Down sound, 2+Left/Right change name, 2+1 quit.

### Heathkit H89
Built with z88dk (`zcc +hdos` / `zcc +cpm`) against the `h89-support` branch of fujinet-lib, checked out at `~/Workspace/fujinet-lib` (the Makefile points both H89 builds there). Both builds share `src/h89`:
* HDOS: `make PLATFORMS=h89-hdos h89-hdos` makes `r2r/h89-hdos/fbs.abs`, and `fbs.h8d`, a 100K H17 disk holding `FBS.ABS`: `MOUNT SY1:` then `RUN SY1:FBS.ABS`
* CP/M: `make PLATFORMS=h89-cpm h89-cpm` makes `r2r/h89-cpm/fbs.com`, and `fbs.h8d`, an H17 disk holding `FBS.COM`: `B:FBS`

The disks need z88dk's `z88dk-appmake` on the PATH. The game draws on the H19 terminal in 80x24 with its graphics characters, reverse video and cursor addressing, and sends only what changes on screen, since the console runs at 9600 baud. Test it in FujiNet Go H89 (`~/Workspace/fujinet-go-h89-desktop`), with the disk in drive 2 beside its HDOS 2.0 or 64K CP/M boot disk.

Controls: the keypad arrows (or `,` `.` `-` `=`) move, SPACE or RETURN selects/fires/readies, `R` refreshes/rotates, ESC (or f1-f5) opens the in-game menu, and `H`, `N`, `S` and `Q` are help, name, sound and quit. Quitting returns to HDOS or CP/M.

### Build Output - in /r2r

The "Ready 2 Run" output files will be in `./r2r`, which can be copied to a TNFS server, etc.

This project uses the MekkoGX Makefiles platform, which should automatically download the Fujinet-Lib dependency.


# Server / Api details

Please visit the server page for more information:

https://github.com/FujiNetWIFI/servers/tree/main/fujinet-game-system/battleship#readme

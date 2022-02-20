# VDC Screen Editor:
# Screen editor for the C128 80 column mode
# Written in 2021 by Xander Mol
# https://github.com/xahmol/VDCScreenEdit
# https://www.idreamtin8bits.com/

# See src/main.c for full credits

# Prerequisites for building:
# - CC65 compiled and included in path with sudo make avail
# - ZIP packages installed: sudo apt-get install zip
# - wput command installed: sudo apt-get install wput

SOURCESMAIN = src/main.c src/vdc_core.c
SOURCESLIB = src/vdc_core_assembly.s src/bootsect.s
OBJECTS = bootsect.bin vdcse.maco.prg vdcse.falt.prg vdcse.fstd.prg vdcse.tscr.prg vdcse.hsc1.prg vdcse.hsc2.prg vdcse.hsc3.prg vdcse.hsc4.prg

ZIP = vdcscreenedit-v090-$(shell date "+%Y%m%d-%H%M").zip
D64 = vdcse.d64
D71 = vdcse.d71
D81 = vdcse.d81
README = README.pdf

# Hostname of Ultimate II+ target for deployment. Edit for proper IP and usb number
ULTHOST = ftp://192.168.1.19/usb1/dev/
ULTHOST2 = ftp://192.168.1.31/usb1/dev/

MAIN = vdcse.prg

CC65_TARGET = c128
CC = cl65
CFLAGS  = -t $(CC65_TARGET) --create-dep $(<:.c=.d) -Os -I include
LDFLAGSMAIN = -t $(CC65_TARGET) -C vdcse-cc65config.cfg -m $(MAIN).map

########################################

.SUFFIXES:
.PHONY: all clean deploy vice
all: $(MAIN) $(D64) $(D71) $(D81) $(ZIP)

ifneq ($(MAKECMDGOALS),clean)
-include $(SOURCESMAIN:.c=.d)
endif

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<
  
$(MAIN): $(SOURCESLIB) $(SOURCESMAIN:.c=.o)
	$(CC) $(LDFLAGSMAIN) -o $@ $^

$(D64):	$(MAIN) $(OBJECTS)
	c1541 -format "vdcse,xm" d64 $(D64)
	c1541 $(D64) -bwrite bootsect.bin 1 0
	c1541 $(D64) -bpoke 18 0 4 $14 %11111110
	c1541 $(D64) -bam 1 1
	c1541 -attach $(D64) -write vdcse.prg vdcse
	c1541 -attach $(D64) -write vdcse.maco.prg vdcse.maco
	c1541 -attach $(D64) -write vdcse.falt.prg vdcse.falt
	c1541 -attach $(D64) -write vdcse.fstd.prg vdcse.fstd
	c1541 -attach $(D64) -write vdcse.tscr.prg vdcse.tscr
	c1541 -attach $(D64) -write vdcse.hsc1.prg vdcse.hsc1
	c1541 -attach $(D64) -write vdcse.hsc2.prg vdcse.hsc2
	c1541 -attach $(D64) -write vdcse.hsc3.prg vdcse.hsc3
	c1541 -attach $(D64) -write vdcse.hsc4.prg vdcse.hsc4

$(D71):	$(MAIN) $(OBJECTS)
	c1541 -format "vdcse,xm" d71 $(D71)
	c1541 $(D71) -bwrite bootsect.bin 1 0
	c1541 $(D71) -bpoke 18 0 4 $14 %11111110
	c1541 $(D71) -bam 1 1
	c1541 -attach $(D71) -write vdcse.prg vdcse
	c1541 -attach $(D71) -write vdcse.maco.prg vdcse.maco
	c1541 -attach $(D71) -write vdcse.falt.prg vdcse.falt
	c1541 -attach $(D71) -write vdcse.fstd.prg vdcse.fstd
	c1541 -attach $(D71) -write vdcse.tscr.prg vdcse.tscr
	c1541 -attach $(D71) -write vdcse.hsc1.prg vdcse.hsc1
	c1541 -attach $(D71) -write vdcse.hsc2.prg vdcse.hsc2
	c1541 -attach $(D71) -write vdcse.hsc3.prg vdcse.hsc3
	c1541 -attach $(D71) -write vdcse.hsc4.prg vdcse.hsc4

$(D81):	$(MAIN) $(OBJECTS)
	c1541 -format "vdcse,xm" d81 $(D81)
	c1541 $(D81) -bwrite bootsect.bin 1 0
	c1541 $(D81) -bpoke 40 1 16 $27 %11111110
	c1541 $(D81) -bam 1 1
	c1541 -attach $(D81) -write vdcse.prg vdcse
	c1541 -attach $(D81) -write vdcse.maco.prg vdcse.maco
	c1541 -attach $(D81) -write vdcse.falt.prg vdcse.falt
	c1541 -attach $(D81) -write vdcse.fstd.prg vdcse.fstd
	c1541 -attach $(D81) -write vdcse.tscr.prg vdcse.tscr
	c1541 -attach $(D81) -write vdcse.hsc1.prg vdcse.hsc1
	c1541 -attach $(D81) -write vdcse.hsc2.prg vdcse.hsc2
	c1541 -attach $(D81) -write vdcse.hsc3.prg vdcse.hsc3
	c1541 -attach $(D81) -write vdcse.hsc4.prg vdcse.hsc4

$(ZIP): $(MAIN) $(OBJECTS) $(D64) $(D71) $(D81) $(README)
	zip $@ $^

clean:
	$(RM) $(SOURCESMAIN:.c=.o) $(SOURCESMAIN:.c=.d) $(MAIN) $(MAIN).map
	
# To deploy software to UII+ enter make deploy. Obviously C128 needs to powered on with UII+ and USB drive connected.
deploy: $(MAIN)
	wput -u $(MAIN) $(OBJECTS) $(D64) $(D71) $(D81) $(ULTHOST)
	wput -u $(MAIN) $(OBJECTS) $(D64) $(D71) $(D81) $(ULTHOST2)

# To run software in VICE
vice: $(D81)
	x128 -autostart $(D81)

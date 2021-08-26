// ====================================================================================
// vdc_core.c
// Functions and definitions which make working with the Commodore 128's VDC easier
//
// Credits for code and inspiration:
//
// C128 Programmers Reference Guide:
// http://www.zimmers.net/anonftp/pub/cbm/manuals/c128/C128_Programmers_Reference_Guide.pdf
//
// Scott Hutter - VDC Core functions inspiration:
// https://github.com/Commodore64128/vdc_gui/blob/master/src/vdc_core.c
// (used as starting point, but channged to inline assembler for core functions, added VDC wait statements and expanded)
//
// Francesco Sblendorio - Screen Utility:
// https://github.com/xlar54/ultimateii-dos-lib/blob/master/src/samples/screen_utility.c
//
// DevDef: Commodore 128 Assembly - Part 3: The 80-column (8563) chip
// https://devdef.blogspot.com/2018/03/commodore-128-assembly-part-3-80-column.html
//
// 6502.org: Practical Memory Move Routines
// http://6502.org/source/general/memory_move.html
//
// =====================================================================================

#include <stdio.h>
#include <string.h>
#include <peekpoke.h>
#include <cbm.h>
#include <conio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <device.h>
#include <accelerator.h>
#include <c128.h>
#include "defines.h"
#include "vdc_core.h"

unsigned char VDC_ReadRegister(unsigned char registeraddress)
{
	// Function to read a VDC register
	// Input: Registernumber, Output: returned register value

	VDC_regadd = registeraddress;
	
	VDC_ReadRegister_core();

	return VDC_regval;						// Return the register value
}

void VDC_WriteRegister(unsigned char registeraddress, unsigned char registervalue)
{
	// Function to write a VDC register
	// Input: Registernumber and value to write

	VDC_regadd = registeraddress;
	VDC_regval = registervalue;

	VDC_WriteRegister_core();
}

void VDC_Poke(int address,  unsigned char value)
{
	// Function to store a value to a VDC address
	// Innput: VDC address and value to store
	VDC_addrh = (address>>8) & 0xff;
	VDC_addrl = address & 0xff;
	VDC_value = value;

	VDC_Poke_core();
}

unsigned char VDC_Peek(int address)
{
	// Function to read a value from a VDC address
	// Innput: VDC address, Output: read value

	VDC_addrh = (address>>8) & 0xff;
	VDC_addrl = address & 0xff;

	VDC_Peek_core();

	/* Return value via VDC register 31 */
	return VDC_value;
}

void VDC_MemCopy(unsigned int sourceaddr, unsigned int destaddr, unsigned int length)
{
	// Function to copy memory from one to another position within VDC memory
	// Input: Sourceaddress, destination address, number of bytes to copy

	VDC_addrh = (sourceaddr>>8) & 0xff;		// Obtain high byte of source address
	VDC_addrl = sourceaddr & 0xff;			// Obtain low byte of source address
	VDC_desth = (destaddr>>8) & 0xff;		// Obtain high byte of destination address
	VDC_destl = destaddr & 0xff;			// Obtain low byte of destination address
	VDC_tmp1 = ((length>>8) & 0xff) + 1;	// Obtain number of 256 byte pages to copy
	VDC_tmp2 = length & 0xff;				// Obtain length in last page to copy
	VDC_value = VDC_ReadRegister(24) | 128;	// Prepae value for copy bit 7 enabled of register 24

	VDC_MemCopy_core();
}

void VDC_HChar(unsigned char row, unsigned char col, unsigned char character, unsigned char length, unsigned char attribute)
{
	// Function to draw horizontal line with given character (draws from left to right)
	// Input: row and column of start position (left end of line), screencode of character to draw line with,
	//		  length in number of character positions, attribute color value

	unsigned int startaddress = VDC_RowColToAddress(row,col);
	VDC_addrh = (startaddress>>8) & 0xff;	// Obtain high byte of start address
	VDC_addrl = startaddress & 0xff;		// Obtain low byte of start address
	VDC_value = VDC_ReadRegister(24) & 128;	// Prepae value for copy bit 7 disabled of register 24
	VDC_tmp1 = character;					// Obtain character value
	VDC_tmp2 = length - 1;					// Obtain length value
	VDC_tmp3 = attribute;					// Ontain attribute value

	VDC_HChar_core();
}

void VDC_VChar(unsigned char row, unsigned char col, unsigned char character, unsigned char length, unsigned char attribute)
{
	// Function to draw vertical line with given character (draws from top to bottom)
	// Input: row and column of start position (top end of line), screencode of character to draw line with,
	//		  length in number of character positions, attribute color value

	unsigned int startaddress = VDC_RowColToAddress(row,col);
	VDC_addrh = (startaddress>>8) & 0xff;	// Obtain high byte of start address
	VDC_addrl = startaddress & 0xff;		// Obtain low byte of start address
	VDC_tmp1 = character;					// Obtain character value
	VDC_tmp2 = length;						// Obtain length value
	VDC_tmp3 = attribute;					// Ontain attribute value

	VDC_VChar_core();
}

void VDC_CopyMemToVDC(unsigned int vdcAddress, unsigned int memAddress, unsigned char memBank, unsigned int length)
{
	// Function to copy memory from VDC memory to standard memory
	// Input: Source VDC address, destination standard memory address and bank, number of bytes to copy

	length--;

	VDC_addrh = (memAddress>>8) & 0xff;					// Obtain high byte of source address
	VDC_addrl = memAddress & 0xff;						// Obtain low byte of source address
	VDC_desth = (vdcAddress>>8) & 0xff;					// Obtain high byte of destination address
	VDC_destl = vdcAddress & 0xff;						// Obtain low byte of destination address
	VDC_tmp1 = ((length>>8) & 0xff);					// Obtain number of 256 byte pages to copy
	VDC_tmp2 = length & 0xff;							// Obtain length in last page to copy
	VDC_tmp3 = (memBank==0)? MMU_BANK0:MMU_BANK1;		// Set proper MMU config based on bank 0 or 1

	VDC_CopyMemToVDC_core();
}

void VDC_CopyVDCToMem(unsigned int vdcAddress, unsigned int memAddress, unsigned char memBank, unsigned int length)
{
	// Function to copy memory from VDC memory to standard memory
	// Input: Source VDC address, destination standard memory address and bank, number of bytes to copy

	length--;

	VDC_addrh = (vdcAddress>>8) & 0xff;					// Obtain high byte of source VDC address
	VDC_addrl = vdcAddress & 0xff;						// Obtain low byte of source VDC address
	VDC_desth = (memAddress>>8) & 0xff;					// Obtain high byte of destination address
	VDC_destl = memAddress & 0xff;						// Obtain low byte of destination address
	VDC_tmp1 = ((length>>8) & 0xff);					// Obtain number of 256 byte pages to copy
	VDC_tmp2 = length & 0xff;							// Obtain length in last page to copy
	VDC_tmp3 = (memBank==0)? MMU_BANK0:MMU_BANK1;		// Set proper MMU config based on bank 0 or 1

	VDC_CopyVDCToMem_core();
}

void VDC_RedefineCharset(unsigned int source, unsigned char sourcebank, unsigned int dest, unsigned char lengthinchars)
{
	// Function to copy charset definition from normal memory to VDC
	// Input: Source normal memory adress where charset defiition resides,
	//		  Destination address in VDC memory,
	//		  Numbers of characters to redefine.
	// Takes charset definition of 8 bytes per character as input.
	// Destination address should be the location pointed as chararter definition address

	VDC_addrh = (source>>8) & 0xff;						// Obtain high byte of destination address
	VDC_addrl = source & 0xff;							// Obtain low byte of destination address
	VDC_tmp2 = (sourcebank==0)? MMU_BANK0:MMU_BANK1;	// Set proper MMU config based on bank 0 or 1
	VDC_desth = (dest>>8) & 0xff;						// Obtain high byte of destination address
	VDC_destl = dest & 0xff;							// Obtain low byte of destination address
	VDC_tmp1 = lengthinchars;							// Obtain number of characters to copy

	VDC_RedefineCharset_core();
}

void VDC_FillArea(unsigned char row, unsigned char col, unsigned char character, unsigned char length, unsigned char heigth, unsigned char attribute)
{
	// Function to draw area with given character (draws from topleft to bottomright)
	// Input: row and column of start position (topleft), screencode of character to draw line with,
	//		  length and heigth in number of character positions, attribute color value

	unsigned int startaddress = VDC_RowColToAddress(row,col);
	VDC_addrh = (startaddress>>8) & 0xff;	// Obtain high byte of start address
	VDC_addrl = startaddress & 0xff;		// Obtain low byte of start address
	VDC_value = VDC_ReadRegister(24) & 128;	// Prepae value for copy bit 7 disabled of register 24
	VDC_tmp1 = character;					// Obtain character value
	VDC_tmp2 = length - 1;					// Obtain length value
	VDC_tmp3 = attribute;					// Ontain attribute value
	VDC_tmp4 = heigth;						// Obtain number of lines

	VDC_FillArea_core();
}

void VDC_Init(void)
{
	unsigned int r = 0;

	// Set 8Kb shared memory size
	POKE(0xd506,0x06);						// Set proper bits in $D506 MMU register for 8Kb shared lower memory

	// Set 2 MHz mode
	POKE(0xd011,PEEK(0xd011)&(~(1<<4)));	// Disable the 5th bit of the SCROLY register to blank VIC screen
	POKE(0xd011,PEEK(0xd011)&(~(1<<7)));	// Disable the 8th bit of the SCROLY register to avoid accidentily setting raster interrupt to high
	set_c128_speed(SPEED_FAST);				// Set C128 speed to FAST (2 Mhz)
	  
    // Init screen
	videomode(VIDEOMODE_80COL);			// Set 80 column mode
    bordercolor(COLOR_BLACK);
    bgcolor(COLOR_BLACK);
    textcolor(COLOR_YELLOW);
    clrscr();

	// Load $1300 area machine code
	r = cbm_open(2, getcurrentdevice(), 2, "vdcse.maco");

	if(r == 0)
	{
		r = cbm_read(2, MACOSTART, MACOSIZE);

		cbm_close(2);

		if(r == 0 )
		{
			printf("Machine code file reading error.", r);
			exit(1);
		}
	}
	else{
		printf("Machine code file opening error.\n");
		exit(1);
	}
}

void VDC_Exit(void)
{
	set_c128_speed(SPEED_SLOW);         	// Switch back to 1Mhz mode for safe exit
	POKE(0xd011,PEEK(0xd011)|(1<<4));		// Enable the 5th bit of the SCROLY register to blank VIC screen
	POKE(0xd011,PEEK(0xd011)&(~(1<<7)));	// Disable the 8th bit of the SCROLY register to avoid accidentily setting raster interrupt to high
	POKE(0xd506,0x04);						// Set proper bits in $D506 MMU register for default shared memory
	clrscr();
}

unsigned char VDC_PetsciiToScreenCode(unsigned char p)
{
	/* Convert Petscii values to screen code values */
	if(p <32)	p = p + 128;
	else if (p > 63  && p < 96 ) p = p - 64;
	else if (p > 95  && p < 128) p = p - 32;
	else if (p > 127 && p < 160) p = p + 64;
	else if (p > 161 && p < 192) p = p - 64;
	else if (p > 191 && p < 254) p = p - 128;
	else if (p == 255) p = 94;
	
	return p;
}

unsigned char VDC_PetsciiToScreenCodeRvs(unsigned char p)
{
	/* Convert Petscii values to screen code values */
	if(p <32)	p = p + 128;
	else if (p > 31  && p < 64 ) p = p + 128;
	else if (p > 63  && p < 96 ) p = p + 64;
	else if (p > 95  && p < 128) p = p - 32;
	else if (p > 127 && p < 160) p = p + 64;
	else if (p > 161 && p < 192) p = p - 64;
	else if (p > 191 && p < 224) p = p - 128;
	else if (p > 223 && p < 255) p = p - 128;
	else if (p == 255) p = 94;

	return p;
}

unsigned int VDC_RowColToAddress(unsigned char row, unsigned char col)
{
	/* Function returns a VDC memory address for a given row and column */

	unsigned int addr;
	addr = row * 80 + col;

	if (addr < 2000)
		return addr;
	else
		return -1;
}

void VDC_BackColor(unsigned char color)
{
	// Function to set VDC Background color with color

	/* Reading from register 26 */
	unsigned char regval = VDC_ReadRegister(26);

	/* Setting the background color bits*/
	regval = (regval & 240) + color;

	/* Writing to register 26 */
	VDC_WriteRegister(26,regval);
}

unsigned char VDC_CursorAt(unsigned char row, unsigned char col)
{
	// Function to set cursor at provides row and column

	unsigned int address;

	if(row > 24 || col > 79)
		return -1;

	address = VDC_RowColToAddress(row, col);
	VDC_WriteRegister(14,(address>>8) & 0xff);
	VDC_WriteRegister(15,address & 0xff);

	return 0;
}

unsigned char VDC_PrintAt(unsigned char row, unsigned char col, char *text, unsigned char attribute)
{
	// Function to print string at specified row and column start position, in reverse or not

	unsigned char x;
	unsigned int address;

	address = VDC_RowColToAddress(row, col);

	if (address != -1)
	{
		x = 0;
		while(text[x] != 0)
		{
			VDC_Poke(address, VDC_PetsciiToScreenCode(text[x]));
			VDC_Poke(address+0x800, attribute);
			address++;
			x++;
		}
		return x;
	}
	else
		return -1;
}

void VDC_LoadCharset(char* filename, unsigned int source, unsigned char sourcebank, unsigned char stdoralt)
{
	// Function to load charset definition from disk and redefine VDC charset using that
	// Input: filename of char set definition file, destination address and bank in normal memory,
	//		  flag for standard charset (0) or alternate charset (1)

	unsigned int length;
	unsigned int baseaddress = (stdoralt == 0)? 0x2000:0x3000;

	// Set device ID
	cbm_k_setlfs(0, getcurrentdevice(), 0);

	// Set filename
	cbm_k_setnam(filename);

	// Set bank
	SetLoadSaveBank(sourcebank);
	
	// Load from file to memory
	length = cbm_k_load(0,source);

	// Redefine VDC charset
	if(length>source)
	{
		VDC_RedefineCharset(source, sourcebank, baseaddress, ((length-source)/8)-1);
	}

	// Restore I/O bank to 0
	SetLoadSaveBank(0);
}

void VDC_LoadScreen(char* filename, unsigned int source, unsigned char sourcebank, unsigned char show)
{
	// Function to load a screen from disk and store to memory, if wanted also copy to VDC
	// Input: filename, memory address, memory bank, flag if copy to VDC is wanted (0=no, 1=yes)

	unsigned int length;

	// Set device ID
	cbm_k_setlfs(0, getcurrentdevice(), 0);

	// Set filename
	cbm_k_setnam(filename);

	// Set bank
	SetLoadSaveBank(sourcebank);
	
	// Load from file to memory
	length = cbm_k_load(0,source);

	// Copy screen to VDC
	if(length>source  && show ==1)
	{
		VDC_CopyMemToVDC(0,source,sourcebank, length-source+1);
	}

	// Restore I/O bank to 0
	SetLoadSaveBank(0);
}

unsigned char VDC_SaveScreen(char* filename, unsigned int bufferaddress, unsigned char bufferbank)
{
	// Function to save a screen to disk via buffer memory
	// Input: filename, buffer memory address, buffer memory bank
	// Output: error code

	unsigned char error;

	// Copy screen from VDC to buffer memory
	VDC_CopyVDCToMem(0,bufferaddress,bufferbank,4096);

	// Set device ID
	cbm_k_setlfs(0, getcurrentdevice(), 0);

	// Set filename
	cbm_k_setnam(filename);

	// Set bank
	SetLoadSaveBank(bufferbank);
	
	// Load from file to memory
	error = cbm_k_save(bufferaddress,bufferaddress+4096);

	// Restore I/O bank to 0
	SetLoadSaveBank(0);

	return error;
} 

unsigned char VDC_Attribute(unsigned char textcolor, unsigned char blink, unsigned char underline, unsigned char reverse, unsigned char alternate)
{
	// Function to calculate attribute code from color and other attribute bits
	// Input: Color code 0-15 and flags for blink, underline, reverse and alternate charset

	return textcolor + (blink*16) + (underline*32) + (reverse*64) + (alternate*128);
}

void VDC_Plot(unsigned char row, unsigned char col, unsigned char screencode, unsigned char attribute)
{
	// Function to plot a screencodes at VDC screem
	// Input: row and column, screencode to plot, attribute code

	unsigned int address = VDC_RowColToAddress(row,col);
	VDC_Poke(address,screencode);
	VDC_Poke(address+0x0800,attribute);
}

void VDC_PlotString(unsigned char row, unsigned char col, char* plotstring, unsigned char length, unsigned char attribute)
{
	// Function to plot a string of screencodes at VDC screem, no trailing zero needed
	// Input: row and column, string to plot, length to plot, attribute code
	
	unsigned char x;

	for(x=0;x<length;x++)
	{
		VDC_Plot(row,col++,plotstring[x],attribute);
	}
}

void SetLoadSaveBank(unsigned char bank)
{
	// Function to set bank for I/O operations
	// Input: banknumber

	VDC_tmp1 = bank;
	SetLoadSaveBank_core();
}
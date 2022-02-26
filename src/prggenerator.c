//Includes
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
#include "prggenerator.h"

// Global variables
char DOSstatus[40];
unsigned char bootdevice;
unsigned char targetdevice;
char filename[21];
char filedest[21];
char buffer[81];
char version[22];
unsigned int screenwidth;
unsigned int screenheight;
unsigned char screenbackground;
unsigned char charsetchanged[2];
unsigned char zp1,zp2,base_low,base_high,poke_value,poke_bank;

// Generic routines
unsigned char dosCommand(const unsigned char lfn, const unsigned char drive, const unsigned char sec_addr, const char *cmd)
{
    // Send DOS command
    // based on version DraCopy 1.0e, then modified.
    // Created 2009 by Sascha Bader.

    int res;
    if (cbm_open(lfn, drive, sec_addr, cmd) != 0)
    {
        return _oserror;
    }

    if (lfn != 15)
    {
        if (cbm_open(15, drive, 15, "") != 0)
        {
            cbm_close(lfn);
            return _oserror;
        }
    }

    DOSstatus[0] = 0;
    res = cbm_read(15, DOSstatus, sizeof(DOSstatus));

    if(lfn != 15)
    {
      cbm_close(15);
    }
    cbm_close(lfn);

    if (res < 1)
    {
        return _oserror;
    }

    return (DOSstatus[0] - 48) * 10 + DOSstatus[1] - 48;
}

unsigned int cmd(const unsigned char device, const char *cmd)
{
    // Prepare DOS command
    // based on version DraCopy 1.0e, then modified.
    // Created 2009 by Sascha Bader.
    
    return dosCommand(15, device, 15, cmd);
}

int textInput(unsigned char xpos, unsigned char ypos, char* str, unsigned char size)
{

    /**
    * input/modify a string.
    * based on version DraCopy 1.0e, then modified.
    * Created 2009 by Sascha Bader.
    * @param[in] xpos screen x where input starts.
    * @param[in] ypos screen y where input starts.
    * @param[in,out] str string that is edited, it can have content and must have at least @p size + 1 bytes. Maximum size     if 255 bytes.
    * @param[in] size maximum length of @p str in bytes.
    * @return -1 if input was aborted.
    * @return >= 0 length of edited string @p str.
    */

    register unsigned char c;
    register unsigned char idx = strlen(str);

    cursor(1);
    cputsxy(xpos,ypos,str);
    
    while(1)
    {
        c = cgetc();
        switch (c)
        {
        case CH_ESC:
        case CH_STOP:
            cursor(0);
            return -1;

        case CH_ENTER:
            idx = strlen(str);
            str[idx] = 0;
            cursor(0);
            return idx;

        case CH_DEL:
            if (idx)
            {
                --idx;
                cputcxy(xpos + idx, ypos, ' ');
                for(c = idx; 1; ++c)
                {
                    unsigned char b = str[c+1];
                    str[c] = b;
                    cputcxy(xpos + c, ypos, b ? b : ' ');
                    if (b == 0) { break; }
                }
                gotoxy(xpos+idx,ypos);
            }
            break;

        case CH_INS:
            c = strlen(str);
            if (c < size && c > 0 && idx < c)
            {
                ++c;
                while(c >= idx)
                {
                    str[c+1] = str[c];
                    if (c == 0) { break; }
                    --c;
                }
                str[idx] = ' ';
                cputsxy(xpos, ypos, str);
                gotoxy(xpos+idx,ypos);
            }
            break;

        case CH_CURS_LEFT:
            if (idx)
            {
                --idx;
                gotoxy(xpos+idx,ypos);
            }
            break;

        case CH_CURS_RIGHT:
            if (idx < strlen(str) && idx < size)
            {
                ++idx;
                gotoxy(xpos+idx,ypos);
            }
            break;

        default:
            if (isprint(c) && idx < size)
            {
                unsigned char flag = (str[idx] == 0);
                str[idx] = c;
                cputc(c);
                ++idx;
                gotoxy(xpos+idx,ypos);
                if (flag) { str[idx+1] = 0; }
                break;
            }
            break;
        }
    }
    return 0;
}

unsigned int load_save_data(char* filename, unsigned char deviceid, unsigned int address, unsigned int size, unsigned char bank, unsigned char saveflag)
{
    // Function to load or save data.
    // Input: filename, device id, destination/source address, bank (0 or 1), saveflag (0=load, 1=save)
    
    unsigned int error;

    // Set device ID
	cbm_k_setlfs(0, deviceid, 0);

    // Set filename
	cbm_k_setnam(filename);

    if(bank)
    {
        // Set bank to 1
        __asm__ (
	    	"\tlda\t#1\n"
	    	"\tldx\t#0\n"
	    	"\tjsr\t$ff68\n"
	    	);
    }
    

    if(saveflag)
    {
        // Save data
        error = cbm_k_save(address, size);
    }
    else
    {
        // Load data
        error = cbm_k_load(0, address);
    }

    // Set load/save bank back to 0
    __asm__ (
		"\tlda\t#0\n"
		"\tldx\t#0\n"
		"\tjsr\t$ff68\n"
		);

    return error;
}

void bank_poke(unsigned int address, unsigned char value, unsigned char bank)
{
    base_high = (address>>8) & 0xff;
    base_low = address & 0xff;
    poke_value = value;
    poke_bank = bank;

    asm("lda $fb");                     // Load contents of ZP $FB
    asm("sta %v",zp1);                  // Safeguard in zp1 variable
    asm("lda $fc");                     // Load contents of ZP $FC
    asm("sta %v",zp2);                  // Safeguard in zp2 variable
    asm("lda %v",base_low);             // Load low byte of base address
    asm("sta $fb");                     // Store at $fb ZP address
    asm("lda %v",base_high);            // Load high byte of base address
    asm("sta $fc");                     // Store at $fb ZP address
    asm("lda #$fb");                    // Load pointer to $FB
    asm("sta $2B9");                    // Store at STAVEC ($2B9)
    asm("lda %v",poke_value);           // Set A to value to poke
    asm("ldy #$00");                    // Set Y index to 0
    asm("ldx %v", poke_bank);           // Set X for bank number
    asm("jsr $FF77");                   // Call to INDSTA kernal routine
    asm("lda %v",zp1);                  // Load zp1 variable
    asm("sta $fb");                     // Restore $fb ZP address
    asm("lda %v",zp2);                  // Load zp1 variable
    asm("sta $fc");                     // Restore $fc ZP address
}

void main()
{
    unsigned int r = 0;
    unsigned char x,newtargetdevice,error,key;
    unsigned char valid = 0;
    unsigned int length;
    unsigned int address;
    unsigned char projbuffer[22];
    char* ptrend;

    // Obtain device number the application was started from
    bootdevice = getcurrentdevice();
    targetdevice = bootdevice;  

    // Set version number in string variable
    sprintf(version,
            "v%2i.%2i - %c%c%c%c%c%c%c%c-%c%c%c%c",
            VERSION_MAJOR, VERSION_MINOR,
            BUILD_YEAR_CH0, BUILD_YEAR_CH1, BUILD_YEAR_CH2, BUILD_YEAR_CH3, BUILD_MONTH_CH0, BUILD_MONTH_CH1, BUILD_DAY_CH0, BUILD_DAY_CH1,BUILD_HOUR_CH0, BUILD_HOUR_CH1, BUILD_MIN_CH0, BUILD_MIN_CH1);

    set_c128_speed(SPEED_FAST);			// Set C128 speed to FAST (2 Mhz)
    videomode(VIDEOMODE_80COL);			// Set 80 column mode
    bordercolor(COLOR_BLACK);
    bgcolor(COLOR_BLACK);
    textcolor(COLOR_YELLOW);
    clrscr();

    cputsxy(0,0,"VDCSE - PRG generator\n\r");
    cprintf("Written by Xander Mol, version %s",version);

    cputsxy(0,3,"Choose drive ID for project to load:");
    do
    {
        sprintf(buffer,"%u",targetdevice);
        textInput(0,4,buffer,2);
        newtargetdevice = (unsigned char)strtol(buffer,&ptrend,10);
        if(newtargetdevice > 7 && newtargetdevice<31)
        {
            valid = 1;
            targetdevice=newtargetdevice;
        }
        else{
            cputsxy(0,4,"Invalid ID. Enter valid one.");
        }
    } while (valid=0);
    cputsxy(0,5,"Choose filename of project to load (without .proj): ");
    textInput(0,6,filename,15);

    cputsxy(0,7,"Choose filename of generated program:");
    textInput(0,8,filedest,20);

    // Check if outtput file already exists
    sprintf(buffer,"r0:%s=%s",filedest,filedest);
    error = cmd(targetdevice,buffer);

    if (error == 63)
    {
        cputsxy(0,9,"Output file exists. Are you sure? Y/N ");
        do
        {
            key = cgetc();
        } while (key!='y' && key!='n');
        cputc(key);
        if(key=='y')
        {
            // Scratch old files
            sprintf(buffer,"s:%s",filedest);
            cmd(targetdevice,buffer);
        }
        else
        {
            exit(1);
        }
    }
    cbm_close(2);

    cprintf("\n\n\rLoading project meta data.\n\r");

    // Load project variables
    sprintf(buffer,"%s.proj",filename);
    length = load_save_data(buffer,targetdevice,(unsigned int)projbuffer,22,0,0);
    if(length<=(unsigned int)projbuffer)
    { 
        cprintf("Read error on reading project file.\n\r");
        exit(1);
    }
    charsetchanged[0]       = projbuffer[ 0];
    charsetchanged[1]       = projbuffer[ 1];
    screenwidth             = projbuffer[ 4]*256+projbuffer[ 5];
    screenheight            = projbuffer[ 6]*256+projbuffer [7];
    screenbackground        = projbuffer[10];

    if(screenwidth!=80 || screenheight!=25)
    {
        cprintf("Only screen dimension of 80x25 supported.\n\r");
        exit(1);
    }

    cprintf("\nGenerating program file.\n\r");
    
    address=BASEADDRESS;

    cprintf("Loading assembly code at %4X.\n\r",address);
    // Load loader program
    length = load_save_data("vdcse2prg.ass",bootdevice,address,ASS_SIZE,1,0);
    if(length<=BASEADDRESS)
    {
        cprintf("Load error on loading assembly code.");
        exit(1);
    }
    address+=ASS_SIZE;

    // Poke version string
    cprintf("Poking version string.\n\r");
    for(x-0;x<22;x++)
    {
        bank_poke(BASEADDRESS+VERSIONADDRESS+x,version[x],1);
    }

    // Load screen
    cprintf("Loading screen data at %4X.\n\r",address);
    bank_poke(BGCOLORADDRESS,screenbackground,1);               // Set background color
    sprintf(buffer,"%s.scrn",filename);
    length = load_save_data(buffer,targetdevice,address,SCREEN_SIZE,1,0);
    if(length<=address)
    {
        cprintf("Load error on loading screen data.");
        exit(1);
    }
    address+=SCREEN_SIZE;

    // Load standard charset if defined
    if(charsetchanged[0])
    {
        cprintf("Loading standard charset at %4X.\n\r",address);
        bank_poke(CHARSTDADDRESS,address&0xff,1);               // Set low byte charset address
        bank_poke(CHARSTDADDRESS+1,(address>>8)&0xff,1);        // Set high byte charset address
        sprintf(buffer,"%s.chrs",filename);
        length = load_save_data(buffer,targetdevice,address,CHAR_SIZE,1,0);
        if(length<=address)
        {
            cprintf("Load error on loading standard charset data.");
            exit(1);
        }
        address+=CHAR_SIZE;
    }

    // Load alternate charset if defined
    if(charsetchanged[1])
    {
        cprintf("Loading alternate charset at %4X.\n\r",address);
        bank_poke(CHARALTADDRESS,address&0xff,1);               // Set low byte charset address
        bank_poke(CHARALTADDRESS+1,(address>>8)&0xff,1);        // Set high byte charset address
        sprintf(buffer,"%s.chra",filename);
        length = load_save_data(buffer,targetdevice,address,CHAR_SIZE,1,0);
        if(length<=address)
        {
            cprintf("Load error on loading alternate charset data.");
            exit(1);
        }
        address+=CHAR_SIZE;
    }

    // Save complete generated program
    cprintf("Saving generated program from %4X to %4X.\n\r",BASEADDRESS,address);
    if(load_save_data(filedest,targetdevice,BASEADDRESS,address,1,1))
    {
        cprintf("Save error on writing generated program.");
        exit(1);
    }

    cprintf("\nFinished!\n\r");
    cprintf("Created ");

    set_c128_speed(SPEED_SLOW);         // Switch back to 1Mhz mode for safe exit

}
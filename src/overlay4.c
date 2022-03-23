/*
VDC Screen Editor
Screen editor for the C128 80 column mode
Written in 2021 by Xander Mol

https://github.com/xahmol/VDCScreenEdit
https://www.idreamtin8bits.com/

Code and resources from others used:

-   CC65 cross compiler:
    https://cc65.github.io/

-   C128 Programmers Reference Guide: For the basic VDC register routines and VDC code inspiration
    http://www.zimmers.net/anonftp/pub/cbm/manuals/c128/C128_Programmers_Reference_Guide.pdf

-   Scott Hutter - VDC Core functions inspiration:
    https://github.com/Commodore64128/vdc_gui/blob/master/src/vdc_core.c
    (used as starting point, but changed to inline assembler for core functions, added VDC wait statements and expanded)

-   Francesco Sblendorio - Screen Utility: used for inspiration:
    https://github.com/xlar54/ultimateii-dos-lib/blob/master/src/samples/screen_utility.c

-   DevDef: Commodore 128 Assembly - Part 3: The 80-column (8563) chip
    https://devdef.blogspot.com/2018/03/commodore-128-assembly-part-3-80-column.html

-   Tips and Tricks for C128: VDC
    http://commodore128.mirkosoft.sk/vdc.html

-   6502.org: Practical Memory Move Routines: Starting point for memory move routines
    http://6502.org/source/general/memory_move.html

-   DraBrowse source code for DOS Command and text input routine
    DraBrowse (db*) is a simple file browser.
    Originally created 2009 by Sascha Bader.
    Used version adapted by Dirk Jagdmann (doj)
    https://github.com/doj/dracopy

-   Bart van Leeuwen: For inspiration and advice while coding.
    Also for providing the excellent Device Manager ROM to make testing on real hardware very easy

-   jab / Artline Designs (Jaakko Luoto) for inspiration for Palette mode and PETSCII visual mode

-   Original windowing system code on Commodore 128 by unknown author.
   
-   Tested using real hardware (C128D and C128DCR) plus VICE.

The code can be used freely as long as you retain
a notice describing original source and author.

THE PROGRAMS ARE DISTRIBUTED IN THE HOPE THAT THEY WILL BE USEFUL,
BUT WITHOUT ANY WARRANTY. USE THEM AT YOUR OWN RISK!
*/

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
#include "vdc_core.h"
#include "defines.h"
#include "main.h"

#pragma code-name ("OVERLAY4");
#pragma rodata-name ("OVERLAY4");

void chareditor()
{
    unsigned char x,y,char_altorstd,char_screencode,key;
    unsigned char xpos=0;
    unsigned char ypos=0;
    unsigned char char_present[8];
    unsigned char char_copy[8];
    unsigned char char_undo[8];
    unsigned char char_buffer[8];
    unsigned int char_address;
    unsigned char charchanged = 0;
    unsigned char altchanged = 0;
    char* ptrend;

    char_altorstd = plotaltchar;
    char_screencode = plotscreencode;
    char_address = charaddress(char_screencode, char_altorstd,0);
    charsetchanged[plotaltchar]=1;
    strcpy(programmode,"Charedit");

    // Load system charset if needed in charset not edited
    if(plotaltchar==0 && charsetchanged[1] ==1)
    {
        VDC_RedefineCharset(CHARSETSYSTEM,1,VDCCHARALT,255);
    }
    if(plotaltchar==1 && charsetchanged[0] ==1)
    {
        VDC_RedefineCharset(CHARSETSYSTEM,1,VDCCHARSTD,255);
    }

    for(y=0;y<8;y++)
    {
        char_present[y]=VDC_Peek(char_address+y);
        char_undo[y]=char_present[y];
    }

    showchareditfield(char_altorstd);
    showchareditgrid(char_screencode, char_altorstd);
    textcolor(vdctoconiocol[mc_menupopup & 0x0f]);
    gotoxy(xpos+71,ypos+3);
    do
    {
        if(showbar) { printstatusbar(); }
        key = cgetc();

        switch (key)
        {
        // Movement
        case CH_CURS_RIGHT:
            if(xpos<7) {xpos++; }
            gotoxy(xpos+71,ypos+3);
            break;
        
        case CH_CURS_LEFT:
            if(xpos>0) {xpos--; }
            gotoxy(xpos+71,ypos+3);
            break;
        
        case CH_CURS_DOWN:
            if(ypos<7) {ypos++; }
            gotoxy(xpos+71,ypos+3);
            break;

        case CH_CURS_UP:
            if(ypos>0) {ypos--; }
            gotoxy(xpos+71,ypos+3);
            break;

        // Next or previous character
        case '+':
        case '-':
            if(key=='+')
            {
                char_screencode++;
            }
            else
            {
                char_screencode--;
            }
            charchanged=1;
            break;

        // Toggle bit
        case CH_SPACE:
            char_present[ypos] ^= 1 << (7-xpos);
            VDC_Poke(char_address+ypos,char_present[ypos]);
            POKEB(charaddress(char_screencode,char_altorstd,1)+ypos,1,char_present[ypos]);
            showchareditgrid(char_screencode, char_altorstd);
            break;

        // Inverse
        case 'i':
            for(y=0;y<8;y++)
            {
                char_present[y] ^= 0xff;
                VDC_Poke(char_address+y,char_present[y]);
                POKEB(charaddress(char_screencode,char_altorstd,1)+y,1,char_present[y]);
            }
            showchareditgrid(char_screencode, char_altorstd);
            break;

        // Delete
        case CH_DEL:
            for(y=0;y<8;y++)
            {
                char_present[y] = 0;
                VDC_Poke(char_address+y,char_present[y]);
                POKEB(charaddress(char_screencode,char_altorstd,1)+y,1,char_present[y]);
            }
            showchareditgrid(char_screencode, char_altorstd);
            break;

        // Undo
        case 'z':
            for(y=0;y<8;y++)
            {
                char_present[y] = char_undo[y];
                VDC_Poke(char_address+y,char_present[y]);
                POKEB(charaddress(char_screencode,char_altorstd,1)+y,1,char_present[y]);
            }
            showchareditgrid(char_screencode, char_altorstd);
            break;

        // Restore from system font
        case 's':
            for(y=0;y<8;y++)
            {
                char_present[y] = PEEKB(CHARSETSYSTEM+y+(char_screencode*8),1);
                VDC_Poke(char_address+y,char_present[y]);
                POKEB(charaddress(char_screencode,char_altorstd,1)+y,1,char_present[y]);
            }
            showchareditgrid(char_screencode, char_altorstd);
            break;

        // Copy
        case 'c':
            for(y=0;y<8;y++)
            {
                char_copy[y] = char_present[y];
            }
            break;

        // Paste
        case 'v':
            for(y=0;y<8;y++)
            {
                char_present[y] = char_copy[y];
                VDC_Poke(char_address+y,char_present[y]);
                POKEB(charaddress(char_screencode,char_altorstd,1)+y,1,char_present[y]);
            }
            showchareditgrid(char_screencode, char_altorstd);
            break;

        // Switch charset
        case 'a':
            char_altorstd = (char_altorstd==0)? 1:0;
            altchanged=1;
            break;

        // Mirror y axis
        case 'y':
            for(y=0;y<8;y++)
            {
                VDC_Poke(char_address+y,char_present[7-y]);
                POKEB(charaddress(char_screencode,char_altorstd,1)+y,1,char_present[7-y]);
            }
            for(y=0;y<8;y++)
            {
                char_present[y]=VDC_Peek(char_address+y);
            }
            showchareditgrid(char_screencode, char_altorstd);
            break;

        // Mirror x axis
        case 'x':
            for(y=0;y<8;y++)
            {
                char_present[y] = (char_present[y] & 0xF0) >> 4 | (char_present[y] & 0x0F) << 4;
                char_present[y] = (char_present[y] & 0xCC) >> 2 | (char_present[y] & 0x33) << 2;
                char_present[y] = (char_present[y] & 0xAA) >> 1 | (char_present[y] & 0x55) << 1;
                VDC_Poke(char_address+y,char_present[y]);
                POKEB(charaddress(char_screencode,char_altorstd,1)+y,1,char_present[y]);
            }
            showchareditgrid(char_screencode, char_altorstd);
            break;

        // Rotate clockwise
        case 'o':
            for(y=0;y<8;y++)
            {
                for(x=0;x<8;x++)
                {
                    if(char_present[y] & (1<<(7-x)))
                    {
                        char_buffer[x] |= (1<<y);
                    }
                    else
                    {
                        char_buffer[x] &= ~(1<<y);
                    }
                }
            }
            for(y=0;y<8;y++)
            {
                char_present[y]=char_buffer[y];
                VDC_Poke(char_address+y,char_present[y]);
                POKEB(charaddress(char_screencode,char_altorstd,1)+y,1,char_present[y]);
            }
            showchareditgrid(char_screencode, char_altorstd);
            break;

        // Scroll up
        case 'u':
            for(y=1;y<8;y++)
            {
                char_buffer[y-1]=char_present[y];
            }
            char_buffer[7]=char_present[0];
            for(y=0;y<8;y++)
            {
                char_present[y]=char_buffer[y];
                VDC_Poke(char_address+y,char_present[y]);
                POKEB(charaddress(char_screencode,char_altorstd,1)+y,1,char_present[y]);
            }
            showchareditgrid(char_screencode, char_altorstd);
            break;

        // Scroll down
        case 'd':
            for(y=1;y<8;y++)
            {
                char_buffer[y]=char_present[y-1];
            }
            char_buffer[0]=char_present[7];
            for(y=0;y<8;y++)
            {
                char_present[y]=char_buffer[y];
                VDC_Poke(char_address+y,char_present[y]);
                POKEB(charaddress(char_screencode,char_altorstd,1)+y,1,char_present[y]);
            }
            showchareditgrid(char_screencode, char_altorstd);
            break;

        // Scroll right
        case 'r':
            for(y=0;y<8;y++)
            {
                char_buffer[y]=char_present[y]>>1;
                if(char_present[y]&0x01) { char_buffer[y]+=0x80; }
            }
            for(y=0;y<8;y++)
            {
                char_present[y]=char_buffer[y];
                VDC_Poke(char_address+y,char_present[y]);
                POKEB(charaddress(char_screencode,char_altorstd,1)+y,1,char_present[y]);
            }
            showchareditgrid(char_screencode, char_altorstd);
            break;
        
        // Scroll left
        case 'l':
            for(y=0;y<8;y++)
            {
                char_buffer[y]=char_present[y]<<1;
                if(char_present[y]&0x80) { char_buffer[y]+=0x01; }
            }
            for(y=0;y<8;y++)
            {
                char_present[y]=char_buffer[y];
                VDC_Poke(char_address+y,char_present[y]);
                POKEB(charaddress(char_screencode,char_altorstd,1)+y,1,char_present[y]);
            }
            showchareditgrid(char_screencode, char_altorstd);
            break;

        // Hex edit
        case 'h':
            sprintf(buffer,"%2X",char_present[ypos]);
            textInput(68,ypos+3,buffer,2);
            char_present[ypos] = (unsigned char)strtol(buffer,&ptrend,16);
            gotoxy(71+xpos,3+ypos);
            cursor(1);
            VDC_Poke(char_address+ypos,char_present[ypos]);
            POKEB(charaddress(char_screencode,char_altorstd,1)+ypos,1,char_present[ypos]);
            showchareditgrid(char_screencode, char_altorstd);
            break;

        // Toggle statusbar
        case CH_F6:
            togglestatusbar();
            break;

        // Help screen
        case CH_F8:
            windowrestore(0);
            helpscreen_load(2);
            if(plotaltchar==0 && charsetchanged[1] ==1)
            {
                VDC_RedefineCharset(CHARSETALTERNATE,1,VDCCHARALT,255);
            }
            showchareditfield(char_altorstd);
            showchareditgrid(char_screencode,char_altorstd);
            break;

        default:
            // 0-9: Favourites select
            if(key>47 && key<58)
            {
                char_screencode = favourites[key-48][0];
                char_altorstd = favourites[key-48][1];
                charchanged=1;
                altchanged=1;
            }
            // Shift 1-9 or *: Store present character in favourites slot
            if(key>32 && key<43)
            {
                favourites[key-33][0] = char_screencode;
                favourites[key-33][1] = char_altorstd;
            }
            break;
        }

        if(charchanged || altchanged)
        {
            if(charchanged)
            {
                charchanged=0;
                char_address = charaddress(char_screencode,char_altorstd,0);
                for(y=0;y<8;y++)
                {
                    char_present[y]=VDC_Peek(char_address+y);
                    char_undo[y]=char_present[y];
                }
            }
            if(altchanged)
            {
                altchanged=0;
                if(char_altorstd==0)
                {
                    VDC_RedefineCharset(CHARSETNORMAL,1,VDCCHARSTD,255);
                    VDC_RedefineCharset(CHARSETSYSTEM,1,VDCCHARALT,255);
                }
                else
                {
                    VDC_RedefineCharset(CHARSETALTERNATE,1,VDCCHARALT,255);
                    VDC_RedefineCharset(CHARSETSYSTEM,1,VDCCHARSTD,255);
                }
                charsetchanged[char_altorstd]=1;
                windowrestore(0);
                showchareditfield(char_altorstd);
            }
            showchareditgrid(char_screencode, char_altorstd);
        }
    } while (key != CH_ESC && key != CH_STOP);

    windowrestore(0);

    if(char_altorstd==0)
    {
        VDC_RedefineCharset(CHARSETALTERNATE,1,VDCCHARALT,255);
    }
    else
    {
        VDC_RedefineCharset(CHARSETNORMAL,1,VDCCHARSTD,255);
    }

    plotscreencode = char_screencode;
    plotaltchar = char_altorstd;
    textcolor(vdctoconiocol[plotcolor]);
    gotoxy(screen_col,screen_row);
    VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
    strcpy(programmode,"Main");
}
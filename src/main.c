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
#include "overlay1.h"
#include "overlay2.h"
#include "overlay3.h"
#include "overlay4.h"

// Overlay data
struct OverlayStruct overlaydata[4];
unsigned char overlay_active = 0;

//Window data
struct WindowStruct Window[9];
unsigned int windowaddress = WINDOWBASEADDRESS;
unsigned char windownumber = 0;

//Menu data
unsigned char menubaroptions = 4;
unsigned char pulldownmenunumber = 8;
char menubartitles[4][12] = {"Screen","File","Charset","Information"};
unsigned char menubarcoords[4] = {1,8,13,21};
unsigned char pulldownmenuoptions[5] = {5,4,4,2,2};
char pulldownmenutitles[5][5][16] = {
    {"Width:      80 ",
     "Height:     25 ",
     "Background:  0 ",
     "Clear          ",
     "Fill           "},
    {"Save screen    ",
     "Load screen    ",
     "Save project   ",
     "Load project   "},
    {"Load standard  ",
     "Load alternate ",
     "Save standard  ",
     "Save alternate "},
    {"Version/credits",
     "Exit program   "},
    {"Yes",\
     "No "}
};

// Undo data
unsigned char vdcmemory;
unsigned char undoenabled = 0;
unsigned int undoaddress;
unsigned char undonumber;
unsigned char undo_undopossible;
unsigned char undo_redopossible;
struct UndoStruct Undo[41];

// Menucolors
unsigned char mc_mb_normal = VDC_LGREEN + VDC_A_REVERSE + VDC_A_ALTCHAR;
unsigned char mc_mb_select = VDC_WHITE + VDC_A_REVERSE + VDC_A_ALTCHAR;
unsigned char mc_pd_normal = VDC_DCYAN + VDC_A_REVERSE + VDC_A_ALTCHAR;
unsigned char mc_pd_select = VDC_LYELLOW + VDC_A_REVERSE + VDC_A_ALTCHAR;
unsigned char mc_menupopup = VDC_WHITE + VDC_A_REVERSE + VDC_A_ALTCHAR;

// Global variables
unsigned char bootdevice;
char DOSstatus[40];
unsigned char charsetchanged[2];
unsigned char appexit;
unsigned char targetdevice;
char filename[21];
char programmode[11];
unsigned char showbar;

unsigned char screen_col;
unsigned char screen_row;
unsigned int xoffset;
unsigned int yoffset;
unsigned int screenwidth;
unsigned int screenheight;
unsigned int screentotal;
unsigned char screenbackground;
unsigned char plotscreencode;
unsigned char plotcolor;
unsigned char plotreverse;
unsigned char plotunderline;
unsigned char plotblink;
unsigned char plotaltchar;
unsigned int select_startx, select_starty, select_endx, select_endy, select_width, select_height, select_accept;
unsigned char rowsel = 0;
unsigned char colsel = 0;
unsigned char palettechar;
unsigned char visualmap = 0;
unsigned char favourites[10][2];

char buffer[81];
char version[22];

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

    textcolor(vdctoconiocol[mc_menupopup & 0x0f]);
    cursor(1);
    VDC_PrintAt(ypos,xpos,str,mc_menupopup);
    gotoxy(xpos+idx,ypos);
    
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
                VDC_Plot(ypos, xpos + idx,CH_SPACE,mc_menupopup);
                for(c = idx; 1; ++c)
                {
                    unsigned char b = str[c+1];
                    str[c] = b;
                    VDC_Plot(ypos, xpos+c, b? VDC_PetsciiToScreenCode(b) : CH_SPACE, mc_menupopup);
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
                VDC_PrintAt(ypos,xpos,str,mc_menupopup);
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
                VDC_Plot(ypos, xpos+idx, VDC_PetsciiToScreenCode(c), mc_menupopup);
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

/* General screen functions */
void cspaces(unsigned char number)
{
    /* Function to print specified number of spaces, cursor set by conio.h functions */

    unsigned char x;

    for(x=0;x<number;x++) { cputc(CH_SPACE); }
}

void printcentered(char* text, unsigned char xpos, unsigned char ypos, unsigned char width)
{
    /* Function to print a text centered
       Input:
       - Text:  Text to be printed
       - Color: Color for text to be printed
       - Width: Width of window to align to    */

    gotoxy(xpos,ypos);

    VDC_FillArea(ypos,xpos,CH_SPACE,width,1,VDC_WHITE+VDC_A_ALTCHAR);
    if(strlen(text)<width)
    {
        cspaces((width-strlen(text))/2-1);
    }
    cputs(text);
}

/* Overlay functions */

void initoverlay()
{
    // Load all overlays into memory if possible

    unsigned char x;
    unsigned int address=OVERLAYBANK0;
    unsigned char destbank=3;

    for(x=0;x<OVERLAYNUMBER;x++)
    {
        // Update load status message
        sprintf(buffer,"Memory overlay %u",x+1);
        printcentered(buffer,29,24,22);
        
        // Compose filename
        sprintf(buffer,"vdcse.ovl%u",x+1);

        // Load overlay file, exit if not found
        if (cbm_load (buffer, bootdevice, NULL) == 0)
        {
            printf("\nLoading overlay file failed\n");
            exit(1);
        }

        // Copy to overlay storage memory location
        //gotoxy(0,x);
        overlaydata[x].bank=destbank;
        //cprintf("Copy to: %u %4X ",destbank,address);

        if(destbank)
        {
            BankMemCopy(OVERLAYLOAD,2,address,destbank-1,OVERLAYSIZE);
            overlaydata[x].address=address;
            address+=OVERLAYSIZE;
            //cprintf("success to: %4x ",address);
            if(destbank==3)
            {
                if(address+OVERLAYSIZE<OVERLAYBANK0 || address+OVERLAYSIZE>0xEFFF)
                {
                    address=OVERLAYBANK1;
                    destbank=4;
                }
            }
            else
            {
                if(address+OVERLAYSIZE<OVERLAYBANK1 || address+OVERLAYSIZE>0xEFFF)
                {
                    destbank=0;
                }
            }
            //cprintf("new: %u %4X",destbank,address);
        }
    }
}

void loadoverlay(unsigned char overlay_select)
{
    // Load memory overlay with given number

    // Returns if overlay allready active
    if(overlay_select != overlay_active)
    {
        overlay_active = overlay_select;
        if(overlaydata[overlay_select-1].bank)
        {
            BankMemCopy(overlaydata[overlay_select-1].address,overlaydata[overlay_select-1].bank-1,OVERLAYLOAD,2,OVERLAYSIZE);
        }
        else
        {
            // Compose filename
            sprintf(buffer,"vdcse.ovl%u",overlay_select);

            // Load overlay file, exit if not found
            if (cbm_load (buffer, bootdevice, NULL) == 0)
            {
                printf("\nLoading overlay file failed\n");
                exit(1);
            }
        }
    }   
}


// Functions for windowing and menu system

void windowsave(unsigned char ypos, unsigned char height, unsigned char loadsyscharset)
{
    /* Function to save a window
       Input:
       - ypos: startline of window
       - height: height of window    
       - loadsyscharset: load syscharset if userdefined charset is loaded enabled (1) or not (0) */
    
    Window[windownumber].address = windowaddress;
    Window[windownumber].ypos = ypos;
    Window[windownumber].height = height;

    // Copy characters
    VDC_CopyVDCToMem(ypos*80,windowaddress,1,height*80);
    windowaddress += height*80;

    // Copy attributes
    VDC_CopyVDCToMem(0x0800+ypos*80,windowaddress,1,height*80);
    windowaddress += height*80;

    windownumber++;

    // Load system charset if needed
    if(loadsyscharset == 1 && charsetchanged[1] == 1)
    {
        VDC_RedefineCharset(CHARSETSYSTEM,1,VDCCHARALT,255);
    }
}

void windowrestore(unsigned char restorealtcharset)
{
    /* Function to restore a window
       Input: restorealtcharset: request to restore user defined charset if needed enabled (1) or not (0) */

    windowaddress = Window[--windownumber].address;

    // Restore characters
    VDC_CopyMemToVDC(Window[windownumber].ypos*80,windowaddress,1,Window[windownumber].height*80);

    // Restore attributes
    VDC_CopyMemToVDC(0x0800+(Window[windownumber].ypos*80),windowaddress+(Window[windownumber].height*80),1,Window[windownumber].height*80);

    // Restore custom charset if needed
    if(restorealtcharset == 1 && charsetchanged[1] == 1)
    {
        VDC_RedefineCharset(CHARSETALTERNATE,1,VDCCHARALT,255);
    }
}

void windownew(unsigned char xpos, unsigned char ypos, unsigned char height, unsigned char width, unsigned char loadsyscharset)
{
    /* Function to make menu border
       Input:
       - xpos: x-coordinate of left upper corner
       - ypos: y-coordinate of right upper corner
       - height: number of rows in window
       - width: window width in characters
        - loadsyscharset: load syscharset if userdefined charset is loaded enabled (1) or not (0) */
 
    windowsave(ypos, height,loadsyscharset);

    VDC_FillArea(ypos,xpos,CH_SPACE,width,height,mc_menupopup);
}

void menuplacebar()
{
    /* Function to print menu bar */

    unsigned char x;

    VDC_FillArea(0,0,CH_SPACE,80,1,mc_mb_normal);
    for(x=0;x<menubaroptions;x++)
    {
        VDC_PrintAt(0,menubarcoords[x],menubartitles[x],mc_mb_normal);
    }
}

unsigned char menupulldown(unsigned char xpos, unsigned char ypos, unsigned char menunumber, unsigned char escapable)
{
    /* Function for pull down menu
       Input:
       - xpos = x-coordinate of upper left corner
       - ypos = y-coordinate of upper left corner
       - menunumber = 
         number of the menu as defined in pulldownmenuoptions array 
       - espacable: ability to escape with escape key enabled (1) or not (0)  */

    unsigned char x;
    unsigned char key;
    unsigned char exit = 0;
    unsigned char menuchoice = 1;

    windowsave(ypos, pulldownmenuoptions[menunumber-1],0);
    for(x=0;x<pulldownmenuoptions[menunumber-1];x++)
    {
        VDC_Plot(ypos+x,xpos,CH_SPACE,mc_pd_normal);
        VDC_PrintAt(ypos+x,xpos+1,pulldownmenutitles[menunumber-1][x],mc_pd_normal);
        VDC_Plot(ypos+x,xpos+strlen(pulldownmenutitles[menunumber-1][x])+1,CH_SPACE,mc_pd_normal);
    }
  
    do
    {
        VDC_Plot(ypos+menuchoice-1,xpos,CH_MINUS,mc_pd_select);
        VDC_PrintAt(ypos+menuchoice-1,xpos+1,pulldownmenutitles[menunumber-1][menuchoice-1],mc_pd_select);
        VDC_Plot(ypos+menuchoice-1,xpos+strlen(pulldownmenutitles[menunumber-1][menuchoice-1])+1,CH_SPACE,mc_pd_select);

        do
        {
            key = cgetc();
        } while (key != CH_ENTER && key != CH_CURS_LEFT && key != CH_CURS_RIGHT && key != CH_CURS_UP && key != CH_CURS_DOWN && key != CH_ESC && key != CH_STOP );

        switch (key)
        {
        case CH_ESC:
        case CH_STOP:
            if(escapable == 1) { exit = 1; menuchoice = 0; }
            break;

        case CH_ENTER:
            exit = 1;
            break;
        
        case CH_CURS_LEFT:
            exit = 1;
            menuchoice = 18;
            break;
        
        case CH_CURS_RIGHT:
            exit = 1;
            menuchoice = 19;
            break;

        case CH_CURS_DOWN:
        case CH_CURS_UP:
            VDC_Plot(ypos+menuchoice-1,xpos,CH_SPACE,mc_pd_normal);
            VDC_PrintAt(ypos+menuchoice-1,xpos+1,pulldownmenutitles[menunumber-1][menuchoice-1],mc_pd_normal);
            VDC_Plot(ypos+menuchoice-1,xpos+strlen(pulldownmenutitles[menunumber-1][menuchoice-1])+1,CH_SPACE,mc_pd_normal);
            if(key==CH_CURS_UP)
            {
                menuchoice--;
                if(menuchoice<1)
                {
                    menuchoice=pulldownmenuoptions[menunumber-1];
                }
            }
            else
            {
                menuchoice++;
                if(menuchoice>pulldownmenuoptions[menunumber-1])
                {
                    menuchoice = 1;
                }
            }
            break;

        default:
            break;
        }
    } while (exit==0);
    windowrestore(0);    
    return menuchoice;
}

unsigned char menumain()
{
    /* Function for main menu selection */

    unsigned char menubarchoice = 1;
    unsigned char menuoptionchoice = 0;
    unsigned char key;
    unsigned char xpos;

    menuplacebar();

    do
    {
        do
        {
            VDC_Plot(0,menubarcoords[menubarchoice-1]-1,CH_SPACE,mc_mb_select);
            VDC_PrintAt(0,menubarcoords[menubarchoice-1],menubartitles[menubarchoice-1],mc_mb_select);
            VDC_Plot(0,menubarcoords[menubarchoice-1]+strlen(menubartitles[menubarchoice-1]),CH_SPACE,mc_mb_select);

            do
            {
                key = cgetc();
            } while (key != CH_ENTER && key != CH_CURS_LEFT && key != CH_CURS_RIGHT && key != CH_ESC && key != CH_STOP);

            VDC_Plot(0,menubarcoords[menubarchoice-1]-1,CH_SPACE,mc_mb_normal);
            VDC_PrintAt(0,menubarcoords[menubarchoice-1],menubartitles[menubarchoice-1],mc_mb_normal);
            VDC_Plot(0,menubarcoords[menubarchoice-1]+strlen(menubartitles[menubarchoice-1]),CH_SPACE,mc_mb_normal);
            
            if(key==CH_CURS_LEFT)
            {
                menubarchoice--;
                if(menubarchoice<1)
                {
                    menubarchoice = menubaroptions;
                }
            }
            else if (key==CH_CURS_RIGHT)
            {
                menubarchoice++;
                if(menubarchoice>menubaroptions)
                {
                    menubarchoice = 1;
                }
            }
        } while (key!=CH_ENTER && key != CH_ESC && key != CH_STOP);
        if (key != CH_ESC && key != CH_STOP)
            {
            xpos=menubarcoords[menubarchoice-1]-1;
            if(xpos+strlen(pulldownmenutitles[menubarchoice-1][0])>38)
            {
                xpos=menubarcoords[menubarchoice-1]+strlen(menubartitles[menubarchoice-1])-strlen(pulldownmenutitles  [menubarchoice-1][0]);
            }
            menuoptionchoice = menupulldown(xpos,1,menubarchoice,1);
            if(menuoptionchoice==18)
            {
                menuoptionchoice=0;
                menubarchoice--;
                if(menubarchoice<1)
                {
                    menubarchoice = menubaroptions;
                }
            }
            if(menuoptionchoice==19)
            {
                menuoptionchoice=0;
                menubarchoice++;
                if(menubarchoice>menubaroptions)
                {
                    menubarchoice = 1;
                }
            }
        }
        else
        {
            menuoptionchoice = 99;
        }
    } while (menuoptionchoice==0);

    return menubarchoice*10+menuoptionchoice;    
}

unsigned char areyousure(char* message, unsigned char syscharset)
{
    /* Pull down menu to verify if player is sure */
    unsigned char choice;

    windownew(8,8,6,30,syscharset);
    VDC_PrintAt(9,10,message,mc_menupopup);
    VDC_PrintAt(10,10,"Are you sure?",mc_menupopup);
    choice = menupulldown(25,11,5,0);
    windowrestore(syscharset);
    return choice;
}

void fileerrormessage(unsigned char error, unsigned char syscharset)
{
    /* Show message for file error encountered */

    windownew(8,8,6,30,syscharset);
    VDC_PrintAt(10,10,"File error!",mc_menupopup);
    if(error<255)
    {
        sprintf(buffer,"Error nr.: %2X",error);
        VDC_PrintAt(12,10,buffer,mc_menupopup);
    }
    VDC_PrintAt(13,10,"Press key.",mc_menupopup);
    cgetc();
    windowrestore(syscharset);    
}

void messagepopup(char* message, unsigned char syscharset)
{
    // Show popup with a message

    windownew(8,8,6,30,syscharset);
    VDC_PrintAt(10,10,message,mc_menupopup);
    VDC_PrintAt(12,10,"Press key.",mc_menupopup);
    cgetc();
    windowrestore(syscharset);  
}

// Generic screen map routines

void printstatusbar()
{
    if(screen_row==24) { return; }

    sprintf(buffer,"%-10s",programmode);
    VDC_PrintAt(24,6,buffer,mc_menupopup);
    sprintf(buffer,"%3u,%3u",screen_col+xoffset,screen_row+yoffset);
    VDC_PrintAt(24,22,buffer,mc_menupopup);
    if(plotaltchar)
    {
        VDC_Plot(24,36,plotscreencode,mc_menupopup);
    }
    else
    {
        VDC_Plot(24,36,plotscreencode,mc_menupopup-VDC_A_ALTCHAR);
    }
    sprintf(buffer,"%2X",plotscreencode);
    VDC_PrintAt(24,38,buffer,mc_menupopup);
    VDC_Plot(24,48,CH_SPACE,plotcolor+VDC_A_REVERSE);
    sprintf(buffer,"%2u",plotcolor);
    VDC_PrintAt(24,50,buffer,mc_menupopup);
    if(plotreverse)
    {
        VDC_PrintAt(24,54,"REV",mc_menupopup);
    }
    else
    {
        VDC_PrintAt(24,54,"   ",mc_menupopup);
    }
    if(plotunderline)
    {
        VDC_PrintAt(24,58,"UND",mc_menupopup);
    }
    else
    {
        VDC_PrintAt(24,58,"   ",mc_menupopup);
    }
    if(plotblink)
    {
        VDC_PrintAt(24,62,"BLI",mc_menupopup);
    }
    else
    {
        VDC_PrintAt(24,62,"   ",mc_menupopup);
    }
    if(plotaltchar)
    {
        VDC_PrintAt(24,66,"ALT",mc_menupopup);
    }
    else
    {
        VDC_PrintAt(24,66,"   ",mc_menupopup);
    }
}

void initstatusbar()
{
    if(screen_row==24) { return; }

    VDC_FillArea(24,0,CH_SPACE,80,1,mc_menupopup);
    VDC_PrintAt(24, 0,"Mode:",mc_menupopup);
    VDC_PrintAt(24,17,"X,Y:",mc_menupopup);
    VDC_PrintAt(24,31,"Char:",mc_menupopup);
    VDC_PrintAt(24,41,"Color:",mc_menupopup);
    VDC_PrintAt(24,73,"F8=Help",mc_menupopup);
    printstatusbar();
}

void hidestatusbar()
{
    VDC_CopyViewPortToVDC(SCREENMAPBASE,1,screenwidth,screenheight,xoffset,yoffset+24,0,24,80,1);
}

void togglestatusbar()
{
    if(screen_row==24) { return; }

    if(showbar)
    {
        showbar=0;
        hidestatusbar();
    }
    else
    {
        showbar=1;
        initstatusbar();
    }
}

unsigned int screenmap_screenaddr(unsigned char row, unsigned char col, unsigned int width)
{
    return SCREENMAPBASE+(row*width)+col;
}

unsigned int screenmap_attraddr(unsigned char row, unsigned char col, unsigned int width, unsigned int height)
{
    // Function to calculate screenmap address for the attribute space
    // Input: row, col, width and height for screenmap
    return SCREENMAPBASE+(row*width)+col+(width*height)+48;
}

void screenmapplot(unsigned char row, unsigned char col, unsigned char screencode, unsigned char attribute)
{
    // Function to plot a screencodes at bank 1 memory screen map
	// Input: row and column, screencode to plot, attribute code

    POKEB(screenmap_screenaddr(row,col,screenwidth),1,screencode);
    POKEB(screenmap_attraddr(row,col,screenwidth,screenheight),1,attribute);
}

void placesignature()
{
    // Place signature in screenmap with program version

    char versiontext[49] = "";
    unsigned char x;
    unsigned int address = SCREENMAPBASE + (screenwidth*screenheight);

    sprintf(versiontext,"VDC Screen Editor %s X.Mol ",version);

    for(x=0;x<strlen(versiontext);x++)
    {
        POKEB(address+x,1,versiontext[x]);
    }
}

void screenmapfill(unsigned char screencode, unsigned char attribute)
{
    // Function to fill screen with the screencode and attribute code provided as input

    unsigned int address = SCREENMAPBASE;
    
    BankMemSet(address,1,screencode,screentotal+48);
    placesignature();
    address += screentotal + 48;
    BankMemSet(address,1,attribute,screentotal);
}

void cursormove(unsigned char left, unsigned char right, unsigned char up, unsigned char down)
{
    // Move cursor and scroll screen if needed
    // Input: flags to enable (1) or disable (0) move in the different directions

    if(left == 1 )
    {
        if(screen_col==0)
        {
            if(xoffset>0)
            {
                gotoxy(screen_col,screen_row);
                VDC_ScrollCopy(SCREENMAPBASE,1,screenwidth,screenheight,xoffset--,yoffset,0,0,80,25,2);
                initstatusbar();
            }
        }
        else
        {
            gotoxy(--screen_col,screen_row);
        }
    }
    if(right == 1 )
    {
        if(screen_col==79)
        {
            if(xoffset+screen_col<screenwidth-1)
            {
                gotoxy(screen_col,screen_row);
                VDC_ScrollCopy(SCREENMAPBASE,1,screenwidth,screenheight,xoffset++,yoffset,0,0,80,25,1);
                initstatusbar();
            }
        }
        else
        {
            gotoxy(++screen_col,screen_row);
        }
    }
    if(up == 1 )
    {
        if(screen_row==0)
        {
            if(yoffset>0)
            {
                gotoxy(screen_col,screen_row);
                VDC_ScrollCopy(SCREENMAPBASE,1,screenwidth,screenheight,xoffset,yoffset--,0,0,80,25,4);
                initstatusbar();
            }
        }
        else
        {
            gotoxy(screen_col,--screen_row);
            if(showbar && screen_row==23) { initstatusbar(); }
        }
    }
    if(down == 1 )
    {
        if(screen_row==23) { hidestatusbar(); }
        if(screen_row==24)
        {
            if(yoffset+screen_row<screenheight-1)
            {
                gotoxy(screen_col,screen_row);
                VDC_ScrollCopy(SCREENMAPBASE,1,screenwidth,screenheight,xoffset,yoffset++,0,0,80,25,8);
                initstatusbar();
            }
        }
        else
        {
            gotoxy(screen_col,++screen_row);
        }
    }
}

// Functions for undo system

void undo_new(unsigned char row, unsigned char col, unsigned char width, unsigned char height)
{
    // Function to create a new undo buffer position

    unsigned char y;
    unsigned char redoroompresent = 1;

    if(undo_redopossible>0)
    {
        undo_undopossible=1;        
        undo_redopossible=0;
    }
    else
    {
        undo_undopossible++;
    }
    undonumber++;
    if(undonumber>40) { undonumber=1;}
    if(undoaddress+(width*height*4)<undoaddress) { undonumber = 1; undoaddress = VDCEXTENDED; }
    if(undoaddress+(width*height*4)>(0xffff - VDCEXTENDED)) { redoroompresent = 0; }
    for(y=0;y<height;y++)
    {
        VDC_CopyMemToVDC(undoaddress+(y*width),screenmap_screenaddr(row+y,col,screenwidth),1,width);
        VDC_CopyMemToVDC(undoaddress+(width*height)+(y*width),screenmap_attraddr(row+y,col,screenwidth,screenheight),1,width);
    }
    Undo[undonumber-1].address = undoaddress;
    if(undonumber<40) { Undo[undonumber].address = 0; } else { Undo[0].address = 0; }
    Undo[undonumber-1].xstart = col;
    Undo[undonumber-1].ystart = row;
    Undo[undonumber-1].width = width;
    Undo[undonumber-1].height = height;
    Undo[undonumber-1].redopresent = redoroompresent;
    undoaddress += width*height*(2+(2*redoroompresent));
    //gotoxy(0,24);
    //cprintf("UN: %u UA: %4X RF: %u NA: %4X UP: %u RP: %u    ",undonumber,Undo[undonumber-1].address,Undo[undonumber-1].redopresent,undoaddress,undo_undopossible,undo_redopossible);   
}

void undo_performundo()
{
    // Function to perform an undo if a filled undo slot is present

    unsigned char y, row, col, width, height;

    if(undo_undopossible>0)
    {
        row = Undo[undonumber-1].ystart;
        col = Undo[undonumber-1].xstart;
        width = Undo[undonumber-1].width;
        height = Undo[undonumber-1].height;
        for(y=0;y<height;y++)
        {
            if(Undo[undonumber-1].redopresent>0)
            {
                VDC_CopyMemToVDC(Undo[undonumber-1].address+(width*height*2)+(y*width),screenmap_screenaddr(row+y,col,screenwidth),1,width);
                VDC_CopyMemToVDC(Undo[undonumber-1].address+(width*height*3)+(y*width),screenmap_attraddr(row+y,col,screenwidth,screenheight),1,width);
            }
            VDC_CopyVDCToMem(Undo[undonumber-1].address+(y*width),screenmap_screenaddr(row+y,col,screenwidth),1,width);
            VDC_CopyVDCToMem(Undo[undonumber-1].address+(width*height)+(y*width),screenmap_attraddr(row+y,col,screenwidth,screenheight),1,width);
        }
        VDC_CopyViewPortToVDC(SCREENMAPBASE,1,screenwidth,screenheight,xoffset,yoffset,0,0,80,25);
        if(showbar) { initstatusbar(); }
        if(Undo[undonumber-1].redopresent>0) { Undo[undonumber-1].redopresent=2; undo_redopossible++; }
        //gotoxy(0,24);
        //cprintf("UN: %u UA: %4X RF: %u ",undonumber,Undo[undonumber-1].address,Undo[undonumber-1].redopresent); 
        undoaddress = Undo[undonumber-1].address;   
        undonumber--;
        if(undonumber==0)
        {
            if(Undo[39].address>0) { undonumber=40; }
        }    
        undo_undopossible--;
        if(undonumber>0 && Undo[undonumber-1].address==0) { undo_undopossible=0; }
        if(undonumber==0 && Undo[39].address==0) { undo_undopossible=0; }
        //cprintf("NN: %u NA: %4X UP: %u RP: %u    ",undonumber,undoaddress,undo_undopossible,undo_redopossible); 
    }
}

void undo_escapeundo()
{
    // Function to cancel an undo slot after escape is pressed in selectmode or movemode

    Undo[undonumber].address = 0;
    undonumber--;
    if(undonumber==0)
    {
        if(Undo[39].address>0) { undonumber=40; }
    }  
}

void undo_performredo()
{
    // Function to perform an redo if a filled redo slot is present

    unsigned char y, row, col, width, height;

    if(undo_redopossible>0)
    {
        if(undonumber<39) { undonumber++; } else { undonumber = 1; }
        row = Undo[undonumber-1].ystart;
        col = Undo[undonumber-1].xstart;
        width = Undo[undonumber-1].width;
        height = Undo[undonumber-1].height;
        for(y=0;y<height;y++)
        {
            VDC_CopyVDCToMem(Undo[undonumber-1].address+(width*height*2)+(y*width),screenmap_screenaddr(row+y,col,screenwidth),1,width);
            VDC_CopyVDCToMem(Undo[undonumber-1].address+(width*height*3)+(y*width),screenmap_attraddr(row+y,col,screenwidth,screenheight),1,width);
        }
        VDC_CopyViewPortToVDC(SCREENMAPBASE,1,screenwidth,screenheight,xoffset,yoffset,0,0,80,25);
        if(showbar) { initstatusbar(); }
        //gotoxy(0,24);
        //cprintf("UN: %u UA: %4X RF: %u ",undonumber,Undo[undonumber-1].address,Undo[undonumber-1].redopresent); 
        undoaddress = Undo[undonumber-1].address;
        undo_undopossible++; 
        undo_redopossible--;
        if(undonumber<39 && Undo[undonumber].redopresent==0) { undo_redopossible=0; }
        if(undonumber==39 && Undo[0].redopresent==0) { undo_redopossible=0; }
        //cprintf("NN: %u NA: %4X UP: %u RP: %u    ",undonumber,undoaddress,undo_undopossible,undo_redopossible); 
    }
}

// Help screens
void helpscreen_load(unsigned char screennumber)
{
    // Function to show selected help screen
    // Input: screennumber: 1-Main mode, 2-Character editor, 3-SelectMoveLinebox, 4-Write/colorwrite mode

    // Load system charset if needed
    if(charsetchanged[1] == 1)
    {
        VDC_RedefineCharset(CHARSETSYSTEM,1,VDCCHARALT,255);
    }

    // Set background color to black and switch cursor off
    VDC_BackColor(VDC_BLACK);
    cursor(0);

    // Load selected help screen
    sprintf(buffer,"vdcse.hsc%u",screennumber);

    if(VDC_LoadScreen(buffer,bootdevice,WINDOWBASEADDRESS,1)>WINDOWBASEADDRESS)
    {
        VDC_CopyMemToVDC(VDCBASETEXT,WINDOWBASEADDRESS,1,4048);
    }
    else
    {
        messagepopup("Insert application disk to view help.",0);
    }
    
    cgetc();

    // Restore screen
    VDC_BackColor(screenbackground);
    VDC_CopyViewPortToVDC(SCREENMAPBASE,1,screenwidth,screenheight,xoffset,yoffset,0,0,80,25);
    if(showbar) { initstatusbar(); }
    if(screennumber!=2)
    {
        gotoxy(screen_col,screen_row);
        VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
    }
    cursor(1);

    // Restore custom charset if needed
    if(charsetchanged[1] == 1)
    {
        VDC_RedefineCharset(CHARSETALTERNATE,1,VDCCHARALT,255);
    }
}

// Application routines
void plotmove(unsigned char direction)
{
    // Drive cursor move
    // Input: ASCII code of cursor key pressed

    VDC_Plot(screen_row,screen_col,PEEKB(screenmap_screenaddr(yoffset+screen_row,xoffset+screen_col,screenwidth),1),PEEKB(screenmap_attraddr(yoffset+screen_row,xoffset+screen_col,screenwidth,screenheight),1));

    switch (direction)
    {
    case CH_CURS_LEFT:
        cursormove(1,0,0,0);
        break;
    
    case CH_CURS_RIGHT:
        cursormove(0,1,0,0);
        break;

    case CH_CURS_UP:
        cursormove(0,0,1,0);
        break;

    case CH_CURS_DOWN:
        cursormove(0,0,0,1);
        break;
    
    default:
        break;
    }

    VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
}

void change_plotcolor(unsigned char newval)
{
    plotcolor=newval;
    textcolor(vdctoconiocol[plotcolor]);
    VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
}

void showchareditfield(unsigned char stdoralt)
{
    // Function to draw char editor background field
    // Input: Flag for which charset is edited, standard (0) or alternate (1)

    unsigned char attribute = mc_menupopup-(VDC_A_ALTCHAR*stdoralt);

    windowsave(0,12,0);
    VDC_FillArea(0,67,CH_SPACE,13,12,attribute);
}

unsigned int charaddress(unsigned char screencode, unsigned char stdoralt, unsigned char vdcormem)
{
    // Function to calculate address of character to edit
    // Input:   screencode to edit, flag for standard (0) or alternate (1) charset,
    //          flag for VDC (0) or bank 1 (1) memory address

    unsigned int address;

    if(vdcormem==0)
    {
        address = (stdoralt==0)? VDCCHARSTD:VDCCHARALT;
        address += screencode*16;
    }
    else
    {
        address = (stdoralt==0)? CHARSETNORMAL:CHARSETALTERNATE;
        address += screencode*8;
    }
    return address;
}

void showchareditgrid(unsigned int screencode, unsigned char stdoralt)
{
    // Function to draw grid with present char to edit

    unsigned char x,y,char_byte,colorbase,colorbit;
    unsigned int address;

    address = charaddress(screencode,stdoralt,0);
    
    colorbase = mc_menupopup - (VDC_A_ALTCHAR*stdoralt);

    sprintf(buffer,"Char %2X %s",screencode,(stdoralt==0)? "Std":"Alt");
    VDC_PrintAt(1,68,buffer,colorbase);

    for(y=0;y<8;y++)
    {
        char_byte = VDC_Peek(address+y);
        sprintf(buffer,"%2X",char_byte);
        VDC_PrintAt(y+3,68,buffer,colorbase);
        for(x=0;x<8;x++)
        {
            if(char_byte & (1<<(7-x)))
            {
                colorbit = colorbase;
            }
            else
            {
                colorbit = colorbase-VDC_A_REVERSE;
            }
            VDC_Plot(y+3,x+71,CH_SPACE,colorbit);
        }
    }
}


void mainmenuloop()
{
    // Function for main menu selection loop

    unsigned char menuchoice;
    
    windowsave(0,1,1);

    do
    {
        menuchoice = menumain();
      
        switch (menuchoice)
        {
        case 11:
            loadoverlay(1);
            resizewidth();
            break;

        case 12:
            loadoverlay(2);
            resizeheight();
            break;
        
        case 13:
            loadoverlay(3);
            changebackgroundcolor();
            break;

        case 14:
            if(undoenabled == 1) { undo_new(0,0,screenwidth,screenheight); }
            screenmapfill(CH_SPACE,VDC_WHITE);
            windowrestore(0);
            VDC_CopyViewPortToVDC(SCREENMAPBASE,1,screenwidth,screenheight,xoffset,yoffset,0,0,80,25);
            windowsave(0,1,0);
            menuplacebar();
            if(showbar) { initstatusbar(); }
            break;
        
        case 15:
            if(undoenabled == 1) { undo_new(0,0,screenwidth,screenheight); }
            screenmapfill(plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
            windowrestore(0);
            VDC_CopyViewPortToVDC(SCREENMAPBASE,1,screenwidth,screenheight,xoffset,yoffset,0,0,80,25);
            windowsave(0,1,0);
            menuplacebar();
            if(showbar) { initstatusbar(); }
            break;

        case 21:
            loadoverlay(3);
            savescreenmap();
            break;

        case 22:
            loadoverlay(3);
            loadscreenmap();
            break;
        
        case 23:
            loadoverlay(3);
            saveproject();
            break;
        
        case 24:
            loadoverlay(3);
            loadproject();
            break;
        
        case 31:
            loadoverlay(3);
            loadcharset(0);
            break;
        
        case 32:
            loadoverlay(3);
            loadcharset(1);
            break;
        
        case 33:
            loadoverlay(3);
            savecharset(0);
            break;

        case 34:
            loadoverlay(3);
            savecharset(1);
            break;

        case 41:
            loadoverlay(3);
            versioninfo();
            break;

        case 42:
            appexit = 1;
            menuchoice = 99;
            break;
        
        case 43:
            undoenabled = (undoenabled==0)? 1:0;
            sprintf(pulldownmenutitles[3][2],"Undo: %s",(undoenabled==1)? "Enabled  ":"Disabled ");
            undoaddress = VDCEXTENDED;                              // Reset undo address
            undonumber = 0;                                         // Reset undo number
            undo_undopossible = 0;                                  // Reset undo possible flag
            undo_redopossible = 0;
            break;

        default:
            break;
        }
    } while (menuchoice < 99);
    
    windowrestore(1);
}

// Main loop

void main()
{
    // Main application initialization, loop and exit
    
    unsigned char key, newval;

    // Reset startvalues global variables
    charsetchanged[0] = 0;
    charsetchanged[1] = 0;
    appexit = 0;
    screen_col = 0;
    screen_row = 0;
    xoffset = 0;
    yoffset = 0;
    screenwidth = 80;
    screenheight = 25;
    screentotal = screenwidth*screenheight;
    screenbackground = 0;
    plotscreencode = 0;
    plotcolor = VDC_WHITE;
    plotreverse = 0;
    plotunderline = 0;
    plotblink = 0;
    plotaltchar = 0;

    sprintf(pulldownmenutitles[0][0],"Width:   %5i ",screenwidth);
    sprintf(pulldownmenutitles[0][1],"Height:  %5i ",screenheight);
    sprintf(pulldownmenutitles[0][2],"Background: %2i ",screenbackground);

    // Obtain device number the application was started from
    bootdevice = getcurrentdevice();
    targetdevice = bootdevice;

    // Set version number in string variable
    sprintf(version,
            "v%2i.%2i - %c%c%c%c%c%c%c%c-%c%c%c%c",
            VERSION_MAJOR, VERSION_MINOR,
            BUILD_YEAR_CH0, BUILD_YEAR_CH1, BUILD_YEAR_CH2, BUILD_YEAR_CH3, BUILD_MONTH_CH0, BUILD_MONTH_CH1, BUILD_DAY_CH0, BUILD_DAY_CH1,BUILD_HOUR_CH0, BUILD_HOUR_CH1, BUILD_MIN_CH0, BUILD_MIN_CH1);

    // Initialise VDC screen and VDC assembly routines
    VDC_Init();

    // Detect VDC memory size and set VDC memory config size to 64K if present
    vdcmemory = VDC_DetectVDCMemSize();
    if(vdcmemory==64)
    {
        VDC_SetExtendedVDCMemSize();                            // Enable VDC 64KB extended memory
        clrscr();                                               // Clear screen to reset screen data
        strcpy(pulldownmenutitles[3][2],"Undo: Enabled  ");     // Enable undo menuoption
        pulldownmenuoptions[3]=3;                               // Enable undo menupotion
        undoenabled = 1;                                        // Set undo enabled flag
        undoaddress = VDCEXTENDED;                              // Reset undo address
        undonumber = 0;                                         // Reset undo number
        undo_undopossible = 0;                                  // Reset undo possible flag
        undo_redopossible = 0;                                  // Reset redo possible flag
    }

    // Copy charsets from ROM
    VDC_CopyCharsetsfromROM();

    // Load and show title screen
    printcentered("Load title screen",29,24,22);
    if(VDC_LoadScreen("vdcse.tscr",bootdevice,SCREENMAPBASE,1)>SCREENMAPBASE)
    {
        VDC_CopyMemToVDC(VDCBASETEXT,SCREENMAPBASE,1,4048);
    }

    // Init overlays
    initoverlay();

    // Load visual PETSCII map mapping data
    printcentered("Load visual PETSCII",29,24,22);
	cbm_k_setlfs(0,bootdevice, 0);
	cbm_k_setnam("vdcse.petv");
	SetLoadSaveBank(0);
	cbm_k_load(0,PETSCIIMAP);

    // Load default charsets to bank 1
    printcentered("Load charsets",29,24,22);
    VDC_LoadCharset("vdcse.falt",bootdevice, CHARSETSYSTEM, 1, 0);
    VDC_LoadCharset("vdcse.fstd",bootdevice, CHARSETNORMAL, 1, 0);
    BankMemCopy(CHARSETSYSTEM,1,CHARSETALTERNATE,1,2048);

    // Clear screen map in bank 1 with spaces in text color white
    screenmapfill(CH_SPACE,VDC_WHITE);
 
    // Wait for key press to start application
    printcentered("Press key to start.",29,24,22);
    cgetc();

    // Clear viewport of titlescreen
    clrscr();

    // Main program loop
    VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
    gotoxy(screen_col,screen_row);
    cursor(1);
    strcpy(programmode,"Main");
    showbar = 1;
    initstatusbar();

    do
    {
        if(showbar) { printstatusbar(); }
        key = cgetc();

        switch (key)
        {
        // Cursor move
        case CH_CURS_LEFT:
        case CH_CURS_RIGHT:
        case CH_CURS_UP:
        case CH_CURS_DOWN:
            plotmove(key);
            break;
        
        // Increase screencode
        case '+':
            plotscreencode++;
            VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
            break;

        // Decrease screencode
        case '-':
            plotscreencode--;
            VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
            break;
        
        // Decrease color
        case ',':
            if(plotcolor==0) { newval = 15; } else { newval = plotcolor - 1; }
            if(newval == screenbackground)
            {
                if(newval==0) { newval = 15; } else { newval--; }
            }
            change_plotcolor(newval);
            break;

        // Increase color
        case '.':
            if(plotcolor==15) { newval = 0; } else { newval = plotcolor + 1; }
            if(newval == screenbackground)
            {
                if(newval==15) { newval = 0; } else { newval++; }
            }
            change_plotcolor(newval);
            break;
        
        // Toggle underline
        case 'u':
            plotunderline = (plotunderline==0)? 1:0;
            VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
            break;

        // Toggle blink
        case 'b':
            plotblink = (plotblink==0)? 1:0;
            VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
            break;

        // Toggle reverse
        case 'r':
            plotreverse = (plotreverse==0)? 1:0;
            VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
            break;

        // Toggle alternate character set
        case 'a':
            plotaltchar = (plotaltchar==0)? 1:0;
            VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
            break;

        // Character eddit mode
        case 'e':
            loadoverlay(4);
            chareditor();
            break;

        // Palette for character selection
        case 'p':
            loadoverlay(1);
            palette();
            break;

        // Grab underlying character and attributes
        case 'g':
            plotscreencode = PEEKB(screenmap_screenaddr(screen_row+yoffset,screen_col+xoffset,screenwidth),1);
            newval = PEEKB(screenmap_attraddr(screen_row+yoffset,screen_col+xoffset,screenwidth,screenheight),1);
            if(newval>128) { plotaltchar = 1; newval -= 128; } else { plotaltchar = 0; }
            if(newval>64) { plotreverse = 1; newval -= 64; } else { plotreverse = 0; }
            if(newval>32) { plotunderline = 1; newval -= 32; } else { plotunderline = 0; }
            if(newval>16) { plotblink =1; newval -+ 16; } else { plotblink =0; }
            plotcolor = newval;
            textcolor(vdctoconiocol[plotcolor]);
            VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
            break;

        // Write mode: type in screencodes
        case 'w':
            loadoverlay(1);
            writemode();
            break;
        
        // Color mode: type colors
        case 'c':
            loadoverlay(1);
            colorwrite();
            break;

        // Line and box mode
        case 'l':
            loadoverlay(2);
            lineandbox(1);
            break;

        // Move mode
        case 'm':
            loadoverlay(2);
            movemode();
            break;

        // Select mode
        case 's':
            loadoverlay(2);
            selectmode();
            break;

        // Undo
        case 'z':
            if(undoenabled==1 && undo_undopossible>0) { undo_performundo(); }
            break;
        
        // Redo
        case 'y':
            if(undoenabled==1 && undo_redopossible>0) { undo_performredo(); }
            break;

        // Try
        case 't':
            loadoverlay(3);
            plot_try();
            break;

        // Increase/decrease plot screencode by 128 (toggle 'RVS ON' and 'RVS OFF')
        case 'i':
            plotscreencode += 128;      // Will increase 128 if <128 and decrease by 128 if >128 by overflow
            VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
            break;        

        // Plot present screencode and attribute
        case CH_SPACE:
            if(undoenabled==1) { undo_new(screen_row+yoffset,screen_col+xoffset,1,1); }
            screenmapplot(screen_row+yoffset,screen_col+xoffset,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
            break;

        // Delete present screencode and attributes
        case CH_DEL:
            if(undoenabled==1) { undo_new(screen_row+yoffset,screen_col+xoffset,1,1); }
            screenmapplot(screen_row+yoffset,screen_col+xoffset,CH_SPACE,VDC_WHITE);
            break;

        // Go to upper left corner
        case CH_HOME:
            screen_row = 0;
            screen_col = 0;
            yoffset = 0;
            xoffset = 0;
            VDC_CopyViewPortToVDC(SCREENMAPBASE,1,screenwidth,screenheight,xoffset,yoffset,screen_col,screen_row,80,25);
            if(showbar) { initstatusbar(); }
            gotoxy(screen_col,screen_row);
            break;

        // Go to menu
        case CH_F1:
            cursor(0);
            mainmenuloop();
            VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
            gotoxy(screen_col,screen_row);
            textcolor(vdctoconiocol[plotcolor]);
            cursor(1);
            break;

        // Toggle statusbar
        case CH_F6:
            togglestatusbar();
            break;

        // Help screen
        case CH_F8:
            helpscreen_load(1);
            break;
        
        default:
            // 0-9: Favourites select
            if(key>47 && key<58)
            {
                plotscreencode = favourites[key-48][0];
                plotaltchar = favourites[key-48][1];
                VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
            }
            // Shift 1-9 or *: Store present character in favourites slot
            if(key>32 && key<43)
            {
                favourites[key-33][0] = plotscreencode;
                favourites[key-33][1] = plotaltchar;
            }
            break;
        }
    } while (appexit==0);

    cursor(0);
    textcolor(COLOR_YELLOW);
    VDC_Exit();
}
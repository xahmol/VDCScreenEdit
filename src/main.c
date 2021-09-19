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

//Window data
struct WindowStruct
{
    unsigned int address;
    unsigned char ypos;
    unsigned char height;
};
struct WindowStruct Window[9];

unsigned int windowaddress = WINDOWBASEADDRESS;
unsigned char windownumber = 0;

//Menu data
unsigned char menubaroptions = 4;
unsigned char pulldownmenunumber = 8;
char menubartitles[4][12] = {"Screen","File","Charset","Information"};
unsigned char menubarcoords[4] = {1,8,13,21};
unsigned char pulldownmenuoptions[5] = {5,4,4,3,2};
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
     "Help           ",
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
struct UndoStruct
{
    unsigned int address;
    unsigned char ystart;
    unsigned char xstart;
    unsigned char height;
    unsigned char width;
    unsigned char redopresent;
};
struct UndoStruct Undo[41];

// Menucolors
unsigned char mc_mb_normal = VDC_LGREEN + VDC_A_REVERSE + VDC_A_ALTCHAR;
unsigned char mc_mb_select = VDC_WHITE + VDC_A_REVERSE + VDC_A_ALTCHAR;
unsigned char mc_pd_normal = VDC_LCYAN + VDC_A_REVERSE + VDC_A_ALTCHAR;
unsigned char mc_pd_select = VDC_LYELLOW + VDC_A_REVERSE + VDC_A_ALTCHAR;
unsigned char mc_menupopup = VDC_WHITE + VDC_A_REVERSE + VDC_A_ALTCHAR;

// Global variables
unsigned char bootdevice;
char DOSstatus[40];
unsigned char charsetchanged[2];
unsigned char appexit;
unsigned char targetdevice;
char filename[21];

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
        VDC_RedefineCharset(CHARSETSYSTEM,1,VDCCHARALT,256);
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
        VDC_RedefineCharset(CHARSETALTERNATE,1,VDCCHARALT,256);
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
        VDC_Plot(ypos+menuchoice-1,xpos,CH_SPACE,mc_pd_select);
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
            }
        }
        else
        {
            gotoxy(screen_col,--screen_row);
        }
    }
    if(down == 1 )
    {
        if(screen_row==24)
        {
            if(yoffset+screen_row<screenheight-1)
            {
                gotoxy(screen_col,screen_row);
                VDC_ScrollCopy(SCREENMAPBASE,1,screenwidth,screenheight,xoffset,yoffset++,0,0,80,25,8);
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
        cprintf("NN: %u NA: %4X UP: %u RP: %u    ",undonumber,undoaddress,undo_undopossible,undo_redopossible); 
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

// Application routines
void plotmove(direction)
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

void writemode()
{
    // Write mode with screencodes

    unsigned char key, screencode;
    unsigned char rvs = 0;

    do
    {
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

        // Toggle blink
        case CH_F1:
            plotblink = (plotblink==0)? 1:0;
            VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
            break;

        // Toggle underline
        case CH_F3:
            plotunderline = (plotunderline==0)? 1:0;
            VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
            break;

        // Toggle reverse
        case CH_F5:
            plotreverse = (plotreverse==0)? 1:0;
            VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
            break;

        // Toggle alternate character set
        case CH_F7:
            plotaltchar = (plotaltchar==0)? 1:0;
            VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
            break;

        // Delete present screencode and attributes
        case CH_DEL:
            screenmapplot(screen_row,screen_col,CH_SPACE,VDC_WHITE);
            VDC_Plot(screen_row,screen_col,CH_SPACE,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
            break;

        // Perform undo
        case CH_F2:
            if(undoenabled==1 && undo_undopossible>0) { undo_performundo(); }
            break;

        // Perform redo
        case CH_F4:
            if(undoenabled==1 && undo_redopossible>0) { undo_performredo(); }
            break;

        // Toggle RVS with the RVS ON and RVS OFF keys (control 9 and control 0)
        case CH_RVSON:
            rvs = 1;
            break;
        
        case CH_RVSOFF:
            rvs = 0;
            break;

        // Color control with Control and Commodore keys plus 0-9 key
        case CH_BLACK:
            change_plotcolor(VDC_BLACK);
            break;
        
        case CH_WHITE:
            change_plotcolor(VDC_WHITE);
            break;
        
        case CH_DRED:
            change_plotcolor(VDC_DRED);
            break;

        case CH_LCYAN:
            change_plotcolor(VDC_LCYAN);
            break;

        case CH_LPURPLE:
            change_plotcolor(VDC_LPURPLE);
            break;

        case CH_DGREEN:
            change_plotcolor(VDC_DGREEN);
            break;

        case CH_DBLUE:
            change_plotcolor(VDC_DBLUE);
            break;

        case CH_LYELLOW:
            change_plotcolor(VDC_LYELLOW);
            break;

        case CH_DPURPLE:
            change_plotcolor(VDC_DPURPLE);
            break;

        case CH_DYELLOW:
            change_plotcolor(VDC_DYELLOW);
            break;
            
        case CH_LRED:
            change_plotcolor(VDC_LRED);
            break;

        case CH_DCYAN:
            change_plotcolor(VDC_DCYAN);
            break;

        case CH_DGREY:
            change_plotcolor(VDC_DGREY);
            break;

        case CH_LGREEN:
            change_plotcolor(VDC_LGREEN);
            break;

        case CH_LBLUE:
            change_plotcolor(VDC_LBLUE);
            break;

        case CH_LGREY:
            change_plotcolor(VDC_LGREY);
            break;

        // Write printable character                
        default:
            if(isprint(key))
            {
                if(undoenabled == 1) { undo_new(screen_row+yoffset,screen_col+xoffset,1,1); }
                if(rvs==0) { screencode = VDC_PetsciiToScreenCode(key); } else { screencode = VDC_PetsciiToScreenCodeRvs(key); }
                screenmapplot(screen_row+yoffset,screen_col+xoffset,screencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
                plotmove(CH_CURS_RIGHT);
            }
            break;
        }
    } while (key != CH_ESC && key != CH_STOP);
}

void colorwrite()
{
    // Write mode with colors

    unsigned char key, attribute;

    do
    {
        key = cgetc();

        // Get old attribute value
        attribute = PEEKB(screenmap_attraddr(screen_row+yoffset,screen_col+xoffset,screenwidth,screenheight),1);

        switch (key)
        {

        // Cursor move
        case CH_CURS_LEFT:
        case CH_CURS_RIGHT:
        case CH_CURS_UP:
        case CH_CURS_DOWN:
            plotmove(key);
            break;

        // Toggle blink
        case CH_F1:
            attribute ^= 0x10;           // Toggle bit 4 for blink
            if(undoenabled == 1) { undo_new(screen_row+yoffset,screen_col+xoffset,1,1); }
            POKEB(screenmap_attraddr(screen_row+yoffset,screen_col+xoffset,screenwidth,screenheight),1,attribute);
            plotmove(CH_CURS_RIGHT);
            break;

        // Perform undo
        case CH_F2:
            if(undoenabled==1 && undo_undopossible>0) { undo_performundo(); }
            break;

        // Perform redo
        case CH_F4:
            if(undoenabled==1 && undo_redopossible>0) { undo_performredo(); }
            break;

        // Toggle underline
        case CH_F3:
            attribute ^= 0x20;           // Toggle bit 5 for underline
            if(undoenabled == 1) { undo_new(screen_row+yoffset,screen_col+xoffset,1,1); }
            POKEB(screenmap_attraddr(screen_row+yoffset,screen_col+xoffset,screenwidth,screenheight),1,attribute);
            plotmove(CH_CURS_RIGHT);
            break;

        // Toggle reverse
        case CH_F5:
            attribute ^= 0x40;           // Toggle bit 6 for reverse
            if(undoenabled == 1) { undo_new(screen_row+yoffset,screen_col+xoffset,1,1); }
            POKEB(screenmap_attraddr(screen_row+yoffset,screen_col+xoffset,screenwidth,screenheight),1,attribute);
            plotmove(CH_CURS_RIGHT);

        // Toggle alternate character set
        case CH_F7:
            attribute ^= 0x80;           // Toggle bit 7 for alternate charset
            if(undoenabled == 1) { undo_new(screen_row+yoffset,screen_col+xoffset,1,1); }
            POKEB(screenmap_attraddr(screen_row+yoffset,screen_col+xoffset,screenwidth,screenheight),1,attribute);
            plotmove(CH_CURS_RIGHT);
            break;            

        default:
            // If keypress is 0-9 or A-F select color
            if(key>47 && key<58)
            {
                attribute &= 0xf0;                  // Erase bits 0-3
                attribute += (key -48);             // Add color 0-9 with key 0-9
                if(undoenabled == 1) { undo_new(screen_row+yoffset,screen_col+xoffset,1,1); }
                POKEB(screenmap_attraddr(screen_row+yoffset,screen_col+xoffset,screenwidth,screenheight),1,attribute);
                plotmove(CH_CURS_RIGHT);
            }
            if(key>64 && key<71)
            {
                attribute &= 0xf0;                  // Erase bits 0-3
                attribute += (key -55);             // Add color 10-15 with key A-F
                if(undoenabled == 1) { undo_new(screen_row+yoffset,screen_col+xoffset,1,1); }
                POKEB(screenmap_attraddr(screen_row+yoffset,screen_col+xoffset,screenwidth,screenheight),1,attribute);
                plotmove(CH_CURS_RIGHT);
            }
            break;
        }
    } while (key != CH_ESC && key != CH_STOP);
}

void plotvisible(unsigned char row, unsigned char col, unsigned char setorrestore)
{
    // Plot or erase part of line or box if in visible viewport
    // Input: row, column, and flag setorrestore to plot new value (1) or restore old value (0)


    if(row>=yoffset && row<=yoffset+24 && col>=xoffset && col<=xoffset+79)
    {
        if(setorrestore==1)
        {
            VDC_Plot(row-yoffset, col-xoffset,plotscreencode,VDC_Attribute(plotcolor,plotblink, plotunderline, plotreverse,plotaltchar));
        }
        else
        {
            VDC_Plot(row-yoffset, col-xoffset,PEEKB(screenmap_screenaddr(row,col,screenwidth),1),PEEKB(screenmap_attraddr(row,col,screenwidth,screenheight),1));
        }
    }
}

void lineandbox(unsigned char draworselect)
{
    // Select line or box from upper left corner using cursor keys, ESC for cancel and ENTER for accept
    // Input: draworselect: Choose select mode (0) or draw mode (1)

    unsigned char key;
    unsigned char x,y;

    select_startx = screen_col + xoffset;
    select_starty = screen_row + yoffset;
    select_endx = select_startx;
    select_endy = select_starty;
    select_accept = 0;

    do
    {
        key = cgetc();

        switch (key)
        {
        case CH_CURS_RIGHT:
            cursormove(0,1,0,0);
            select_endx = screen_col + xoffset;
            for(y=select_starty;y<select_endy+1;y++)
            {
                plotvisible(y,select_endx,1);
            }
            break;

        case CH_CURS_LEFT:
            if(select_endx>select_startx)
            {
                cursormove(1,0,0,0);
                for(y=select_starty;y<select_endy+1;y++)
                {
                    plotvisible(y,select_endx,0);
                }
                select_endx = screen_col + xoffset;
            }
            break;

        case CH_CURS_UP:
            if(select_endy>select_starty)
            {
                cursormove(0,0,1,0);
                for(x=select_startx;x<select_endx+1;x++)
                {
                    plotvisible(select_endy,x,0);
                }
                select_endy = screen_row + yoffset;
            }
            break;

        case CH_CURS_DOWN:
            cursormove(0,0,0,1);
            select_endy = screen_row + yoffset;
            for(x=select_startx;x<select_endx+1;x++)
            {
                plotvisible(select_endy,x,1);
            }
            break;
        
        default:
            break;
        }
    } while (key!=CH_ESC && key != CH_STOP && key != CH_ENTER);

    if(key==CH_ENTER)
    {
        select_width = select_endx-select_startx+1;
        select_height = select_endy-select_starty+1;
        undo_new(select_starty,select_startx,select_width,select_height);
    }  

    if(key==CH_ENTER && draworselect ==1)
    {
        for(y=select_starty;y<select_endy+1;y++)
        {
            BankMemSet(screenmap_screenaddr(y,select_startx,screenwidth),1,plotscreencode,select_width);
            BankMemSet(screenmap_attraddr(y,select_startx,screenwidth,screenheight),1,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse,plotaltchar),select_width);
        }
    }
    else
    {
        VDC_CopyViewPortToVDC(SCREENMAPBASE,1,screenwidth,screenheight,xoffset,yoffset,0,0,80,25);
        if(key==CH_ENTER) { select_accept=1; }
    }
}

void movemode()
{
    // Function to move the 80x25 viewport

    unsigned char key,y;
    unsigned char moved = 0;

    cursor(0);
    VDC_Plot(screen_row,screen_col,PEEKB(screenmap_screenaddr(yoffset+screen_row,xoffset+screen_col,screenwidth),1),PEEKB(screenmap_attraddr(yoffset+screen_row,xoffset+screen_col,screenwidth,screenheight),1));

    if(undoenabled == 1) { undo_new(0,0,80,25); }

    do
    {
        key = cgetc();

        switch (key)
        {
        case CH_CURS_RIGHT:
            VDC_ScrollMove(0,0,80,25,2);
            VDC_VChar(0,0,CH_SPACE,25,VDC_WHITE);
            moved=1;
            break;
        
        case CH_CURS_LEFT:
            VDC_ScrollMove(0,0,80,25,1);
            VDC_VChar(0,79,CH_SPACE,25,VDC_WHITE);
            moved=1;
            break;

        case CH_CURS_UP:
            VDC_ScrollMove(0,0,80,25,8);
            VDC_HChar(24,0,CH_SPACE,80,VDC_WHITE);
            moved=1;
            break;
        
        case CH_CURS_DOWN:
            VDC_ScrollMove(0,0,80,25,4);
            VDC_HChar(0,0,CH_SPACE,80,VDC_WHITE);
            moved=1;
            break;
        
        default:
            break;
        }
    } while (key != CH_ENTER && key != CH_ESC && key != CH_STOP);

    if(moved==1)
    {
        if(key==CH_ENTER)
        {
            for(y=0;y<25;y++)
            {
                VDC_CopyVDCToMem(VDCBASETEXT+(y*80),screenmap_screenaddr(y+yoffset,xoffset,screenwidth),1,80);
                VDC_CopyVDCToMem(VDCBASEATTR+(y*80),screenmap_attraddr(y+yoffset,xoffset,screenwidth,screenheight),1,80);
            }
        }
        VDC_CopyViewPortToVDC(SCREENMAPBASE,1,screenwidth,screenheight,xoffset,yoffset,0,0,80,25);
    }
    else
    {
        if(undoenabled == 1) { undo_escapeundo(); }
    }

    cursor(1);
    VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
}

void selectmode()
{
    // Function to select a screen area to delete, cut, copy or paint

    unsigned char key,movekey,x,y,ycount;

    movekey = 0;
    lineandbox(0);
    if(select_accept == 0) { return; }

    do
    {
        key=cgetc();
    } while (key !='d' && key !='x' && key !='c' && key != 'p' && key !='a' && key != CH_ESC && key != CH_STOP );

    if(key!=CH_ESC && key != CH_STOP)
    {
        if((key=='x' || key=='c')&&(select_width>4096))
        {
            messagepopup("Selection too big.",1);
            return;
        }

        if(key=='x' || key=='c')
        {
            do
            {
                movekey = cgetc();

                switch (movekey)
                {
                // Cursor move
                case CH_CURS_LEFT:
                case CH_CURS_RIGHT:
                case CH_CURS_UP:
                case CH_CURS_DOWN:
                    plotmove(movekey);
                    break;

                default:
                    break;
                }
            } while (movekey != CH_ESC && movekey != CH_STOP && movekey != CH_ENTER);

            if(movekey==CH_ENTER)
            {
                if((screen_col+xoffset+select_width>screenwidth) || (screen_row+yoffset+select_height>screenheight))
                {
                    messagepopup("Selection does not fit.",1);
                    return;
                }

                for(ycount=0;ycount<select_height;ycount++)
                {
                    y=(screen_row+yoffset>=select_starty)? select_height-ycount-1 : ycount;
                    VDC_CopyMemToVDC(VDCSWAPTEXT,screenmap_screenaddr(select_starty+y,select_startx,screenwidth),1, select_width);
                    if(key=='x') { BankMemSet(screenmap_screenaddr(select_starty+y,select_startx,screenwidth),1,CH_SPACE,select_width); }
                    VDC_CopyVDCToMem(VDCSWAPTEXT,screenmap_screenaddr(screen_row+yoffset+y,screen_col+xoffset,screenwidth),1,select_width);
                    VDC_CopyMemToVDC(VDCSWAPTEXT,screenmap_attraddr(select_starty+y,select_startx,screenwidth,  screenheight),1,select_width);
                    if(key=='x') { BankMemSet(screenmap_attraddr(select_starty+y,select_startx,screenwidth,screenheight),1,VDC_WHITE,select_width); }
                    VDC_CopyVDCToMem(VDCSWAPTEXT,screenmap_attraddr(screen_row+yoffset+y,screen_col+xoffset,screenwidth,  screenheight),1,select_width);
                }
            }
        }

        if( key=='d' && movekey!=CH_ESC && movekey != CH_STOP)
        {
            for(y=0;y<select_height;y++)
            {
                BankMemSet(screenmap_screenaddr(select_starty+y,select_startx,screenwidth),1,CH_SPACE,select_width);
                BankMemSet(screenmap_attraddr(select_starty+y,select_startx,screenwidth,screenheight),1,VDC_WHITE,select_width);
            }
        }

        if(key=='a')
        {
            for(y=0;y<select_height;y++)
            {
                BankMemSet(screenmap_attraddr(select_starty+y,select_startx,screenwidth,screenheight),1,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar),select_width);
            }
        }

        if(key=='p')
        {
            for(y=0;y<select_height;y++)
            {
                for(x=0;x<select_width;x++)
                {
                    POKEB(screenmap_attraddr(select_starty+y,select_startx+x,screenwidth,screenheight),1,(PEEKB(screenmap_attraddr(select_starty+y,select_startx+x,screenwidth,screenheight),1) & 0xf0)+plotcolor);
                }
            }
        }

        VDC_CopyViewPortToVDC(SCREENMAPBASE,1,screenwidth,screenheight,xoffset,yoffset,0,0,80,25);
    }
    else
    {
        undo_escapeundo();
    }
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
    char* ptrend;

    char_altorstd = plotaltchar;
    char_screencode = plotscreencode;
    char_address = charaddress(char_screencode, char_altorstd,0);
    charsetchanged[plotaltchar]=1;

    // Load system charset if needed in charset not edited
    if(plotaltchar==0 && charsetchanged[1] ==1)
    {
        VDC_RedefineCharset(CHARSETSYSTEM,1,VDCCHARALT,256);
    }
    if(plotaltchar==1 && charsetchanged[0] ==1)
    {
        VDC_RedefineCharset(CHARSETSYSTEM,1,VDCCHARSTD,256);
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
            char_address = charaddress(char_screencode,char_altorstd,0);
            for(y=0;y<8;y++)
            {
                char_present[y]=VDC_Peek(char_address+y);
                char_undo[y]=char_present[y];
            }
            showchareditgrid(char_screencode, char_altorstd);
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
            if(char_altorstd==0)
            {
                VDC_RedefineCharset(CHARSETNORMAL,1,VDCCHARSTD,256);
                VDC_RedefineCharset(CHARSETSYSTEM,1,VDCCHARALT,256);
            }
            else
            {
                VDC_RedefineCharset(CHARSETALTERNATE,1,VDCCHARALT,256);
                VDC_RedefineCharset(CHARSETSYSTEM,1,VDCCHARSTD,256);
            }
            charsetchanged[char_altorstd]=1;
            windowrestore(0);
            showchareditfield(char_altorstd);
            showchareditgrid(char_screencode, char_altorstd);
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

        default:
            break;
        }
    } while (key != CH_ESC && key != CH_STOP);

    windowrestore(0);

    if(char_altorstd==0)
    {
        VDC_RedefineCharset(CHARSETALTERNATE,1,VDCCHARALT,256);
    }
    else
    {
        VDC_RedefineCharset(CHARSETNORMAL,1,VDCCHARSTD,256);
    }

    textcolor(vdctoconiocol[plotcolor]);
    gotoxy(screen_col,screen_row);
}

void resizewidth()
{
    // Function to resize screen canvas width

    unsigned int newwidth = screenwidth;
    unsigned int maxsize = MEMORYLIMIT - SCREENMAPBASE;
    unsigned char areyousure = 0;
    unsigned char sizechanged = 0;
    unsigned int y;
    char* ptrend;

    windownew(20,5,12,40,0);

    VDC_PrintAt(6,21,"Resize canvas width",mc_menupopup+VDC_A_UNDERLINE);
    VDC_PrintAt(8,21,"Enter new width:",mc_menupopup);

    sprintf(buffer,"%i",screenwidth);
    textInput(21,9,buffer,4);
    newwidth = (unsigned int)strtol(buffer,&ptrend,10);

    if((newwidth*screenheight*2) + 48 > maxsize || newwidth<80 )
    {
        VDC_PrintAt(11,21,"New size unsupported. Press key.",mc_menupopup);
        cgetc();
    }
    else
    {
        if(newwidth < screenwidth)
        {
            VDC_PrintAt(11,21,"Shrinking might delete data.",mc_menupopup);
            VDC_PrintAt(12,21,"Are you sure?",mc_menupopup);
            areyousure = menupulldown(25,13,5,0);
            if(areyousure==1)
            {
                for(y=1;y<screenheight;y++)
                {
                    VDC_CopyMemToVDC(VDCSWAPTEXT,screenmap_screenaddr(y,0,screenwidth),1,newwidth);
                    VDC_CopyVDCToMem(VDCSWAPTEXT,screenmap_screenaddr(y,0,newwidth),1,newwidth);
                }
                for(y=0;y<screenheight;y++)
                {
                    VDC_CopyMemToVDC(VDCSWAPTEXT,screenmap_attraddr(y,0,screenwidth,screenheight),1,newwidth);
                    VDC_CopyVDCToMem(VDCSWAPTEXT,screenmap_attraddr(y,0,newwidth,screenheight),1,newwidth);
                }
                if(screen_col>newwidth-1) { screen_col=newwidth-1; }
                sizechanged = 1;
            }
        }
        if(newwidth > screenwidth)
        {
            for(y=0;y<screenheight;y++)
            {
                VDC_CopyMemToVDC(VDCSWAPTEXT,screenmap_attraddr(screenheight-y-1,0,screenwidth,screenheight),1,screenwidth);
                VDC_CopyVDCToMem(VDCSWAPTEXT,screenmap_attraddr(screenheight-y-1,0,newwidth,screenheight),1,screenwidth);
                BankMemSet(screenmap_attraddr(screenheight-y-1,screenwidth,newwidth,screenheight),1,VDC_WHITE,newwidth-screenwidth);
            }
            for(y=0;y<screenheight;y++)
            {
                VDC_CopyMemToVDC(VDCSWAPTEXT,screenmap_screenaddr(screenheight-y-1,0,screenwidth),1,screenwidth);
                VDC_CopyVDCToMem(VDCSWAPTEXT,screenmap_screenaddr(screenheight-y-1,0,newwidth),1,screenwidth);
                BankMemSet(screenmap_screenaddr(screenheight-y-1,screenwidth,newwidth),1,CH_SPACE,newwidth-screenwidth);
            }
            sizechanged = 1;
        }
    }

    windowrestore(0);

    if(sizechanged==1)
    {
        screenwidth = newwidth;
        screentotal = screenwidth * screenheight;
        xoffset = 0;
        placesignature();
        VDC_CopyViewPortToVDC(SCREENMAPBASE,1,screenwidth,screenheight,xoffset,yoffset,0,0,80,25);
        sprintf(pulldownmenutitles[0][0],"Width:   %5i ",screenwidth);
        menuplacebar();
        undo_undopossible=0;
        undo_redopossible=0;
    }
}

void resizeheight()
{
    // Function to resize screen camvas height

    unsigned int newheight = screenheight;
    unsigned int maxsize = MEMORYLIMIT - SCREENMAPBASE;
    unsigned char areyousure = 0;
    unsigned char sizechanged = 0;
    unsigned char y;
    char* ptrend;

    windownew(20,5,12,40,0);

    VDC_PrintAt(6,21,"Resize canvas height",mc_menupopup+VDC_A_UNDERLINE);
    VDC_PrintAt(8,21,"Enter new height:",mc_menupopup);

    sprintf(buffer,"%i",screenheight);
    textInput(21,9,buffer,4);
    newheight = (unsigned int)strtol(buffer,&ptrend,10);

    if((newheight*screenwidth*2) + 48 > maxsize || newheight < 25)
    {
        VDC_PrintAt(11,21,"New size unsupported. Press key.",mc_menupopup);
        cgetc();
    }
    else
    {
        if(newheight < screenheight)
        {
            VDC_PrintAt(11,21,"Shrinking might delete data.",mc_menupopup);
            VDC_PrintAt(12,21,"Are you sure?",mc_menupopup);
            areyousure = menupulldown(25,13,5,0);
            if(areyousure==1)
            {
                BankMemCopy(screenmap_attraddr(0,0,screenwidth,screenheight),1,screenmap_attraddr(0,0,screenwidth,newheight),1,screenheight*screenwidth);
                if(screen_row>newheight-1) { screen_row=newheight-1; }
                sizechanged = 1;
            }
        }
        if(newheight > screenheight)
        {
            for(y=0;y<screenheight;y++)
            {
                BankMemCopy(screenmap_attraddr(screenheight-y-1,0,screenwidth,screenheight),1,screenmap_attraddr(screenheight-y-1,0,screenwidth,newheight),1,screenwidth);
            }
            BankMemSet(screenmap_attraddr(screenheight,0,screenwidth,newheight),1,VDC_WHITE,(newheight-screenheight)*screenwidth);
            BankMemSet(screenmap_screenaddr(screenheight,0,screenwidth),1,CH_SPACE,(newheight-screenheight)*screenwidth);
            sizechanged = 1;
        }
    }

    windowrestore(0);

    if(sizechanged==1)
    {
        screenheight = newheight;
        screentotal = screenwidth * screenheight;
        yoffset=0;
        placesignature();
        VDC_CopyViewPortToVDC(SCREENMAPBASE,1,screenwidth,screenheight,xoffset,yoffset,0,0,80,25);
        sprintf(pulldownmenutitles[0][1],"Height:  %5i ",screenheight);
        menuplacebar();
        undo_undopossible=0;
        undo_redopossible=0;
    }
}

void changebackgroundcolor()
{
    // Function to change background color

    unsigned char key;
    unsigned char newcolor = screenbackground;
    unsigned char changed = 0;

    windownew(20,5,12,40,0);

    VDC_PrintAt(6,21,"Change background color",mc_menupopup+VDC_A_UNDERLINE);
    sprintf(buffer,"Color: %2i",newcolor);
    VDC_PrintAt(8,21,buffer,mc_menupopup);
    VDC_PrintAt(10,21,"Press:",mc_menupopup);
    VDC_PrintAt(11,21,"+:     Increase color number",mc_menupopup);
    VDC_PrintAt(12,21,"-:     Decrease color number",mc_menupopup);
    VDC_PrintAt(13,21,"ENTER: Accept color",mc_menupopup);
    VDC_PrintAt(14,21,"ESC:   Cancel",mc_menupopup);

    do
    {
        do
        {
            key = cgetc();
        } while (key != CH_ENTER && key != CH_ESC && key !=CH_STOP && key != '+' && key != '-');

        switch (key)
        {
        case '+':
            newcolor++;
            if(newcolor>15) { newcolor = 0; }
            changed=1;
            break;

        case '-':
            if(newcolor==0) { newcolor = 15; } else { newcolor--; }
            changed=1;
            break;
        
        case CH_ESC:
        case CH_STOP:
            changed=0;
            VDC_BackColor(screenbackground);
            break;

        default:
            break;
        }

        if(changed == 1)
        {
            VDC_BackColor(newcolor);
            sprintf(buffer,"Color: %2i",newcolor);
            VDC_PrintAt(8,21,buffer,mc_menupopup);
        }
    } while (key != CH_ENTER && key != CH_ESC && key != CH_STOP );
    
    if(changed=1)
    {
        screenbackground=newcolor;

        // Change menu palette based on background color

        // Default palette if black or dark grey background
        if(screenbackground==VDC_BLACK || screenbackground==VDC_DGREY)
        {
            mc_mb_normal = VDC_LGREEN + VDC_A_REVERSE + VDC_A_ALTCHAR;
            mc_mb_select = VDC_WHITE + VDC_A_REVERSE + VDC_A_ALTCHAR;
            mc_pd_normal = VDC_LCYAN + VDC_A_REVERSE + VDC_A_ALTCHAR;
            mc_pd_select = VDC_LYELLOW + VDC_A_REVERSE + VDC_A_ALTCHAR;
            mc_menupopup = VDC_WHITE + VDC_A_REVERSE + VDC_A_ALTCHAR;
        }
        else
        {
            // Palette for background colors with intensity bit enabled
            if(screenbackground & 0x01)
            {
                mc_mb_normal = VDC_BLACK + VDC_A_REVERSE + VDC_A_ALTCHAR;
                mc_mb_select = VDC_BLACK + VDC_A_ALTCHAR;
                mc_pd_normal = VDC_BLACK + VDC_A_REVERSE + VDC_A_ALTCHAR;
                mc_pd_select = VDC_BLACK + VDC_A_ALTCHAR;
                mc_menupopup = VDC_BLACK + VDC_A_REVERSE + VDC_A_ALTCHAR;
            }
            // Palette for background color with intensity bit disabled if not black/dgrey
            else
            {
                mc_mb_normal = VDC_WHITE + VDC_A_REVERSE + VDC_A_ALTCHAR;
                mc_mb_select = VDC_WHITE + VDC_A_ALTCHAR;
                mc_pd_normal = VDC_WHITE + VDC_A_REVERSE + VDC_A_ALTCHAR;
                mc_pd_select = VDC_WHITE + VDC_A_ALTCHAR;
                mc_menupopup = VDC_WHITE + VDC_A_REVERSE + VDC_A_ALTCHAR;
            }
        }
                
        sprintf(pulldownmenutitles[0][2],"Background: %2i ",screenbackground);
    }
    
    windowrestore(0);    
}

void chooseidandfilename(char* headertext, unsigned char maxlen)
{
    // Function to present dialogue to enter device id and filename
    // Input: Headertext to print, maximum length of filename input string

    unsigned char newtargetdevice;
    unsigned char valid = 0;
    char* ptrend;

    windownew(20,5,12,40,0);
    VDC_PrintAt(6,21,headertext,mc_menupopup+VDC_A_UNDERLINE);
    do
    {
        VDC_PrintAt(8,21,"Choose drive ID:",mc_menupopup);
        sprintf(buffer,"%u",targetdevice);
        textInput(21,9,buffer,2);
        newtargetdevice = (unsigned char)strtol(buffer,&ptrend,10);
        if(newtargetdevice > 7 && newtargetdevice<31)
        {
            valid = 1;
            targetdevice=newtargetdevice;
        }
        else{
            VDC_PrintAt(10,21,"Invalid ID. Enter valid one.",mc_menupopup);
        }
    } while (valid==0);
    VDC_PrintAt(10,21,"Choose filename:            ",mc_menupopup);
    textInput(21,11,filename,maxlen);
}

unsigned char checkiffileexists(char* filetocheck, unsigned char id)
{
    // Check if file exists and, if yes, ask confirmation of overwrite
    
    unsigned char proceed = 1;
    unsigned char yesno;
    unsigned char error;

    sprintf(buffer,"r0:%s=%s",filetocheck,filetocheck);
    error = cmd(id,buffer);

    if (error == 63)
    {
        yesno = areyousure("File exists.",0);
        if(yesno==2)
        {
            proceed = 0;
        }
    }
    cbm_close(2);

    return proceed;
}

void loadscreenmap()
{
    // Function to load screenmap

    unsigned int lastreadaddress, newwidth, newheight;
    unsigned int maxsize = MEMORYLIMIT - SCREENMAPBASE;
    char* ptrend;
  
    chooseidandfilename("Load screen",15);

    VDC_PrintAt(12,21,"Enter screen width:",mc_menupopup);
    sprintf(buffer,"%i",screenwidth);
    textInput(21,13,buffer,4);
    newwidth = (unsigned int)strtol(buffer,&ptrend,10);

    VDC_PrintAt(14,21,"Enter screen height:",mc_menupopup);
    sprintf(buffer,"%i",screenheight);
    textInput(21,15,buffer,4);
    newheight = (unsigned int)strtol(buffer,&ptrend,10);

    if((newwidth*newheight*2) + 48 > maxsize || newwidth<80 || newheight<25)
    {
        VDC_PrintAt(16,21,"New size unsupported. Press key.",mc_menupopup);
        cgetc();
        windowrestore(0);
    }
    else
    {
        windowrestore(0);

        lastreadaddress = VDC_LoadScreen(filename,targetdevice,SCREENMAPBASE,1);

        if(lastreadaddress>SCREENMAPBASE)
        {
            windowrestore(0);
            screenwidth = newwidth;
            screenheight = newheight;
            VDC_CopyViewPortToVDC(SCREENMAPBASE,1,screenwidth,screenheight,xoffset,yoffset,0,0,80,25);
            windowsave(0,1,0);
            menuplacebar();
            undo_undopossible=0;
            undo_redopossible=0;
        }
    }
}

void savescreenmap()
{
    // Function to save screenmap

    unsigned char error;
  
    chooseidandfilename("Save screen",15);

    windowrestore(0);

    if(checkiffileexists(filename,targetdevice)==1)
    {
        // Scratch old file
        sprintf(buffer,"s:%s",filename);
        cmd(targetdevice,buffer);

        // Set device ID
	    cbm_k_setlfs(0, targetdevice, 0);
    
	    // Set filename
	    cbm_k_setnam(filename);
    
	    // Set bank
	    SetLoadSaveBank(1);
    
	    // Load from file to memory
	    error = cbm_k_save(SCREENMAPBASE,SCREENMAPBASE+(screenwidth*screenheight*2)+48);
    
	    // Restore I/O bank to 0
	    SetLoadSaveBank(0);
    
        if(error) { fileerrormessage(error,0); }
    }
}

void saveproject()
{
    // Function to save project (screen, charsets and metadata)

    unsigned char error;
    unsigned char projbuffer[22];

    chooseidandfilename("Save project",10);

    sprintf(buffer,"%s.proj",filename);

    windowrestore(0);

    if(checkiffileexists(buffer,targetdevice)==1)
    {
        // Scratch old files
        sprintf(buffer,"s:%s.proj",filename);
        cmd(targetdevice,buffer);
        sprintf(buffer,"s:%s.scrn",filename);
        cmd(targetdevice,buffer);
        sprintf(buffer,"s:%s.chrs",filename);
        cmd(targetdevice,buffer);
        sprintf(buffer,"s:%s.chra",filename);
        cmd(targetdevice,buffer);

        // Store project data to buffer variable
	    SetLoadSaveBank(0);
        projbuffer[ 0] = charsetchanged[0];
        projbuffer[ 1] = charsetchanged[1];
        projbuffer[ 2] = screen_col;
        projbuffer[ 3] = screen_row;
        projbuffer[ 4] = (screenwidth>>8) & 0xff;
        projbuffer[ 5] = screenwidth & 0xff;
        projbuffer[ 6] = (screenheight>>8) & 0xff;
        projbuffer[ 7] = screenheight & 0xff;
        projbuffer[ 8] = (screentotal>>8) & 0xff;
        projbuffer[ 9] = screentotal & 0xff;
        projbuffer[10] = screenbackground;
        projbuffer[11] = mc_mb_normal;
        projbuffer[12] = mc_mb_select;
        projbuffer[13] = mc_pd_normal;
        projbuffer[14] = mc_pd_select;
        projbuffer[15] = mc_menupopup;
        projbuffer[16] = plotscreencode;
        projbuffer[17] = plotcolor;
        projbuffer[18] = plotreverse;
        projbuffer[19] = plotunderline;
        projbuffer[20] = plotblink;
        projbuffer[21] = plotaltchar;
	    cbm_k_setlfs(0, targetdevice, 0);
        sprintf(buffer,"%s.proj",filename);
	    cbm_k_setnam(buffer);
	    error = cbm_k_save((unsigned int)projbuffer,(unsigned int)projbuffer+22);
        if(error) { fileerrormessage(error,0); }

        // Store screen data
        SetLoadSaveBank(1);
        cbm_k_setlfs(0, targetdevice, 0);
        sprintf(buffer,"%s.scrn",filename);
	    cbm_k_setnam(buffer);
	    error = cbm_k_save(SCREENMAPBASE,SCREENMAPBASE+(screenwidth*screenheight*2)+48);
        if(error) { fileerrormessage(error,0); }

        // Store standard charset
        if(charsetchanged[0]==1)
        {
            cbm_k_setlfs(0, targetdevice, 0);
            sprintf(buffer,"%s.chrs",filename);
	        cbm_k_setnam(buffer);
	        error = cbm_k_save(CHARSETNORMAL,CHARSETNORMAL+256*8);
            if(error) { fileerrormessage(error,0); }
        }

        // Store alternate charset
        if(charsetchanged[1]==1)
        {
            cbm_k_setlfs(0, targetdevice, 0);
            sprintf(buffer,"%s.chra",filename);
	        cbm_k_setnam(buffer);
	        error = cbm_k_save(CHARSETALTERNATE,CHARSETALTERNATE+256*8);
            if(error) { fileerrormessage(error,0); }
        }
    
	    // Restore I/O bank to 0
	    SetLoadSaveBank(0);        
    }
}

void loadproject()
{
    // Function to load project (screen, charsets and metadata)

    unsigned int lastreadaddress;
    unsigned char projbuffer[22];

    chooseidandfilename("Load project",10);

    windowrestore(0);

    // Load project variables
    sprintf(buffer,"%s.proj",filename);
	cbm_k_setlfs(0,targetdevice, 0);
	cbm_k_setnam(buffer);
	SetLoadSaveBank(0);
	lastreadaddress = cbm_k_load(0,(unsigned int)projbuffer);
    if(lastreadaddress<=(unsigned int)projbuffer) { return; }
    charsetchanged[0]       = projbuffer[ 0];
    charsetchanged[1]       = projbuffer[ 1];
    screen_col              = projbuffer[ 2];
    screen_row              = projbuffer[ 3];
    screenwidth             = projbuffer[ 4]*256+projbuffer[ 5];
    screenheight            = projbuffer[ 6]*256+projbuffer [7];
    screentotal             = projbuffer[ 8]*256+projbuffer[ 9];
    screenbackground        = projbuffer[10];
    mc_mb_normal            = projbuffer[11];
    mc_mb_select            = projbuffer[12];
    mc_pd_normal            = projbuffer[13];
    mc_pd_select            = projbuffer[14];
    mc_menupopup            = projbuffer[15];
    plotscreencode          = projbuffer[16];
    plotcolor               = projbuffer[17];
    plotreverse             = projbuffer[18];
    plotunderline           = projbuffer[19];
    plotblink               = projbuffer[20];
    plotaltchar             = projbuffer[21];

    // Load screen
    sprintf(buffer,"%s.scrn",filename);
    lastreadaddress = VDC_LoadScreen(buffer,targetdevice,SCREENMAPBASE,1);
    if(lastreadaddress>SCREENMAPBASE)
    {
        windowrestore(0);
        VDC_CopyViewPortToVDC(SCREENMAPBASE,1,screenwidth,screenheight,xoffset,yoffset,0,0,80,25);
        windowsave(0,1,0);
        menuplacebar();
        undo_undopossible=0;
        undo_redopossible=0;
    }

    // Load standard charset
    if(charsetchanged[0]==1)
    {
        sprintf(buffer,"%s.chrs",filename);
        VDC_LoadCharset(buffer,targetdevice,CHARSETNORMAL,1,1);
    }

    // Load standard charset
    if(charsetchanged[1]==1)
    {
        sprintf(buffer,"%s.chra",filename);
        VDC_LoadCharset(buffer,targetdevice,CHARSETALTERNATE,1,2);
    }
}

void loadcharset(unsigned char stdoralt)
{
    // Function to load charset
    // Input: stdoralt: standard charset (0) or alternate charset (1)

    unsigned int lastreadaddress, charsetaddress;
  
    chooseidandfilename("Load character set",15);

    charsetaddress = (stdoralt==0)? CHARSETNORMAL : CHARSETALTERNATE;

    windowrestore(0);

    lastreadaddress = VDC_LoadCharset(filename,targetdevice,charsetaddress,1,0);

    if(lastreadaddress>charsetaddress)
    {
        if(stdoralt==0)
        {
            VDC_RedefineCharset(charsetaddress,1,VDCCHARSTD,256);
        }
        charsetchanged[stdoralt]=1;
    }
}

void savecharset(unsigned char stdoralt)
{
    // Function to save charset
    // Input: stdoralt: standard charset (0) or alternate charset (1)

    unsigned char error;
    unsigned int charsetaddress;
  
    chooseidandfilename("Save character set",15);

    charsetaddress = (stdoralt==0)? CHARSETNORMAL : CHARSETALTERNATE;

    windowrestore(0);

    if(checkiffileexists(filename,targetdevice)==1)
    {
        // Scratch old file
        sprintf(buffer,"s:%s",filename);
        cmd(targetdevice,buffer);

        // Set device ID
	    cbm_k_setlfs(0, targetdevice, 0);

	    // Set filename
	    cbm_k_setnam(filename);

	    // Set bank
	    SetLoadSaveBank(1);
    
	    // Load from file to memory
	    error = cbm_k_save(charsetaddress,charsetaddress+256*8);

	    // Restore I/O bank to 0
	    SetLoadSaveBank(0);

        if(error) { fileerrormessage(error,0); }
    }
}

void versioninfo()
{
    windownew(5,5,15,60,1);
    VDC_PrintAt(6,6,"Version information and credits",mc_menupopup+VDC_A_UNDERLINE);
    VDC_PrintAt(8,6,"VDC Screen Editor",mc_menupopup);
    VDC_PrintAt(9,6,"Written in 2021 by Xander Mol",mc_menupopup);
    sprintf(buffer,"Version: %s",version);
    VDC_PrintAt(11,6,buffer,mc_menupopup);
    VDC_PrintAt(13,6,"Full source code, documentation and credits at:",mc_menupopup);
    VDC_PrintAt(14,6,"https://github.com/xahmol/VDCScreenEdit",mc_menupopup);
    VDC_PrintAt(16,6,"(C) 2021, IDreamtIn8Bits.com",mc_menupopup);
    VDC_PrintAt(18,6,"Press a key to continue.",mc_menupopup);
    cgetc();
    windowrestore(1);
}

void helpscreen()
{
    windownew(5,5,15,70,1);
    VDC_PrintAt(6,6,"Help",mc_menupopup+VDC_A_UNDERLINE);
    VDC_PrintAt(18,6,"Press a key to continue.",mc_menupopup);
    cgetc();
    windowrestore(1);
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
            resizewidth();
            break;

        case 12:
            resizeheight();
            break;
        
        case 13:
            changebackgroundcolor();
            break;

        case 14:
            if(undoenabled == 1) { undo_new(0,0,screenwidth,screenheight); }
            screenmapfill(CH_SPACE,VDC_WHITE);
            windowrestore(0);
            VDC_CopyViewPortToVDC(SCREENMAPBASE,1,screenwidth,screenheight,xoffset,yoffset,0,0,80,25);
            windowsave(0,1,0);
            menuplacebar();
            break;
        
        case 15:
            if(undoenabled == 1) { undo_new(0,0,screenwidth,screenheight); }
            screenmapfill(plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
            windowrestore(0);
            VDC_CopyViewPortToVDC(SCREENMAPBASE,1,screenwidth,screenheight,xoffset,yoffset,0,0,80,25);
            windowsave(0,1,0);
            menuplacebar();
            break;

        case 21:
            savescreenmap();
            break;

        case 22:
            loadscreenmap();
            break;
        
        case 23:
            saveproject();
            break;
        
        case 24:
            loadproject();
            break;
        
        case 31:
            loadcharset(0);
            break;
        
        case 32:
            loadcharset(1);
            break;
        
        case 33:
            savecharset(0);
            break;

        case 34:
            savecharset(1);
            break;

        case 41:
            versioninfo();
            break;

        case 42:
            helpscreen();
            break;

        case 43:
            appexit = 1;
            menuchoice = 99;
            break;
        
        case 44:
            undoenabled = (undoenabled==0)? 1:0;
            sprintf(pulldownmenutitles[3][3],"Undo: %s",(undoenabled==1)? "Enabled  ":"Disabled ");
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
        strcpy(pulldownmenutitles[3][3],"Undo: Enabled  ");     // Enable undo menuoption
        pulldownmenuoptions[3]=4;                               // Enable undo menupotion
        undoenabled = 1;                                        // Set undo enabled flag
        undoaddress = VDCEXTENDED;                              // Reset undo address
        undonumber = 0;                                         // Reset undo number
        undo_undopossible = 0;                                  // Reset undo possible flag
        undo_redopossible = 0;                                  // Reset redo possible flag
    }

    // Copy charsets from ROM
    VDC_CopyCharsetsfromROM();

    // Load and show title screen
    if(VDC_LoadScreen("vdcse.tscr",bootdevice,SCREENMAPBASE,1)>SCREENMAPBASE)
    {
        VDC_CopyMemToVDC(VDCBASETEXT,SCREENMAPBASE,1,4048);
    }

    // Load default charsets to bank 1
    VDC_LoadCharset("vdcse.falt",bootdevice, CHARSETSYSTEM, 1, 0);
    VDC_LoadCharset("vdcse.fstd",bootdevice, CHARSETNORMAL, 1, 0);
    BankMemCopy(CHARSETSYSTEM,1,CHARSETALTERNATE,1,2048);

    // Clear screen map in bank 1 with spaces in text color white
    screenmapfill(CH_SPACE,VDC_WHITE);
 
    // Wait for key press to start application
    VDC_PrintAt(24,30,"Press key to start.",VDC_WHITE+VDC_A_ALTCHAR);
    cgetc();

    // Clear viewport of titlescreen
    clrscr();

    // Main program loop
    VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
    gotoxy(screen_col,screen_row);
    cursor(1);

    do
    {
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
            chareditor();
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
            writemode();
            break;
        
        // Color mode: type colors
        case 'c':
            colorwrite();
            break;

        // Line and box mode
        case 'l':
            lineandbox(1);
            break;

        // Move mode
        case 'm':
            movemode();
            break;

        // Select mode
        case 's':
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

        // Increase/decrease plot screencode by 128 (toggle 'RVS ON' and 'RVS OFF')
        case 'i':
            plotscreencode += 128;      // Will increase 128 if <128 and decrease by 128 if >128 by overflow
            VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
            break;        

        // Plot present screencode and attribute
        case CH_SPACE:
            if(undoenabled==1) { undo_new(screen_row,screen_col,1,1); }
            screenmapplot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
            break;

        // Delete present screencode and attributes
        case CH_DEL:
            if(undoenabled==1) { undo_new(screen_row,screen_col,1,1); }
            screenmapplot(screen_row,screen_col,CH_SPACE,VDC_WHITE);
            break;

        // Go to upper left corner
        case CH_HOME:
            screen_row = 0;
            screen_col = 0;
            yoffset = 0;
            xoffset = 0;
            VDC_CopyViewPortToVDC(SCREENMAPBASE,1,screenwidth,screenheight,xoffset,yoffset,screen_col,screen_row,80,25);
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
        
        default:
            break;
        }
    } while (appexit==0);

    cursor(0);
    textcolor(COLOR_YELLOW);
    VDC_Exit();
}
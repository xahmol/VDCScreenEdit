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

#pragma code-name ("OVERLAY3");
#pragma rodata-name ("OVERLAY3");

int chooseidandfilename(char* headertext, unsigned char maxlen)
{
    // Function to present dialogue to enter device id and filename
    // Input: Headertext to print, maximum length of filename input string

    unsigned char newtargetdevice;
    int valid = 0;
    char* ptrend;

    windownew(20,5,12,40,0);
    VDC_PrintAt(6,21,headertext,mc_menupopup+VDC_A_UNDERLINE);
    do
    {
        VDC_PrintAt(8,21,"Choose drive ID:",mc_menupopup);
        sprintf(buffer,"%u",targetdevice);
        if(textInput(21,9,buffer,2)==-1) { return -1; }
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
    return textInput(21,11,filename,maxlen);
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
        else
        {
            proceed = 2;
        }
    } else {
        if(error) {
            fileerrormessage(error,0);
            proceed = 0;
        }
    }

    return proceed;
}

void loadscreenmap()
{
    // Function to load screenmap

    unsigned int lastreadaddress, newwidth, newheight;
    unsigned int maxsize = MEMORYLIMIT - SCREENMAPBASE;
    char* ptrend;
    int escapeflag;
  
    escapeflag = chooseidandfilename("Load screen",15);

    if(escapeflag==-1) { windowrestore(0); return; }

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
            if(showbar) { initstatusbar(); }
            undo_undopossible=0;
            undo_redopossible=0;
        }
    }
}

void savescreenmap()
{
    // Function to save screenmap

    unsigned char error, overwrite;
    int escapeflag;
  
    escapeflag = chooseidandfilename("Save screen",15);

    windowrestore(0);

    if(escapeflag==-1) { return; }

    overwrite = checkiffileexists(filename,targetdevice);

    if(overwrite)
    {
        // Scratch old file
        if(overwrite==2)
        {
            sprintf(buffer,"s:%s",filename);
            cmd(targetdevice,buffer);
        }

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

    unsigned char error,overwrite;
    char projbuffer[22];
    char tempfilename[21];
    int escapeflag;
  
    escapeflag = chooseidandfilename("Save project",10);

    windowrestore(0);

    if(escapeflag==-1) { return; }

    sprintf(tempfilename,"%s.proj",filename);

    overwrite = checkiffileexists(filename,targetdevice);

    if(overwrite)
    {
        // Scratch old files
        if(overwrite==2)
        {
            sprintf(buffer,"s:%s.proj",filename);
            cmd(targetdevice,buffer);
            sprintf(buffer,"s:%s.scrn",filename);
            cmd(targetdevice,buffer);
            sprintf(buffer,"s:%s.chrs",filename);
            cmd(targetdevice,buffer);
            sprintf(buffer,"s:%s.chra",filename);
            cmd(targetdevice,buffer);
        }

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
    int escapeflag;
  
    escapeflag = chooseidandfilename("Load project",10);

    windowrestore(0);

    if(escapeflag==-1) { return; }

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
    sprintf(pulldownmenutitles[0][0],"Width:   %5i ",screenwidth);
    screenheight            = projbuffer[ 6]*256+projbuffer [7];
    sprintf(pulldownmenutitles[0][1],"Height:  %5i ",screenheight);
    screentotal             = projbuffer[ 8]*256+projbuffer[ 9];
    screenbackground        = projbuffer[10];
    VDC_BackColor(screenbackground);
    sprintf(buffer,"Color: %2i",screenbackground);
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
        if(showbar) { initstatusbar(); }
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
    int escapeflag;
  
    escapeflag = chooseidandfilename("Load character set",15);

    windowrestore(0);

    if(escapeflag==-1) { return; }

    charsetaddress = (stdoralt==0)? CHARSETNORMAL : CHARSETALTERNATE;

    lastreadaddress = VDC_LoadCharset(filename,targetdevice,charsetaddress,1,0);

    if(lastreadaddress>charsetaddress)
    {
        if(stdoralt==0)
        {
            VDC_RedefineCharset(charsetaddress,1,VDCCHARSTD,255);
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
    int escapeflag;
  
    escapeflag = chooseidandfilename("Save character set",15);

    windowrestore(0);

    if(escapeflag==-1) { return; }

    charsetaddress = (stdoralt==0)? CHARSETNORMAL : CHARSETALTERNATE;

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
            mc_pd_normal = VDC_DCYAN + VDC_A_REVERSE + VDC_A_ALTCHAR;
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
    windowrestore(0);
}

void plot_try()
{
    unsigned char key;

    strcpy(programmode,"Try");
    if(showbar) { printstatusbar(); }
    cursor(0);
    key = cgetc();
    if(key==CH_SPACE)
    {
        if(undoenabled==1) { undo_new(screen_row+yoffset,screen_col+xoffset,1,1); }
        screenmapplot(screen_row+yoffset,screen_col+xoffset,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
    }
    strcpy(programmode,"Main");
    cursor(1);
}
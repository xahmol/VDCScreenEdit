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

#pragma code-name ("OVERLAY2");
#pragma rodata-name ("OVERLAY2");

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

    if(draworselect)
    {
        strcpy(programmode,"Line/Box");
    }

    do
    {
        if(showbar) { printstatusbar(); }
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

        // Toggle statusbar
        case CH_F6:
            togglestatusbar();
            break;
        
        case CH_F8:
            if(select_startx==select_endx && select_starty==select_endy) helpscreen_load(3);
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
        VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
    }
    else
    {
        VDC_CopyViewPortToVDC(SCREENMAPBASE,1,screenwidth,screenheight,xoffset,yoffset,0,0,80,25);
        if(showbar) { initstatusbar(); }
        if(key==CH_ENTER) { select_accept=1; }
    }
    if(draworselect)
    {
        strcpy(programmode,"Main");
    }
}

void movemode()
{
    // Function to move the 80x25 viewport

    unsigned char key,y;
    unsigned char moved = 0;

    strcpy(programmode,"Move");

    cursor(0);
    VDC_Plot(screen_row,screen_col,PEEKB(screenmap_screenaddr(yoffset+screen_row,xoffset+screen_col,screenwidth),1),PEEKB(screenmap_attraddr(yoffset+screen_row,xoffset+screen_col,screenwidth,screenheight),1));
    

    if(undoenabled == 1) { undo_new(0,0,80,25); }
    if(showbar) { hidestatusbar(); }

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

        case CH_F8:
            helpscreen_load(3);
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
        if(showbar) { initstatusbar(); }
    }
    else
    {
        if(undoenabled == 1) { undo_escapeundo(); }
    }

    cursor(1);
    VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
    strcpy(programmode,"Main");
    if(showbar) { printstatusbar(); }
}

void selectmode()
{
    // Function to select a screen area to delete, cut, copy or paint

    unsigned char key,movekey,x,y,ycount;

    strcpy(programmode,"Select");

    movekey = 0;
    lineandbox(0);
    if(select_accept == 0) { return; }

    strcpy(programmode,"X/C/D/A/P?");

    do
    {
        if(showbar) { printstatusbar(); }
        key=cgetc();

        // Toggle statusbar
        if(key==CH_F6)
        {
            togglestatusbar();
        }

        if(key==CH_F8) { helpscreen_load(3); }

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
            if(key=='x')
            {
                strcpy(programmode,"Cut");
            }
            else
            {
                strcpy(programmode,"Copy");
            }
            do
            {
                if(showbar) { printstatusbar(); }
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

                case CH_F8:
                    helpscreen_load(3);
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

                if(key=='c' ) { undo_escapeundo(); }
                undo_new(screen_row+yoffset,screen_col+xoffset,select_width,select_height);
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

        if( key=='d')
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
        if(showbar) { initstatusbar(); }
        VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
    }
    else
    {
        undo_escapeundo();
    }
    strcpy(programmode,"Main");
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
        if(showbar) { initstatusbar(); }
        undo_undopossible=0;
        undo_redopossible=0;
    }
}
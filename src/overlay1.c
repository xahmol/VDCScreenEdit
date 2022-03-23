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

#pragma code-name ("OVERLAY1");
#pragma rodata-name ("OVERLAY1");

void writemode()
{
    // Write mode with screencodes

    unsigned char key, screencode;
    unsigned char rvs = 0;

    strcpy(programmode,"Write");

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
            if(undoenabled == 1) { undo_new(screen_row+yoffset,screen_col+xoffset,1,1); }
            screenmapplot(screen_row+yoffset,screen_col+xoffset,CH_SPACE,VDC_WHITE);
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

        // Toggle statusbar
        case CH_F6:
            togglestatusbar();
            break;

        case CH_F8:
            helpscreen_load(4);
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
    strcpy(programmode,"Main");
}

void colorwrite()
{
    // Write mode with colors

    unsigned char key, attribute;

    strcpy(programmode,"Colorwrite");
    do
    {
        if(showbar) { printstatusbar(); }
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
        
        // Toggle statusbar
        case CH_F6:
            togglestatusbar();
            break;

        case CH_F8:
            helpscreen_load(4);
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
    strcpy(programmode,"Main");
}

void palette_draw()
{
    // Draw window for character palette

    unsigned char attribute = mc_menupopup-VDC_A_ALTCHAR;
    unsigned char x,y;
    unsigned char counter = 0;
    unsigned int petsciiaddress = PETSCIIMAP;

    windowsave(0,21,0);
    VDC_FillArea(0,45,CH_SPACE,34,21,attribute);
    textcolor(vdctoconiocol[mc_menupopup & 0x0f]);

    // Set coordinate of present char if no visual map
    if(visualmap==0)
    {
        rowsel = palettechar/32 + plotaltchar*9 + 2;
        colsel = palettechar%32;
    }

    // Favourites palette
    for(x=0;x<10;x++)
    {
        VDC_Plot(1,46+x,favourites[x][0],attribute+favourites[x][1]*VDC_A_ALTCHAR);
    }

    // Full charsets
    for(y=0;y<8;y++)
    {
        for(x=0;x<32;x++)
        {
            if(visualmap)
            {
                VDC_Plot( 3+y,46+x,PEEK(petsciiaddress),attribute);
                VDC_Plot(12+y,46+x,PEEK(petsciiaddress),attribute+VDC_A_ALTCHAR);
                if(PEEK(petsciiaddress++)==palettechar)
                {
                    rowsel = y+2+(9*plotaltchar);
                    colsel = x;
                }
            }
            else
            {
                VDC_Plot( 3+y,46+x,counter,attribute);
                VDC_Plot(12+y,46+x,counter,attribute+VDC_A_ALTCHAR);
            }
            counter++;
        }
    }
}

void palette_returnscreencode()
{
    // Get screencode from palette position

    if(rowsel==0)
    {
        palettechar = favourites[colsel][0];
        plotaltchar = favourites[colsel][1];
    }
    if(rowsel>1 && rowsel<10)
    {
        if(visualmap)
        {
            palettechar = PEEK(PETSCIIMAP+colsel + (rowsel-2)*32);
        }
        else
        {
            palettechar = colsel + (rowsel-2)*32;
        }
        
        plotaltchar = 0;
    }
    if(rowsel>10)
    {
        if(visualmap)
        {
            palettechar = PEEK(PETSCIIMAP+colsel + (rowsel-11)*32);
        }
        else
        {
            palettechar = colsel + (rowsel-11)*32;
        }

        plotaltchar = 1;
    }
}

void palette()
{
    // Show character set palette

    unsigned char attribute = mc_menupopup-VDC_A_ALTCHAR;
    unsigned char key;

    palettechar = plotscreencode;

    strcpy(programmode,"Palette");

    palette_draw();
    gotoxy(46+colsel,1+rowsel);

    // Get key loop
    do
    {
        if(showbar) { printstatusbar(); }
        key=cgetc();

        switch (key)
        {
        // Cursor movemeny
        case CH_CURS_RIGHT:       
        case CH_CURS_LEFT:
        case CH_CURS_DOWN:
        case CH_CURS_UP:
            if(key==CH_CURS_RIGHT) { colsel++; }
            if(key==CH_CURS_LEFT)
            {
                if(colsel>0)
                {
                    colsel--;
                }
                else
                {
                    colsel=31;
                    if(rowsel>0)
                    {
                        rowsel--;
                        if(rowsel==1 || rowsel==10) { rowsel--;}
                        if(rowsel==0) { colsel=9; }
                    }
                    else
                    {
                        rowsel=18;
                    }
                }
            }
            if(key==CH_CURS_DOWN) { rowsel++; }
            if(key==CH_CURS_UP)
            {
                if(rowsel>0)
                {
                    rowsel--;
                    if(rowsel==1 || rowsel==10) { rowsel--;}
                }
                else
                {
                    rowsel=18;
                }
            }
            if(colsel>9 && rowsel==0) { colsel=0; rowsel=2; }
            if(colsel>31) { colsel=0; rowsel++; }
            if(rowsel>18) { rowsel=0; }
            if(rowsel==1 || rowsel==10) { rowsel++;}
            gotoxy(46+colsel,1+rowsel);
            break;

        // Select character
        case CH_SPACE:
        case CH_ENTER:
            palette_returnscreencode();
            plotscreencode = palettechar;
            key = CH_ESC;
            break;

        // Toggle visual PETSCII map
        case 'v':
            windowrestore(0);
            palette_returnscreencode();
            visualmap = (visualmap)?0:1;
            palette_draw();
            gotoxy(46+colsel,1+rowsel);
            break;

        // Toggle statusbar
        case CH_F6:
            togglestatusbar();
            break;

        // Help screen
        case CH_F8:
            windowrestore(0);
            helpscreen_load(2);
            palette_draw();
            break;
        
        default:
            // Store in favourites
            if(key>47 && key<58 && rowsel>0)
            {
                palette_returnscreencode();
                favourites[key-48][0] = palettechar;
                if(rowsel<10)
                {
                    favourites[key-48][1] = 0;
                }
                else
                {
                    favourites[key-48][1] = 1;
                }
                VDC_Plot(1,key-2,favourites[key-48][0],attribute+favourites[key-48][1]*VDC_A_ALTCHAR);
            }
            break;
        }
    } while (key != CH_ESC && key != CH_STOP);

    windowrestore(0);
    textcolor(vdctoconiocol[plotcolor]);
    gotoxy(screen_col,screen_row);
    VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
    strcpy(programmode,"Main");
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
        if(showbar) { initstatusbar(); }
        undo_undopossible=0;
        undo_redopossible=0;
    }
}

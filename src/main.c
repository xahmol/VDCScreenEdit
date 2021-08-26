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
char menubartitles[4][12] = {"Game","Disc","Music","Information"};
unsigned char menubarcoords[4] = {1,6,11,17};
unsigned char pulldownmenuoptions[8] = {3,3,3,1,2,2,3,5};
char pulldownmenutitles[8][5][16] = {
    {"Throw dice",
     "Restart   ",
     "Stop      "},
    {"Save game   ",
     "Load game   ",
     "Autosave off"},
    {"Next   ",
     "Stop   ",
     "Restart"},
    {"Credits"},
    {"Yes",
     "No "},
    {"Restart"
    ,"Stop   "},
    {"Continue game",
     "New game     ",
     "Stop         "},
    {"Autosave       ",
     "Slot 1: Empty  ",
     "Slot 2: Empty  ",
     "Slot 3: Empty  ",
     "Slot 4: Empty  "}
};

/* Functions for windowing and menu system */

void windowsave(unsigned char ypos, unsigned char height)
{
    /* Function to save a window
       Input:
       - ypos: startline of window
       - height: height of window    */
    
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
}

void windowrestore()
{
    /* Function to restore a window */
    windowaddress = Window[--windownumber].address;

    // Restore characters
    VDC_CopyMemToVDC(Window[windownumber].ypos*80,windowaddress,1,Window[windownumber].height*80);

    // Restore attributes
    VDC_CopyMemToVDC(0x0800+(Window[windownumber].ypos*80),windowaddress+(Window[windownumber].height*80),1,Window[windownumber].height*80);
}

void menumakeborder(unsigned char xpos, unsigned char ypos, unsigned char height, unsigned char width)
{
    /* Function to make menu border
       Input:
       - xpos: x-coordinate of left upper corner
       - ypos: y-coordinate of right upper corner
       - height: number of rows in window
       - width: window width in characters        */
 
    windowsave(ypos, height+4);

    VDC_FillArea(ypos+1,xpos+1,C_SPACE,width,height,VDC_LYELLOW);
    VDC_Plot(ypos,xpos,C_LOWRIGHT,VDC_LRED);
    VDC_HChar(ypos,xpos+1,C_LOWLINE,width,VDC_LRED);
    VDC_Plot(ypos,xpos+width+1,C_LOWLEFT,VDC_LRED);
    VDC_VChar(ypos+1,xpos,C_RIGHTLINE,height,VDC_LRED);
    VDC_VChar(ypos+1,xpos+width+1,C_LEFTLINE,height,VDC_LRED);
    VDC_Plot(ypos+height+1,xpos,C_UPRIGHT,VDC_LRED);
    VDC_HChar(ypos+height+1,xpos+1,C_UPLINE,width,VDC_LRED);
    VDC_Plot(ypos+height+1,xpos+width+1,C_UPLEFT,VDC_LRED);
}

void menuplacebar()
{
    /* Function to print menu bar */

    unsigned char x;

    for(x=0;x<menubaroptions;x++)
    {
        VDC_PrintAt(1,menubarcoords[x],menubartitles[x],VDC_DGREEN+VDC_A_REVERSE);
    }
}

unsigned char menupulldown(unsigned char xpos, unsigned char ypos, unsigned char menunumber)
{
    /* Function for pull down menu
       Input:
       - xpos = x-coordinate of upper left corner
       - ypos = y-coordinate of upper left corner
       - menunumber = 
         number of the menu as defined in pulldownmenuoptions array */

    unsigned char x;
    char validkeys[6];
    unsigned char key;
    unsigned char exit = 0;
    unsigned char menuchoice = 1;

    windowsave(ypos, pulldownmenuoptions[menunumber-1]+4);
    if(menunumber>menubaroptions)
    {
        VDC_Plot(ypos,xpos,C_LOWRIGHT,VDC_LRED);
        VDC_HChar(ypos,xpos+1,C_LOWLINE,strlen(pulldownmenutitles[menunumber-1][0])+2,VDC_LRED);
        VDC_Plot(ypos,xpos+strlen(pulldownmenutitles[menunumber-1][0])+3,C_LOWLEFT,VDC_LRED);
    }
    for(x=0;x<pulldownmenuoptions[menunumber-1];x++)
    {
        VDC_Plot(ypos+x+1,xpos,C_RIGHTLINE,VDC_LRED);
        VDC_Plot(ypos+x+1,xpos+1,C_SPACE,VDC_LCYAN+VDC_A_REVERSE);
        VDC_PrintAt(ypos+x+1,xpos+2,pulldownmenutitles[menunumber-1][x],VDC_LCYAN+VDC_A_REVERSE);
        VDC_Plot(ypos+x+1,xpos+strlen(pulldownmenutitles[menunumber-1][x])+2,C_SPACE,VDC_LCYAN+VDC_A_REVERSE);
        VDC_Plot(ypos+x+1,xpos+strlen(pulldownmenutitles[menunumber-1][x])+3,C_LEFTLINE,VDC_LRED);
    }
    VDC_Plot(ypos+pulldownmenuoptions[menunumber-1]+1,xpos,C_UPRIGHT,VDC_LRED);
    VDC_HChar(ypos+pulldownmenuoptions[menunumber-1]+1,xpos+1,C_UPLINE,strlen(pulldownmenutitles[menunumber-1][0])+2,VDC_LRED);
    VDC_Plot(ypos+pulldownmenuoptions[menunumber-1]+1,xpos+strlen(pulldownmenutitles[menunumber-1][0])+3,C_UPLEFT,VDC_LRED);

    strcpy(validkeys, updownenter);
    if(menunumber<=menubaroptions)
    {
        strcat(validkeys, leftright);
    }
    
    do
    {
        VDC_Plot(ypos+menuchoice,xpos+1,C_SPACE,VDC_LYELLOW+VDC_A_REVERSE);
        VDC_PrintAt(ypos+menuchoice,xpos+2,pulldownmenutitles[menunumber-1][menuchoice-1],VDC_LYELLOW+VDC_A_REVERSE);
        VDC_Plot(ypos+menuchoice,xpos+strlen(pulldownmenutitles[menunumber-1][menuchoice-1])+2,C_SPACE,VDC_LYELLOW+VDC_A_REVERSE);

        key = getkey(validkeys,joyinterface);
        switch (key)
        {
        case C_ENTER:
            exit = 1;
            break;
        
        case C_LEFT:
            exit = 1;
            menuchoice = 18;
            break;
        
        case C_RIGHT:
            exit = 1;
            menuchoice = 19;
            break;

        case C_DOWN:
        case C_UP:
            VDC_Plot(ypos+menuchoice,xpos+1,C_SPACE,VDC_LCYAN+VDC_A_REVERSE);
            VDC_PrintAt(ypos+menuchoice,xpos+2,pulldownmenutitles[menunumber-1][menuchoice-1],VDC_LCYAN+VDC_A_REVERSE);
            VDC_Plot(ypos+menuchoice,xpos+strlen(pulldownmenutitles[menunumber-1][menuchoice-1])+2,C_SPACE,VDC_LCYAN+VDC_A_REVERSE);
            if(key==C_UP)
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
    windowrestore();    
    return menuchoice;
}

unsigned char menumain()
{
    /* Function for main menu selection */

    unsigned char menubarchoice = 1;
    unsigned char menuoptionchoice = 0;
    unsigned char key;
    char validkeys[4] = {C_LEFT,C_RIGHT,C_ENTER,0};
    unsigned char xpos;

    do
    {
        do
        {
            VDC_Plot(1,menubarcoords[menubarchoice-1]-1,C_SPACE,VDC_WHITE+VDC_A_REVERSE+VDC_A_ALTCHAR);
            VDC_PrintAt(1,menubarcoords[menubarchoice-1],menubartitles[menubarchoice-1],VDC_WHITE+VDC_A_REVERSE+VDC_A_ALTCHAR);
            VDC_Plot(1,menubarcoords[menubarchoice-1]+strlen(menubartitles[menubarchoice-1]),C_SPACE,VDC_WHITE+VDC_A_REVERSE+VDC_A_ALTCHAR);

            key = getkey(validkeys,joyinterface);

            VDC_Plot(1,menubarcoords[menubarchoice-1]-1,C_SPACE,VDC_DGREEN+VDC_A_REVERSE+VDC_A_ALTCHAR);
            VDC_PrintAt(1,menubarcoords[menubarchoice-1],menubartitles[menubarchoice-1],VDC_DGREEN+VDC_A_REVERSE+VDC_A_ALTCHAR);
            VDC_Plot(1,menubarcoords[menubarchoice-1]+strlen(menubartitles[menubarchoice-1]),C_SPACE,VDC_DGREEN+VDC_A_REVERSE+VDC_A_ALTCHAR);
            
            if(key==C_LEFT)
            {
                menubarchoice--;
                if(menubarchoice<1)
                {
                    menubarchoice = menubaroptions;
                }
            }
            else if (key==C_RIGHT)
            {
                menubarchoice++;
                if(menubarchoice>menubaroptions)
                {
                    menubarchoice = 1;
                }
            }
        } while (key!=C_ENTER);
        xpos=menubarcoords[menubarchoice-1]-1;
        if(xpos+strlen(pulldownmenutitles[menubarchoice-1][0])>38)
        {
            xpos=menubarcoords[menubarchoice-1]+strlen(menubartitles[menubarchoice-1])-strlen(pulldownmenutitles[menubarchoice-1][0]);
        }
        menuoptionchoice = menupulldown(xpos,1,menubarchoice);
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
    } while (menuoptionchoice==0);
    return menubarchoice*10+menuoptionchoice;    
}

unsigned char areyousure()
{
    /* Pull down menu to verify if player is sure */
    unsigned char choice;

    menumakeborder(8,8,6,30);
    cputsxy(10,10,"Are you sure?");
    choice = menupulldown(25,11,5);
    windowrestore();
    return choice;
}

void fileerrormessage(unsigned char error)
{
    /* Show message for file error encountered */

    menumakeborder(8,8,6,30);
    cputsxy(10,10,"File error!");
    if(error<255)
    {
        gotoxy(10,12);
        cprintf("Error nr.: %2X",error);
    }
    cputsxy(10,12,"Press key.");
    getkey("",1);
    windowrestore();    
}
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

void main()
{
    unsigned char x = 5;
    unsigned char y = 5;
    unsigned int screenwidth = 160;
    unsigned int screenheigth = 50;
    unsigned char key;
    unsigned char direction;
    unsigned char lines, sets, chars;

    VDC_Init();
    gotoxy(0,0);
    printf("Bankmemset.\n");
    BankMemSet(SCREENMAPBASE,1,102,screenwidth*screenheigth);
    BankMemSet(SCREENMAPBASE+(screenwidth*screenheigth)+48,1,13,screenwidth*screenheigth);

    cgetc();
    printf("Poking.\n");

    for(lines=0;lines<5;lines++)
    {
        for(sets=0;sets<16;sets++)
        {
            for(chars=0;chars<10;chars++)
            {
                POKEB(SCREENMAPBASE + (lines*10*screenwidth) + (sets*10) + chars,1,chars+48);
                POKEB(SCREENMAPBASE + (screenwidth*screenheigth) + 48 + (lines*10*screenwidth) + (sets*10) + chars,1,chars+1);
            }
        }
    }

    cgetc();
    printf("Copy viewport.");
    VDC_CopyViewPortToVDC(SCREENMAPBASE,1,screenwidth,screenheigth,0,0,0,0,80,25);

    cgetc();
    cputsxy(0,2,"BankMemCopy.");
    BankMemCopy(SCREENMAPBASE,1,SCREENMAPBASE+5*screenwidth,1,5*screenwidth);
    BankMemCopy(SCREENMAPBASE+(screenwidth*screenheigth)+48,1,SCREENMAPBASE+(screenwidth*screenheigth)+48+5*screenwidth,1,5*screenwidth);
    VDC_CopyViewPortToVDC(SCREENMAPBASE,1,screenwidth,screenheigth,0,0,0,0,80,25);

    cgetc();

    do
    {
        direction = 0;
        do
        {
            key = cgetc();
        } while (key != CH_CURS_UP && key != CH_CURS_DOWN && key != CH_CURS_LEFT && key != CH_CURS_RIGHT && key != CH_ESC);
        
        switch (key)
        {
        case CH_CURS_UP:
            if(y>0) { y--; direction = SCROLL_DOWN; }
            break;
            
        case CH_CURS_DOWN:
            if(y<screenheigth-15) { y++; direction = SCROLL_UP; }
            break;

        case CH_CURS_LEFT:
            if(x>0) { x--; direction = SCROLL_RIGHT; }
            break;
            
        case CH_CURS_RIGHT:
            if(x<screenwidth-60) { x++; direction = SCROLL_LEFT; }
            break;

        default:
            break;
        }

        gotoxy(0,2);
        cprintf("X: %3i Y: %3i  ",x,y);

        if(direction) { VDC_ScrollCopy(SCREENMAPBASE,1,screenwidth,screenheigth,x,y,5,5,60,15,direction); }

    } while (key != CH_ESC);

    VDC_Exit();
}
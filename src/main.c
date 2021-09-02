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
unsigned char pulldownmenuoptions[5] = {5,2,4,3,2};
char pulldownmenutitles[5][5][16] = {
    {"Width:      80 ",
     "Height:     25 ",
     "Background:  0 ",
     "Clear          ",
     "Fill           "},
    {"Save screen    ",
     "Load screen    "},
    {"Load standard  ",
     "Load alternate ",
     "Save standard  ",
     "Save alternate "},
    {"Version/credits",
     "Help           ",
     "Exit program   "},
    {"Yes",
     "No "}
};

// Global variables
unsigned char bootdevice;
unsigned char charsetchanged[2] = { 0,0 };

unsigned char xcoord = 0;
unsigned char ycoord = 0;
unsigned char xoffset = 0;
unsigned char yoffset = 0;
unsigned int screenwidth = 80;
unsigned int screenheigth = 25;
unsigned int screentotal = 80*25;

char buffer[81];
char version[22];

// Generic routines

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
    VDC_PlotString(ypos,xpos,str,strlen(str),VDC_WHITE+VDC_A_REVERSE+VDC_A_ALTCHAR);
    gotoxy(xpos+idx,ypos);
    
    while(1)
    {
        c = cgetc();
        switch (c)
        {
        case CH_ESC:
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
                VDC_Plot(ypos, xpos + idx,CH_SPACE,VDC_WHITE+VDC_A_REVERSE+VDC_A_ALTCHAR);
                for(c = idx; 1; ++c)
                {
                    unsigned char b = str[c+1];
                    str[c] = b;
                    VDC_Plot(ypos, xpos+c, b? VDC_PetsciiToScreenCode(b) : CH_SPACE, VDC_WHITE+VDC_A_REVERSE+VDC_A_ALTCHAR);
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
                VDC_PlotString(ypos,xpos,str,strlen(str),VDC_WHITE+VDC_A_REVERSE+VDC_A_ALTCHAR);
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
                VDC_Plot(ypos, xpos+idx, VDC_PetsciiToScreenCode(c), VDC_WHITE+VDC_A_REVERSE);
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

    // Restore customn charset if needed
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

    VDC_FillArea(ypos,xpos,CH_SPACE,width,height,VDC_WHITE+VDC_A_REVERSE+VDC_A_ALTCHAR);
}

void menuplacebar()
{
    /* Function to print menu bar */

    unsigned char x;

    VDC_FillArea(0,0,32,80,1,VDC_DGREEN+VDC_A_REVERSE+VDC_A_ALTCHAR);
    for(x=0;x<menubaroptions;x++)
    {
        VDC_PrintAt(0,menubarcoords[x],menubartitles[x],VDC_DGREEN+VDC_A_REVERSE+VDC_A_ALTCHAR);
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
        VDC_Plot(ypos+x,xpos,CH_SPACE,VDC_LCYAN+VDC_A_REVERSE+VDC_A_ALTCHAR);
        VDC_PrintAt(ypos+x,xpos+1,pulldownmenutitles[menunumber-1][x],VDC_LCYAN+VDC_A_REVERSE+VDC_A_ALTCHAR);
        VDC_Plot(ypos+x,xpos+strlen(pulldownmenutitles[menunumber-1][x])+1,CH_SPACE,VDC_LCYAN+VDC_A_REVERSE+VDC_A_ALTCHAR);
    }
  
    do
    {
        VDC_Plot(ypos+menuchoice-1,xpos,CH_SPACE,VDC_LYELLOW+VDC_A_REVERSE+VDC_A_ALTCHAR);
        VDC_PrintAt(ypos+menuchoice-1,xpos+1,pulldownmenutitles[menunumber-1][menuchoice-1],VDC_LYELLOW+VDC_A_REVERSE+VDC_A_ALTCHAR);
        VDC_Plot(ypos+menuchoice-1,xpos+strlen(pulldownmenutitles[menunumber-1][menuchoice-1])+1,CH_SPACE,VDC_LYELLOW+VDC_A_REVERSE+VDC_A_ALTCHAR);

        do
        {
            key = cgetc();
        } while (key != CH_ENTER && key != CH_CURS_LEFT && key != CH_CURS_RIGHT && key != CH_CURS_UP && key != CH_CURS_DOWN && key != CH_ESC);

        switch (key)
        {
        case CH_ESC:
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
            VDC_Plot(ypos+menuchoice-1,xpos,CH_SPACE,VDC_LCYAN+VDC_A_REVERSE+VDC_A_ALTCHAR);
            VDC_PrintAt(ypos+menuchoice-1,xpos+1,pulldownmenutitles[menunumber-1][menuchoice-1],VDC_LCYAN+VDC_A_REVERSE+VDC_A_ALTCHAR);
            VDC_Plot(ypos+menuchoice-1,xpos+strlen(pulldownmenutitles[menunumber-1][menuchoice-1])+1,CH_SPACE,VDC_LCYAN+VDC_A_REVERSE+VDC_A_ALTCHAR);
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
            VDC_Plot(0,menubarcoords[menubarchoice-1]-1,CH_SPACE,VDC_WHITE+VDC_A_REVERSE+VDC_A_ALTCHAR);
            VDC_PrintAt(0,menubarcoords[menubarchoice-1],menubartitles[menubarchoice-1],VDC_WHITE+VDC_A_REVERSE+VDC_A_ALTCHAR);
            VDC_Plot(0,menubarcoords[menubarchoice-1]+strlen(menubartitles[menubarchoice-1]),CH_SPACE,VDC_WHITE+VDC_A_REVERSE+VDC_A_ALTCHAR);

            do
            {
                key = cgetc();
            } while (key != CH_ENTER && key != CH_CURS_LEFT && key != CH_CURS_RIGHT && key != CH_ESC);

            VDC_Plot(0,menubarcoords[menubarchoice-1]-1,CH_SPACE,VDC_DGREEN+VDC_A_REVERSE+VDC_A_ALTCHAR);
            VDC_PrintAt(0,menubarcoords[menubarchoice-1],menubartitles[menubarchoice-1],VDC_DGREEN+VDC_A_REVERSE+VDC_A_ALTCHAR);
            VDC_Plot(0,menubarcoords[menubarchoice-1]+strlen(menubartitles[menubarchoice-1]),CH_SPACE,VDC_DGREEN+VDC_A_REVERSE+VDC_A_ALTCHAR);
            
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
        } while (key!=CH_ENTER && key != CH_ESC);
        if (key != CH_ESC)
            {
            xpos=menubarcoords[menubarchoice-1]-1;
            if(xpos+strlen(pulldownmenutitles[menubarchoice-1][0])>38)
            {
                xpos=menubarcoords[menubarchoice-1]+strlen(menubartitles[menubarchoice-1])-strlen(pulldownmenutitles    [menubarchoice-1][0]);
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

unsigned char areyousure(unsigned char syscharset)
{
    /* Pull down menu to verify if player is sure */
    unsigned char choice;

    windownew(8,8,6,30,syscharset);
    VDC_PrintAt(10,10,"Are you sure?",VDC_WHITE+VDC_A_REVERSE+VDC_A_ALTCHAR);
    choice = menupulldown(25,11,5,0);
    windowrestore(syscharset);
    return choice;
}

void fileerrormessage(unsigned char error, unsigned char syscharset)
{
    /* Show message for file error encountered */

    windownew(8,8,6,30,syscharset);
    VDC_PrintAt(10,10,"File error!",VDC_WHITE+VDC_A_REVERSE+VDC_A_ALTCHAR);
    if(error<255)
    {
        sprintf(buffer,"Error nr.: %2X",error);
        VDC_PrintAt(12,10,buffer,VDC_WHITE+VDC_A_REVERSE+VDC_A_ALTCHAR);
    }
    VDC_PrintAt(13,10,"Press key.",VDC_WHITE+VDC_A_REVERSE+VDC_A_ALTCHAR);
    cgetc();
    windowrestore(syscharset);    
}

// Generic screen map routines

void screenmapplot(unsigned char row, unsigned char col, unsigned char screencode, unsigned char attribute)
{
    // Function to plot a screencodes at bank 1 memory screen map
	// Input: row and column, screencode to plot, attribute code

    unsigned int address = SCREENMAPBASE + (row*screenwidth) + col;
    POKEB(address,1,screencode);
    POKEB(address+(screenwidth*screenheigth)+48,1,attribute);
}

void placesignature()
{
    char versiontext[49] = "";
    unsigned char x;
    unsigned int address = SCREENMAPBASE + (screenwidth*screenheigth);

    sprintf(versiontext,"VDC Screen Editor %s X.Mol ",version);

    for(x=0;x<strlen(versiontext);x++)
    {
        POKEB(address+x,1,versiontext[x]);
    }
}

void screenmapfill(unsigned char screencode, unsigned char attribute)
{
    unsigned int address = SCREENMAPBASE;
    
    BankMemSet(address,1,screencode,screentotal+48);
    placesignature();
    address += screentotal + 48;
    BankMemSet(address,1,attribute,screentotal);
}

// Application routines
void resizewidth()
{
    unsigned int newwidth = screenwidth;
    unsigned int maxsize = MEMORYLIMIT - SCREENMAPBASE;
    unsigned char areyousure = 0;
    unsigned char sizechanged = 0;
    unsigned int x,y;
    char* ptrend;

    windownew(20,5,12,40,0);

    VDC_PrintAt(6,21,"Resize canvas width",VDC_WHITE+VDC_A_REVERSE+VDC_A_UNDERLINE+VDC_A_ALTCHAR);
    VDC_PrintAt(8,21,"Enter new width:",VDC_WHITE+VDC_A_REVERSE+VDC_A_ALTCHAR);

    sprintf(buffer,"%i",screenwidth);
    textInput(21,9,buffer,4);
    newwidth = (unsigned int)strtol(buffer,&ptrend,10);

    if((newwidth*screenheigth*2) + 48 > maxsize)
    {
        VDC_PrintAt(11,21,"New size too big. Press key.",VDC_WHITE+VDC_A_REVERSE+VDC_A_ALTCHAR);
        cgetc();
    }
    else
    {
        if(newwidth < screenwidth)
        {
            VDC_PrintAt(11,21,"Shrinking might delete data.",VDC_WHITE+VDC_A_REVERSE+VDC_A_ALTCHAR);
            VDC_PrintAt(12,21,"Are you sure?",VDC_WHITE+VDC_A_REVERSE+VDC_A_ALTCHAR);
            areyousure = menupulldown(25,13,5,0);
            for(x=1;x<screenheigth;x++)
            {
                BankMemCopy(SCREENMAPBASE+(x*screenwidth),1,SCREENMAPBASE+(x*newwidth),1,newwidth);
            }
            for(x=0;x<screenheigth;x++)
            {
                BankMemCopy(SCREENMAPBASE+screentotal+48+(x*screenwidth),1,SCREENMAPBASE+(newwidth*screenheigth)+48+(x*newwidth),1,newwidth);
            }
            if(xcoord>newwidth-1) { xcoord=newwidth-1; }
            sizechanged = 1;
        }
        if(newwidth > screenwidth)
        {
            for(x=0;x<screenheigth;x++)
            {
                y = screenheigth-x-1;
                BankMemCopy(SCREENMAPBASE+screentotal+48+(y*screenwidth),1,SCREENMAPBASE+(newwidth*screenheigth)+48+(y*newwidth),1,screenwidth);
                BankMemSet(SCREENMAPBASE+(newwidth*screenheigth)+48+(y*newwidth)+screenwidth,1,VDC_WHITE,newwidth-screenwidth);
            }
            for(x=0;x<screenheigth;x++)
            {
                y = screenheigth-x-1;
                BankMemCopy(SCREENMAPBASE+(y*screenwidth),1,SCREENMAPBASE+(y*newwidth),1,screenwidth);
                BankMemSet(SCREENMAPBASE+(y*newwidth)+screenwidth,1,CH_SPACE,newwidth-screenwidth);
            }
            sizechanged = 1;
        }
    }

    windowrestore(0);

    if(sizechanged==1)
    {
        screenwidth = newwidth;
        screentotal - screenwidth * screenheigth;
        placesignature();
        VDC_CopyViewPortToVDC(SCREENMAPBASE,1,screenwidth,screenheigth,xoffset,yoffset,0,0,80,25);
        sprintf(pulldownmenutitles[0][0],"Width:   %5i ",screenwidth);
        menuplacebar();
    }
}

void mainmenuloop()
{
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

        default:
            break;
        }
    } while (menuchoice < 99);
    
    windowrestore(1);
}

// Main loop

void main()
{
    // Obtain device number the application was started from
    bootdevice = getcurrentdevice();

    // Set version number in string variable
    sprintf(version,
            "v%2i.%2i - %c%c%c%c%c%c%c%c-%c%c%c%c",
            VERSION_MAJOR, VERSION_MINOR,
            BUILD_YEAR_CH0, BUILD_YEAR_CH1, BUILD_YEAR_CH2, BUILD_YEAR_CH3, BUILD_MONTH_CH0, BUILD_MONTH_CH1, BUILD_DAY_CH0, BUILD_DAY_CH1,BUILD_HOUR_CH0, BUILD_HOUR_CH1, BUILD_MIN_CH0, BUILD_MIN_CH1);

    // Initialise VDC screen and VDC assembly routines
    VDC_Init();

    // Load system charset to bank 1
    VDC_LoadCharset("vdcse.sfon", CHARSETSYSTEM, 1, 0);

    // Clear screen map in bank 1 with spaces in text color white
    screenmapfill(CH_SPACE,VDC_WHITE);

    mainmenuloop();

    VDC_Exit();
}
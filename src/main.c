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
    {"Yes",
     "No "}
};

// Menucolors
unsigned char mc_mb_normal = VDC_LGREEN + VDC_A_REVERSE + VDC_A_ALTCHAR;
unsigned char mc_mb_select = VDC_WHITE + VDC_A_REVERSE + VDC_A_ALTCHAR;
unsigned char mc_pd_normal = VDC_LCYAN + VDC_A_REVERSE + VDC_A_ALTCHAR;
unsigned char mc_pd_select = VDC_LYELLOW + VDC_A_REVERSE + VDC_A_ALTCHAR;
unsigned char mc_menupopup = VDC_WHITE + VDC_A_REVERSE + VDC_A_ALTCHAR;

// Global variables
unsigned char bootdevice;
unsigned char charsetchanged[2];
unsigned char appexit;
unsigned char targetdevice;
char filename[21];

unsigned char screen_col;
unsigned char screen_row;
unsigned char xoffset;
unsigned char yoffset;
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
    VDC_PrintAt(ypos,xpos,str,mc_menupopup);
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

    // Restore customn charset if needed
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
        VDC_Plot(ypos+menuchoice-1,xpos,CH_SPACE,mc_pd_select);
        VDC_PrintAt(ypos+menuchoice-1,xpos+1,pulldownmenutitles[menunumber-1][menuchoice-1],mc_pd_select);
        VDC_Plot(ypos+menuchoice-1,xpos+strlen(pulldownmenutitles[menunumber-1][menuchoice-1])+1,CH_SPACE,mc_pd_select);

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
            } while (key != CH_ENTER && key != CH_CURS_LEFT && key != CH_CURS_RIGHT && key != CH_ESC);

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

// Generic screen map routines

unsigned int screenmap_screenaddr(unsigned char row, unsigned char col, unsigned int width)
{
    return SCREENMAPBASE+(row*width)+col;
}

unsigned int screenmap_attraddr(unsigned char row, unsigned char col, unsigned int width, unsigned int heigth)
{
    return SCREENMAPBASE+(row*width)+col+(width*heigth)+48;
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
    unsigned int address = SCREENMAPBASE;
    
    BankMemSet(address,1,screencode,screentotal+48);
    placesignature();
    address += screentotal + 48;
    BankMemSet(address,1,attribute,screentotal);
}

void cursormove(unsigned char left, unsigned char right, unsigned char up, unsigned char down)
{
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

// Application routines
void plotmove(direction)
{
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

void writemode()
{
    unsigned char key;

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

        // Delete present screencode and attributes
        case CH_DEL:
            screenmapplot(screen_row,screen_col,CH_SPACE,VDC_WHITE);
            VDC_Plot(screen_col,screen_row,CH_SPACE,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
            break;

        // Write printable character                
        default:
            if(isprint(key))
            {
                screenmapplot(screen_row+yoffset,screen_col+xoffset,VDC_PetsciiToScreenCode(key),VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
                //VDC_Plot(screen_row,screen_col,VDC_PetsciiToScreenCode(key),VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
                plotmove(CH_CURS_RIGHT);
            }
            break;
        }
    } while (key != CH_ESC);
}

void colorwrite()
{
    unsigned char key;

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

        default:
            // If keypress is 0-9 or A-F select color
            if(key>47 && key<58)
            {
                POKEB(screenmap_attraddr(screen_row+yoffset,screen_col+xoffset,screenwidth,screenheight),1,VDC_Attribute(key-48, plotblink, plotunderline, plotreverse, plotaltchar));
                plotmove(CH_CURS_RIGHT);
            }
            if(key>64 && key<71)
            {
                POKEB(screenmap_attraddr(screen_row+yoffset,screen_col+xoffset,screenwidth,screenheight),1,VDC_Attribute(key-55, plotblink, plotunderline, plotreverse, plotaltchar));
                plotmove(CH_CURS_RIGHT);
            }
            break;
        }
    } while (key != CH_ESC);
}

void resizewidth()
{
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
                    BankMemCopy(screenmap_screenaddr(y,0,screenwidth),1,screenmap_screenaddr(y,0,newwidth),1,newwidth);
                }
                for(y=0;y<screenheight;y++)
                {
                    BankMemCopy(screenmap_attraddr(y,0,screenwidth,screenheight),1,screenmap_attraddr(y,0,newwidth,screenheight),1,newwidth);
                }
                if(screen_col>newwidth-1) { screen_col=newwidth-1; }
                sizechanged = 1;
            }
        }
        if(newwidth > screenwidth)
        {
            for(y=0;y<screenheight;y++)
            {
                BankMemCopy(screenmap_attraddr(screenheight-y-1,0,screenwidth,screenheight),1,screenmap_attraddr(screenheight-y-1,0,newwidth,screenheight),1,screenwidth);
                BankMemSet(screenmap_attraddr(screenheight-y-1,screenwidth,newwidth,screenheight),1,VDC_WHITE,newwidth-screenwidth);
            }
            for(y=0;y<screenheight;y++)
            {
                BankMemCopy(screenmap_screenaddr(screenheight-y-1,0,screenwidth),1,screenmap_screenaddr(screenheight-y-1,0,newwidth),1,screenwidth);
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
    }
}

void resizeheight()
{
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
    }
}

void changebackgroundcolor()
{
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
        } while (key != CH_ENTER && key != CH_ESC && key != '+' && key != '-');

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
    } while (key != CH_ENTER && key != CH_ESC);
    
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

void chooseidandfilename(char* headertext)
{
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
    textInput(21,11,filename,15);
}

void loadscreenmap()
{
    unsigned int lastreadaddress, newwidth, newheigth;
    unsigned int maxsize = MEMORYLIMIT - SCREENMAPBASE;
    char* ptrend;
  
    chooseidandfilename("Load screen");

    VDC_PrintAt(12,21,"Enter screen width:",mc_menupopup);
    sprintf(buffer,"%i",screenwidth);
    textInput(21,13,buffer,4);
    newwidth = (unsigned int)strtol(buffer,&ptrend,10);

    VDC_PrintAt(14,21,"Enter screen heigth:",mc_menupopup);
    sprintf(buffer,"%i",screenheight);
    textInput(21,15,buffer,4);
    newheigth = (unsigned int)strtol(buffer,&ptrend,10);

    if((newwidth*newheigth*2) + 48 > maxsize || newwidth<80 || newheigth<25)
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
            screenheight = newheigth;
            VDC_CopyViewPortToVDC(SCREENMAPBASE,1,screenwidth,screenheight,xoffset,yoffset,0,0,80,25);
            windowsave(0,1,0);
            menuplacebar();
        }
    }
}

void savescreenmap()
{
    unsigned char error;
  
    chooseidandfilename("Save screen");

    windowrestore(0);

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

void loadcharset(unsigned char stdoralt)
{
    unsigned int lastreadaddress, charsetaddress;
  
    chooseidandfilename("Load chararcter set");

    charsetaddress = (stdoralt==0)? CHARSETNORMAL : CHARSETALTERNATE;

    windowrestore(0);

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
    unsigned char error;
    unsigned int charsetaddress;
  
    chooseidandfilename("Save chararcter set");

    charsetaddress = (stdoralt==0)? CHARSETNORMAL : CHARSETALTERNATE;

    windowrestore(0);

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

        case 12:
            resizeheight();
            break;
        
        case 13:
            changebackgroundcolor();
            break;

        case 14:
            screenmapfill(CH_SPACE,VDC_WHITE);
            VDC_CopyViewPortToVDC(SCREENMAPBASE,1,screenwidth,screenheight,xoffset,yoffset,0,0,80,25);
            menuplacebar();
            break;
        
        case 15:
            screenmapfill(plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
            VDC_CopyViewPortToVDC(SCREENMAPBASE,1,screenwidth,screenheight,xoffset,yoffset,0,0,80,25);
            menuplacebar();
            break;

        case 21:
            savescreenmap();
            break;

        case 22:
            loadscreenmap();
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

        case 43:
            appexit = 1;
            menuchoice = 99;
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

    // Load system charset to bank 1
    VDC_LoadCharset("vdcse.sfon",bootdevice, CHARSETSYSTEM, 1, 2);

    // Clear screen map in bank 1 with spaces in text color white
    screenmapfill(CH_SPACE,VDC_WHITE);

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
            plotcolor=newval;
            textcolor(vdctoconiocol[plotcolor]);
            VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
            break;

        // Increase color
        case '.':
            if(plotcolor==15) { newval = 0; } else { newval = plotcolor + 1; }
            if(newval == screenbackground)
            {
                if(newval==15) { newval = 0; } else { newval++; }
            }
            plotcolor=newval;
            textcolor(vdctoconiocol[plotcolor]);
            VDC_Plot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
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
            break;

        case 'w':
            writemode();
            break;
        
        case 'c':
            colorwrite();
            break;

        // Plot present screencode and attribute
        case CH_SPACE:
            screenmapplot(screen_row,screen_col,plotscreencode,VDC_Attribute(plotcolor, plotblink, plotunderline, plotreverse, plotaltchar));
            break;

        // Delete present screencode and attributes
        case CH_DEL:
            screenmapplot(screen_row,screen_col,CH_SPACE,VDC_WHITE);
            break;

        // Go to menu
        case CH_F1:
            cursor(0);
            mainmenuloop();
            gotoxy(screen_col,screen_row);
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
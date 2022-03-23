#ifndef __MAIN_H_
#define __MAIN_H_

unsigned char dosCommand(const unsigned char lfn, const unsigned char drive, const unsigned char sec_addr, const char *cmd);
unsigned int cmd(const unsigned char device, const char *cmd);
int textInput(unsigned char xpos, unsigned char ypos, char* str, unsigned char size);
void initoverlay();
void loadoverlay(unsigned char overlay_select);
void windowsave(unsigned char ypos, unsigned char height, unsigned char loadsyscharset);
void windowrestore(unsigned char restorealtcharset);
void windownew(unsigned char xpos, unsigned char ypos, unsigned char height, unsigned char width, unsigned char loadsyscharset);
void menuplacebar();
unsigned char menupulldown(unsigned char xpos, unsigned char ypos, unsigned char menunumber, unsigned char escapable);
unsigned char menumain();
unsigned char areyousure(char* message, unsigned char syscharset);
void fileerrormessage(unsigned char error, unsigned char syscharset);
void messagepopup(char* message, unsigned char syscharset);
unsigned int screenmap_screenaddr(unsigned char row, unsigned char col, unsigned int width);
unsigned int screenmap_attraddr(unsigned char row, unsigned char col, unsigned int width, unsigned int height);
void screenmapplot(unsigned char row, unsigned char col, unsigned char screencode, unsigned char attribute);
void placesignature();
void screenmapfill(unsigned char screencode, unsigned char attribute);
void cursormove(unsigned char left, unsigned char right, unsigned char up, unsigned char down);
void undo_new(unsigned char row, unsigned char col, unsigned char width, unsigned char height);
void undo_performundo();
void undo_escapeundo();
void undo_performredo();
void helpscreen_load(unsigned char screennumber);
void plotmove(unsigned char direction);
void change_plotcolor(unsigned char newval);
void printstatusbar();
void initstatusbar();
void hidestatusbar();
void togglestatusbar();
void showchareditfield(unsigned char stdoralt);
unsigned int charaddress(unsigned char screencode, unsigned char stdoralt, unsigned char vdcormem);
void showchareditgrid(unsigned int screencode, unsigned char stdoralt);

#endif // __MAIN_H_
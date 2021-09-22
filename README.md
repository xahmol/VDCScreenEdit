# VDC Screen Editor
C128 80 column screen editor

## Contents:

[Version history and download](#version-history-and-download)

[Introduction](#introduction)

[Main menu](#main-menu)

[Main mode](#main-mode)

[Character editor](#character-editor)

[Select mode](#select-mode)

[Move mode](#move-mode)

[Line and box mode](#line-and-box-mode)

[Write mode](#write-mode)

[Color write mode](#color-write-mode)

[Color value reference](#color-value-reference)

[Attribute code reference](#attribute-code-reference)

[File format reference](#file-format-reference)

[Credits](#credits)


 ![VDCSE Title Screen](https://github.com/xahmol/VDCScreenEdit/blob/main/screenshots/VDCSE%20Titlescreen.png?raw=true)

## Version history and download
([Back to contents](#contents))

Link to latest build:

Version 0.9 - 20210922-1331:
- First released beta version

## Introduction
([Back to contents](#contents))

VDC Screen Editor is an editor to create text based screens for the Commodore 128 VDC 80 column mode. It fully supports using user defined character sets and allows the use of two 256 character character sets at the same time.

Main features of the program:
- Support for screen maps larger than 80x25 characters. Screens can be up to 30 KiB (30.720 bytes), all sizes fitting in that memory with width of 80 at minimum and heigth of 25 at minimum are supported. NB: As both the character data as the attribute data needs to be stored, a screen takes width times height times 2 bytes in storage. So 30k would fit up to 7 standard 80x25 screens to be distributed over width and height, so e.g. 2 screens wide and 3 screens high (160x75 characters).
- Supports resizing canvas size, clear or fill the canvas
- Support for loading two user defined charsets (standard charset and alternate charset, should be standard 'C64' charsets of 256 characters of 8 bits width and 8 bits height).
- Includes a simple character editor to change characters on the fly and directly see the result in your designed screen (for editing a full character set one of the many alternatives for C64 character set editing is suggested).
- Supports all the attribute values the VDC offers, blink, underline, reverse and alternate character set. For example the reverse attribute removes the need to have a reverse copy of every character set stored in the upper 128 positions of the character set, creating more positions that can be freely used to design own graphics.
- 64KiB VDC memory only: An undo/redo system is included with up to 40 positions to go back or forward (depending on the size of the change in characters). As this uses a lot of memory, this feature is only available if 64 KiB of VDC memory is available (such as in the C128DCR).
- Write mode to freely type characters with the keyboard, supporting all printable PETSCII characters and also supporting Commodore or Control + 0-9 keys for selecting colors and RVS On/Off
- Color write mode to freely type attributes and colors
- Line and box mode for drawing lines and boxes
- Select mode to cut, copy, delete or repaint (only color or all attributes) the selection.
- Move mode to scroll the screen contents (due to memory constraints only for the 80x25 viewport)

## Main menu:
([Back to contents](#contents))

From [main mode](#main-mode), press **F1** to go to the main menu. The following menu will pop up:
![VDCSE Main Menu](https://github.com/xahmol/VDCScreenEdit/blob/main/screenshots/VDCSE%20Main%20menu.png?raw=true)

(NB: Note that if your design uses a changed alternate character set, the program will load the standard system font and you design might temporarily look incorrect. This will be restored on exiting the main menu. Also, the colors of the main menu and the highlight colors might differ if you have chosen a non-black background color, to ensure visibility of the menus).

Navigation in this menu is performed by the following keys:

|Key|Description
|---|---|
|**Cursor LEFT / RIGHT**|Move between main menu options
|**Cursor UP/ DOWN**|Move between pulldown menu options
|**RETURN**|Select highlighted menu option
|**ESC** / **STOP**|Leave menu and go back

**_Screen menu_**

![Screen menu](https://github.com/xahmol/VDCScreenEdit/blob/main/screenshots/VDCSE%20Screen%20menu.png?raw=true)

*Width: Resize width*

Resize the canvas width by entering the new width. You can both shrink as expand the width. Minimum width is 80, maximum width depends on the canvas height and the result fitting in the maximum of 30 KiB memory size allocation.

Note that with shrinking the width you might loose data, as all characters right of the new width will be lost. That is why on shrinking a pulldown menu will pop-up asking if you are sure. Select the desired answer (yellow highlighted position if using a black background).

NB: NO UNDO IS AVAILABLE FOR THIS, so if you confirm shrinking the size, all lost data is lost irretrievably.

![Resize width](https://github.com/xahmol/VDCScreenEdit/blob/main/screenshots/VDCSE%20Screen%20menu%20-%20width.png?raw=true)

*Height: Resize height*

Similar to resize width, with this option you can resize the height in the same way. Minimum height is 25, maximum again dependent on width given maximum of 30 KiB memory allocation.

Also here: on shrinking you might loose data, which is lost if you confirm.

![Resize height](https://github.com/xahmol/VDCScreenEdit/blob/main/screenshots/VDCSE%20Screen%20menu%20-%20height.png?raw=true)

*Background: Change background color*

Select the background color. Note that if the chosen color is not black or grey, the menu and popup colors will be changed to black on colors with intensity bit on, and white on colors with intensity bit off.

The color can be selected with the **+** and **-** keys to increase resp. decrease the color value. The background color will change directly accordingly. Press **ENTER** to accept the new color, or **ESC** or **STOP** to cancel.

![Change background color](https://github.com/xahmol/VDCScreenEdit/blob/main/screenshots/VDCSE%20Screen%20menu%20-%20bgcolor.png?raw=true)

*Clear: Clear the canvas*

Selecting this menu option will clear the canvas (which means filling the canvas with spaces, with attribute code for the color white, no other attributes). No confirmation will be asked (but undo is available if 64 KiB VDC memory is present).

*Fill: Fill the canvas*

Similar to clear, but this will fill the canvas with the present selected screencode and attributes (so the values that the cursor was showing).

**_File menu_**

![File menu](https://github.com/xahmol/VDCScreenEdit/blob/main/screenshots/VDCSE%20File%20menu.png?raw=true)

*Save screen*

This option saves the present canvas to disk. First the device ID number is asked of the device to save to (should be between 8 and 30 and pointing to an active disk system with that ID number). Then the filename is asked (max 15 characters in length).

If a file with that name is already existing, confirmation is asked. Confirming will delete the old file before saving the new file.

In case of a file error, a popup will be shown with the error number.

![Save screen](https://github.com/xahmol/VDCScreenEdit/blob/main/screenshots/VDCSE%20File%20menu%20-%20save.png?raw=true)

*Load screen*

With this option you can load a screen from disk. Dialogue for this option is very similar to the Save screen option above. Device ID and filename will be asked, but next to that the width and height in characters will be asked as that can not be read from a standard screen file.

![Load screen](https://github.com/xahmol/VDCScreenEdit/blob/main/screenshots/VDCSE%20File%20menu%20-%20load.png?raw=true)

*Save project*

Similar to save screen, but with this option also the canvas metadata (width, height, present cursor position etc.) and the character sets if altered will be saved. Maximum filename length is now 10 to allow for an .xxxx suffix as it will save up to four files: filename.proj for the metadata, filename.scrn for the screen data, filename.chr1 for the standard charset and filename.chr2 for the alternate charset.

![Save project](https://github.com/xahmol/VDCScreenEdit/blob/main/screenshots/VDCSE%20File%20menu%20-%20projsav.png?raw=true)

*Load project*
Loads a project: the metadata, the screen and the charsets. Provide the filename without the .xxxx suffix (.proj,.scrn,.chr1 and .chr2). As the canvas width and height is now read from the metadata, no user input on canvas size is needed.

![Load project](https://github.com/xahmol/VDCScreenEdit/blob/main/screenshots/VDCSE%20File%20menu%20-%20projload.png?raw=true)

**_Charset: Load and save character sets_**

![Charset menu](https://github.com/xahmol/VDCScreenEdit/blob/main/screenshots/VDCSE%20Charset%20menu.png?raw=true)

In this menu you can select the options to Load or Save character sets. Select the options standard to load or save the standard character sets, or alternate for alternate character set.
Dialogue of these options is similar to the screen save and load options: enter device ID and filename.

**_Information: Version information, exit program or toggle Undo enabled_**

Depending on the available VDC memory size two or three options are available in this menu. As the Undo system is only available if 64 KiB VDC memory size is present, the third submenu option here is only shown if 64 KiB VDC memory is detected.

*Menu with 16 KiB VDC memory:*

![Information menu 16KiB VDC](https://github.com/xahmol/VDCScreenEdit/blob/main/screenshots/VDCSE%20Screen%20Information%2016k.png?raw=true)

*Menu with 64 KiB VDC memory:*

![Information menu 64KiB VDC](https://github.com/xahmol/VDCScreenEdit/blob/main/screenshots/VDCSE%20Screen%20Information%2064k.png?raw=true)

*Information*

This option shows a popup with version information.

![Information](https://github.com/xahmol/VDCScreenEdit/blob/main/screenshots/VDCSE%20Version%20info.png?raw=true)

*Exit program*

With this option you can exit the program. NB: No confirmation will be asked and unsaved work will be lost.

*Undo: Enabled / Disabled  : Toggle if Undo system is active or not*

Only available if 64 KiB VDC memory is detected: This option toggles if the Undo system is enabled or not. Default is enabled, but if preferred for speed, Undo can be disabled here.

## Main mode
([Back to contents](#contents))

After the title screen, the program starts in this mode. At start the screen shows this:

![Screen in main mode](https://github.com/xahmol/VDCScreenEdit/blob/main/screenshots/VDCSE%20empty%20start%20screen.png?raw=true)

Only a blinking cursor with the presently selected screencode and attributes is visible.

Press these keys in main mode for editing:

|Key|Description
|---|---|
|**Cursor keys**|Move cursor
|**+**|Next character (increase screen code)
|**-**|Previous character (decrease screen code)
|**,**|Previous color (decrease color number)
|**.**|Next color (increase color number)
|**SPACE**|Plot with present screen code and attributes
|**DEL**|Clear present cursor position (plot white space)
|**U**|Toggle '**U**nderline' attribute
|**B**|Toggle '**B**link' attribute
|**R**|Toggle '**R**everse' attribute
|**A**|Toggle '**A**lternate character set' attribute
|**E**|Go to 'character **E**dit mode' with present screen code
|**G**|**G**rab underlying character and attribute at cursor position
|**W**|Go to '**W**rite mode'
|**C**|Go to '**C**olor write mode'
|**L**|Go to '**L**ine and box mode'
|**M**|Go to '**M**ove mode'
|**S**|Go to '**S**elect mode'
|**Z**|Undo
|**Y**|Redo
|**I**|Toggle '**I**nverse': toggle increase/decrease screencode by 128
|**HOME**|Move cursor to upper left corner of canvas
|**F1**|Go to main menu
|**F8**|Help screen

*Moving cursor*

Press the **cursor keys** to move the cursor around the screen. If the canvas size is bigger than the 80x25 screensize, the sreen will scroll on reaching the edges.

Pressing **HOME** will return the cursor to the upper left position.

*Selecting the screencode to plot*

The **+** or **-** key will increase resp. decrease the selected screencode by one. The cursor will update to the presently selected screencode.

Pressing **I** will increase the screencode by 128 if the present screencode is lower than 128, otherwise decrease by 128. This will emulate RVS On / RVS Off.

*Selecting the attributes to plot*

Increase or decrease the [color code](#color-value-reference) by one by pressing the **.** resp. **,** key. Pressing **U**, **B**, **R** or **A** will toggle the **U**nderline, **B**link, **R**everse or **A**lternate charset attribute.
The cursor will update to show the updated attribute code.

*Plotting and deleting a character*

Press **SPACE** to plot the presently selected character in the presently selected attributes at the present cursor position. **DEL** will delete the character and attribute value at the present position.

*Grabbing a character*

Pressing **G** will 'grab' the character and attributes at the present cursor position and change the selected character screencode and attribute to these values for use in all other edit functions.

*Character edit mode*

This will enter [character edit mode](#character-editor) and start with editing the presently selected screencode. Tip: if you want to edit a specific character on the screen, grab that character first by moving the cursor on that character and press **G** for grab.

*Enter edit modes*

Press **S** ([Select mode](#select-mode)) , **M** ([Move mode](#move-mode)), **L** ([Line and box mode](#line-and-box-mode)), **W** ([Write mode](#write-mode)) or **C** ([Color write mode](#color-write-mode)) for entering the corresponding edit modes.
Reference is made to the specific sections in this readme for these modes (click the links). From all modes, return to main mode by pressing **ESC** or **STOP**.

NB: No visible clue is given which mode is activated (due to constraints by not being able to take unaltered charsets for granted and the cursor already used for showing screencode and attribute selected).


## Character editor:
([Back to contents](#contents))

|Key|Description
|---|---|
|**Cursor keys**|Move cursor
|**+**|Next character (increase screen code)
|**-**|Previous character (decrease screen code)
|**SPACE**|Toggle pixel at cursor position (plot/delete pixel)
|**DEL**|Clear character (delete all pixels of present character)
|**I**|**I**nverse character
|**Z**|**U**ndo: revert present character to original state
|**S**|Re**s**tore character from system character set (=lower case system ROM charset)
|**C**|**C**opy present character
|**V**|Paste present character
|**A**|Toggle between standard and **A**lternate charset and vice versa
|**X / Y**|Mirror in **X** axis or **Y**-axis
|**O**|R**O**tate clockwise
|**L** / **R** / **U** / **D**|Scroll **L**eft, **R**ight, **U**p or **D**own
|**H**|Input **H**ex value for line at cursor position
|**ESC** / **STOP**|Leave character mode and go back to main mode
|**F8**|Help screen

## Select mode:
([Back to contents](#contents))

|Key|Description
|---|---|
|**X**|Cut: Delete selection at old position and paste at new position
|**C**|**C**opy: Copy selection at new position, leaving selection unchanged at old position
|**D**|**D**elete selection (fill with spaces)
|**A**|Paint with **A**ttribute: change attribute value of selection to present attribute value
|**P**|**P**aint with color: change only the color value of selection
|**RETURN**|Accept selection / accept new position
|**ESC** / **STOP**|Cancel and go back to main mode
|**Cursor keys**|Expand/shrink in the selected direction / Move cursor to select destination position
|**F8**|Help screen

## Move mode:
([Back to contents](#contents))

|Key|Description
|---|---|
|**Cursor keys**|Move in the selected direction
|**RETURN**|Accept moved position
|**ESC** / **STOP**|Cancel and go back to main mode
|**F8**|Help screen

## Line and box mode:
([Back to contents](#contents))

|Key|Description
|---|---|
|**Cursor keys**|Expand/shrink in the selected direction
|**RETURN**|Accept line or box
|**ESC** / **STOP**|Cancel and go back to main mode
|**F8**|Help screen

## Write mode:
([Back to contents](#contents))

|Key|Description
|---|---|
|**Cursor keys**|Move in the selected direction
|**DEL**|Clear present cursor position (plot white space)
|**F1**|Toggle 'blink' attribute
|**F3**|Toggle 'underline' attribute
|**F5**|Toggle 'reverse' attribute
|**F7**|Toggle '*alternate character set' attribute
|**F2**|Undo
|**F4**|Redo
|**C=** / **CONTROL** + **1-8**|Select color
|**CONTROL** + **9 / 0**|RVS On / RVS Off (toggle screencode + 128)
|**ESC** / **STOP**|Go back to main mode
|**F8**|Help screen
|**Other keys**|Plot corresponding character (if printable)

## Color write mode:
([Back to contents](#contents))

|Key|Description
|---|---|
|**Cursor keys**|Move in the selected direction
|**0-9** / **A-F**|Plot color with corresponding hex number
|**F1**|Toggle 'blink' attribute
|**F3**|Toggle 'underline' attribute
|**F5**|Toggle 'reverse' attribute
|**F7**|Toggle '*alternate character set' attribute
|**F2**|Undo
|**F4**|Redo
|**ESC** / **STOP**|Go back to main mode
|**F8**|Help screen

## Color value reference:
([Back to contents](#contents))

|Color|Decimal|Color write key|Write mode key|
|---|--:|--:|---|
|Black|0|0|**CONTROL+1**
|Dark Grey|1|1|**C= + 5**
|Dark Blue|2|2|**CONTROL+7**
|Light Blue|3|3|**C= + 7**
|Dark Green|4|4|**CONTROL + 6**
|Light Green|5|5|**C= + 6**
|Dark Cyan|6|6|**C= + 4**
|Light Cyan|7|7|**CONTROL + 4**
|Dark Red|8|8|**CONTROL + 3**
|Light Red|9|9|**C= + 3**
|Dark Purple|10|A|**C= + 1**
|Light Purple|11|B|**CONTROL + 5**
|Dark Yellow|12|C|**C= + 2**
|Light Yellow|13|D|**CONTROL + 8**
|Light Grey|14|E|**C= + 8**
|White|15|F|**CONTROL+2**

## Attribute code reference:
([Back to contents](#contents))

The VDC chip uses a byte per character position with the following meaning of the bits in that byte (for character mode):

|Bit|7|6|5|4|3|2|1|0|
|---|---|---|---|---|---|---|---|---|
|Meaning of bit|Alternate|Reverse|Underline|Blink|Red|Green|Blue|Intensity|

This means an attribute code is calculated like:

|Add value|Meaning|
|---|---|
|0-15|Color value 0 to 15, see [color value reference](#color-value-reference) above.
|16|Enable Blink|
|32|Enable Underline|
|64|Enable Reverse|
|128|Enable alternate character set (not enabled shows the character in the standard character set)

Example:

- Light yellow text with underline and in alternate character set: 13+32+128=173
- Darkgreen text in reverse and standard character set: 4+64=68

Note that in VDCSE calculation of these attribute codes by yourselves is not necessary, the program will do so for you given the selected attributes and color. In memory however this is how the codes are stored.

## File format reference
([Back to contents](#contents))

As both the character data as the attribute data needs to be stored, a screen takes width times height times 2 bytes in storage. A padding of 48 bytes is used to separate character and attribute data in order to be able to load a standard 80x25 screen for both character and attribute data in one go at the default VDC memory position of $0000 for text and $0800 for attributes.

For a default 80 characters wide and 25 characters high screen this would result that data would be stored as such:

|Offset to start address in bytes (decimal)|Offset in hex|Description|
|--:|--:|---|
|0|0|Start of text character data using [screen codes](https://sta.c64.org/cbm64scr.html)
|2000|07D0|Start of 48 byte padding. This is used to place a VDCSE version signature
|2048|0800|Start of attribute data using VDC [attribute codes](#attribute-code-reference).

For screensizes greater than 80x25 this would translate to:

|Offset to start address in bytes (decimal)|Description|
|--:|---|
|0|Start of text character data using [screen codes](https://sta.c64.org/cbm64scr.html)
|Width * Height|Start of 48 byte padding. This is used to place a VDCSE version signature
|(Width * Height)+48|Start of attribute data using VDC [attribute codes](#attribute-code-reference).

## Credits
([Back to contents](#contents))

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

    DraBrowse (db*) is a simple file browser. Originally created 2009 by Sascha Bader. Used version adapted by Dirk Jagdmann (doj).

    https://github.com/doj/dracopy

-   Bart van Leeuwen: For inspiration and advice while coding. Also for providing the excellent Device Manager ROM to make testing on real hardware very easy

-   Original windowing system code on Commodore 128 by unknown author.
   
-   Tested using real hardware (C128D and C128DCR) plus VICE.

The code can be used freely as long as you retain a notice describing original source and author.

THE PROGRAMS ARE DISTRIBUTED IN THE HOPE THAT THEY WILL BE USEFUL, BUT WITHOUT ANY WARRANTY. USE THEM AT YOUR OWN RISK!

([Back to contents](#contents))
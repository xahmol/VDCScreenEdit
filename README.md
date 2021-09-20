# VDC Screen Editor
 C128 80 column screen editor

Main mode:

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

Main menu:

|Key|Description
|---|---|
|**Cursor LEFT / RIGHT**|Move between main menu options
|**Cursor UP/ DOWN**|Move between pulldown menu options
|**RETURN**|Select highlighted menu option
|**ESC** / **STOP**|Leave menu and go back

Character editor:

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

Select mode:
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

Move mode:
|Key|Description
|---|---|
|**Cursor keys**|Move in the selected direction
|**RETURN**|Accept moved position
|**ESC** / **STOP**|Cancel and go back to main mode
|**F8**|Help screen

Line and box mode:
|Key|Description
|---|---|
|**Cursor keys**|Expand/shrink in the selected direction
|**RETURN**|Accept line or box
|**ESC** / **STOP**|Cancel and go back to main mode
|**F8**|Help screen

Write mode:
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

Color write mode:
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

Color values:
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
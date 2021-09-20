#ifndef __DEFINES_H_
#define __DEFINES_H_

/* Machine code area addresses mapping */
#define MACOSTART           0x1300      // Start of machine code area
#define MACOSIZE            0x0800      // Length of machine code area

/* Bank 1 memory addresses mapping */
#define WINDOWBASEADDRESS   0x2000      // Base address for windows system data, 8k reserved
#define CHARSETSYSTEM       0x4000      // Base address for system charset
#define CHARSETNORMAL       0x4800      // Base address for normal charset
#define CHARSETALTERNATE    0x5000      // Base address for alternate charset
#define SCREENMAPBASE       0x5800      // Base address for screen map
#define MEMORYLIMIT         0xCFFF      // Upper memory limit address for address map

/* Char defines */
#define CH_SPACE            32          // Screencode for space
#define CH_BLACK            144         // Petscii control code for black           CTRL-1
#define CH_WHITE            5           // Petscii control code for white           CTRL-2
#define CH_DRED             28          // Petscii control code for dark red        CTRL-3
#define CH_LCYAN            159         // Petscii control code for light cyan      CTRL-4
#define CH_LPURPLE          156         // Petscii control code for light purple    CTRL-5
#define CH_DGREEN           30          // Petscii control code for dark green      CTRL-6
#define CH_DBLUE            31          // Petscii control code for dark blue       CTRL-7
#define CH_LYELLOW          158         // Petscii control code for light yellow    CTRL-8
#define CH_RVSON            18          // Petscii control code for RVS ON          CTRL-9
#define CH_RVSOFF           146         // Petscii control code for RVS OFF         CTRL-0
#define CH_DPURPLE          129         // Petscii control code for dark purple     C=-1
#define CH_DYELLOW          149         // Petscii control code for dark yellow     C=-2
#define CH_LRED             150         // Petscii control code for light red       C=-3
#define CH_DCYAN            151         // Petscii control code for dark cyan       C=-4
#define CH_DGREY            152         // Petscii control code for dark grey       C=-5
#define CH_LGREEN           153         // Petscii control code for light green     C=-6
#define CH_LBLUE            154         // Petscii control code for light blue      C=-7
#define CH_LGREY            155         // Petscii control code for light grey      C=-8


/* Declaration global variables as externals */
extern unsigned char bootdevice;

/* Defines for versioning */
/* Version number */
#define VERSION_MAJOR 0
#define VERSION_MINOR 1
/* Build year */
#define BUILD_YEAR_CH0 (__DATE__[ 7])
#define BUILD_YEAR_CH1 (__DATE__[ 8])
#define BUILD_YEAR_CH2 (__DATE__[ 9])
#define BUILD_YEAR_CH3 (__DATE__[10])
/* Build month */
#define BUILD_MONTH_IS_JAN (__DATE__[0] == 'J' && __DATE__[1] == 'a' && __DATE__[2] == 'n')
#define BUILD_MONTH_IS_FEB (__DATE__[0] == 'F')
#define BUILD_MONTH_IS_MAR (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'r')
#define BUILD_MONTH_IS_APR (__DATE__[0] == 'A' && __DATE__[1] == 'p')
#define BUILD_MONTH_IS_MAY (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'y')
#define BUILD_MONTH_IS_JUN (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'n')
#define BUILD_MONTH_IS_JUL (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'l')
#define BUILD_MONTH_IS_AUG (__DATE__[0] == 'A' && __DATE__[1] == 'u')
#define BUILD_MONTH_IS_SEP (__DATE__[0] == 'S')
#define BUILD_MONTH_IS_OCT (__DATE__[0] == 'O')
#define BUILD_MONTH_IS_NOV (__DATE__[0] == 'N')
#define BUILD_MONTH_IS_DEC (__DATE__[0] == 'D')
#define BUILD_MONTH_CH0 \
    ((BUILD_MONTH_IS_OCT || BUILD_MONTH_IS_NOV || BUILD_MONTH_IS_DEC) ? '1' : '0')
#define BUILD_MONTH_CH1 \
    ( \
        (BUILD_MONTH_IS_JAN) ? '1' : \
        (BUILD_MONTH_IS_FEB) ? '2' : \
        (BUILD_MONTH_IS_MAR) ? '3' : \
        (BUILD_MONTH_IS_APR) ? '4' : \
        (BUILD_MONTH_IS_MAY) ? '5' : \
        (BUILD_MONTH_IS_JUN) ? '6' : \
        (BUILD_MONTH_IS_JUL) ? '7' : \
        (BUILD_MONTH_IS_AUG) ? '8' : \
        (BUILD_MONTH_IS_SEP) ? '9' : \
        (BUILD_MONTH_IS_OCT) ? '0' : \
        (BUILD_MONTH_IS_NOV) ? '1' : \
        (BUILD_MONTH_IS_DEC) ? '2' : \
        /* error default */    '?' \
    )
/* Build day */
#define BUILD_DAY_CH0 ((__DATE__[4] >= '0') ? (__DATE__[4]) : '0')
#define BUILD_DAY_CH1 (__DATE__[ 5])
/* Build hour */
#define BUILD_HOUR_CH0 (__TIME__[0])
#define BUILD_HOUR_CH1 (__TIME__[1])
/* Build minute */
#define BUILD_MIN_CH0 (__TIME__[3])
#define BUILD_MIN_CH1 (__TIME__[4])

#endif // __DEFINES_H_
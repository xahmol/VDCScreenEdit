#ifndef __PRGGENERATOR_H_
#define __PRGGENERATOR_H_

#define ASS_SIZE        0x0183
#define MAC_SIZE        0x0053
#define SCREEN_SIZE     0x1000
#define CHAR_SIZE       0x0800

#define BASEADDRESS     0x1C01
#define VERSIONADDRESS  0x3D
#define MACOADDRESS     0x0C00

#define BGCOLORADDRESS  0x1C7B
#define CHARSTDADDRESS  0x1C7C
#define CHARALTADDRESS  0x1C7E

// Defines for MMU modes, MMU $FF00 configuration values
#define MMU_BANK0               0x3e  // Bank 0 with full RAM apart from I/O area
#define MMU_BANK1               0x7e  // Bank 1 with full RAM apart from I/O area        
#define MMU_BANK2               0x3f  // Bank 0 with full RAM
#define MMU_BANK3               0x7f  // Bank 1 with full RAM

// Variables in core Functions
extern unsigned char VDC_addrh;
extern unsigned char VDC_addrl;
extern unsigned char VDC_value;
extern unsigned char VDC_tmp1;
extern unsigned char VDC_tmp3;

// Import assembly core Functions
void SetLoadSaveBank_core();
void POKEB_core();

// Function Prototypes
void SetLoadSaveBank(unsigned char bank);
void POKEB(unsigned int address, unsigned char bank, unsigned char value);

#endif // __PRGGENERATOR_H_
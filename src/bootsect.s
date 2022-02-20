; Create bootsector for building the disk images
; Credit to Scott Robison for this method    
    
    .segment "BOOTSECT"
    .define executable "vdcse"

    ; signature that identifies this as a boot sector
    .byte "cbm"

    ; load address for extra sectors (not used)
    .word 0

    ; ram block for extra sectors (not used)
    .byte 0

    ; number of extra sectors to load (none)
    .byte 0

    ; message to display after BOOT text
    .byte "vdc screen editor", 0

    ; file to load (not used)
    .byte 0

    ; boot routine
    LDA $BA
    AND #3
    TAY

    LDA CMD_LO,Y
    TAX
    LDA CMD_HI,Y
    TAY

    JMP $AFA5 ; J_EXECUTE_A_LINE

    ; BASIC commands to execute
CMD_8:
    .byte "run", $22, executable, $22, ",u8", $00
CMD_9:
    .byte "run", $22, executable, $22, ",u9", $00
CMD_10:
    .byte "run", $22, executable, $22, ",u10", $00
CMD_11:
    .byte "run", $22, executable, $22, ",u11", $00

CMD_LO:
    .byte <(CMD_8-1),<(CMD_9-1),<(CMD_10-1),<(CMD_11-1)
CMD_HI:
    .byte >(CMD_8-1),>(CMD_9-1),>(CMD_10-1),>(CMD_11-1)

    ; pad the remaining bytes in the sector with nul
    .align  256


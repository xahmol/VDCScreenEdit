; VDC Screen Editor
; Screen editor for the C128 80 column mode
; Written in 2021 by Xander Mol

; https://github.com/xahmol/VDCScreenEdit
; https://www.idreamtin8bits.com/

; Machine code routines for VDCSE2PRG generator program

	.export		_SetLoadSaveBank_core
	.export		_POKEB_core
	.export		_VDC_addrh
	.export		_VDC_addrl
	.export		_VDC_value
	.export		_VDC_tmp1
	.export		_VDC_tmp3
    
    .segment "GENMACO"

    ; Load address
    .word $0C00

    _VDC_addrh:
    	.res	1
    _VDC_addrl:
    	.res	1
    _VDC_value:
    	.res	1
    _VDC_tmp1:
    	.res	1
    _VDC_tmp3:
    	.res	1
    ZPtmp1:
    	.res	1
    ZPtmp2:
    	.res	1
    MemConfTmp:
    	.res	1

    ; ------------------------------------------------------------------------------------------
    SaveMMUandZP:
    ; Function to safeguard memory configuration and ZP addresses and set selected MMU
    ; Input:	_VDC_tmp3	= selected MMU config
    ; Output:	MemConfTmp	= Temporary location to safeguard MMU config
    ;		    ZPtmp1		= Temporary location to safeguard ZP $fb
    ;			ZPtmp2		= Temporary location to safeguard ZP $fc
    ; ------------------------------------------------------------------------------------------

    	; Safeguard memory configuration and set memory config to selected MMU config
    	lda	$ff00							; Obtain present memory configuration
    	sta MemConfTmp						; Store in temp location for safeguarding
    	lda _VDC_tmp3						; Obtain selected MMU config
    	sta $ff00							; Store selected MMU config
    
    	; Store $FA and $FB addresses for safety to be restored at exit
    	lda $fb								; Obtain present value at $fb
    	sta ZPtmp1							; Store to be restored later
    	lda $fc								; Obtain present value at $fc
    	sta ZPtmp2							; Store to be restored later
    	rts

    ; ------------------------------------------------------------------------------------------
    RestoreMMUandZP:
    ; Function to restore memory configuration and ZP addresses and set selected MMU
    ; Input:	MemConfTmp	= Temporary location to safeguard MMU config
    ;		    ZPtmp1		= Temporary location to safeguard ZP $fb
    ;			ZPtmp2		= Temporary location to safeguard ZP $fc
    ; ------------------------------------------------------------------------------------------

	; Restore $fb and $fc
	lda ZPtmp1							; Obtain stored value of $fb
	sta $fb								; Restore value
	lda ZPtmp2							; Obtain stored value of $fc
	sta $fc								; Restore value

	; Restore memory configuration
	lda MemConfTmp						; Obtain saved memory config
	sta $ff00							; Restore memory config
    rts

    ; ------------------------------------------------------------------------------------------
    _SetLoadSaveBank_core:
    ; Function to set bank for I/O operations
    ; Input:	VDC_tmp1 = bank number
    ; ------------------------------------------------------------------------------------------
    	lda _VDC_tmp1						; Obtain bank number to load/save in
    	ldx #0								; Set bank for filename as 0
    	jsr $ff68							; Call SETBNK kernal function
    	rts

    ; ------------------------------------------------------------------------------------------
    _POKEB_core:
    ; Function to poke to a memory position in specified bank
    ; Input:	VDC_addrh and VDC_addrl:	high and low byte of address to poke
    ;			VDC_tmp3:					MMU config for poke
    ;			VDC_value:					value to poke
    ; ------------------------------------------------------------------------------------------

    	jsr SaveMMUandZP					; Safeguard MMU/ZP and set MMU

    	; Set address pointer in zero-page
    	lda _VDC_addrl						; Obtain low byte in A
    	sta $fb								; Store low byte in pointer
    	lda _VDC_addrh						; Obtain high byte in A
    	sta $fc								; Store high byte in pointer

    	; Store value in specified address
    	ldy #$00							; Clear Y index
    	lda _VDC_value						; Load value in A
    	sta ($fb),y							; Store at destination

    	jsr RestoreMMUandZP					; Restore MU/ZP
        rts
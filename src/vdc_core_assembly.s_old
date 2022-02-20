; ====================================================================================
; vdc_core_assembly.s
; Core assembly routines for vdc_core.c
;
; Credits for code and inspiration:
;
; C128 Programmers Reference Guide:
; http://www.zimmers.net/anonftp/pub/cbm/manuals/c128/C128_Programmers_Reference_Guide.pdf
;
; Scott Hutter - VDC Core functions inspiration:
; https://github.com/Commodore64128/vdc_gui/blob/master/src/vdc_core.c
; (used as starting point, but channged to inline assembler for core functions, added VDC wait statements and expanded)
;
; Francesco Sblendorio - Screen Utility:
; https://github.com/xlar54/ultimateii-dos-lib/blob/master/src/samples/screen_utility.c
;
; DevDef: Commodore 128 Assembly - Part 3: The 80-column (8563) chip
; https://devdef.blogspot.com/2018/03/commodore-128-assembly-part-3-80-column.html
;
; Tips and Tricks for C128: VDC
; http://commodore128.mirkosoft.sk/vdc.html
;
; 6502.org: Practical Memory Move Routines
; http://6502.org/source/general/memory_move.html
;
; =====================================================================================


    .export		_VDC_ReadRegister_core
	.export		_VDC_WriteRegister_core
	.export		_VDC_Poke_core
	.export		_VDC_Peek_core
	.export		_VDC_DetectVDCMemSize_core
	.export		_VDC_SetExtendedVDCMemSize
	.export		_VDC_CopyCharsetsfromROM
	.export		_VDC_SetCursorMode_core
	.export		_VDC_MemCopy_core
	.export		_VDC_HChar_core
	.export		_VDC_VChar_core
	.export		_VDC_CopyMemToVDC_core
	.export		_VDC_CopyVDCToMem_core
	.export		_VDC_RedefineCharset_core
	.export		_VDC_FillArea_core
	.export		_VDC_CopyViewPortToVDC_core
	.export		_VDC_ScrollCopy_core
	.export		_SetLoadSaveBank_core
	.export		_POKEB_core
	.export		_PEEKB_core
	.export		_BankMemCopy_core
	.export		_BankMemSet_core
    .export		_VDC_regadd
	.export		_VDC_regval
	.export		_VDC_addrh
	.export		_VDC_addrl
	.export		_VDC_desth
	.export		_VDC_destl
	.export 	_VDC_strideh
	.export		_VDC_stridel
	.export		_VDC_value
	.export		_VDC_tmp1
	.export		_VDC_tmp2
	.export		_VDC_tmp3
	.export		_VDC_tmp4

VDC_ADDRESS_REGISTER    = $D600
VDC_DATA_REGISTER       = $D601

.segment	"MACO"

_VDC_regadd:
	.res	1
_VDC_regval:
	.res	1
_VDC_addrh:
	.res	1
_VDC_addrl:
	.res	1
_VDC_desth:
	.res	1
_VDC_destl:
	.res	1
_VDC_strideh:
	.res	1
_VDC_stridel:
	.res	1
_VDC_value:
	.res	1
_VDC_tmp1:
	.res	1
_VDC_tmp2:
	.res	1
_VDC_tmp3:
	.res	1
_VDC_tmp4:
	.res	1
ZPtmp1:
	.res	1
ZPtmp2:
	.res	1
ZPtmp3:
	.res	1
ZPtmp4:
	.res	1
MemConfTmp:
	.res	1

; ------------------------------------------------------------------------------------------
_VDC_ReadRegister_core:
; Function to read a VDC register
; Input:	VDC_regadd = register number
; Output:	VDC_regval = read value
; ------------------------------------------------------------------------------------------

	ldx _VDC_regadd                     ; Load register address in X
	stx VDC_ADDRESS_REGISTER            ; Store X in VDC address register
notyetready:							; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER            ; Check status bit 7 of VDC address register
	bpl notyetready                     ; Continue loop if status is not ready
	lda VDC_DATA_REGISTER               ; Load data to A from VDC data register
	sta _VDC_regval                     ; Load A to return variable
    rts

; ------------------------------------------------------------------------------------------
_VDC_WriteRegister_core:
; Function to write a VDC register
; Input:	VDC_regadd = register numnber
;			VDC_regval = value to write
; ------------------------------------------------------------------------------------------

    ldx _VDC_regadd                     ; Load register address in X
	lda _VDC_regval				        ; Load register value in A
notyetready2:							; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER            ; Check status bit 7 of VDC address register
	bpl notyetready2                    ; Continue loop if status is not ready
	sta VDC_DATA_REGISTER               ; Store A to VDC data
    rts

; ------------------------------------------------------------------------------------------
_VDC_Poke_core:
; Function to store a value to a VDC address
; Input:	VDC_addrh = VDC address high byte
;			VDC_addrl = VDC address low byte
;			VDC_value = value to write
; ------------------------------------------------------------------------------------------

    ldx #$12                            ; Load $12 for register 18 (VDC RAM address high) in X	
	lda _VDC_addrh                      ; Load high byte of address in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waithighaddress:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waithighaddress			        ; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data
	inx		    						; Increase X for register 19 (VDC RAM address low)
	lda _VDC_addrl      				; Load low byte of address in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitlowaddress:							; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER           	; Check status bit 7 of VDC address register
	bpl waitlowaddress      			; Continue loop if status is not ready
	sta VDC_DATA_REGISTER       		; Store A to VDC data
	ldx #$1f    						; Load $1f for register 31 (VDC data) in X	
	lda _VDC_value       				; Load high byte of address in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitvalue:								; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitvalue       				; Continue loop if status is not ready
	sta VDC_DATA_REGISTER               ; Store A to VDC data
    rts

; ------------------------------------------------------------------------------------------
_VDC_Peek_core:
; Function to read a value from a VDC address
; Input:	VDC_addrh = VDC address high byte
;			VDC_addrl = VDC address low byte
; Output:	VDC_value = read value
; ------------------------------------------------------------------------------------------

    ldx #$12    						; Load $12 for register 18 (VDC RAM address high) in X	
	lda _VDC_addrh      				; Load high byte of address in A
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waithighaddress2:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waithighaddress2        		; Continue loop if status is not ready
	sta VDC_DATA_REGISTER       		; Store A to VDC data
	inx					    			; Increase X for register 19 (VDC RAM address low)
	lda _VDC_addrl	    	    		; Load low byte of address in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitlowaddress2:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waitlowaddress2			        ; Continue loop if status is not ready
	sta VDC_DATA_REGISTER		        ; Store A to VDC data
	ldx #$1f    						; Load $1f for register 31 (VDC data) in X	
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitvalue2:								; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitvalue2			        	; Continue loop if status is not ready
	lda VDC_DATA_REGISTER	        	; Load VDC data to A
	sta _VDC_value			        	; Load A to return variable
    rts

; ------------------------------------------------------------------------------------------
_VDC_DetectVDCMemSize_core:
; Function to detect the VDC memory size
; Output:	VDC_value = memory size in KB (16 or 64)
; ------------------------------------------------------------------------------------------

	; Setting memory mode to 64KB
	; Reading register 28, safeguarding value, setting bit 4 and storing back to register 28
	ldx #$1c							; Load $1c for register 28 in X
	stx VDC_ADDRESS_REGISTER            ; Store X in VDC address register
notyetreadydms1:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER            ; Check status bit 7 of VDC address register
	bpl notyetreadydms1                 ; Continue loop if status is not ready
	lda VDC_DATA_REGISTER               ; Load data to A from VDC data register
	tay									; Transfer A to Y to save value for later restore
	ora #$10							; Set bit 4 of A
	stx VDC_ADDRESS_REGISTER            ; Store X in VDC address register
notyetreadydms2:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER            ; Check status bit 7 of VDC address register
	bpl notyetreadydms2                 ; Continue loop if status is not ready
	sta VDC_DATA_REGISTER               ; Store A to VDC data

	; Writing a $00 value to VDC $1fff
	ldx #$12                            ; Load $12 for register 18 (VDC RAM address high) in X	
	lda #$1f                    		; Load high byte of address in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waithighaddressdms1:					; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waithighaddressdms1		        ; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data
	inx		    						; Increase X for register 19 (VDC RAM address low)
	lda #$ff      						; Load low byte of address in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitlowaddressdms1:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER           	; Check status bit 7 of VDC address register
	bpl waitlowaddressdms1    			; Continue loop if status is not ready
	sta VDC_DATA_REGISTER       		; Store A to VDC data
	ldx #$1f    						; Load $1f for register 31 (VDC data) in X	
	lda #$00       						; Load value to store in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitvaluedms1:							; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitvaluedms1     				; Continue loop if status is not ready
	sta VDC_DATA_REGISTER               ; Store A to VDC data

	; Writing a $ff value to VDC $9fff
	ldx #$12                            ; Load $12 for register 18 (VDC RAM address high) in X	
	lda #$9f                    		; Load high byte of address in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waithighaddressdms2:					; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waithighaddressdms2		        ; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data
	inx		    						; Increase X for register 19 (VDC RAM address low)
	lda #$ff      						; Load low byte of address in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitlowaddressdms2:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER           	; Check status bit 7 of VDC address register
	bpl waitlowaddressdms2    			; Continue loop if status is not ready
	sta VDC_DATA_REGISTER       		; Store A to VDC data
	ldx #$1f    						; Load $1f for register 31 (VDC data) in X	
	lda #$ff       						; Load value to store in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitvaluedms2:							; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitvaluedms2     				; Continue loop if status is not ready
	sta VDC_DATA_REGISTER               ; Store A to VDC data

	; Reading back value of VDC $1fff
    ldx #$12    						; Load $12 for register 18 (VDC RAM address high) in X	
	lda #$1f		     				; Load high byte of address in A
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waithighaddressdms3:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waithighaddressdms3        		; Continue loop if status is not ready
	sta VDC_DATA_REGISTER       		; Store A to VDC data
	inx					    			; Increase X for register 19 (VDC RAM address low)
	lda #$ff    	    				; Load low byte of address in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitlowaddressdms3:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waitlowaddressdms3			    ; Continue loop if status is not ready
	sta VDC_DATA_REGISTER		        ; Store A to VDC data
	ldx #$1f    						; Load $1f for register 31 (VDC data) in X	
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitvaluedms3:							; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitvaluedms3		        	; Continue loop if status is not ready
	lda VDC_DATA_REGISTER	        	; Load VDC data to A

	; Comparing value with $ff to see if 64KB address could be read
	bne sixteendetected					; If not equal 16KB detected, so branch
	lda #64								; Load 64 as value to A
	jmp dmsend							; Jump to end of routine
sixteendetected:						; Label for 16KB detected
	lda #16								; Load 16 as value to A

	; Restore bit 4 of register 28
dmsend:									; Label for end of routine
	sta _VDC_value						; Load KB size to return value
	tya									; Transfer value stored in Y back to A
	ldx #$1c							; Store $1c in A for register 28
	stx VDC_ADDRESS_REGISTER            ; Store X in VDC address register
notyetreadydms3:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER            ; Check status bit 7 of VDC address register
	bpl notyetreadydms3                 ; Continue loop if status is not ready
	sta VDC_DATA_REGISTER               ; Store A to VDC data
	rts									; Return

; ------------------------------------------------------------------------------------------
_VDC_SetExtendedVDCMemSize:
; Function to set VDC in 64k memory configuration
; NB: Charsets need to be copied from ROM again after doing this
; ------------------------------------------------------------------------------------------

	; Setting memory mode to 64KB
	; Reading register 28, safeguarding value, setting bit 4 and storing back to register 28
	ldx #$1c							; Load $1c for register 28 in X
	stx VDC_ADDRESS_REGISTER            ; Store X in VDC address register
notyetreadysev1:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER            ; Check status bit 7 of VDC address register
	bpl notyetreadysev1	                ; Continue loop if status is not ready
	lda VDC_DATA_REGISTER               ; Load data to A from VDC data register
	ora #$10							; Set bit 4 of A
	stx VDC_ADDRESS_REGISTER            ; Store X in VDC address register
notyetreadysev2:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER            ; Check status bit 7 of VDC address register
	bpl notyetreadysev2                 ; Continue loop if status is not ready
	sta VDC_DATA_REGISTER               ; Store A to VDC data
	rts

; ------------------------------------------------------------------------------------------
_VDC_CopyCharsetsfromROM:
; Function to set VDC in 64k memory configuration
; NB: Charsets need to be copied from ROM again after doing this
; ------------------------------------------------------------------------------------------

	; Kernal call to DLCHR kernal function to copy charsets from ROM to VDC
	jsr	$FF62							;initialize 8563 char. defns.
	rts

; ------------------------------------------------------------------------------------------
_VDC_SetCursorMode_core:
; Function to set cursor mode
; Input:	VDC_value = Cursormode value
; ------------------------------------------------------------------------------------------

	ldx #$12    						; Load $12 for register 18 (VDC RAM address high) in X	
	lda #$00		      				; Load zero
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waithighaddresscm:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waithighaddresscm        		; Continue loop if status is not ready
	sta VDC_DATA_REGISTER       		; Store A to VDC data
	inx					    			; Increase X for register 19 (VDC RAM address low)
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitlowaddresscm:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waitlowaddresscm		        ; Continue loop if status is not ready
	sta VDC_DATA_REGISTER		        ; Store A to VDC data
	ldx #$0A							; Load $0A for register 10 in X
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitvaluecm:							; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitvaluecm       				; Continue loop if status is not ready
	sta VDC_DATA_REGISTER               ; Store A to VDC data
    rts

; ------------------------------------------------------------------------------------------
_VDC_MemCopy_core:
; Function to copy memory from one to another position within VDC memory
; Input:	VDC_addrh = high byte of source address
;			VDC_addrl = low byte of source address
;			VDC_desth = high byte of destination address
;			VDC_destl = low byte of destination address
;			VDC_tmp1 = number of 256 byte pages to copy
;			VDC_tmp2 = length in last page to copy
; ------------------------------------------------------------------------------------------

loopmemcpy:
	; Hi-byte of the destination address to register 18
	ldx #$12    						; Load $12 for register 18 (VDC RAM address high) in X	
	lda _VDC_desth      				; Load high byte of dest in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitdesthighaddress:					; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitdesthighaddress     		; Continue loop if status is not ready
	sta VDC_DATA_REGISTER       		; Store A to VDC data

	; Lo-byte of the destination address to register 19
	ldx #$13    						; Load $13 for register 19 (VDC RAM address high) in X	
	lda _VDC_destl       				; Load high byte of dest in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitdestlowaddress:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waitdestlowaddress	        	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data

	; Set the copy bit (bit 7) of register 24 (block copy mode)
	ldx #$18    						; Load $18 for register 24 (block copy mode) in X	
	lda #$80			        		; Set copy bit
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitsetcopybit:							; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waitsetcopybit		        	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data

	; Hi-byte of the source address to block copy source register 32
	ldx #$20					    	; Load $20 for register 32 (block copy source) in X	
	lda _VDC_addrh			        	; Load high byte of source in A
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitsrchighaddress:				    	; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waitsrchighaddress		        ; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data
	
	; Lo-byte of the source address to block copy source register 33
	ldx #$21					    	; Load $21 for register 33 (block copy source) in X	
	lda _VDC_addrl		        		; Load low byte of source in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitsrclowaddress:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitsrclowaddress		        ; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data
	
	; Number of bytes to copy
	ldx #$1E    						; Load $1E for register 30 (word count) in X
	lda _VDC_tmp1		        		; Load page counter in A
	cmp #$01    						; Check if this is the last page
	bne notyetlastpage			        ; Branch to 'not yet last page' if not equal
	lda _VDC_tmp2		        		; Set length in last page
	jmp lastpage		        		; Goto last page label
notyetlastpage:							; Label for not yet last page
	lda #$ff    						; Set length for 256 bytes
lastpage:								; Label for jmp if last page
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitsetlength:							; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waitsetlength		        	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data

	; Decrease page counter and loop until last page
	inc _VDC_desth		        		; Increase destination address page counter
	inc _VDC_addrh		        		; Increase source address page counter
	dec _VDC_tmp1		        		; Decrease page counter
	bne loopmemcpy				        ; Repeat loop until page counter is zero
    rts

; ------------------------------------------------------------------------------------------
_VDC_HChar_core:
; Function to draw horizontal line with given character (draws from left to right)
; Input:	VDC_addrh = igh byte of start address
;			VDC_addrl = ow byte of start address
;			VDC_tmp1 = character value
;			VDC_tmp2 = length value
;			VDC_tmp3 = attribute value
; ------------------------------------------------------------------------------------------

	; Hi-byte of the destination address to register 18
	ldx #$12    						; Load $12 for register 18 (VDC RAM address high) in X	
	lda _VDC_addrh	        			; Load high byte of start in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitstarthighaddress:					; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waitstarthighaddress	        ; Continue loop if status is not ready
	sta VDC_DATA_REGISTER		        ; Store A to VDC data

	; Lo-byte of the destination address to register 19
	ldx #$13    						; Load $13 for register 19 (VDC RAM address high) in X	
	lda _VDC_addrl		        		; Load high byte of start in A
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitstartlowaddress:					; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waitstartlowaddress		        ; Continue loop if status is not ready
	sta VDC_DATA_REGISTER		        ; Store A to VDC data

	; Store character to write in data register 31
	ldx #$1f    						; Load $1f for register 31 (VDC data) in X	
	lda _VDC_tmp1			        	; Load character value in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitvalue4:								; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waitvalue4			        	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data

	; Clear the copy bit (bit 7) of register 24 (block copy mode)
	ldx #$18    						; Load $18 for register 24 (block copy mode) in X	
	lda #$00				        	; Load 0 in A
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitclearcopybit:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waitclearcopybit		        ; Continue loop if status is not ready
	sta VDC_DATA_REGISTER		        ; Store A to VDC data

	; Store lenth in data register 30
	ldx #$1e    						; Load $1f for register 30 (word count) in X	
	lda _VDC_tmp2			        	; Load character value in A
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitlength:								; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitlength			        	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data

	; Continue with copying attribute values
	clc									; Clear carry
	lda _VDC_addrh						; Load high byte of start address again in A
	adc #$08							; Add 8 pages to get charachter attribute address

	; Hi-byte of the destination attribute address to register 18
	ldx #$12    						; Load $12 for register 18 (VDC RAM address high) in X	
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitstarthigatthaddress:				; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waitstarthigatthaddress	        ; Continue loop if status is not ready
	sta VDC_DATA_REGISTER		        ; Store A to VDC data

	; Lo-byte of the destination attribute address to register 19
	ldx #$13    						; Load $13 for register 19 (VDC RAM address high) in X	
	lda _VDC_addrl		        		; Load high byte of start in A
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitstartlowattaddress:					; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waitstartlowattaddress		    ; Continue loop if status is not ready
	sta VDC_DATA_REGISTER		        ; Store A to VDC data

	; Store attribute to write in data register 31
	ldx #$1f    						; Load $1f for register 31 (VDC data) in X	
	lda _VDC_tmp3			        	; Load attribute value in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitvalueatt:							; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waitvalueatt			       	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data

	; Clear the copy bit (bit 7) of register 24 (block copy mode)
	ldx #$18    						; Load $18 for register 24 (block copy mode) in X	
	lda #$00				        	; Load prepared value with bit 7 set in A
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitclearcopybitatt:					; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waitclearcopybitatt		        ; Continue loop if status is not ready
	sta VDC_DATA_REGISTER		        ; Store A to VDC data

	; Store lenth in data register 30
	ldx #$1e    						; Load $1f for register 30 (word count) in X	
	lda _VDC_tmp2			        	; Load character value in A
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitlengthatt:							; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitlengthatt			       	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data
    rts

; ------------------------------------------------------------------------------------------
_VDC_VChar_core:
; Function to draw vertical line with given character (draws from top to bottom)
; Input:	VDC_addrh = high byte of start address
;			VDC_addrl = low byte of start address
;			VDC_tmp1 = character value
;			VDC_tmp2 = length value
;			VDC_tmp3 = attribute value
; ------------------------------------------------------------------------------------------

loopvchar:
	; Hi-byte of the destination address to register 18
	ldx #$12    						; Load $12 for register 18 (VDC RAM address high) in X	
	lda _VDC_addrh		        		; Load high byte of start in A
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitstarthighaddress1:					; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitstarthighaddress1        	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER		        ; Store A to VDC data

	; Lo-byte of the destination address to register 19
	ldx #$13    						; Load $13 for register 19 (VDC RAM address high) in X	
	lda _VDC_addrl			        	; Load high byte of start in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitstartlowaddress1:					; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waitstartlowaddress1        	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data

	; Store character to write in data register 31
	ldx #$1f    						; Load $1f for register 31 (VDC data) in X	
	lda _VDC_tmp1			        	; Load character value in A
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitvalue5:								; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitvalue5			        	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER           	; Store A to VDC data

	; Continue with attribute value
	clc									; CLear carry
	lda _VDC_addrh						; Load high byte of start address again in A
	adc #$08							; Add 8 pages to get charachter attribute address

	; Hi-byte of the destination attribute address to register 18
	ldx #$12    						; Load $12 for register 18 (VDC RAM address high) in X	
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitstarthighattaddress1:				; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitstarthighattaddress1       	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER		        ; Store A to VDC data

	; Lo-byte of the destination attribute address to register 19
	ldx #$13    						; Load $13 for register 19 (VDC RAM address high) in X	
	lda _VDC_addrl			        	; Load high byte of start in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitstartlowattaddress1:				; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waitstartlowattaddress1        	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data

	; Store attribute to write in data register 31
	ldx #$1f    						; Load $1f for register 31 (VDC data) in X	
	lda _VDC_tmp3			        	; Load attribute value in A
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitvalueatt5:							; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitvalueatt5		        	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER           	; Store A to VDC data

	; Increase start address with 80 for next line
	clc 								; Clear carry
	lda _VDC_addrl	        			; Load low byte of address to A
	adc #$50    						; Add 80 with carry
	sta _VDC_addrl			        	; Store result back
	lda _VDC_addrh	        			; Load high byte of address to A
	adc #$00    						; Add 0 with carry
	sta _VDC_addrh	        			; Store result back

	; Loop until length reaches zero
	dec _VDC_tmp2		        		; Decrease length counter
	bne loopvchar		        		; Loop if not zero
    rts

; ------------------------------------------------------------------------------------------
_VDC_CopyMemToVDC_core:
; Function to copy memory from VDC memory to standard memory
; Input:	VDC_addrh = high byte of source address
;			VDC_addrl = low byte of source address
;			VDC_desth = high byte of VDC destination address
;			VDC_destl = low byte of VDC destination address
;			VDC_tmp1 = number of 256 byte pages to copy
;			VDC_tmp2 = length in last page to copy
;			VDC_tmp3 = MMU config of source
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

	; Set address pointer in zero-page
	lda _VDC_addrl						; Obtain low byte in A
	sta $fb								; Store low byte in pointer
	lda _VDC_addrh						; Obtain high byte in A
	sta $fc								; Store high byte in pointer

	; Hi-byte of the source VDC address to register 18
	ldx #$12    						; Load $12 for register 18 (VDC RAM address high) in X	
	lda _VDC_desth		        		; Load high byte of address in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waithighaddressm2v:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waithighaddressm2v	        	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data

	; Low-byte of the source VDC address to register 19
	inx 								; Increase X for register 19 (VDC RAM address low)
	lda _VDC_destl      				; Load low byte of address in A
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitlowaddressm2v:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitlowaddressm2v	        	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data

	; Start of copy loop
	ldy #$00    						; Set Y as counter on 0
	
	; Read value and store at VDC address
copyloopm2v:							; Start of copy loop
	lda ($fb),y							; Load source data
	ldx #$1f    						; Load $1f for register 31 (VDC data) in X
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitvaluem2v:							; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitvaluem2v		        	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data

	; Increase source address (VDC auto increments)
	inc $fb								; Increment low byte of source address
	bne nextm2v1						; If not yet zero, branch to next label
	inc $fc								; Increment high byte of source address
nextm2v1:								; Next label
	dec _VDC_tmp2						; Decrease low byte of length
	lda _VDC_tmp2						; Load low byte of length to A
	cmp #$ff							; Check if below zero
	bne copyloopm2v						; Continue loop if not yet below zero
	dec _VDC_tmp1						; Decrease high byte of length
	lda _VDC_tmp1						; Load high byte of length to A
	cmp #$ff							; Check if below zero
	bne copyloopm2v						; Continue loop if not yet below zero

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
_VDC_CopyVDCToMem_core:
; Function to copy memory from VDC memory to standard memory
; Input:	VDC_addrh = high byte of VDC source address
;			VDC_addrl = low byte of VDC source address
;			VDC_desth = high byte of destination address
;			VDC_destl = low byte of destination address
;			VDC_tmp1 = number of 256 byte pages to copy
;			VDC_tmp2 = length in last page to copy
;			VDC_tmp3 = memory bank of destination
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

	; Set address pointer in zero-page and STAVEC vector
	lda _VDC_destl						; Obtain low byte in A
	sta $fb								; Store low byte in pointer
	lda _VDC_desth						; Obtain high byte in A
	sta $fc								; Store high byte in pointer
	lda #$fb							; Load $fb address value in A
	sta $2b9							; Save in STAVEC vector

	; Start of copy loop
	ldy #$00    						; Set Y as counter on 0

copyloopv2m:							; Start of copy loop

	; Hi-byte of the source VDC address to register 18
	ldx #$12    						; Load $12 for register 18 (VDC RAM address high) in X	
	lda _VDC_addrh		        		; Load high byte of address in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waithighaddressv2m:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waithighaddressv2m	        	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data

	; Low-byte of the source VDC address to register 19
	inx 								; Increase X for register 19 (VDC RAM address low)
	lda _VDC_addrl      				; Load low byte of address in A
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitlowaddressv2m:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitlowaddressv2m	        	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data
	
	; Read VDC value and store at destination address
	ldx #$1f    						; Load $1f for register 31 (VDC data) in X
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitvaluev2m:							; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitvaluev2m		        	; Continue loop if status is not ready
	lda VDC_DATA_REGISTER	        	; Read VDC data to A
	sta ($fb),y							; Store in target memory

	; Increase VDC source address and target memory address
	inc $fb								; Increment low byte of target address
	bne nextv2m1						; If not yet zero, branch to next label
	inc $fc								; Increment high byte of target address
nextv2m1:								; Next label
	inc _VDC_addrl						; Increment low byte of VDC address
	bne nextv2m2						; If not yet zero, branch to next label
	inc _VDC_addrh						; Increment hight byte of VDC address
nextv2m2:								; Next label
	dec _VDC_tmp2						; Decrease low byte of length
	lda _VDC_tmp2						; Load low byte of length to A
	cmp #$ff							; Check if below zero
	bne copyloopv2m						; Continue loop if not yet below zero
	dec _VDC_tmp1						; Decrease high byte of length
	lda _VDC_tmp1						; Load high byte of length to A
	cmp #$ff							; Check if below zero
	bne copyloopv2m						; Continue loop if not yet below zero

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
_VDC_RedefineCharset_core:
; Function to copy charset definition from normal memory to VDC
; Input:	VDC_addrh = (source>>8) & 0xff;			// Obtain high byte of destination address
;			VDC_addrl = source & 0xff;				// Obtain low byte of destination address
;			VDC_tmp2 = sourcebank;					// Obtain bank number for source
;			VDC_desth = (dest>>8) & 0xff;			// Obtain high byte of destination address
;			VDC_destl = dest & 0xff;				// Obtain low byte of destination address
;			VDC_tmp1 = lengthinchars;				// Obtain number of characters to copy
; ------------------------------------------------------------------------------------------

	; Safeguard memory configuration and set memory config to selected MMU config
	lda	$ff00							; Obtain present memory configuration
	sta MemConfTmp						; Store in temp location for safeguarding
	lda _VDC_tmp2						; Obtain selected MMU config
	sta $ff00							; Store selected MMU config
	
	; Store $FA and $FB addresses for safety to be restored at exit
	lda $fb								; Obtain present value at $fb
	sta ZPtmp1							; Store to be restored later
	lda $fc								; Obtain present value at $fc
	sta ZPtmp2							; Store to be restored later

	; Set address pointer in zero-page
	lda _VDC_addrl						; Obtain low byte in A
	sta $fb								; Store low byte in pointer
	lda _VDC_addrh						; Obtain high byte in A
	sta $fc								; Store high byte in pointer

	; Hi-byte of the destination VDC address to register 18
	ldx #$12    						; Load $12 for register 18 (VDC RAM address high) in X	
	lda _VDC_desth		        		; Load high byte of address in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waithighaddress4:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waithighaddress4	        	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data

	; Low-byte of the destination VDC address to register 19
	inx 								; Increase X for register 19 (VDC RAM address low)
	lda _VDC_destl      				; Load low byte of address in A
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitlowaddress4:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitlowaddress4		        	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data

	; Start of copy loop
	ldy #$00    						; Set Y as counter on 0
looprc1:								; Start of outer loop
	
	; Read value from data register
looprc2:								; Start of 8 bytes character copy loop
	lda ($fb),y							; Load from source address
	ldx #$1f    						; Load $1f for register 31 (VDC data) in X
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitvalue6:								; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitvalue6			        	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data

	; Count 8 bytes per char
	iny 								; Increase Y counter
	cpy #$08    						; Is counter at 8?
	bcc looprc2				        	; If not yet 8, go to start of char copy loop

	; Add 8 bytes of zero padding per char
	lda #$00    						; Set 0 value to use as padding in A
looprc3:								; Start of padding loop
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitvalue7:								; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waitvalue7			        	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data
	dey 								; Decrease Y counter
	bne looprc3		        			; Continue padding looop if counter is not yet zero

	; Next character
	clc 								; Clear carry
	lda $fb		       				 	; Load low byte of source address in A
	adc #$08    						; Add 8 to address with carry
	sta $fb		       				 	; Store new address low byte
	lda $fc      						; Load high byte of source address in A
	adc #$00    						; Add zero with carry to A
	sta $fc        						; Store new address high byte
	dec _VDC_tmp1			        	; Decrease character length counter
	bne looprc1				        	; Branch for outer loop if not yet zero

	; Copy one final char
	ldy #$00       						; Set Y as counter on 0
looprc4:								; Start of 8 bytes character copy loop
	lda ($fb),y							; Load from source address
	ldx #$1f    						; Load $1f for register 31 (VDC data) in X
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitvalue8:								; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitvalue8			        	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data

	; Count 8 bytes per char
	iny 								; Increase Y counter
	cpy #$08    						; Is counter at 8?
	bcc looprc4       					; If not yet 8, go to start of char copy loop

; Add 8 bytes of zero padding per char
	lda #$00    						; Set 0 value to use as padding in A
looprc5:								; Start of padding loop
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitvalue9:								; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waitvalue9			        	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data
	dey 								; Decrease Y counter
	bne looprc5		        			; Continue padding looop if counter is not yet zero

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
_VDC_FillArea_core:
; Function to draw area with given character (draws from topleft to bottomright)
; Input:	VDC_addrh = high byte of start address
;			VDC_addrl = low byte of start address
;			VDC_tmp1 = haracter value
;			VDC_tmp2 = length value
;			VDC_tmp3 = attribute value
;			VDC_tmp4 = number of lines
; ------------------------------------------------------------------------------------------

loopdrawline:
	jsr _VDC_HChar_core					; Draw line

	; Increase start address with 80 for next line
	clc 								; Clear carry
	lda _VDC_addrl	        			; Load low byte of address to A
	adc #$50    						; Add 80 with carry
	sta _VDC_addrl			        	; Store result back
	lda _VDC_addrh	        			; Load high byte of address to A
	adc #$00    						; Add 0 with carry
	sta _VDC_addrh	        			; Store result back

	; Decrease line counter and loop until zero
	dec _VDC_tmp4						; Decrease line counter
	bne loopdrawline					; Continue until counter is zero
	rts

; ------------------------------------------------------------------------------------------
_VDC_CopyViewPortToVDC_core:
; Function to copy memory from VDC memory to standard memory
; Input:	VDC_addrh = high byte of source address
;			VDC_addrl = low byte of source address
;			VDC_desth = high byte of VDC destination address
;			VDC_destl = low byte of VDC destination address
;			VDC_strideh = high byte of characters per line in source
;			VDC_stridel = low byte of characters per line in source
;			VDC_tmp1 = number lines to copy
;			VDC_tmp2 = length per line to copy
;			VDC_tmp3 = MMU config of source
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

	; Set address pointer in zero-page
	lda _VDC_addrl						; Obtain low byte in A
	sta $fb								; Store low byte in pointer
	lda _VDC_addrh						; Obtain high byte in A
	sta $fc								; Store high byte in pointer

	; Start of copy loop
	ldy #$00    						; Set Y as counter on 0
outerloopvp:							; Start of outer loop
	lda _VDC_tmp2						; Load length of line
	sta _VDC_tmp4						; Store length to counter

	; Hi-byte of the source VDC address to register 18
	ldx #$12    						; Load $12 for register 18 (VDC RAM address high) in X	
	lda _VDC_desth		        		; Load high byte of address in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waithighaddressvp:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waithighaddressvp	        	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data

	; Low-byte of the source VDC address to register 19
	inx 								; Increase X for register 19 (VDC RAM address low)
	lda _VDC_destl      				; Load low byte of address in A
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitlowaddressvp:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitlowaddressvp	        	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data

	; Read value and store at VDC address
copyloopvp:								; Start of copy loop
	lda ($fb),y							; Load source data
	ldx #$1f    						; Load $1f for register 31 (VDC data) in X
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitvaluevp:							; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitvaluevp		     		   	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data

	; Increase source address (VDC auto increments)
	inc $fb								; Increment low byte of source address
	bne nextvp1							; If not yet zero, branch to next label
	inc $fc								; Increment high byte of source address
nextvp1:								; Next label
	dec _VDC_tmp4						; Decrease counter for line
	lda _VDC_tmp4						; Load counter to A
	cmp #$ff							; Check if below zero
	bne copyloopvp						; Continue loop if not yet below zero

	; Add stride to addresses for next line
	clc									; Clear carry
	lda	$fb								; Load low byte of source address
	adc _VDC_stridel					; Add low byte of stride
	sta $fb								; Store low byte of source
	lda $fc								; Load high byte of source address
	adc _VDC_strideh					; Add high byte of stride
	sta $fc								; Store high byte of source address
	clc									; Clear carry
	lda _VDC_destl						; Load low byte of VDC destination
	adc #$50							; Add 80 characters for next line
	sta _VDC_destl						; Store low byte of VDC destination
	lda _VDC_desth						; Load high byte of VDC destination
	adc #$00							; Add 0 with carry
	sta _VDC_desth						; Store high byte of VDC destination
	dec _VDC_tmp1						; Decrease counter number of lines
	lda _VDC_tmp1						; Load counter to A
	cmp #$ff							; Check if below zero
	bne outerloopvp						; Continue outer loop if not yet below zero

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
_VDC_ScrollCopy_core:
; Function to copy memory from one to another position within VDC memory
; Input:	VDC_addrh = high byte of source address
;			VDC_addrl = low byte of source address
;			VDC_desth = high byte of destination address
;			VDC_destl = low byte of destination address
;			VDC_tmp1 = number of lines to copy
;			VDC_tmp2 = length per line to copy
; ------------------------------------------------------------------------------------------

loopscrollcpy:
	; Hi-byte of the destination address to register 18
	ldx #$12    						; Load $12 for register 18 (VDC RAM address high) in X	
	lda _VDC_desth      				; Load high byte of dest in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitdesthighaddresssc:					; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitdesthighaddresssc     		; Continue loop if status is not ready
	sta VDC_DATA_REGISTER       		; Store A to VDC data

	; Lo-byte of the destination address to register 19
	ldx #$13    						; Load $13 for register 19 (VDC RAM address high) in X	
	lda _VDC_destl       				; Load high byte of dest in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitdestlowaddresssc:					; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waitdestlowaddresssc	       	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data

	; Set the copy bit (bit 7) of register 24 (block copy mode)
	ldx #$18    						; Load $18 for register 24 (block copy mode) in X	
	lda #$80			        		; Load prepared value with bit 7 set in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitsetcopybitsc:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waitsetcopybitsc		       	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data

	; Hi-byte of the source address to block copy source register 32
	ldx #$20					    	; Load $20 for register 32 (block copy source) in X	
	lda _VDC_addrh			        	; Load high byte of source in A
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitsrchighaddresssc:			    	; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waitsrchighaddresssc	        ; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data
	
	; Lo-byte of the source address to block copy source register 33
	ldx #$21					    	; Load $21 for register 33 (block copy source) in X	
	lda _VDC_addrl		        		; Load low byte of source in A
	stx VDC_ADDRESS_REGISTER        	; Store X in VDC address register
waitsrclowaddresssc:					; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER        	; Check status bit 7 of VDC address register
	bpl waitsrclowaddresssc		        ; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data

	; Number of bytes to copy
	ldx #$1E    						; Load $1E for register 30 (word count) in X
	lda _VDC_tmp2		        		; Set length
	stx VDC_ADDRESS_REGISTER	        ; Store X in VDC address register
waitsetlengthsc:						; Start of wait loop to wait for VDC status ready
	bit VDC_ADDRESS_REGISTER	        ; Check status bit 7 of VDC address register
	bpl waitsetlengthsc		        	; Continue loop if status is not ready
	sta VDC_DATA_REGISTER	        	; Store A to VDC data

	; Increase destination address for one line
	clc									; Clear carry
	lda _VDC_destl		        		; Load low byte of destination
	adc #$50							; Add 80 charachters for next line
	sta _VDC_destl						; Store low byte of destination
	lda _VDC_desth						; Load high byte of destination
	adc #$00							; Add 0 to add carry if needed
	sta _VDC_desth						; Store high byte of destination

	; Increase source address for one line
	clc									; Clear carry
	lda _VDC_addrl		        		; Load low byte of destination
	adc #$50							; Add 80 charachters for next line
	sta _VDC_addrl						; Store low byte of destination
	lda _VDC_addrh						; Load high byte of destination
	adc #$00							; Add 0 to add carry if needed
	sta _VDC_addrh						; Store high byte of destination	

	; Decrease line counter and loop until last page
	dec _VDC_tmp1		        		; Decrease line counter
	lda _VDC_tmp1						; Load counter to A
	cmp #$ff							; Check if below zero
	beq scrollcpyend			        ; Go to end if counter is zero
	jmp loopscrollcpy					; Jump to start of copy loop if not zero
scrollcpyend:
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
;			VDC_tmp1:					MMU config for poke
;			VDC_value:					value to poke
; ------------------------------------------------------------------------------------------

	; Safeguard memory configuration and set memory config to selected MMU config
	lda	$ff00							; Obtain present memory configuration
	sta MemConfTmp						; Store in temp location for safeguarding
	lda _VDC_tmp1						; Obtain selected MMU config
	sta $ff00							; Store selected MMU config

	; Store $FA and $FB addresses for safety to be restored at exit
	lda $fb								; Obtain present value at $fb
	sta ZPtmp1							; Store to be restored later
	lda $fc								; Obtain present value at $fc
	sta ZPtmp2							; Store to be restored later

	; Set address pointer in zero-page
	lda _VDC_addrl						; Obtain low byte in A
	sta $fb								; Store low byte in pointer
	lda _VDC_addrh						; Obtain high byte in A
	sta $fc								; Store high byte in pointer

	; Store value in specified address
	ldy #$00							; Clear Y index
	lda _VDC_value						; Load value in A
	sta ($fb),y							; Store at destination

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
_PEEKB_core:
; Function to peek a memory position in specified bank
; Input:	VDC_addrh and VDC_addrl:	high and low byte of address to peek
;			VDC_tmp1:					MMU config for peek
; Output:	VDC_value:					value peeked at address
; ------------------------------------------------------------------------------------------

	; Safeguard memory configuration and set memory config to selected MMU config
	lda	$ff00							; Obtain present memory configuration
	sta MemConfTmp						; Store in temp location for safeguarding
	lda _VDC_tmp1						; Obtain selected MMU config
	sta $ff00							; Store selected MMU config

	; Store $FA and $FB addresses for safety to be restored at exit
	lda $fb								; Obtain present value at $fb
	sta ZPtmp1							; Store to be restored later
	lda $fc								; Obtain present value at $fc
	sta ZPtmp2							; Store to be restored later

	; Set address pointer in zero-page
	lda _VDC_addrl						; Obtain low byte in A
	sta $fb								; Store low byte in pointer
	lda _VDC_addrh						; Obtain high byte in A
	sta $fc								; Store high byte in pointer

	; Store value in specified address
	ldy #$00							; Clear Y index
	lda ($fb),y							; Load value from source address
	sta _VDC_value						; Store value in variable to return

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
_BankMemCopy_core:
; Function to copy memory to another place in memory which user defined banks
; Input:	VDC_addrh = high byte of source address
;			VDC_addrl = low byte of source address
;			VDC_desth = high byte of VDC destination address
;			VDC_destl = low byte of VDC destination address
;			VDC_tmp1 = number of 256 byte pages to copy
;			VDC_tmp2 = length in last page to copy
;			VDC_tmp3 = MMU config of source
;			VDC_tmp4 = MMU config of destination
; ------------------------------------------------------------------------------------------

	; Safeguard memory configuration and set memory config to selected MMU config
	lda	$ff00							; Obtain present memory configuration
	sta MemConfTmp						; Store in temp location for safeguarding

	; Store $FA and $FB addresses for safety to be restored at exit
	lda $fb								; Obtain present value at $fb
	sta ZPtmp1							; Store to be restored later
	lda $fc								; Obtain present value at $fc
	sta ZPtmp2							; Store to be restored later
	lda $fd								; Obtain present value at $fc
	sta ZPtmp3							; Store to be restored later
	lda $fe								; Obtain present value at $fc
	sta ZPtmp4							; Store to be restored later

	; Set source address pointer in zero-page
	lda _VDC_addrl						; Obtain low byte in A
	sta $fb								; Store low byte in pointer
	lda _VDC_addrh						; Obtain high byte in A
	sta $fc								; Store high byte in pointer

	; Set destination address pointer in zero-page
	lda _VDC_destl						; Obtain low byte in A
	sta $fd								; Store low byte in pointer
	lda _VDC_desth						; Obtain high byte in A
	sta $fe								; Store high byte in pointer

	; Start of copy loop
	ldy #$00    						; Set Y as counter on 0
	
copyloopbmc:							; Start of copy loop
	; Set MMU configuration for source
	lda _VDC_tmp3						; Obtain selected MMU config for source
	sta $ff00							; Store selected MMU config for source

	; Read value at source address
	lda ($fb),y							; Load source data

	; Set MMU configuration for destination
	ldx _VDC_tmp4						; Obtain selected MMU config for source
	stx $ff00							; Store selected MMU config for source

	; Store value at destination address
	sta ($fd),y 						; Store data at destination

	; Increase addresses
	inc $fb								; Increment low byte of source address
	bne nextbmc1						; If not yet zero, branch to next label
	inc $fc								; Increment high byte of source address
nextbmc1:								; Next label
	inc $fd								; Increment low byte of source address
	bne nextbmc2						; If not yet zero, branch to next label
	inc $fe								; Increment high byte of source address
nextbmc2:								; Next label

	; Decrease counters
	dec _VDC_tmp2						; Decrease low byte of length
	lda _VDC_tmp2						; Load low byte of length to A
	cmp #$ff							; Check if below zero
	bne copyloopbmc						; Continue loop if not yet below zero
	dec _VDC_tmp1						; Decrease high byte of length
	lda _VDC_tmp1						; Load high byte of length to A
	cmp #$ff							; Check if below zero
	bne copyloopbmc						; Continue loop if not yet below zero

; Restore ZP addresses
	lda ZPtmp1							; Obtain stored value of $fb
	sta $fb								; Restore value
	lda ZPtmp2							; Obtain stored value of $fc
	sta $fc								; Restore value
	lda ZPtmp3							; Obtain stored value of $fd
	sta $fd								; Restore value
	lda ZPtmp4							; Obtain stored value of $fe
	sta $fe								; Restore value

; Restore memory configuration
	lda MemConfTmp						; Obtain saved memory config
	sta $ff00							; Restore memory config
    rts

; ------------------------------------------------------------------------------------------
_BankMemSet_core:
; Function to set memory in given bank to specific value
; Input:	VDC_addrh = high byte of source address
;			VDC_addrl = low byte of source address
;			VDC_value = value to set at all memory positions
;			VDC_tmp1 = number of 256 byte pages to copy
;			VDC_tmp2 = length in last page to copy
;			VDC_tmp3 = MMU config of source
; ------------------------------------------------------------------------------------------

	; Safeguard memory configuration and set memory config to selected MMU config
	lda	$ff00							; Obtain present memory configuration
	sta MemConfTmp						; Store in temp location for safeguarding

	; Set MMU configuration for source
	lda _VDC_tmp3						; Obtain selected MMU config for source
	sta $ff00							; Store selected MMU config for source

	; Store $FA and $FB addresses for safety to be restored at exit
	lda $fb								; Obtain present value at $fb
	sta ZPtmp1							; Store to be restored later
	lda $fc								; Obtain present value at $fc
	sta ZPtmp2							; Store to be restored later

	; Set source address pointer in zero-page
	lda _VDC_addrl						; Obtain low byte in A
	sta $fb								; Store low byte in pointer
	lda _VDC_addrh						; Obtain high byte in A
	sta $fc								; Store high byte in pointer

	; Start of copy loop
	ldy #$00    						; Set Y as counter on 0
	
copyloopbms:							; Start of copy loop
	; Store value at address
	lda _VDC_value						; Load value to set in A
	sta ($fb),y							; Load source data

	; Increase addresses
	inc $fb								; Increment low byte of source address
	bne nextbms1						; If not yet zero, branch to next label
	inc $fc								; Increment high byte of source address
nextbms1:								; Next label

	; Decrease counters
	dec _VDC_tmp2						; Decrease low byte of length
	lda _VDC_tmp2						; Load low byte of length to A
	cmp #$ff							; Check if below zero
	bne copyloopbms						; Continue loop if not yet below zero
	dec _VDC_tmp1						; Decrease high byte of length
	lda _VDC_tmp1						; Load high byte of length to A
	cmp #$ff							; Check if below zero
	bne copyloopbms						; Continue loop if not yet below zero

; Restore ZP addresses
	lda ZPtmp1							; Obtain stored value of $fb
	sta $fb								; Restore value
	lda ZPtmp2							; Obtain stored value of $fc
	sta $fc								; Restore value

; Restore memory configuration
	lda MemConfTmp						; Obtain saved memory config
	sta $ff00							; Restore memory config
    rts
; VDC Screen Editor
; Screen editor for the C128 80 column mode
; Written in 2021 by Xander Mol

; https://github.com/xahmol/VDCScreenEdit
; https://www.idreamtin8bits.com/

; Data for visual petscii charset mapping
; Inspired by Petmate and petscii.krissz.hu
; Suggested by jab / Artline Designs (Jaakko Luoto)
    
    .segment "PETSCII"

    ; Load address
    .word $0C00

    ; Data
    .byte $20,$01,$02,$03,$04,$05,$06,$07,$08,$09,$0A,$0B,$0C,$0D,$0E,$0F
    .byte $A0,$81,$82,$83,$84,$85,$86,$87,$88,$89,$8A,$8B,$8C,$8D,$8E,$8F
    
    .byte $10,$11,$12,$13,$14,$15,$16,$17,$18,$19,$1A,$2E,$2C,$3B,$21,$3F
    .byte $90,$91,$92,$93,$94,$95,$96,$97,$98,$99,$9A,$AE,$AC,$BB,$A1,$BF
    
    .byte $30,$31,$32,$33,$34,$35,$36,$37,$38,$39,$22,$23,$24,$25,$26,$27
    .byte $B0,$B1,$B2,$B3,$B4,$B5,$B6,$B7,$B8,$B9,$A2,$A3,$A4,$A5,$A6,$A7
    
    .byte $70,$6E,$6C,$7B,$55,$49,$4F,$50,$71,$72,$28,$29,$3C,$3E,$4E,$4D
    .byte $F0,$EE,$EC,$FB,$D5,$C9,$CF,$D0,$F1,$F2,$A8,$A9,$BC,$BE,$CE,$CD
    
    .byte $6D,$7D,$7C,$7E,$4A,$4B,$4C,$7A,$6B,$73,$1B,$1D,$1F,$1E,$5F,$69
    .byte $ED,$FD,$FC,$FE,$CA,$CB,$CC,$FA,$EB,$F3,$9B,$9D,$9F,$9E,$DF,$E9
    
    .byte $64,$6F,$79,$62,$78,$77,$63,$74,$65,$75,$61,$76,$67,$6A,$5B,$2B
    .byte $E4,$EF,$F9,$E2,$F8,$F7,$E3,$F4,$E5,$F5,$E1,$F6,$E7,$EA,$DB,$AB
    
    .byte $52,$46,$40,$2D,$43,$44,$45,$54,$47,$42,$5D,$48,$59,$2F,$56,$2A
    .byte $D2,$C6,$C0,$AD,$C3,$C4,$C5,$D4,$C7,$C2,$DD,$C8,$D9,$AF,$D6,$AA
    
    .byte $3D,$3A,$1C,$00,$7F,$68,$5C,$66,$51,$57,$41,$53,$58,$5A,$5E,$60
    .byte $BD,$BA,$9C,$80,$FF,$E8,$DC,$E6,$D1,$D7,$C1,$D3,$D8,$DA,$DE,$E0

    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    

    
    
    
    
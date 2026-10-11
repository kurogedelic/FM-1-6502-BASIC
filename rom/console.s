; Minimal 6502 monitor: echo characters received at $D000.
; This is a hardware-I/O integration test, NOT Microsoft BASIC.
.setcpu "6502"
.segment "CODE"
reset:
    ldx #$ff
    txs
    ldx #$00
print_banner:
    lda banner,x
    beq poll
    sta $d000
    inx
    bne print_banner
poll:
    lda $d001
    beq poll
    lda $d000
    cmp #$0a
    bne echo
    lda #$0d
echo:
    sta $d000
    cmp #$0d
    bne poll
    lda #$0a
    sta $d000
    jmp poll
banner:
    .byte "FM-1 6502 CONSOLE",13,10,0
.segment "VECTORS"
    .word reset,reset,reset

; FM-1 platform layer ("kernal") for Microsoft BASIC-M6502.
;
; BASIC-M6502 (cc65 "min" build) expects its host to provide a small set of
; jump-table entries at $FFC0-$FFE9 (Commodore KERNAL layout). This file
; implements them on top of the FM-1 virtual terminal:
;
;   $D000 read  : dequeue one input character
;   $D000 write : emit one output character
;   $D001 read  : non-zero when an input character is available
;
; Conventions implemented here:
;   * Output: CR (13) is a newline; it is sent to the terminal as CR LF.
;     LF (10) coming from BASIC is dropped (BASIC sends CR LF itself).
;   * Input: CQINCH is line oriented. A line is collected with echo and
;     editing (BS/DEL erase one character, Ctrl-U erases the line, Ctrl-C
;     abandons it), lowercase is folded to uppercase, and the finished line
;     is returned one character at a time ending in CR. Return is NOT echoed
;     here: BASIC prints CR LF itself after a line is read.
;   * Ctrl-C (3) typed while a program runs is detected by ISCNTC, which
;     enters BASIC's STOP handler. Other keys typed meanwhile are queued.
;   * LOAD / SAVE / VERIFY are not available and raise ILLEGAL QUANTITY.
;
; All routines called from BASIC preserve X and Y (BASIC relies on that).

.setcpu "6502"
.import STOP, INIT, FCERR

IO_DATA   = $D000
IO_STATUS = $D001

LINMAX    = 80          ; longest line accepted by the editor
RINGMASK  = $0F         ; 16 byte type-ahead ring

.segment "KBSS"
ring:     .res RINGMASK+1
ring_head: .res 1
ring_tail: .res 1
line_rdy: .res 1        ; non-zero while a collected line is being handed out
line_pos: .res 1
tmp:      .res 1
linebuf:  .res LINMAX+2

.segment "KERNAL"

; ---------------------------------------------------------------------------
reset:
    sei
    cld
    lda #0
    sta ring_head
    sta ring_tail
    sta line_rdy
    sta line_pos
    jmp INIT                ; BASIC cold start (sets up the stack itself)

irq:
    rti

; ---------------------------------------------------------------------------
; OUTCH: write A to the terminal. Preserves A, X, Y.
kout:
    cmp #10
    beq @done               ; drop LF, CR already produced CR LF
    sta IO_DATA
    cmp #13
    bne @done
    lda #10
    sta IO_DATA
    lda #13
@done:
    rts

; ---------------------------------------------------------------------------
; ring buffer helpers (preserve X, Y)
; ring_put: A = character; carry set if full (character dropped)
ring_put:
    sta tmp
    lda ring_head
    clc
    adc #1
    and #RINGMASK
    cmp ring_tail
    beq @full
    pha
    txa
    pha                     ; save X
    ldx ring_head
    lda tmp
    sta ring,x
    pla
    tax
    pla
    sta ring_head
    clc
    rts
@full:
    sec
    rts

; ring_get: carry clear and A = character, or carry set if empty
ring_get:
    lda ring_tail
    cmp ring_head
    beq @empty
    txa
    pha
    ldx ring_tail
    lda ring,x
    sta tmp
    inx
    txa
    and #RINGMASK
    sta ring_tail
    pla
    tax
    lda tmp
    clc
    rts
@empty:
    sec
    rts

; ---------------------------------------------------------------------------
; getc: blocking read of one raw character into A (queued first). Preserves X, Y.
getc:
    jsr ring_get
    bcc @got
@poll:
    lda IO_STATUS
    beq @poll
    lda IO_DATA
@got:
    rts

; ---------------------------------------------------------------------------
; CZGETL: non-blocking read for GET. A = character or 0. Preserves X, Y.
kget:
    jsr ring_get
    bcc @got
    lda IO_STATUS
    beq @none
    lda IO_DATA
    rts
@none:
    lda #0
@got:
    rts

; ---------------------------------------------------------------------------
; ISCNTC: called by BASIC between statements. Drains pending input into the
; type-ahead ring; if Ctrl-C is seen, discard type-ahead and enter STOP with
; Z=1, C=1 (STOP's "Ctrl-C" flag convention). Preserves X, Y.
kiscntc:
@next:
    lda IO_STATUS
    beq @ret
    lda ring_head           ; room in the ring? (never read a char we cannot keep)
    clc
    adc #1
    and #RINGMASK
    cmp ring_tail
    beq @ret                ; full: leave the rest in the terminal queue
    lda IO_DATA
    cmp #3
    beq @break
    jsr ring_put
    jmp @next
@break:
    lda ring_tail
    sta ring_head           ; flush type-ahead
    lda #3
    cmp #3                  ; Z=1, C=1
    jmp STOP                ; pops the JSR ISCNTC return address, prints BREAK
@ret:
    rts

; ---------------------------------------------------------------------------
; CQINCH: BASIC's character input. Preserves X, Y; returns A.
kinch:
    txa
    pha
    tya
    pha
    lda line_rdy
    bne @deliver
    jsr readline
@deliver:
    ldx line_pos
    lda linebuf,x
    inx
    stx line_pos
    cmp #13
    bne @out
    ldx #0
    stx line_rdy
@out:
    sta tmp
    pla
    tay
    pla
    tax
    lda tmp
    rts

; readline: collect an edited line into linebuf (terminated by CR).
readline:
    ldx #0
@next:
    jsr getc
    cmp #13
    beq @done
    cmp #10
    beq @done
    cmp #8
    beq @bs
    cmp #127
    beq @bs
    cmp #21                 ; Ctrl-U
    beq @kill
    cmp #3                  ; Ctrl-C abandons the line
    beq @cancel
    cmp #' '
    bcc @next               ; other control characters are ignored
    cmp #127
    bcs @next
    cmp #'a'
    bcc @store
    cmp #'z'+1
    bcs @store
    and #$DF                ; fold to uppercase
@store:
    cpx #LINMAX
    bcs @next               ; line full
    sta linebuf,x
    inx
    jsr kout                ; echo
    jmp @next
@bs:
    cpx #0
    beq @next
    dex
    lda #8
    jsr kout
    jmp @next
@kill:
    cpx #0
    beq @next
    dex
    lda #8
    jsr kout
    jmp @kill
@cancel:
    ldx #0
@done:
    lda #13
    sta linebuf,x
    lda #1
    sta line_rdy
    lda #0
    sta line_pos
    rts

; ---------------------------------------------------------------------------
; Unsupported file operations: LOAD, SAVE, VERIFY.
kunsupported:
    jmp FCERR
knop:
    rts

; ---------------------------------------------------------------------------
.segment "KJUMP"
    jmp knop                ; $FFC0 CQOPEN
    jmp knop                ; $FFC3 CQCLOS
    jmp knop                ; $FFC6 CQOIN
    jmp knop                ; $FFC9 CQOOUT
    jmp knop                ; $FFCC CQCCHN
    jmp kinch               ; $FFCF CQINCH
    jmp kout                ; $FFD2 OUTCH
    jmp kunsupported        ; $FFD5 CQLOAD
    jmp kunsupported        ; $FFD8 CQSAVE
    jmp kunsupported        ; $FFDB CQVERF
    jmp knop                ; $FFDE CQSYS
    jmp kiscntc             ; $FFE1 ISCNTC
    jmp kget                ; $FFE4 CZGETL
    jmp knop                ; $FFE7 CQCALL

.segment "VECTORS"
    .word irq               ; NMI
    .word reset             ; RESET
    .word irq               ; IRQ/BRK

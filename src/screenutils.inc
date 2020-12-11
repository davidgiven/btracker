
; Prints the hex nibble A at scrptr

.print_h4
    and #&0f
	tay
	lda hex_table, y
.print_char
    ldy #0
    sta (scrptr), y
{
    inc scrptr+0
    bne noinc
    inc scrptr+1
.noinc
    rts
}

; Hex lookup table.

.hex_table
	equs "0123456789abcdef"


; Prints A at scrptr

.print_h8
{
    pha
    lsr a
    lsr a
    lsr a
    lsr a
    jsr print_h4

    pla
    and #&0f
    jmp print_h4
}

; Advances the screen pointer by A bytes.

.advance_scrptr
{
    clc
    adc scrptr+0
    sta scrptr+0
    bcc noinc
    inc scrptr+1
.noinc
    rts
}

; Retards the screen pointer by A bytes.

.retard_scrptr
{
    sta w
    sec
    lda scrptr+0
    sbc w
    sta scrptr+0
    bcs nodec
    dec scrptr+1
.nodec
    rts
}

; Display a packed banner at the top of the screen, starting at scrptr, with
; the address in w. The packing is ludicrously simple: bytes are written, 01-31
; repeat the next byte that many times, 00 ends.

.draw_packed
{
	ldy #0
.loop
    lda (w), y
    beq done
    incw w
    cmp #32
    blt repeat

    sta (scrptr), y
    incw scrptr
    jmp loop

.repeat
    tax
    lda (w), y
    incw w
.loop2
    sta (scrptr), y
    incw scrptr
    dex
    bne loop2
    jmp loop

.done
    rts
}

; Draw the standard header on the top two lines.

.draw_standard_header
{
    lda #lo(data)
    sta w+0
    lda #hi(data)
    sta w+1
    lda #0
    sta scrptr+0
    lda #&7c
    sta scrptr+1
    jmp draw_packed

.data
    equb 128+4      ; blue
    equs "  File"
    equb 128+4      ; blue
    equs "  Pattern"
    equs 128+4      ; blue
    equb 15, ' '
    equb 128+1
    equs "f0-f3 "

    equb 144+6      ; cyan graphics
    equb 30, &23
    equb 8, &23
    equb ' '

    equb 0
}

; Highlight a tab, with the left side in x and the right side in y.

.draw_tab
{
    lda #128+6      ; cyan
    sta &7c00, x
    lda #157        ; background
    sta &7c01, x
    lda #128+4      ; blue
    sta &7c02, x
    lda #128+6      ; cyan
    sta &7c00, y
    lda #156        ; background off
    sta &7c01, y
    rts
}

; vim: sw=4 ts=4 et ft=asm


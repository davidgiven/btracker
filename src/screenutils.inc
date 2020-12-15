
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

; Moves the cursor to the address at YYXX.
.move_cursor
{
    lda #14
    sta CRTC_ADDRESS
    tya
    sec
    sbc #&54
    sta CRTC_DATA
    lda #15
    sta CRTC_ADDRESS
    txa
    sta CRTC_DATA
    rts
}

; Turns the text cursor on.

.cursor_on
{
    lda #10
    sta CRTC_ADDRESS
    lda #64
    sta CRTC_DATA
    rts
}

; Turns the text cursor off.

.cursor_off
{
    lda #10
    sta CRTC_ADDRESS
    lda #32
    sta CRTC_DATA
    rts
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

; Display a packed banner at the top of the screen, with
; the address in w. The packing is ludicrously simple: there's an initial
; destination address, and then bytes are written, 01-31 repeat the next byte
; that many times, 00 ends.

.draw_packed
{
	ldy #0
    lda (w), y
    sta scrptr+0
    iny
    lda (w), y
    sta scrptr+1

    clc
    lda w+0
    adc #2
    sta w+0
    bcc noinc
    inc w+1
.noinc
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
    jmp draw_packed

.data
    equw &7c00
    equb 128+4      ; blue
    equs "  File"
    equb 128+4      ; blue
    equs "  Pattern"
    equs 128+4      ; blue
    equs "  Tone"
    equs 128+4      ; blue
    equb 8, ' '
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
    sta &7c00, y
    lda #156        ; background off
    sta &7c01, y
    rts
}

; Plots a point at X, Y on the teletext grid. Sets w to the address and leaves the pixel in A.
; Y is left 0 ready for dereferencing.

.plot
{
    ; Y direction: look up the line in the table.

    lda teletext_line_table_lo, y
    sta w+0
    lda teletext_line_table_hi, y
    sta w+1

    lda remainder_by_three, y
    tay

    ; X direction: divide by two and add to get the address.

    txa
    lsr a
    clc
    adc w+0
    sta w+0
    bcc noadd
    inc w+1
.noadd

    ; Fetch the pixel value.

    txa
    lsr a
    tya
    rol a
    tax
    lda pixel_table, x

    print ~P%
    ldy #0
    rts
}

; Start address of each teletext line.

.teletext_line_table_hi
    for i, 0, 74
        equb hi(&7c00 + (i div 3)*40)
    next

.teletext_line_table_lo
    for i, 0, 74
        equb lo(&7c00 + (i div 3)*40)
    next

.remainder_by_three
    for i, 0, 74
        equb i mod 3
    next

.pixel_table
    equb 1, 2, 4, 8, &10, &40

; vim: sw=4 ts=4 et ft=asm


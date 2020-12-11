
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

; Hex lookup table.
.hex_table
	equs "0123456789abcdef"

; vim: sw=4 ts=4 et ft=asm


; Given a keytable specified by name, looks through it and jumps to the
; appropriate routine, or falls out the bottom.

macro key_lookup key_table
    ldy #0
.loop
    cmp key_table, y
    bne no_key
    lda key_table+2, y
    pha
    lda key_table+1, y
    pha
    rts
.no_key
    iny
    iny
    iny
    ldx key_table, y
    bne loop
endmacro

macro key n, label
    equb n
    equw label - 1
endmacro

; Converts the ASCII code in A to a nibble. Returns C set on error.

.parse_digit
{
    cmp #'0'
    blt bad

    cmp #'9'
    bge not_number
    sec
    sbc #'0'
.good
    clc
    rts

.not_number
    and #lo(not(&20))
    sec
    sbc #'A' - 10
    cmp #&10
    blt good
.bad
    sec
    rts
}

; vim: ts=4 sw=4 et ft=asm


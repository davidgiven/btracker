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


; vim: ts=4 sw=4 et ft=asm


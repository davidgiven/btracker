.tone_editor
{
    ldx #0
    txs
    jsr setup_screen_for_tone_editor
    jsr panic

.editloop
    lda #&81
    ldx #0
    ldy #0
    jsr OSBYTE
    bcs editloop

    txa
    key_lookup key_table
    jmp editloop

.key_table
    key 161, pattern_editor
    key 27, panic
    equb 0

    .panic
    {
        lda #0
        sta playing
        ldx #2
    .loop
        sta volume, x
        dex
        bpl loop
        rts
    }

}

.setup_screen_for_tone_editor
{
    jsr draw_standard_header

    ldx #17
    ldy #25
    jsr draw_tab
    lda #lo(data)
    sta w+0
    lda #hi(data)
    sta w+1
    ldx #80
    jsr draw_packed
    rts

.data
    equw &7c00 + 80

    equb 20, ' '
    equb 20, ' '

    equb 20, ' '
    equb 20, ' '

    equb 20, ' '
    equb 20, ' '

    equb 20, ' '
    equb 20, ' '

    equb 20, ' '
    equb 20, ' '

    equb 3, ' '
    equb 128+6      ; cyan
    equs "+"
    equb 144+7      ; white
    equb &28
    equb 31, ' '

    equb 7, ' '
    equb 144+7      ; white
    equb &62
    equb 31, ' '

    equb 7, ' '
    equb 144+7      ; white
    equb &28
    equb 31, ' '

    equb 3, ' '
    equb 128+1      ; red
    equb 'P'
    equb 3, 144+7    ; white
    equb &62
    equb 31, ' '

    equb 3, ' '
    equb 128+1      ; red
    equb 'I'
    equb 3, 144+7   ; white
    equb &28
    equb 31, ' '

    equb 3, ' '
    equb 128+1      ; red
    equb 'T'
    equb 128+6      ; cyan
    equs "0"
    equb 144+7      ; white
    equb &62
    equb 31, &28, &28

    equb 2, ' '
    equb 128+1      ; red
    equb 'C'
    equb 3, 144+7   ; white
    equb &28
    equb 31, ' '

    equb 3, ' '
    equb 128+1      ; red
    equb 'H'
    equb 3, 144+7   ; white
    equb &62
    equb 31, ' '

    equb 7, ' '
    equb 144+7      ; white
    equb &28
    equb 31, ' '

    equb 7, ' '
    equb 144+7      ; white
    equb &62
    equb 31, ' '

    equb 5, ' '
    equb 128+6      ; cyan
    equs "-"
    equb 144+7      ; white
    equb &28
    equb 31, ' '

    equb 20, ' ', 23, ' '

    equb 128+1      ; red
    equb 'V'
    equb 128+6      ; cyan
    equs "0"
    equb 144+7      ; white
    equb 31, &28, 2, &28

    equb 2, ' '
    equb 128+1      ; red
    equb 'O'
    equb 3, 144+7      ; white
    equb &28
    equb 31, ' '

    equb 3, ' '
    equb 128+1      ; red
    equb 'L'
    equb 3, 144+7      ; white
    equb &62
    equb 31, ' '

    equb 3, ' '
    equb 128+1      ; red
    equb 'U'
    equb 3, 144+7      ; white
    equb &28
    equb 31, ' '

    equb 3, ' '
    equb 128+1      ; red
    equb 'M'
    equb 3, 144+7      ; white
    equb &62
    equb 31, ' '

    equb 3, ' '
    equb 128+1      ; red
    equb 'E'
    equb 128+6      ; cyan
    equs "-"
    equb 144+7      ; white
    equb &28
    equb 31, ' '
    equb 0
}

; vim: ts=4 sw=4 et ft=asm


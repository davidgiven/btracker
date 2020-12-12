.tone_editor
{
    ldx #0
    txs
    jsr setup_screen_for_tone_editor

    jmp P%
	rts
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
    equb 0
}

; vim: ts=4 sw=4 et ft=asm


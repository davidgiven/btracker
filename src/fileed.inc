.file_editor
{
    ldx #0
    txs
    stx playing
    jsr redraw
    jsr panic

.editloop
    jsr OSRDCH
    jsr do_key
    jmp editloop

.do_key
    key_lookup key_table
    rts

.key_table
    key 162, pattern_editor
    key 163, tone_editor
    equb 0

.redraw
    {
        jsr cursor_off
        jsr draw_standard_header

        ldx #0
        ldy #8
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
        equb 31, ' ', 21, ' '
        equb &93, &20, &20, &34, &20, &20, &20, &20, &20, &20, &20, &34, 28, ' '
        equb &93, &20, &20, &22, &34, &70, &70, &2c, &64, &70, &6a, 29, ' '
        equb &93, &20, &20, &20, &36, &20, &20, &20, &20, &93, &29, &30, 28, ' '
        equb &93, &60, &38, &6e, &30, &97, &2d, &60, &70, &2b, &93, &69, &27, &2d, &64, 25, ' '
        equb &93, &37, &32, &32, &32, &74, &30, &20, &20, &20, &20, &3a, &32, &32, &7a, 25, ' '
        equb &93, &29, &38, &28, &28, &28, &2b, &2f, &2f, &2f, &2b, &28, &28, &38, &26, 25, ' '
        equb &93, &20, &32, &29, &3d, &65, &71, &61, &61, &61, &39, &6c, &73, &70, &20, &60, &25, 23, ' '
        equb &93, &36, &22, &63, &35, &20, &22, &23, &23, &21, &20, &20, &29, &30, &23, &21, 24, ' '
        equb &93, &61, &2c, &7a, &7f, &7c, &74, &70, &70, &70, &70, &70, &7c, &75, &30, 25, ' '
        equb &93, &25, &20, &6a, &7f, &7f, &7f, &7f, &7f, &7f, &7f, &7f, &7f, &35, &22, &69, 24, ' '
        equb &93, &20, &20, &22, &67, &23, &2f, &2f, &2f, &2f, &2f, &63, &27, 27, ' '
        equb &93, &20, &20, &20, &38, &2d, &64, &20, &20, &20, &70, &77, 28, ' '
        equb &93, &20, &20, &6a, &20, &20, &20, &2b, &3f, &23, &20, &22, &23, &24, 26, ' '
        equb &97, &20, &20, &20, &20, &91, &20, &20, &21, 31, ' '
        equb 20, ' ', 22, ' '
        equb &81, &8d
        equs "b-tracker"
        equb 29, ' '
        equb &81, &8d
        equs "b-tracker", 25, ' '
        equb &82
        equs "(C) 2020 David Given"

        equb 24, ' '
        equb 30, ' '

        equb 128+3
        equs "(N)"
        equb 128+6
        equs "New file"
        equs "   "

        equb 128+3
        equs "(L)"
        equb 128+6
        equs "Load file"

        equb 10, ' '

        equb 128+3
        equs "(*)"
        equb 128+6
        equs "Command"
        equs "    "

        equb 128+3
        equs "(S)"
        equb 128+6
        equs "Save file"

        equb 25, ' '
        equb 20, ' '

        equb 0
    }
}

; vim: ts=4 sw=4 et ft=asm


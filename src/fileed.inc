.file_editor
{
    ldx #0
    txs
    stx playing
    jsr set_raw_keyboard
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
    key 130, pattern_editor
    key 131, tone_editor
    key 'N', new_file
    key 'L', load_file
    key 'S', save_file
    key '*', star_command
    equb 0

.new_file
{
    jsr draw_dialogue
    ldx #lo(data)
    ldy #hi(data)
    jsr draw_packed
    jsr are_you_sure
    bcc ret
    jsr clear_all_data
.ret
    jmp file_editor

.data
    equw &7c00 + 12*40 + 0
    equb 128+1      ; cyan
    equs "Discard current file and start again!"
    equb 0
}

.load_file
{
    jsr draw_dialogue
    ldx #lo(data)
    ldy #hi(data)
    jsr draw_packed
    jsr read_string
    bcs abort

    jsr clear_all_data

    ldx #osfileblock_end - osfileblock - 1
.loop
    lda osfileblock, x
    sta BUFFER1, x
    dex
    bpl loop

    lda #&ff ; load
    ldx #lo(BUFFER1)
    ldy #hi(BUFFER1)
    jsr OSFILE

.abort
    jmp file_editor

.osfileblock
    equw BUFFER
    equd MUSIC_DATA
    equd 0
    equd 0
    equd 0
.osfileblock_end

.data
    equw &7c00 + 12*40 + 0
    equb 128+6      ; cyan
    equs "Enter filename to load from:"
    equb 0
}

.save_file
{
    jsr draw_dialogue
    ldx #lo(data)
    ldy #hi(data)
    jsr draw_packed
    jsr read_string
    bcs abort

    lda #12
    jsr OSWRCH
    jsr cursor_on

    ldx #osfileblock_end - osfileblock - 1
.loop
    lda osfileblock, x
    sta BUFFER1, x
    dex
    bpl loop

    lda numpatterns
    clc
    adc #hi(PATTERN_DATA)
    sta endaddress-osfileblock+BUFFER1+1
    lda #0 ; save
    ldx #lo(BUFFER1)
    ldy #hi(BUFFER1)
    jsr OSFILE

.abort
    jmp file_editor

.osfileblock
    equw BUFFER
    equd MUSIC_DATA
    equd 0
    equd MUSIC_DATA
.endaddress
    equd &7c00
.osfileblock_end

.data
    equw &7c00 + 12*40 + 0
    equb 128+6      ; cyan
    equs "Enter filename to save to:"
    equb 0
}

.redraw
    {
        jsr cursor_off
        jsr draw_standard_header

        ldx #0
        ldy #8
        jsr draw_tab
        ldx #lo(data)
        ldy #hi(data)
        jmp draw_packed

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

.clear_all_data
{
    lda #12
    jsr OSWRCH

    ; Wipe everything in the header area to zeroes (the pattern data is written
    ; completely later).

    {
        lda #hi(MUSIC_DATA)
        sta w+1
        lda #0
        sta w+0
    .outerloop
        ldy #0
        tya
    .innerloop
        sta (w), y
        iny
        bne innerloop

        inc w+1
        lda w+1
        cmp #hi(PATTERN_DATA)
        bne outerloop
    }

    lda #32
    sta tempo
    sta patternlength
    lda #1
    sta sequencelength
    sta numpatterns

    ; Initialise sequence.

    {
        ldx #1
        lda #&ff
    .loop
        sta sequence, x
        inx
        cpx #128
        bne loop
    }

    ; Initialise tone data.

    {
        ldx #15
    .loop
        lda #1
        sta samplespeed, x
        sta pitchscale, x
        lda #64
        sta repeatend, x
        dex
        bpl loop
    }

    ; Wipe pattern data.

    {
        lda #hi(PATTERN_DATA)
        sta w+1
        ; w+0 is still 0 from above
    .outerloop
        ldy #0
    .innerloop
        lda #FIRST_COMMAND + 'B' - 'A'
        sta (w), y
        iny
        iny
        sta (w), y
        iny
        iny
        sta (w), y
        iny
        iny
        sta (w), y

        tya
        sec
        sbc #5
        tay

        lda #&0f
        sta (w), y
        iny
        iny
        sta (w), y
        iny
        iny
        sta (w), y
        iny
        iny
        sta (w), y
        iny
        
        bne innerloop

        inc w+1
        lda w+1
        cmp #&7c
        bne outerloop
    }

    ; Reset editor parameters.

    lda #0
    sta rowno
    sta patternno
    sta seqindex
    sta edittone
    jsr reset_row_pointer
    rts
}

; vim: ts=4 sw=4 et ft=asm


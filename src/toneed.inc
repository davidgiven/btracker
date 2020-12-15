PITCH_GRAPH_X = 14
PITCH_GRAPH_Y = 37

VOLUME_GRAPH_X = 14
VOLUME_GRAPH_Y = 58

.tone_editor
{
    ldx #0
    txs
    stx playing
    jsr select_tone
    jsr redraw
    jsr panic

.editloop
    jsr draw_cursor
    lda #&81
    ldx #0
    ldy #0
    jsr OSBYTE
    bcs editloop
    
    txa
    jsr do_key
    jmp editloop

.do_key
    key_lookup key_table
    rts

.key_table
    key 161, pattern_editor
    key 136, key_left
    key 137, key_right
    key 43, key_increment
    key 45, key_decrement
    key 27, panic
    equb 0

    .panic
    {
        lda #0
        ldx #3
    .loop
        sta volume, x
        dex
        bpl loop
        rts
    }

    .key_left
    {
        lda editcursor
        sec
        sbc #1
        and #63
        sta editcursor
        rts
    }

    .key_right
    {
        lda editcursor
        clc
        adc #1
        and #63
        sta editcursor
        rts
    }

    .key_increment
    {
        ldy editcursor
        lda (editsampledata), y
        clc
        adc #1
        cmp #&10
        beq nochange
        sta (editsampledata), y
    .nochange
        jmp redraw
    }

    .key_decrement
    {
        ldy editcursor
        lda (editsampledata), y
        sec
        sbc #1
        cmp #lo(-&10)
        beq nochange
        sta (editsampledata), y
    .nochange
        jmp redraw
    }

    .select_tone
    {
        ; Fetch the pointer to the sample data.

        lda edittone
        asl a
        rol a
        sta editsampledata+0 ; low byte of sample data address

        lda edittone
        asl a
        clc
        adc #hi(TONE_DATA)
        sta editsampledata+1 ; high byte of sample data address

        rts
    }

    .redraw
    {
        jsr setup_screen_for_tone_editor
        jmp draw_graphs
    }

    .draw_graphs
    {
        lda #63              ; X position
        sta p

    .loop
        ldy p
        sec
        lda #PITCH_GRAPH_Y
        sbc (editsampledata), y ; pitch
        tay
        lda p
        clc
        adc #PITCH_GRAPH_X
        tax
        jsr plot
        ora (w), y
        sta (w), y
        
        lda p
        clc
        adc #64
        tay
        clc
        lda #VOLUME_GRAPH_Y
        adc (editsampledata), y ; volume
        tay
        lda p
        clc
        adc #VOLUME_GRAPH_X
        tax
        jsr plot
        ora (w), y
        sta (w), y

        dec p
        bpl loop
        rts
    }

    .draw_cursor
    {
        lda editcursor
        sec
        lda #PITCH_GRAPH_Y
        sbc (editsampledata), y ; pitch
        pha
        tay
        iny
        jsr doplot
        pla
        tay
        dey
    .doplot
        lda editcursor
        clc
        adc #PITCH_GRAPH_X
        tax
        jsr plot
        eor (w), y
        sta (w), y
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
}

; vim: ts=4 sw=4 et ft=asm


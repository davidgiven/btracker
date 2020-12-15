PITCH_GRAPH_X = 12
PITCH_GRAPH_Y = 37

VOLUME_GRAPH_X = 12
VOLUME_GRAPH_Y = 58

TONE_ADDRESS        = &7c00 + 14 + 2*40
SCALE_ADDRESS       = TONE_ADDRESS + 40
SAMPLERATE_ADDRESS  = SCALE_ADDRESS + 40
REPEATSTART_ADDRESS = SAMPLERATE_ADDRESS + 40
REPEATEND_ADDRESS   = REPEATSTART_ADDRESS + 6

EDIT_TONE = 0
EDIT_SCALE = 1
EDIT_SAMPLERATE = 2
EDIT_REPEATSTART = 3
EDIT_REPEATEND = 4
EDIT_PITCH = 5
EDIT_VOLUME = 6

.tone_editor
{
    ldx #0
    txs
    stx playing
    jsr select_tone
    jsr redraw
    jsr panic

.editloop
    jsr move_hardware_cursor
    lda #&81
    ldx #32         ; 320ms
    ldy #0
    jsr OSBYTE
    txa
    bcc key_pressed_with_cursor_off
    jsr toggle_teletext_cursor
    lda #&81
    ldx #32         ; 320ms
    ldy #0
    jsr OSBYTE
    txa
    bcc key_pressed_with_cursor_on
    jsr toggle_teletext_cursor
    jmp editloop
    
.key_pressed_with_cursor_on
    pha
    jsr toggle_teletext_cursor
    pla
.key_pressed_with_cursor_off
    jsr do_key
    jmp editloop

.do_key
    key_lookup key_table
    rts

.key_table
    key 161, pattern_editor
    key 139, key_up
    key 138, key_down
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

    .key_up
    {
        lda editcursory
        sec
        sbc #1
        cmp #&ff
        bne t
        lda #EDIT_VOLUME
    .t
        sta editcursory
        rts
    }

    .key_down
    {
        lda editcursory
        clc
        adc #1
        cmp #EDIT_VOLUME+1
        bne t
        lda #0
    .t
        sta editcursory
        rts
    }

    .key_left
    {
        lda editcursorx
        sec
        sbc #1
        and #63
        sta editcursorx
        rts
    }

    .key_right
    {
        lda editcursorx
        clc
        adc #1
        and #63
        sta editcursorx
        rts
    }

    .key_increment
    {
        lda #1
        jmp adjust_value
    }
    .key_decrement
    {
        lda #lo(-1)
        jmp adjust_value
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
        jsr cursor_off
        jsr setup_screen_for_tone_editor

        ; Draw the status displays.

        lda #lo(TONE_ADDRESS)
        sta scrptr+0
        lda #hi(TONE_ADDRESS)
        sta scrptr+1
        lda edittone
        jsr print_h4

        lda #40-1
        jsr advance_scrptr
        lda #1
        jsr print_h8

        lda #40-2
        jsr advance_scrptr
        ldx edittone
        lda samplespeed, x
        jsr print_h8

        lda #40-2
        jsr advance_scrptr
        ldx edittone
        lda repeatstart, x
        jsr print_h8

        lda #4
        jsr advance_scrptr
        ldx edittone
        lda repeatend, x
        jsr print_h8

        ; Draw the graphs.

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
        sec
        lda #VOLUME_GRAPH_Y
        sbc (editsampledata), y ; volume
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

    .move_hardware_cursor
    {
        lda editcursory
        cmp #EDIT_TONE
        beq edit_tone
        cmp #EDIT_SCALE
        beq edit_scale
        cmp #EDIT_SAMPLERATE
        beq edit_samplerate
        cmp #EDIT_REPEATSTART
        beq edit_repeatstart
        cmp #EDIT_REPEATEND
        beq edit_repeatend
        jmp cursor_off

    .edit_tone
        ldx #lo(TONE_ADDRESS)
        ldy #hi(TONE_ADDRESS)
        jsr move_cursor
        jmp cursor_on

    .edit_scale
        ldx #lo(SCALE_ADDRESS)
        ldy #hi(SCALE_ADDRESS)
        jsr move_cursor
        jmp cursor_on

    .edit_samplerate
        ldx #lo(SAMPLERATE_ADDRESS)
        ldy #hi(SAMPLERATE_ADDRESS)
        jsr move_cursor
        jmp cursor_on

    .edit_repeatstart
        ldx #lo(REPEATSTART_ADDRESS)
        ldy #hi(REPEATSTART_ADDRESS)
        jsr move_cursor
        jmp cursor_on

    .edit_repeatend
        ldx #lo(REPEATEND_ADDRESS)
        ldy #hi(REPEATEND_ADDRESS)
        jsr move_cursor
        jmp cursor_on
    }

    .toggle_teletext_cursor
    {
        lda editcursory
        cmp #EDIT_PITCH
        beq edit_pitch
        cmp #EDIT_VOLUME
        beq edit_volume
        rts

    .edit_pitch
        ldy editcursorx
        sec
        lda #PITCH_GRAPH_Y
        sbc (editsampledata), y ; pitch
        tay
        lda #PITCH_GRAPH_X
    .cursor_on_graph
        clc
        adc editcursorx
        tax
        jsr plot
        eor (w), y
        sta (w), y
        jmp cursor_off

    .edit_volume
        lda editcursorx
        clc
        adc #64
        tay
        sec
        lda #VOLUME_GRAPH_Y
        sbc (editsampledata), y ; volume
        tay
        lda #VOLUME_GRAPH_X
        jmp cursor_on_graph
    }

    ; On entry, A is the delta.
    .adjust_value
    {
        ldx editcursory
        cpx #EDIT_TONE
        beq edit_tone
        cpx #EDIT_SCALE
        beq edit_scale
        cpx #EDIT_SAMPLERATE
        beq edit_samplerate
        cpx #EDIT_REPEATSTART
        beq edit_repeatstart
        cpx #EDIT_REPEATEND
        beq edit_repeatend
        cpx #EDIT_VOLUME
        beq edit_volume
    .edit_pitch
    {
        ldy editcursorx
        clc
        adc (editsampledata), y ; pitch
        cmp #&10
        bne not_toobig
        lda #&f
    .not_toobig
        cmp #lo(-&10)
        bne not_toosmall
        lda #lo(-&f)
    .not_toosmall
        sta (editsampledata), y
        jmp redraw
    }

    .edit_volume
    {
        tax
        lda editcursorx
        clc
        adc #64
        tay
        txa
        clc
        adc (editsampledata), y ; pitch
        cmp #1
        bne not_toobig
        lda #0
    .not_toobig
        cmp #lo(-&10)
        bne not_toosmall
        lda #lo(-&f)
    .not_toosmall
        sta (editsampledata), y
        jmp redraw
    }

    .edit_tone
        clc
        adc edittone
        and #&f
        sta edittone
        jsr select_tone
        jmp redraw

    .edit_scale
        jmp redraw

    .edit_samplerate
        ldx edittone
        clc
        adc samplespeed, x
        sta samplespeed, x
        jmp redraw

    .edit_repeatstart
        ldx edittone
        clc
        adc repeatstart, x
        and #63
        sta repeatstart, x
        jmp redraw

    .edit_repeatend
        ldx edittone
        clc
        adc repeatend, x
        and #63
        sta repeatend, x
        jmp redraw
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

        equb 128+6      ; cyan
        equs "Tone:"
        equb 128+7      ; white
        equb 31, ' ', 2, ' '

        equb 128+6      ; cyan
        equs "Pitch scale:"
        equb 128+7      ; white
        equb 26, ' '

        equb 128+6      ; cyan
        equs "Sample rate:"
        equb 128+7      ; white
        equb 26, ' '

        equb 128+6      ; cyan
        equs "Repeat:"
        equb 128+7      ; white
        equb 7, ' '
        equb 128+6      ; cyan
        equs "to"
        equb 128+7      ; white
        equb 20, ' '

        equb 20, ' '
        equb 20, ' '

        equb 2, ' '
        equb 128+6      ; cyan
        equs "+"
        equb 144+7      ; white
        equb &2c
        equb 31, ' '
        equb ' ', &2c

        equb 5, ' '
        equb 144+7      ; white
        equb &62
        equb 31, ' '
        equb ' ', &31

        equb 5, ' '
        equb 144+7      ; white
        equb &28
        equb 31, ' '
        equb ' ', &24

        equb ' '
        equb 128+1      ; red
        equb 'P'
        equb 3, 144+7    ; white
        equb &62
        equb 31, ' '
        equb ' ', &31

        equb ' '
        equb 128+1      ; red
        equb 'I'
        equb 3, 144+7   ; white
        equb &28
        equb 31, ' '
        equb ' ', &24

        equb ' '
        equb 128+1      ; red
        equb 'T'
        equb 128+6      ; cyan
        equs "0"
        equb 144+7      ; white
        equb &6e
        equb 31, ' '
        equb ' ', &3d

        equb ' '
        equb 128+1      ; red
        equb 'C'
        equb 3, 144+7   ; white
        equb &28
        equb 31, ' '
        equb ' ', &24

        equb ' '
        equb 128+1      ; red
        equb 'H'
        equb 3, 144+7   ; white
        equb &62
        equb 31, ' '
        equb ' ', &31

        equb 5, ' '
        equb 144+7      ; white
        equb &28
        equb 31, ' '
        equb ' ', &24

        equb 5, ' '
        equb 144+7      ; white
        equb &62
        equb 31, ' '
        equb ' ', &31

        equb 3, ' '
        equb 128+6      ; cyan
        equs "-"
        equb 144+7      ; white
        equb &2c
        equb 31, ' '
        equb ' ', &2c

        equb 20, ' ', 21, ' '

        equb 128+1      ; red
        equb 'V'
        equb 128+6      ; cyan
        equs "0"
        equb 144+7      ; white
        equb &2c
        equb 31, ' '
        equb ' ', &2c

        equb ' '
        equb 128+1      ; red
        equb 'O'
        equb 3, 144+7      ; white
        equb &62
        equb 31, ' '
        equb ' ', &31

        equb ' '
        equb 128+1      ; red
        equb 'L'
        equb 3, 144+7      ; white
        equb &28
        equb 31, ' '
        equb ' ', &24

        equb ' '
        equb 128+1      ; red
        equb 'U'
        equb 3, 144+7      ; white
        equb &62
        equb 31, ' '
        equb ' ', &31

        equb ' '
        equb 128+1      ; red
        equb 'M'
        equb 3, 144+7      ; white
        equb &28
        equb 31, ' '
        equb ' ', &24

        equb ' '
        equb 128+1      ; red
        equb 'E'
        equb 128+6      ; cyan
        equs "-"
        equb 144+7      ; white
        equb &2e
        equb 31, ' '
        equb ' ', &2d
        equb 0
    }
}

; vim: ts=4 sw=4 et ft=asm


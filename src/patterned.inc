; --- Main routine ----------------------------------------------------------

.pattern_editor
{
    ldx #0
    txs
    jsr setup_screen_for_pattern_editor
.current_note_has_changed
    jsr draw_sequence
    jsr play_current_note
.main_loop
    jsr draw_screen
    jsr cursor_on

    ; Play the appropriate number of ticks of music.

.playloop
    lda #KBD_IRQ
    bit SHEILA+SYSTEM_VIA+VIA_IFR
    bne keypress

    lda tickcount
    bpl playloop

    ; If we fall out the bottom, we're out of ticks. If we're actually playing,
    ; advance to the next row.

    lda playing
    beq main_loop

    ldx rowno
    inx
    cpx patternlength
    blt nowrap

    bit looping
    bne no_advance
    ldy seqindex
    iny
    cpy #SEQUENCE_LENGTH
    beq back_to_start
    lda sequence, y
    bpl reload_pattern
.back_to_start
    ldy #0
    lda sequence, y
.reload_pattern
    sty seqindex
    sta patternno
.no_advance
    ldx #0
.nowrap
    stx rowno

    jmp current_note_has_changed

.keypress
    cli
    jsr do_patterneditor_keypress
    jmp current_note_has_changed
}

; --- Pattern editor key entry ----------------------------------------------

.do_patterneditor_keypress
{
    lda #&81
    ldx #0
    ldy #0
    jsr OSBYTE
    bcs ret

    txa
    key_lookup key_table

    {
        cmp #'0'
        blt no
        cmp #'9'+1
        blt number_key
    .no
    }

    {
        cmp #'A'
        blt no
        cmp #'Z'+1
        blt letter_key
    .no
    }

    {
        cmp #'a'
        blt no
        cmp #'z'+1
        blt letter_key
    .no
    }
.ret
    rts

    .key_table
        key 161, file_editor
        key 163, tone_editor
        key 139, key_up
        key 138, key_down
        key 136, key_left
        key 137, key_right
        key 9, key_tab
        key '+', key_increment
        key '-', key_decrement
        key '<', key_tempo_down
        key '>', key_tempo_up
        key ',', key_pattern_shorter
        key '.', key_pattern_longer
        key 32, key_pause
        key 27, key_stop
        key 'S'-'@', key_save_to_sequence
        key 'A'-'@', key_insert_after_into_sequence
        key 'W'-'@', key_insert_before_into_sequence
        key 'Q'-'@', key_delete_from_sequence
        key 'N'-'@', key_new_pattern
        key 'L'-'@', key_loop
        key 'D'-'@', key_duplicate_pattern
        equb 0

    .number_key
    {
        ora #&10
        sec
        sbc #'0'
        pha
        jsr detect_ctrl
        beq ctrl
        pla
        jmp set_value

    .ctrl
        pla
        jmp key_mute
    }

    .letter_key
    {
        and #lo(not(&20))
        pha
        jsr detect_shift
        beq shifted
        pla
        sec
        sbc #'A'-10
        jmp set_value

    .shifted
        pla
        sec
        sbc #'@'
    .*command_key
        clc
        adc #FIRST_COMMAND-1
        tax
        lda cursorx
        lsr a
        and #&0e
        tay
        txa
        sta (rowptr), y
        rts
    }

    .key_up
    {
        jsr detect_ctrl
        beq ctrl

        lda rowno
        sec
        sbc #1
        and #&1f
        sta rowno
        rts

    .ctrl
        lda patternno
        clc
        adc #1
        cmp numpatterns
        bne t
        lda #0
    .t
        sta patternno
        jmp reset_row_pointer
    }

    .key_down
    {
        jsr detect_ctrl
        beq ctrl

        lda rowno
        clc
        adc #1
        and #&1f
        sta rowno
        rts

    .ctrl
        lda patternno
        sec
        sbc #1
        bpl t
        ldx numpatterns
        dex
        txa
    .t
        sta patternno
        jmp reset_row_pointer
    }

    .key_left
    {
        jsr detect_ctrl
        beq ctrl

        lda cursorx
        beq ret
        dec cursorx
    .ret
        rts

    .ctrl
        ldx seqindex
        beq ret
        dex
        stx seqindex
        lda sequence, x
        sta patternno
        jsr draw_sequence
        jmp reset_row_pointer
    }

    .key_right
    {
        jsr detect_ctrl
        beq ctrl

        lda cursorx
        cmp #15
        beq ret
        inc cursorx
    .ret
        rts

    .ctrl
        ldx seqindex
        cpx #127
        beq ret
        inx
        lda sequence, x
        bmi ret
        sta patternno
        stx seqindex
        jsr draw_sequence
        jmp reset_row_pointer
    }

    .key_tab
    {
        jsr detect_ctrl
        bne n
        lda #9
        jmp command_key
    .n
        jsr detect_shift
        beq shifted

        lda cursorx
        clc
        adc #4
    .write
        and #&0f
        sta cursorx
        rts

    .shifted
        lda cursorx
        sec
        sbc #4
        jmp write
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

    .key_tempo_down
    {
        dec tempo
        bne ret
        inc tempo
    .ret
        rts
    }

    .key_tempo_up
    {
        inc tempo
        bne ret
        dec tempo
    .ret
        rts
    }

    .key_pattern_shorter
    {
        dec patternlength
        bne ret
        inc patternlength
    .ret
        rts
    }

    .key_pattern_longer
    {
        ldx patternlength
        cpx #&20
        beq ret
        inx
        stx patternlength
    .ret
        rts
    }

    .key_pause
    {
        lda playing
        eor #&ff
        sta playing
        rts
    }

    .key_stop
    {
        lda #0
        sta playing
        jmp panic
    }

    .key_save_to_sequence
    {
        ldx seqindex
        lda patternno
        sta sequence, x
        jmp draw_sequence
    }

    .key_insert_before_into_sequence
    {
        ldx #127
    .loop
        lda sequence-1, x
        sta sequence, x
        dex
        cpx seqindex
        bne loop

        ldx seqindex
        lda patternno
        sta sequence, x
        jmp draw_sequence
    }

    .key_insert_after_into_sequence
    {
        ldx #127
    .loop
        lda sequence-1, x
        sta sequence, x
        dex
        cpx seqindex
        bne loop

        ldx seqindex
        inx
        cpx #128
        beq ret
        stx seqindex
        lda patternno
        sta sequence, x
    .ret
        jmp draw_sequence
    }

    .key_delete_from_sequence
    {
        ldx seqindex
        inx
        bmi ret             ; don't delete if we're on the last seq index
        lda sequence, x
        bpl go              ; don't delete the last item
    .ret
        rts

    .go
        ldx seqindex
    .loop
        lda sequence+1, x
        sta sequence, x
        inx
        cpx #127
        bne loop

        ldx seqindex
        lda sequence, x
        sta patternno
        jsr reset_row_pointer
        jmp draw_sequence
    }

    .key_new_pattern
    {
        ldx numpatterns
        cpx #MAX_PATTERNS
        bne insert
    .noroom
        jsr draw_dialogue
        ldx #lo(toomany)
        ldy #hi(toomany)
        jsr draw_packed
        jmp press_any_key

    .insert
        stx patternno
        inx
        stx numpatterns

    .exit
        jsr reset_row_pointer
        jmp draw_sequence

    .toomany
        equw &7c00 + 12*40 + 15
        equb 128+1      ; read
        equs "No room!"
        equb 0
    }

    .key_loop
    {
        lda looping
        eor #&ff
        sta looping
        rts
    }

    .key_mute
    {
        tax
        dex
        cpx #4
        bge ret
        lda mute, x
        eor #&ff
        sta mute, x
    .ret
        rts
    }

    .key_duplicate_pattern
    {
        jsr draw_dialogue
        ldx #lo(message_data)
        ldy #hi(message_data)
        jsr draw_packed
        jsr read_number
        bcs ret

        cmp numpatterns
        bge out_of_range

        sta p                   ; destination pattern
        clc
        adc #hi(PATTERN_DATA)
        sta w+1
        lda #0
        sta w+0                 ; w contains destination address
        sta rowptr+0            ; rowptr is source

        ldy #0
    .loop
        lda (rowptr), y
        sta (w), y
        iny
        bne loop

        lda p
        sta patternno
        rts
        
    .out_of_range
        jsr draw_dialogue
        ldx #lo(out_of_range_data)
        ldy #hi(out_of_range_data)
        jsr draw_packed
        jsr press_any_key
    .ret
        rts

    .message_data
        equw &7c00 + 12*40 + 0
        equb 128+6      ; cyan
        equs "Enter pattern number to copy to:"
        equb 0

    .out_of_range_data
        equw &7c00 + 12*40 + 5
        equb 128+1      ; red
        equs "Pattern number out of range!"
        equb 0
    }

    .detect_shift
    {
        lda #&81
        ldy #&ff
        ldx #&ff ; shift
        jsr OSBYTE
        cpy #&ff
        rts
    }

    .detect_ctrl
    {
        lda #&81
        ldy #&ff
        ldx #&fe ; control
        jsr OSBYTE
        cpy #&ff
        rts
    }
}

; --- Editor logic ----------------------------------------------------------

; Adjusts the value under the cursor by A.

{
    .drum
    {
        txa
        clc
        adc (rowptr), y
        and #7
        sta (rowptr), y
        rts
    }

    .drumt
    {
        lda (rowptr), y
        eor #4
        sta (rowptr), y
        rts
    }

.*adjust_value
    tax
    lda cursorx
    lsr a
    and #&fe
    tay

    lda (rowptr), y     ; if not a note, do nothing
    cmp #FIRST_COMMAND
    bge nochange

    lda cursorx
    cmp #12
    beq drum
    cmp #13
    beq drumt
    and #&03
    beq pitch
    cmp #1
    beq octave
    cmp #2
    beq tone
    .volume
    {
        iny
        lda (rowptr), y
        and #&f0
        sta p

        txa
        bmi negative

        lda (rowptr), y
        and #&0f
        cmp #&0f
        beq nochange
        clc
        adc #1
        ora p
        sta (rowptr), y
        rts

    .negative
        lda (rowptr), y
        and #&0f
        beq nochange
        sec
        sbc #1
        ora p
        sta (rowptr), y
    .^nochange
        rts
    }

    .tone
    {
        iny
        lda (rowptr), y
        and #&0f
        sta p

        txa
        bmi negative

        lda (rowptr), y
        and #&f0
        cmp #&f0
        beq nochange
        clc
        adc #&10
        ora p
        sta (rowptr), y
        rts

    .negative
        lda (rowptr), y
        and #&f0
        beq nochange
        sec
        sbc #&10
        ora p
        sta (rowptr), y
    .nochange
        rts
    }

    .pitch
    {
        txa
        bpl positive
        lda #3
        jmp change_pitch_down
    .positive
        lda #3
    }
    .change_pitch_up
    {
        clc
        adc (rowptr), y
        cmp #NUM_PITCHES
        bcs nochange
        sta (rowptr), y
    .nochange
        rts
    }

    .octave
    {
        lda (rowptr), y
        cmp #FIRST_COMMAND
        bge nochange
        txa
        bmi negative
        lda #12*3
        jmp change_pitch_up
    .negative
        lda #12*3
    }
    .change_pitch_down
    {
        eor #&ff
        sec
        adc (rowptr), y
        bcc nochange
        sta (rowptr), y
    .nochange
        rts
    }
}

; Sets the value under the cursor to A.

{
    .drum
    {
        txa
        and #7
        sta (rowptr), y
        rts
    }

.*set_value
    tax
    lda cursorx
    lsr a
    and #&fe
    tay

    lda (rowptr), y     ; if not a note, pick one at random
    cmp #FIRST_COMMAND
    blt is_a_note
    lda #C_1
    sta (rowptr), y
.is_a_note

    lda cursorx
    cmp #12
    beq drum
    cmp #13
    beq drum
    and #&03
    cmp #2
    beq tone
    cmp #3
    beq volume
    
    ; Changing pitch or octave, depending on key pressed.

    cpx #10
    blt octave
    .pitch
    {
        txa
        sec
        sbc #10
        blt nochange

        pha
        lda (rowptr), y
        tax
        lda note_decode_table, x    ; decode to octave, pitch
        lsr a
        lsr a
        lsr a
        lsr a                       ; extract octave number
        tax
        lda octave_to_note_table, x ; get pitch of C for this octave
        sta p
        pla

        tax
        lda name_to_note_table, x   ; get relative pitch of this note
        clc
        adc p                       ; adjust for octave
        sta (rowptr), y
    .^nochange
        rts
    }

    .volume
        iny
        lda (rowptr), y
        and #&f0
        sta p

        txa
        ora p
        sta (rowptr), y
        rts
        
    .tone
        iny
        lda (rowptr), y
        and #&0f
        sta p

        txa
        asl a
        asl a
        asl a
        asl a
        ora p
        sta (rowptr), y
        rts
        
    .octave
    {
        stx p

        lda (rowptr), y
        tax
        lda note_decode_table, x    ; decode to octave, pitch
        lsr a
        lsr a
        lsr a
        lsr a
        cmp p                       ; test current octave against desired
        blt raise

    .lower
        tax
        lda (rowptr), y
    .decrease_loop
        sec
        sbc #12*3
        bcc no_change
        dex
        cpx p
        bne decrease_loop
        jmp ret

    .raise
        tax
        lda (rowptr), y
    .increase_loop
        clc
        adc #12*3
        cmp #NUM_PITCHES
        bge no_change
        inx
        cpx p
        bne increase_loop
    .ret
        sta (rowptr), y
    .no_change
        rts
    }

}

; --- Pattern drawing -------------------------------------------------------

; Draw the sequence view.

SEQUENCE_ADDRESS = &7c00 + 2*40 + 1
.draw_sequence
{
    ldy #38
    lda #' '
.loop1
    sta SEQUENCE_ADDRESS, y
    dey
    bpl loop1

    lda #lo(SEQUENCE_ADDRESS)
    sta scrptr+0
    lda #hi(SEQUENCE_ADDRESS)
    sta scrptr+1

    ldx #0          ; count
    lda seqindex
    sec
    sbc #6
    tay             ; sequence index
.loop2
    cpy #0
    bpl draw
.skip
    lda #3
    jsr advance_scrptr
    jmp next
.draw
    lda sequence, y
    bmi skip
    stx p
    sty q
    jsr print_h8
    lda #' '
    jsr print_char
    ldx p
    ldy q
.next
    iny
    inx
    cpx #13
    bne loop2

    rts
}

; Draw the note view..

LINE_PADDING = 0
PATTERN_NUMBER_ADDRESS = &7c00 + 3*40 + 19
TEMPO_ADDRESS = &7c00 + 5*40 + 17
PATTERNLENGTH_ADDRESS = &7c00 + 6*40 + 17
MUTE_ADDRESS = &7c00 + 7*40 + 7
.draw_screen
{
    lda #lo(PATTERN_NUMBER_ADDRESS)
    sta scrptr+0
    lda #hi(PATTERN_NUMBER_ADDRESS)
    sta scrptr+1
    lda patternno
    jsr print_h8
    lda #lo(PATTERN_NUMBER_ADDRESS + 40)
    sta scrptr+0
    lda patternno
    jsr print_h8

    lda #lo(TEMPO_ADDRESS)
    sta scrptr+0
    lda #hi(TEMPO_ADDRESS)
    sta scrptr+1
    lda tempo
    jsr print_h8

    lda #lo(PATTERNLENGTH_ADDRESS)
    sta scrptr+0
    lda #hi(PATTERNLENGTH_ADDRESS)
    sta scrptr+1
    lda patternlength
    jsr print_h8

    {
        bit looping
        beq looping_off
        ldx #lo(looping_on_msg)
        ldy #hi(looping_on_msg)
        jmp go
    .looping_off
        ldx #lo(looping_off_msg)
        ldy #hi(looping_off_msg)
    .go
        jsr draw_packed
    }

    {
        ldx #3
        ldy #24
    .loop
        lda #128+6
        clc
        adc mute, x         ; -1 for yes, 0 for no
        sta MUTE_ADDRESS, y
        tya
        sec
        sbc #8
        tay
        dex
        bpl loop
    }

    lda #lo(MIDDLE_ROW_ADDRESS)
    sta scrptr+0
    lda #hi(MIDDLE_ROW_ADDRESS)
    sta scrptr+1
    lda rowptr+0
    sta disptr+0
    lda rowptr+1
    sta disptr+1

    lda rowno
    sta disrow

    lda #9
.down_loop
    pha

    jsr draw_row
    lda #LINE_PADDING
    jsr advance_scrptr
    inc disrow

    pla
    sec
    sbc #1
    bne down_loop

    lda #lo(MIDDLE_ROW_ADDRESS-40)
    sta scrptr+0
    lda #hi(MIDDLE_ROW_ADDRESS-40)
    sta scrptr+1
    sec
    lda rowptr+0
    sbc #ROW_LENGTH
    sta disptr+0
    lda rowptr+1
    sbc #0
    sta disptr+1

    lda rowno
    sta disrow
    dec disrow

    lda #8
.up_loop
    pha

    jsr draw_row
    lda #80-LINE_PADDING
    jsr retard_scrptr
    dec disrow

    ; disptr has been advanced to the next row, so to get the previous row,
    ; we need to go back two..

    lda disptr+0
    sec
    sbc #ROW_LENGTH*2
    sta disptr+0

    pla
    sec
    sbc #1
    bne up_loop

    ; Place the cursor.

    ldy cursorx
    ldx editor_cursor_table, y
    ldy #hi(MIDDLE_ROW_ADDRESS)
    jsr move_cursor

    rts

.looping_on_msg
    equw &7c00 + 5*40 + 31
    equb 128+7      ; white
    equs "Looping"
    equb 0

.looping_off_msg
    equw &7c00 + 5*40 + 31
    equb 128+7      ; white
    equs "Playing"
    equb 0
}

.draw_row
{
    lda disrow
    cmp #NUM_STEPS
    bcs blank_row

    and #&07
    beq cyan_background
    cmp #4
    beq blue_background
    lda #' '
    jsr print_char
    jsr print_char
    jmp line

.cyan_background
    lda #128+1      ; cyan
    jsr print_char
    lda #157        ; background
    jsr print_char
    jmp line

.blue_background
    lda #128+4      ; cyan
    jsr print_char
    lda #157        ; background
    jsr print_char
    jmp line

.line
    lda #128+3      ; yellow
    jsr print_char
    lda disrow
    jsr print_h8
    lda #128+7      ; white
    jsr print_char
    jsr print_char
    jsr draw_note
    jsr print_space
    jsr draw_note
    jsr print_space
    jsr draw_note
    jsr print_space
    jsr draw_drum
    jsr print_space
    lda #156
    jsr print_char
    rts

.blank_row
    ldy #39
    lda #0
.loop
    sta (scrptr), y
    dey
    bpl loop

    lda #40-LINE_PADDING
    jmp advance_scrptr

.background_color_table
    equb 128+6
    equb 128
    equb 32
    equb 32
    equb 128+4
    equb 32
    equb 32
    equb 32
}

; Draw the note at disptr to scrptr.
; Advances both.

.draw_note
{
    ; Note

    ldy #0
    lda (disptr), y
    cmp #FIRST_COMMAND
    bge is_a_command
    jsr draw_raw_note
.next
    lda #' '
    jsr print_char

    ; Tone/volume/control

    ldy #1
    lda (disptr), y
    jsr print_h8

.done
    lda #128+7
    jsr print_char
    inc disptr+0
    inc disptr+0
    rts

.*draw_drum
    ldy #0
    lda (disptr), y
    cmp #FIRST_COMMAND
    bge is_a_command
    jsr draw_raw_drum
    jmp next

.is_a_command
    cmp #FIRST_COMMAND + ('O' - 'A')
    beq off_command
    cmp #FIRST_COMMAND + ('B' - 'A')
    beq blank_command

    pha
    lda #128+3
    jsr print_char
    jsr print_char
    pla
    sec
    sbc #FIRST_COMMAND - 'A'
    jsr print_char
    jmp next

.off_command
    lda #'='
    pha
    jmp banner

.blank_command
    lda #'.'
    pha
.banner
    {
        lda #' '
        jsr print_char
        pla
        ldx #3
    .loop
        jsr print_char
        dex
        bpl loop
        lda #' '
        jsr print_char
        jmp done
    }
}

; Draws the raw note in A to scrptr. Advances it. Does not draw the tone/volume byte.

.draw_raw_note
{
    tax
    lda note_decode_table, x
    bmi not_a_note
    pha
    and #&0f
    asl a
    tay
    pha
    lda note_to_name_table, y
    jsr print_char
    pla
    tay
    iny
    lda note_to_name_table, y
    jsr print_char
    pla
    lsr a
    lsr a
    lsr a
    lsr a
    jmp print_h4

.not_a_note
    lda #'?'
    jsr print_char
    txa
    jmp print_h8
}

; Draws the raw drum thingy in A to scrptr. Advances it. Does not draw the tone/volume byte.

.draw_raw_drum
{
    asl a
    clc
    adc (disptr), y
    tax
    lda drum_to_name_tabe, x
    jsr print_char
    inx
    lda drum_to_name_tabe, x
    jsr print_char
    inx
    lda drum_to_name_tabe, x
    jmp print_char
}

.setup_screen_for_pattern_editor
{
    jsr draw_standard_header

    ldx #7
    ldy #18
    jsr draw_tab
    ldx #lo(data)
    ldy #hi(data)
    jmp draw_packed

.data
    equw &7c00 + 80

    equb 128+3      ; yellow
    equb 20, ' '
    equb 19, ' '

    equb 144+6      ; cyan
    equb 15, &70
    equb &7a
    equb 128+7      ; white
    equb 141        ; double height
    equs "00"
    equb 140        ; normal height
    equb 144+6      ; cyan
    equb &75
    equb 15, &70
    equb ' '

    equb 141
    equb 18, ' '
    equs "00"
    equb 19, ' '

    equb 128+6      ; cyan
    equs "Current tempo:"
    equb 128+7      ; white
    equb 24, ' '

    equb 128+6      ; cyan
    equs "Pattern length:"
    equb 128+7      ; white
    equb 23, ' '

    equb 144+6      ; cyan
    equb 6, &2c
    equb 128+6      ; cyan
    equs "CH 1"
    equb 144+6
    equb &2c, &2c
    equb 128+6      ; cyan
    equs "CH 2"
    equb 144+6
    equb &2c, &2c
    equb 128+6      ; cyan
    equs "CH 3"
    equb 144+6
    equb &2c, &2c
    equb 128+6      ; cyan
    equs "DRUM"
    equb 144+6
    equb 2, &2c
    equb 0
}

.note_to_name_table
    equs "C-"
    equs "C#"
    equs "D-"
    equs "D#"
    equs "E-"
    equs "F-"
    equs "F#"
    equs "G-"
    equs "G#"
    equs "A-"
    equs "A#"
    equs "B-"

.drum_to_name_tabe
    equs "HiT"
    equs "MdT"
    equs "LoT"
    equs "c2T"
    equs "Hi "
    equs "Md "
    equs "Lo "
    equs "c2 "

.name_to_note_table
    equb  9*3 ; "A-"
              ; "A#"
    equb 11*3 ; "B-"
    equb  0*3 ; "C-"
              ; "C#"
    equb  2*3 ; "D-"
              ; "D#"
    equb  4*3 ; "E-"
    equb  5*3 ; "F-"
              ; "F#"
    equb  7*3 ; "G-"
              ; "G#"

.octave_to_note_table
    equb 0*12*3, 1*12*3, 2*12*3, 3*12*3, 4*12*3, 5*12*3

; The low byte of the positions of the cursor for the editor.
.editor_cursor_table
    for note, 0, 3
        equb lo(MIDDLE_ROW_ADDRESS) + 7 + 8*note + 0
        equb lo(MIDDLE_ROW_ADDRESS) + 7 + 8*note + 2
        equb lo(MIDDLE_ROW_ADDRESS) + 7 + 8*note + 4
        equb lo(MIDDLE_ROW_ADDRESS) + 7 + 8*note + 5
    next

; vim: ts=4 sw=4 et ft=asm


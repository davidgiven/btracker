; --- Pattern editor key entry ----------------------------------------------

.do_patterneditor_keypress
{
    lda #&81
    ldx #0
    ldy #0
    jsr OSBYTE
    bcs ret

    cpx #139
    beq key_up
    cpx #138
    beq key_down
    cpx #136
    beq key_left
    cpx #137
    beq key_right
    cpx #9
    beq key_tab
    cpx #43
    beq key_increment
    cpx #45
    beq key_decrement
    cpx #60
    beq tempo_down
    cpx #62
    beq tempo_up
    cpx #32
    beq playstop
    cpx #27
    beq panic

    {
        cpx #'0'
        blt no
        cpx #'9'+1
        blt number_key
    .no
    }

    {
        cpx #'A'
        blt no
        cpx #'G'+1
        blt letter_key
    .no
    }
    
.ret
    rts

    .key_up
    print ~key_up
    {
        lda rowno
        sec
        sbc #1
        and #&1f
        sta rowno

        sec
        lda rowptr+0
        sbc #ROW_LENGTH
        sta rowptr+0
        rts
    }

    .key_down
    {
        lda rowno
        clc
        adc #1
        and #&1f
        sta rowno

        clc
        lda rowptr+0
        adc #ROW_LENGTH
        sta rowptr+0
        rts
    }

    .key_left
    {
        lda cursorx
        beq ret
        dec cursorx
    .ret
        rts
    }

    .key_right
    {
        lda cursorx
        cmp #15
        beq ret
        inc cursorx
    .ret
        rts
    }

    .key_tab
    {
        lda cursorx
        clc
        adc #4
        and #&0f
        sta cursorx
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

    .number_key
    {
        txa
        sec
        sbc #'0'
        jmp set_value
    }

    .letter_key
    {
        txa
        sec
        sbc #'A'-10
        jmp set_value
    }

    .tempo_down
    {
        dec tempo
        bne ret
        inc tempo
    .ret
        rts
    }

    .tempo_up
    {
        inc tempo
        bne ret
        dec tempo
    .ret
        rts
    }

    .playstop
    {
        lda playing
        eor #&ff
        sta playing
        rts
    }

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

; --- Editor logic ----------------------------------------------------------

; Adjusts the value under the cursor by A.

.adjust_value
{
    tax
    lda cursorx
    lsr a
    and #&fe
    tay
    lda cursorx
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
.nochange
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

.set_value
{
    tax
    lda cursorx
    lsr a
    and #&fe
    tay
    lda cursorx
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
.nochange
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

; Draw the entire screen.

.draw_screen
{
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
    lda #5
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
    lda #80-5
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
}

.draw_row
{
    lda disrow
    cmp #NUM_STEPS
    bcs blank_row

    jsr print_h8
    lda #2
    jsr advance_scrptr
    jsr draw_note
    lda #1
    jsr advance_scrptr
    jsr draw_note
    lda #1
    jsr advance_scrptr
    jsr draw_note
    lda #1
    jsr advance_scrptr
    jsr draw_note
    rts

.blank_row
    ldy #39
    lda #0
.loop
    sta (scrptr), y
    dey
    bpl loop

    lda #40-5
    jmp advance_scrptr
}

; Draw the note at disptr to scrptr.
; Advances both.

.draw_note
{
    ; Pitch

    ldy #0
    lda (disptr), y
    cmp #FIRST_COMMAND
    bge is_a_command
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
    jsr print_h4
    jmp next

.not_a_note
    lda #'?'
    jsr print_char
    txa
    jsr print_h8
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

; vim: ts=4 sw=4 et ft=asm


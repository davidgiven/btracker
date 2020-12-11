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
    lda #6
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
    lda #80-6
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
    lda #2
    jsr advance_scrptr
    jsr draw_note
    lda #2
    jsr advance_scrptr
    jsr draw_note
    lda #2
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

    lda #40-6
    jmp advance_scrptr
}

; Draw the note at disptr to scrptr.
; Advances both.

.draw_note
{
    ; Pitch

    ldy #0
    lda (disptr), y
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
    inc scrptr+0
    bne t1
    inc scrptr+1
.t1

    ; Tone/volume/control

    ldy #1
    lda (disptr), y
    jsr print_h8

    inc disptr+0
    inc disptr+0
    rts
}

; vim: ts=4 sw=4 et ft=asm


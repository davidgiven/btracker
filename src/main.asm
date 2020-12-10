
OSRDCH  = &FFE0
OSWRCH  = &FFEE
OSBYTE  = &FFF4
OSFILE  = &FFDD
SHEILA  = &FE00

IFR     = 13
IER     = 14

CRTC_ADDRESS = SHEILA + &00 + 0
CRTC_DATA    = SHEILA + &00 + 1

SYSTEM_VIA  = &40
KBD_IRQ     = 1<<0
VSYNC_IRQ   = 1<<1
ADC_IRQ     = 1<<4
CLOCK_IRQ   = 1<<6

NUM_PATTERNS = 64
NUM_VOICES = 4

; One pattern is 256 bytes.
NUM_STEPS = 32
NOTE_LENGTH = 2 ; pitch, volume/tone; or: command, param

ROW_LENGTH = NOTE_LENGTH * NUM_VOICES
PATTERN_DATA = &7c00 - (NUM_PATTERNS * NUM_STEPS * ROW_LENGTH)
MUSIC_DATA = PATTERN_DATA - &100

; Notes

NUM_PITCHES = 201

; Screen layout.

; The middle row musn't span a page boundary.
MIDDLE_ROW = 16
MIDDLE_ROW_ADDRESS = &7c00 + (MIDDLE_ROW*40) + 1

;  0123456789012345678901234567890123456789
;     00 : F# 8f | F# 8f | F# 8f | XX 8f
org &00
.pitch      equb 0, 0, 0    ; master copy for note
.volume     equb 0, 0, 0
.cpitch     equb 0, 0, 0    ; current copy (based on tone procedure)
.cvolume    equb 0, 0, 0
.rpitch     equb 0, 0, 0    ; what the chip is currently playing
.rvolume    equb 0, 0, 0
.tone       equb 0, 0, 0    ; current tone
.tonet      equb 0, 0, 0    ; tone tick count
.w          equb 0
.q          equb 0
.p          equb 0
.rowptr     equw 0          ; pointer to current row
.patternno  equb 0          ; current pattern number
.rowno      equb 0          ; current row number
.disptr     equw 0          ; pointer to row being displayed
.disrow     equb 0          ; row number being displayed
.scrptr     equw 0          ; screen pointer
.cursorx    equb 0          ; position of cursor (0-15)
.tickcount  equb 0          ; ticks left in the current note
guard &9f

mapchar '#', 95             ; mode 7 character set

macro bge label
    bcs label
endmacro

macro blt label
    bcc label
endmacro

org &1b00
guard PATTERN_DATA

; --- Main program ----------------------------------------------------------

._start
    lda #&ff
    sta volume + 0
    sta volume + 1
    sta volume + 2
    jsr reset_row_pointer
.current_note_has_changed
    jsr play_current_note
.main_loop
{
    jsr draw_screen

    ; Play the appropriate number of ticks of music.
.playloop
    jsr process_tones
    jsr update_all_channels
    
    sei
.delayloop
    lda #KBD_IRQ
    bit SHEILA+SYSTEM_VIA+IFR
    bne keypress

    lda #CLOCK_IRQ
    bit SHEILA+SYSTEM_VIA+IFR
    beq delayloop
    cli

    dec tickcount
    bne playloop

    ; If we fall out the bottom, we're out of ticks, and so need to advance to
    ; the next row.

    lda rowptr+0
    clc
    adc #ROW_LENGTH
    sta rowptr+0
    
    lda rowno
    clc
    adc #1
    and #&1f
    sta rowno

    jmp current_note_has_changed

.keypress
    cli
    jsr do_keypress
    jmp current_note_has_changed
}

.do_keypress
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
}

.key_up
{
    lda rowno
    beq ret

    dec rowno

    sec
    lda rowptr+0
    sbc #ROW_LENGTH
    sta rowptr+0
.ret
    rts
}

.key_down
{
    lda rowno
    cmp #NUM_STEPS-1
    beq ret

    inc rowno

    clc
    lda rowptr+0
    adc #ROW_LENGTH
    sta rowptr+0
.ret
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

; Prints the hex nibble A at scrptr
.print_h4
    and #&0f
    jsr nibble_to_ascii
.print_char
    ldy #0
    sta (scrptr), y
{
    inc scrptr+0
    bne noinc
    inc scrptr+1
.noinc
    rts
}

; Prints A at scrptr
.print_h8
{
    pha
    lsr a
    lsr a
    lsr a
    lsr a
    jsr print_h4

    pla
    and #&0f
    jmp print_h4
}

; Advances the screen pointer by A bytes.
.advance_scrptr
{
    clc
    adc scrptr+0
    sta scrptr+0
    bcc noinc
    inc scrptr+1
.noinc
    rts
}

; Retards the screen pointer by A bytes.
.retard_scrptr
{
    sta w
    sec
    lda scrptr+0
    sbc w
    sta scrptr+0
    bcs nodec
    dec scrptr+1
.nodec
    rts
}

; Convert a nibble to ASCII.

.nibble_to_ascii
{
    cmp #10
    bcc less_than_ten
    adc #&66
.less_than_ten
    eor #%00110000
    rts
}

; --- Tone management -------------------------------------------------------

.process_tones
{
    ldx #0

.loop
    lda pitch, x
    sta cpitch, x

    lda volume, x
    sta cvolume, x

    inx
    cpx #3
    bne loop
    rts
}

; --- Playback --------------------------------------------------------------

; Cue up a new pattern.

.reset_row_pointer
{
    lda patternno
    clc
    adc #hi(PATTERN_DATA)
    sta rowptr+1
    lda #0
    sta rowptr+0
    sta rowno
    rts
}

; Starts playing the notes at rowptr.

.play_current_note
{
    ldy #0
    ldx #0

.loop
    lda (rowptr), y
    iny
    cmp #NUM_PITCHES
    bcc is_note
    iny
    jmp next

.is_note
    sta pitch, x
    lda (rowptr), y
    and #&0f
    sta volume, x
    lda (rowptr), y
    lsr a
    lsr a
    lsr a
    lsr a
    sta tone, x
    lda #0
    sta tonet, x
    iny

.next
    inx
    cpx #3
    bne loop

    lda tempo
    sta tickcount
    rts
}

; Updates the sound chip with the current data, after tone processing.

.update_all_channels
{
    ldx #0 ; channel
.loop
    ; Command byte and low four bits of pitch

    txa
    lsr a
    ror a
    ror a
    ror a
    sta w

    lda cpitch, x   ; get pitch byte
    cmp rpitch, x   ; chip already set for this pitch?
    beq do_volume   ; yes, skip write and go straight for volume
    sta rpitch, x   ; update current value
    tay
    lda w
    ora pitch_cmd_table_1, y
    jsr poke_sound_chip

    ; High six bits.

    lda pitch_cmd_table_2, y
    jsr poke_sound_chip

    ; Volume byte

.do_volume
    lda cvolume, x  ; get volume byte
    cmp rvolume, x  ; chip already set for this volume?
    beq next        ; nothing to do
    sta rvolume, x  ; update current value
    eor #&0f
    and #&0f
    ora w
    ora #&90
    jsr poke_sound_chip

.next
    inx
    cpx #3
    bne loop

    rts
}

; Writes the byte in A to the sound chip.
; Must be called with interrupts off.

.poke_sound_chip
{
    sei             ; interrupts off
    pha

    lda #&ff
    sta &fe43       ; VIA direction bits

    pla
    sta &fe41       ; write byte

    lda #0          ; sound chip write pin low
    sta $fe40
    nop             ; delay while the sound chip thinks
    nop
    nop
    nop
    nop
    nop
    lda #$08        ; sound chip write pin high
    sta $fe40

    cli             ; interrupts on
    rts
}

; Moves the cursor to the address at YYXX.
.move_cursor
{
    lda #14
    sta CRTC_ADDRESS
    tya
    sec
    sbc #&54
    sta CRTC_DATA
    lda #15
    sta CRTC_ADDRESS
    txa
    sta CRTC_DATA
    rts
}

; Note lookup table.
;
; This works in three parts.
;
; 1. Internal -> MIDI
;
; Internally we use numbers from 0 up, where each number is a third of a
; semitone and 0 is MIDI note 48, or C3. (This is the lowest note the sound
; chip can produce.) The highest note is MIDI note 115, or G8. This gives us a
; range of 67 MIDI notes, occupying internal pitches 0..201.
;
; Middle C (C4) is midi note 60, or 261.626Hz, which means it's (60-48)*2 = 24
; in our internal numbering.
;
; 2. MIDI -> frequency
;
; MIDI notes can be converted to a frequency with:
;
;   freq = 440 * 2^((midi-69)/12)
;
; 3. Frequency -> chip pitch
;
; The BBC's sound chip produces tones like this:
;
;                           Input clock (Hz)
;   Frequency (Hz) = -----------------------------
;                     2 x register value x divider
;
; For the BBC Micro, the clock is 4MHz and the divider is 16, so that
; gives us:
;
;   freq = 4M / (32 * n)
;
; ...so we can calculate n with:
;
;   n = 4M / (32 * freq)
;
; See https://www.smspower.org/Development/SN76489#ToneChannels
;
; 4. Chip pitch -> command
;
; To actually set the pitch, we need to send two bytes to the chip; the first
; contains the bottom four bits, the second the top ten bits. Actually doing
; this bit shuffling at runtime is a waste of time so we precompute it.

.pitch_cmd_table_1 org P% + NUM_PITCHES
.pitch_cmd_table_2 org P% + NUM_PITCHES
{
    for i, 0, NUM_PITCHES-1
        midi = i/3 + 48
        freq = 440 * 2^((midi-69)/12)
        pitch10 = 4000000 / (32 * freq)
        command1 = (pitch10 and &0f) or &80
        command2 = pitch10 >> 4
        org pitch_cmd_table_1 + i
        equb command1
        org pitch_cmd_table_2 + i
        equb command2
    next
}

; For every note, this table contains a BCD-encoded representation of octave
; (top nibble) and note (bottom nibble), where the nibble 0 is a C. If the
; note is not representable (i.e. not a whole note), the result is &ff.
.note_decode_table
{
    for i, 0, NUM_PITCHES-1
        if (i mod 3) <> 0
            equb &ff
        else
            semis = i div 3
            equb ((semis div 12)<<4) or (semis mod 12)
        endif
    next
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
    equb lo(MIDDLE_ROW_ADDRESS) + 4
    equb lo(MIDDLE_ROW_ADDRESS) + 6
    equb lo(MIDDLE_ROW_ADDRESS) + 8
    equb lo(MIDDLE_ROW_ADDRESS) + 9
    equb lo(MIDDLE_ROW_ADDRESS) + 12
    equb lo(MIDDLE_ROW_ADDRESS) + 14
    equb lo(MIDDLE_ROW_ADDRESS) + 16
    equb lo(MIDDLE_ROW_ADDRESS) + 17
    equb lo(MIDDLE_ROW_ADDRESS) + 20
    equb lo(MIDDLE_ROW_ADDRESS) + 22
    equb lo(MIDDLE_ROW_ADDRESS) + 24
    equb lo(MIDDLE_ROW_ADDRESS) + 25
    equb lo(MIDDLE_ROW_ADDRESS) + 28
    equb lo(MIDDLE_ROW_ADDRESS) + 30
    equb lo(MIDDLE_ROW_ADDRESS) + 32
    equb lo(MIDDLE_ROW_ADDRESS) + 33
    equb lo(MIDDLE_ROW_ADDRESS) + 36
    equb lo(MIDDLE_ROW_ADDRESS) + 38
    equb lo(MIDDLE_ROW_ADDRESS) + 40
    equb lo(MIDDLE_ROW_ADDRESS) + 41

; --- End of main program ---------------------------------------------------

; Everything from _top onward is only used for initialisation and is then
; overwritten with data.

._top
    ; Initialise the screen.

    {
        ldy #0
        ldx #init_vdu_end - init_vdu
    .loop
        lda init_vdu, y
        jsr OSWRCH
        iny
        dex
        bne loop
    }

    ; Wipe zero page.

    {
        ldx #&a0
        lda #0
    .loop
        sta &ff, x
        dex
        bne loop
    }

    ; Various bits of system configuration.

    lda #4          ; cursor keys
    ldx #1          ; ...make characters
    jsr OSBYTE

    lda #229        ; ESCAPE key
    ldx #1          ; ...makes character
    ldy #0
    jsr OSBYTE

    lda #16         ; ADC channels
    ldx #0          ; ...disabled
    jsr OSBYTE

    lda #10
    sta CRTC_ADDRESS
    lda #64
    sta CRTC_DATA

    ; Load the data file.

    lda #&ff
    ldx #lo(load_cb)
    ldy #hi(load_cb)
    jsr OSFILE

    ; Launch the program proper.

    jmp _start

.init_vdu
    equb 22, 7 ; mode 7
.init_vdu_end

.filename
    equs "data", 13
.load_cb
    equw filename
    equw MUSIC_DATA
    equw 0
    equw 0
    equw 0

._end

print "top=", ~_top, " data=", ~PATTERN_DATA
save "!boot", _start, _end, _top

clear MUSIC_DATA, &7c00
org MUSIC_DATA
    ; Header
.tempo  equb 2

    ; Pattern 0

org PATTERN_DATA
    equb 24, &0f, 27, &0f, 30, &0f, 33, &0f
    equb 00, &0f, 00, &0f, 00, &0f, 00, &0f
    equb &ff, 0,  &ff, 0,  &ff, 0,  &ff, 0

save "data", MUSIC_DATA, &7c00

; vim: ts=4 sw=4 et


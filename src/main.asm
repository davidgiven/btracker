
OSRDCH  = &FFE0
OSWRCH  = &FFEE
OSBYTE  = &FFF4
OSFILE  = &FFDD
SHEILA  = &FE00

IRQ1V   = &0204

VIA_T1CL    = 4
VIA_T1CH    = 5
VIA_T1LL    = 6
VIA_T1LH    = 7
VIA_ACR     = 11
VIA_PCR     = 12
VIA_IFR     = 13
VIA_IER     = 14

CRTC_ADDRESS = SHEILA + &00 + 0
CRTC_DATA    = SHEILA + &00 + 1

SYSTEM_VIA  = &40
USER_VIA    = &60

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

org &00
.pitch        equb 0, 0, 0    ; master copy for note
.volume       equb 0, 0, 0
.w            equb 0
.q            equb 0
.p            equb 0
.rowptr       equw 0          ; pointer to current row
.patternno    equb 0          ; current pattern number
.rowno        equb 0          ; current row number
.disptr       equw 0          ; pointer to row being displayed
.disrow       equb 0          ; row number being displayed
.scrptr       equw 0          ; screen pointer
.cursorx      equb 0          ; position of cursor (0-15)

; Used by the interrupt-driven player

.cpitch        equb 0, 0, 0    ; current copy (based on tone procedure)
.cvolume       equb 0, 0, 0
.rpitch        equb 0, 0, 0    ; what the chip is currently playing
.rvolume       equb 0, 0, 0
.tone          equb 0, 0, 0    ; current tone
.tonet         equb 0, 0, 0    ; tone tick count
.oldirqvector  equw 0          ; previous vector in chain
.tickcount     equb 0          ; ticks left in the current note

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

    lda #19
    jsr OSBYTE

.playloop
    lda #KBD_IRQ
    bit SHEILA+SYSTEM_VIA+VIA_IFR
    bne keypress

    lda tickcount
    bpl playloop

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
    jsr do_patterneditor_keypress
    jmp current_note_has_changed
}

include "src/patterned.inc"
include "src/screenutils.inc"
include "src/player.inc"

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
    sei             ; atomic wrt the interrupt-driven player

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
    cli
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

; Calls the subroutine whose pointer is in p.
.jsr_p
    jmp (p)

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
include "src/init.inc";
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


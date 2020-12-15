
putbasic "src/modtest.bas", "modtest"
putbasic "src/sattest.bas", "sattest"

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

; Notes

NUM_PITCHES = 201
FIRST_COMMAND = 256-26

; Screen layout.

; The middle row musn't span a page boundary.
MIDDLE_ROW = 16
MIDDLE_ROW_ADDRESS = &7c00 + (MIDDLE_ROW*40)

org &00
guard &9f

.pitch        equb 0, 0, 0, 0  ; master copy for note
.volume       equb 0, 0, 0, 0
.w            equw 0
.q            equb 0
.p            equb 0

; Used by the interrupt-driven player

.cpitch                 equb 0, 0, 0, 0 ; current copy (based on tone procedure)
.cvolume                equb 0, 0, 0, 0
.rpitch                 equb 0, 0, 0, 0 ; what the chip is currently playing
.rvolume                equb 0, 0, 0, 0
.sampletimer            equb 0, 0, 0, 0 ; time of next sample for voice
.samplecount            equb 0, 0, 0, 0 ; index of current sample for voice
.tone                   equb 0, 0, 0, 0 ; current tone
.oldirqvector           equw 0          ; previous vector in chain
.tickcount              equb 0          ; ticks left in the current note
.ticks                  equb 0          ; global clock
.iw                     equw 0          ; interrupt workspace

; Pattern editor variables

.rowptr                 equw 0    ; pointer to current row
.patternno              equb 0    ; current pattern number
.rowno                  equb 0    ; current row number
.disptr                 equw 0    ; pointer to row being displayed
.disrow                 equb 0    ; row number being displayed
.scrptr                 equw 0    ; screen pointer
.cursorx                equb 0    ; position of cursor (0-15)
.playing                equb 0    ; are we playing or not?

; Tone editor variables

.edittone               equb 0    ; the tone we're currently editing
.editsampledata         equw 0    ; pointer to the current tone's sample data
.editcursorx            equb 0    ; current cursor position in a graph
.editcursory            equb 0    ; what thing is being edited
.editnote               equb 0    ; the current test note
.editchannel            equb 0    ; what channel we're playing the test note on

print "Zero page usage:", ~P%, "out of 9f"

mapchar '#', 95             ; mode 7 character set

macro bge label
    bcs label
endmacro

macro blt label
    bcc label
endmacro

macro incw label
    inc label+0
    bne t
    inc label+1
.t
endmacro

org &1b00
guard PATTERN_DATA

; --- Main program ----------------------------------------------------------

._start
    jmp _top
.main
    lda #0
    sta rowno
    jsr reset_row_pointer
    jmp pattern_editor

include "src/utils.inc"
include "src/patterned.inc"
include "src/toneed.inc"
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
    lda rowno
    asl a           ; multiply by eight, = ROW_LENGTH
    asl a
    asl a
    sta rowptr+0
    rts
}

; Starts playing the notes at rowptr.

{
.off_command
    ; Off: turn off this channel.
    lda #0
    sta volume, x
    jmp next

.*play_current_note
    ldx #3              ; channel
    sei                 ; atomic wrt the interrupt-driven player

.loop
    txa
    asl a
    tay                 ; y is offset to note
    lda (rowptr), y
    iny
    cmp #NUM_PITCHES
    blt is_note
    cmp #FIRST_COMMAND + ('O' - 'A')
    beq off_command
.done
    jmp next

.is_note
    sta pitch, x        ; set master pitch

    lda (rowptr), y
    tay
    and #&0f
    sta volume, x       ; set master volume

    lda #0
    sta samplecount, x  ; sample count starts at zero
    lda ticks
    sta sampletimer, x  ; first event happens immediately

    tya
    lsr a
    lsr a
    lsr a
    lsr a
    sta tone, x         ; compute tone

.next
    dex
    bmi exit
    jmp loop
.exit

    lda tempo
    sta tickcount
    cli
    rts
}

; Calls the subroutine whose pointer is in p.
.jsr_w
    jmp (w)

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

; --- End of main program ---------------------------------------------------

; Everything from _top onward is only used for initialisation and is then
; overwritten with data.

._top
include "src/init.inc"
._end

print "top=", ~_top, " data=", ~PATTERN_DATA
save "btrack", _start, _end, _top

include "src/testfile.inc"

; vim: ts=4 sw=4 et


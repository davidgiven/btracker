
putbasic "src/modtest.bas", "modtest"
putbasic "src/sattest.bas", "sattest"

OSCLI   = &FFF7
OSWORD  = &FFF1
OSRDCH  = &FFE0
OSASCI  = &FFE3
OSNEWL  = &FFE7
OSWRCH  = &FFEE
OSBYTE  = &FFF4
OSFILE  = &FFDD
SHEILA  = &FE00

BRKV    = &0202
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

TIMER_PERIOD = 5 ; ms
TIMER_TICKS = 1e6 * (TIMER_PERIOD * 1e-3)

BUFFER      = &400 ; general purpose 256-byte buffer
BUFFER1     = &500 ; and another one

; One pattern is 256 bytes.
SEQUENCE_LENGTH = 128
NUM_STEPS = 32
NOTE_LENGTH = 2 ; pitch, volume/tone; or: command, param
MAX_PATTERNS = 68
NUM_VOICES = 4
TONE_SAMPLES = 64

ROW_LENGTH = NOTE_LENGTH * NUM_VOICES
PATTERN_DATA = &7c00 - (MAX_PATTERNS * NUM_STEPS * ROW_LENGTH)
TONE_DATA = PATTERN_DATA - (TONE_SAMPLES*2*16)
MUSIC_DATA = TONE_DATA - &100

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
.ip                     equb 0
.iq                     equb 0

; Pattern editor variables

.rowptr                 equw 0    ; pointer to current row
.patternno              equb 0    ; current pattern number
.seqindex               equb 0    ; current index into the sequence
.rowno                  equb 0    ; current row number
.disptr                 equw 0    ; pointer to row being displayed
.disrow                 equb 0    ; row number being displayed
.scrptr                 equw 0    ; screen pointer
.cursorx                equb 0    ; position of cursor (0-15)
.playing                equb 0    ; are we playing or not?
.looping                equb 0    ; are we looping the current pattern or not?
.mute                   equb 0, 0, 0, 0 ; whether this channel is muted

; Tone editor variables

.edittone               equb 0    ; the tone we're currently editing
.editsampledata         equw 0    ; pointer to the current tone's sample data
.editcursorx            equb 0    ; current cursor position in a graph
.editcursory            equb 0    ; what thing is being edited
.editnote               equb 0    ; the current test note
.editchannel            equb 0    ; what channel we're playing the test note on

; File editor (and misc) variables

.textmode               equb 0    ; set if we're in 'text mode'

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

org &1300
guard MUSIC_DATA

; --- Main program ----------------------------------------------------------

._start
    ; Initialise the screen.

    lda #22
    jsr OSWRCH
    lda #7
    jsr OSWRCH

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

    lda #16         ; ADC channels
    ldx #0          ; ...disabled
    jsr OSBYTE

    ; Install the player interrupt handler (which will start playing immediately).

    sei
    lda IRQ1V+0
    sta oldirqvector+0
    lda IRQ1V+1
    sta oldirqvector+1
    lda #lo(player_irq_cb)
    sta IRQ1V+0
    lda #hi(player_irq_cb)
    sta IRQ1V+1
    cli

    ; Install the brk handler.

    lda #lo(brk_cb)
    sta BRKV+0
    lda #hi(brk_cb)
    sta BRKV+1

    ; The playback interrupt runs at 1kHz, based on timer 1 of the user VIA.
    ; This is decremented at 1MHz (but takes two cycles to load). An interrupt
    ; is generated when it reaches zero.

    lda #lo(TIMER_TICKS - 2)
    sta SHEILA+USER_VIA+VIA_T1LL
    lda #hi(TIMER_TICKS - 2)
    sta SHEILA+USER_VIA+VIA_T1LH ; program reset latches
    lda #&c0
    sta SHEILA+USER_VIA+VIA_IER  ; enable TIMER1 interrupts
    lda #&40
    sta SHEILA+USER_VIA+VIA_ACR  ; continuous TIMER1 interrupts
    lda #hi(TIMER_TICKS - 2)
    sta SHEILA+USER_VIA+VIA_T1CH ; start timer

    ; Initialisation and go start the UI.

    jsr set_raw_keyboard
    jsr clear_all_data
    jmp file_editor

include "src/utils.inc"
include "src/patterned.inc"
include "src/toneed.inc"
include "src/fileed.inc"
include "src/screenutils.inc"
include "src/player.inc"

; --- System handling -------------------------------------------------------

; Set the keyboard to 'raw' mode --- function keys and ESCAPE send characters.

.set_raw_keyboard
    lda #229        ; ESCAPE key
    ldx #1          ; ...makes character
    ldy #0
    jsr OSBYTE

    lda #4          ; cursor keys
    ldx #1          ; ...make characters
    jsr OSBYTE

    lda #225        ; function keys
    ldx #160        ; ...make character codes
    jmp OSBYTE

.set_cooked_keyboard
    lda #229        ; ESCAPE key
    ldx #0          ; ...is special
    ldy #0
    jsr OSBYTE

    lda #4          ; cursor keys
    ldx #0          ; ...are cursor keys
    jsr OSBYTE

    lda #225        ; function keys
    ldx #0          ; ...are function keys
    jmp OSBYTE

.star_command
{
    jsr enter_text_mode
.loop
    lda #'*'
    jsr OSWRCH
    ldx #lo(oswordblock)
    ldy #hi(oswordblock)
    lda #0
    jsr OSWORD
    bcs escape_exit
    ldx #lo(BUFFER)
    ldy #hi(BUFFER)
    jsr OSCLI
    lda #&7e        ; this is all wrong
    jsr OSBYTE
    jmp loop

.escape_exit
    lda #&7e        ; acknowledge ESCAPE
    jsr OSBYTE
.ret
    lda #22         ; the screen may have scrolled, so reset it
    jsr OSWRCH
    lda #7
    jsr OSWRCH
    jsr set_raw_keyboard
    lda #0
    sta textmode
    jmp file_editor
}

.enter_text_mode
{
    bit textmode
    bne ret
    dec textmode

    ldy #0
.ploop
    lda banner, y
    jsr OSASCI
    iny
    cpy #banner_end - banner
    bne ploop

    jsr set_cooked_keyboard

.ret
    rts

.banner
    equb 22, 7
    equs "Press ESCAPE to return to the menu.", 13, 13
.banner_end
}

.brk_cb
{
    jsr enter_text_mode

    ldy #1
.loop
    lda (&fd), y
    beq exit
    jsr OSWRCH
    iny
    jmp loop
.exit
    jsr OSNEWL
    jmp star_command
}

.oswordblock
    equw BUFFER
    equb 255
    equb 32
    equb 255

; --- Playback --------------------------------------------------------------

; Compute rowptr based on patternno and rowno.

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
    jsr reset_row_pointer
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
    lda mute, x
    beq not_mute
    lda #0
    sta volume, x
.not_mute

    dex
    bmi exit
    jmp loop
.exit

    lda tempo
    sta tickcount
    cli
    rts
}

; Stops playing ASAP.

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

; And now, the *other* table: the BBC's sound chip uses a 15-bit divider for
; the noise channels, so if you're using any of the c2 tones, you need
; different pitch values in channel 2 for the noise to be in tune.

.drum_pitch_cmd_table_1 org P% + NUM_PITCHES
.drum_pitch_cmd_table_2 org P% + NUM_PITCHES
{
    for i, 0, NUM_PITCHES-1
        midi = i/3 + 48
        freq = 440 * 2^((midi-69)/12)
        pitch10 = 4000000 / (30 * freq)
        command1 = (pitch10 and &0f) or &80
        command2 = pitch10 >> 4
        org drum_pitch_cmd_table_1 + i
        equb command1
        org drum_pitch_cmd_table_2 + i
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

._top

print "top=", ~_top, " data=", ~MUSIC_DATA
save "btrack", _start, _top

include "src/testfile.inc"

; vim: ts=4 sw=4 et


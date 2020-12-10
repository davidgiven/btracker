
OSRDCH	= &FFE0
OSWRCH	= &FFEE
OSBYTE	= &FFF4
OSFILE  = &FFDD
SHEILA  = &FE00

IFR		= 13
IER		= 14

CRTC_ADDRESS = SHEILA + &00 + 0
CRTC_DATA    = SHEILA + &00 + 1

SYSTEM_VIA 	= &40
KBD_IRQ    	= 1<<0
VSYNC_IRQ	= 1<<1
ADC_IRQ		= 1<<4
CLOCK_IRQ	= 1<<6

NUM_PATTERNS = 64
NUM_VOICES = 4

; One pattern is 256 bytes.
NUM_STEPS = 32
NOTE_LENGTH = 2 ; pitch, volume/tone; or: command, param

ROW_LENGTH = NOTE_LENGTH * NUM_VOICES
PATTERN_DATA = &7c00 - (NUM_PATTERNS * NUM_STEPS * ROW_LENGTH)

; Notes

NUM_PITCHES = 201

; Screen layout.

MIDDLE_ROW = 16
MIDDLE_ROW_ADDRESS = &7c00 + (MIDDLE_ROW*40) + 1

;  0123456789012345678901234567890123456789
;     00 : F# 8f | F# 8f | F# 8f | XX 8f
org &00
.pitch      equb 0, 0, 0
.volume		equb 0, 0, 0
.w	   		equb 0
.q			equb 0
.p			equb 0
.rowptr     equw 0			; pointer to current row
.patternno  equb 0			; current pattern number
.rowno		equb 0			; current row number
.disptr		equw 0			; pointer to row being displayed
.disrow     equb 0          ; row number being displayed
.scrptr		equw 0			; screen pointer
guard &9f

org &1b00
guard PATTERN_DATA

; --- Main program ----------------------------------------------------------

._start
	lda #&ff
	sta volume + 0
	sta volume + 1
	sta volume + 2
	jsr reset_row_pointer
	ldx #0
	ldy #&7c
	jsr gotoxy

	ldx #0
.main_loop
	stx pitch+0
	txa
	clc
	adc #1
	sta pitch+1
	txa
	clc
	adc #4
	sta pitch+2
	txa
	pha
	jsr update_all_channels
	jsr draw_screen

	lda #KBD_IRQ
	bit SHEILA+SYSTEM_VIA+IFR
	bne key_pressed

	lda #19
	jsr OSBYTE

	pla
	tax
	inx
	cpx #NUM_PITCHES
	bne main_loop
	ldx #0
	jmp main_loop

.key_pressed
	lda #&81
	ldx #0
	ldy #0
	jsr OSBYTE
	bcs main_loop

	cpx #139
	beq key_up
	cpx #138
	beq key_down

	jmp main_loop

.key_up
{
	lda rowno
	beq main_loop

	dec rowno

	sec
	lda rowptr+0
	sbc #ROW_LENGTH
	sta rowptr+0
	jmp main_loop
}

.key_down
{
	lda rowno
	cmp #NUM_STEPS-1
	beq main_loop

	inc rowno

	clc
	lda rowptr+0
	adc #ROW_LENGTH
	sta rowptr+0
	jmp main_loop
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
	lda rowptr+0
	sta disptr+0
	lda rowptr+1
	sta disptr+1

	lda rowno
	sta disrow

	lda #9
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
	jsr print_h8

	lda #0
	jsr print_h4

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

; Prints the hex nibble A at scrptr, advancing y
.print_h4
{
	and #&0f
	jsr nibble_to_ascii
	ldy #0
	sta (scrptr), y

	inc scrptr+0
	bne noinc
	inc scrptr+1
.noinc
	rts
}

; Prints A at scrptr, advancing y
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

; Copy the current note at rowptr to zero page.

.play_current_note
{
	
}

; Updates the sound chip with the data in zero page.

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
	ldy pitch, x	; get pitch byte
	ora pitch_cmd_table_1, y
	jsr poke_sound_chip

	; High six bits.

	lda pitch_cmd_table_2, y
	jsr poke_sound_chip

	; Volume byte

	sec
	lda #&ff
	sbc volume, x
	lsr a
	lsr a
	lsr a
	lsr a
	ora w
	ora #&90
	jsr poke_sound_chip

	inx
	cpx #3
	bne loop

	rts
}

; Writes the byte in A to the sound chip.
; Must be called with interrupts off.

.poke_sound_chip
{
	sei				; interrupts off
	pha

	lda #&ff
	sta &fe43		; VIA direction bits

	pla
	sta &fe41		; write byte

	lda #0			; sound chip write pin low
	sta $fe40
	nop				; delay while the sound chip thinks
	nop
	nop
	nop
	nop
	nop
	lda #$08		; sound chip write pin high
	sta $fe40

	cli				; interrupts on
	rts
}

; Moves the cursor to X, Y.
.gotoxy
{
	lda #14
	sta CRTC_ADDRESS
	txa
	sta CRTC_DATA
	lda #15
	sta CRTC_ADDRESS
	tya
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

.note_name_table
	equs "C "
	equs "C#"
	equs "D "
	equs "D#"
	equs "E "
	equs "F "
	equs "F#"
	equs "G "
	equs "G#"
	equs "A "
	equs "A#"
	equs "B "

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

	lda #4 			; cursor keys
	ldx #1			; ...make characters
	jsr OSBYTE

	lda #229		; ESCAPE key
	ldx #1			; ...makes character
	ldy #0
	jsr OSBYTE

	lda #16			; ADC channels
	ldx #0			; ...disabled
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
	equw PATTERN_DATA
	equw 0
	equw 0
	equw 0

._end

print "top=", ~_top, " data=", ~PATTERN_DATA
save "!boot", _start, _end, _top

clear PATTERN_DATA, &7c00
org PATTERN_DATA

	; Pattern 0

	equb 24, &0f, 25, &0f, 26, &0f, 27, &0f

save "data", PATTERN_DATA, &7c00


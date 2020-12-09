
OSRDCH	= &FFE0
OSWRCH	= &FFEE
OSBYTE	= &FFF4
SHEILA  = &FE00

IFR		= 13
IER		= 14

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

; Screen layout.

MIDDLE_ROW = 10
MIDDLE_ROW_ADDRESS = &7c00 + (MIDDLE_ROW*40)

;  0123456789012345678901234567890123456789
;     00 : F# 8f | F# 8f | F# 8f | XX 8f
org &00
.pitchlo    equb 0, 0, 0
.pitchhi	equb 0, 0, 0
.volume		equb 0, 0, 0
.w	   		equb 0
.q			equb 0
.rowptr     equw 0			; pointer to current row
.patternno  equb 0			; current pattern number
.rowno		equb 0			; current row number
.disptr		equw 0			; pointer to row being displayed
.disrow     equb 0          ; row number being displayed
.scrptr		equw 0			; screen pointer
guard &9f

org &1b00
guard PATTERN_DATA
._start
	lda #&80
	sta pitchhi + 0
	lda #&ff
	sta volume + 0
	jsr reset_row_pointer

.main_loop
	inc pitchhi + 0
	jsr update_all_channels
	jsr draw_screen

	lda #KBD_IRQ
	bit SHEILA+SYSTEM_VIA+IFR
	bne key_pressed

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
	lda #3
	jsr advance_scrptr
	jsr draw_note
	lda #3
	jsr advance_scrptr
	jsr draw_note
	lda #3
	jsr advance_scrptr
	jsr draw_note
	lda #3
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

; Prints A at scrptr, advancing y
.print_h8
{
	pha
	lsr a
	lsr a
	lsr a
	lsr a
	jsr nibble_to_ascii
	ldy #0
	sta (scrptr), y

	pla
	and #&0f
	jsr nibble_to_ascii
	ldy #1
	sta (scrptr), y

	lda #2
	; fall through
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

; Updates the sound chip with the data in zero page.

.update_all_channels
{
	ldy #0 ; channel
.loop
	; Command byte and low four bits of pitch

	tya
	asl a
	asl a
	asl a
	asl a
	asl a
	ora #&80		; command byte for tone
	sta w

	lda pitchhi, y
	and #&03
	clc
	rol a
	rol a
	rol a
	sta q
	lda pitchlo, y
	and #&c0
	lsr a
	lsr a
	ora q
	ora w
	jsr poke_sound_chip

	; High four bits of pitch

	lda pitchhi, y
	lsr a
	lsr a
	jsr poke_sound_chip

	; Volume byte

	sec
	lda #&ff
	sbc volume, y
	lsr a
	lsr a
	lsr a
	lsr a
	ora w
	ora #&10
	jsr poke_sound_chip

	iny
	cpy #3
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

	; Launch the program proper.

	jmp _start

.init_vdu
	equb 22, 7
	equb 23, 1, 0, 0, 0, 0, 0, 0, 0, 0
.init_vdu_end

._end

print "top=", ~_top, " data=", ~PATTERN_DATA
save "!boot", _start, _end, _top



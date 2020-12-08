
OSWRCH	= &FFEE
OSBYTE	= &FFF4

NUM_PATTERNS = 32

; One pattern is 256 bytes.
NUM_STEPS = 64
NOTE_LENGTH = 4 ; tone, volume/voice, command, parameter

PATTERN_DATA = &7c00 - (NUM_PATTERNS * NUM_STEPS * NOTE_LENGTH)

org &00
.pitchlo    equb 0, 0, 0
.pitchhi	equb 0, 0, 0
.volume		equb 0, 0, 0
.w	   		equb 0
.q			equb 0
guard &9f

org &1b00
guard PATTERN_DATA
._start

	lda #&80
	sta pitchhi + 0
	lda #&ff
	sta volume + 0
	lda #0
	sta volume + 1
	sta volume + 2

.loop
	lda #19
	jsr OSBYTE
	inc pitchhi + 0
	jsr update_all_channels
	jmp loop

	rts

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

.poke_sound_chip
{
	pha
	sei				; disable interrupts

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
	
	cli				; reenable interrupts again
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



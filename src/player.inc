; This is the interrupt-driven player itself. It runs on every 100Hz timer tick.

.player_irq_cb
{
    cld
    lda &fc
    pha

    lda #&40 ; TIMER1
    bit SHEILA+USER_VIA+VIA_IFR
    beq irq_exit
    lda #lo(TIMER_TICKS - 2)
    sta SHEILA+USER_VIA+VIA_T1CL ; clear interrupt
    lda #hi(TIMER_TICKS - 2)
    sta SHEILA+USER_VIA+VIA_T1CH ; clear interrupt

	txa
	pha
	tya
	pha

    jsr player_routine

	pla
	tay
	pla
	tax

.irq_exit
    pla
	jmp (oldirqvector)
}

; Actual player routine, called from the interrupt handler.

.player_routine
{
    ; Updates the sound chip with the current data, after tone processing.

    {
        ldx #3 ; channel
    .loop
        ; Calculates the current tone.

        jsr tone_generator

        ; Pitch byte(s)

        txa
        lsr a
        ror a
        ror a
        ror a
        sta iw           ; channel part of command byte

        lda cpitch, x    ; get pitch byte
        cmp rpitch, x    ; chip already set for this pitch?
        beq do_volume    ; yes, skip write and go straight for volume
        sta rpitch, x    ; update current value
        cpx #3           ; drum track is special
        beq drum

        tay
        lda iw
        ora pitch_cmd_table_1, y 
        jsr poke_sound_chip
        lda pitch_cmd_table_2, y
    .poke
        jsr poke_sound_chip

        ; Volume byte

    .do_volume
        lda cvolume, x    ; get volume byte
        cmp rvolume, x    ; chip already set for this volume?
        beq next          ; nothing to do
        sta rvolume, x    ; update current value
        eor #&0f
        and #&0f
        ora iw
        ora #&90
        jsr poke_sound_chip

    .next
        dex
        bpl loop
        jmp exit

    .drum
        and #&0f
        ora #&e0
        jmp poke
    .exit
    }

    ; Adjust timers.

    dec tickcount
    inc ticks
    rts
}

; Writes the byte in A to the sound chip.
; Must be called with interrupts off.

.poke_sound_chip
{
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

    rts
}

; After an addition, this adjusts the value in A to take into account the note
; modulus.

macro modulus_adjust
    bcs overflow
    cmp #NUM_PITCHES
    bcc exit
.overflow
    sbc #NUM_PITCHES            ; carry guaranteed set
.exit:
endmacro

; After an addition, this forces the value to be saturated to one of the two values.
macro saturation_adjust lo, hi
    bmi negative
    cmp #hi+1
    blt exit
    lda #hi
    jmp exit

.negative
    cmp #lo
    bge exit
    lda #lo
.exit
endmacro

; Called with the channel in x.
.tone_generator
{
    lda sampletimer, x
    cmp ticks
    bne ret

    ; Compute the time of the next event.

    lda tone, x
    tay             ; y contains tone number
    clc     
    lda ticks
    adc samplespeed, y
    sta sampletimer, x

    ; Fetch the pointer to the sample data.

    tya
    asl a
    rol a
    sta iw+0        ; low byte of sample data address

    tya
    asl a
    clc
    adc #hi(TONE_DATA)
    sta iw+1        ; high byte of sample data address

    ; Compute pitch.

    {
        ldy samplecount, x
        clc
        lda (iw), y
        bpl positive
        adc #NUM_PITCHES
        clc
    .positive
        adc pitch, x
        modulus_adjust
        sta cpitch, x
    }

    ; Compute volume.

    {
        clc
        tya
        adc #64
        tay
        clc
        lda volume, x
        adc (iw), y
        bpl positive
        lda #0
    .positive
        cmp #&10
        blt in_range
        lda #&f
    .in_range
        sta cvolume, x
    }

    ; Increment sample count.

    ldy tone, x
    lda samplecount, x
    clc
    adc #1
    cmp repeatend, y
    bne no_repeat
    lda repeatstart, y
.no_repeat
    and #TONE_SAMPLES-1
    sta samplecount, x
.ret
    rts
}

; vim: ts=4 sw=4 et ft=asm

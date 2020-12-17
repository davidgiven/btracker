; This is the interrupt-driven player itself. It runs on every 100Hz timer tick.

.player_irq_cb
{
    cld

    lda #&40 ; TIMER1
    bit SHEILA+USER_VIA+VIA_IFR
    bne user_via_interrupt

    lda &fc
    jmp (oldirqvector)

    ; This is one of our interrupts.

.user_via_interrupt
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
    lda &fc
    rti
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
        bne do_pitch     ; no, write it
        lda cvolume, x
        cmp rvolume, x
        beq next         ; volumes are the same too, do nothing
        clc
        lda cvolume, x
        beq do_pitch     ; volume changing to zero, update pitch
        lda rvolume, x
        beq do_pitch     ; volume changing from zero, update pitch
        jmp do_volume    ; otherwise, update the volume but not the pitch

    .do_pitch
        lda cpitch, x
        sta rpitch, x    ; update current value
        cpx #3           ; drum track is special
        beq drum

        tay
        lda cvolume, x   ; check to see if this note is silent
        beq alternate_tuning ; if so, use the other set of commands

        lda iw
        ora pitch_cmd_table_1, y 
        jsr poke_sound_chip
        lda pitch_cmd_table_2, y
        jmp poke

    .alternate_tuning
        lda iw
        ora drum_pitch_cmd_table_1, y
        jsr poke_sound_chip
        lda drum_pitch_cmd_table_2, y
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
    lsr a
    clc
    adc #hi(TONE_DATA)
    sta iw+1        ; high byte of sample data address

    ; Compute pitch.

    {
        ldy samplecount, x
        clc
        lda (iw), y

        ; Multiply by the pitch scale.

        cmp #&80
        ror a       ; prime carry bit for loop; arithmetic shift
        sta ip
        ldy #8
    .mulloop
        bcc noadd
        clc
        adc pitchscale, x
    .noadd
        ror a
        ror ip
        dey
        bne mulloop

        ; Modules adjust pitch.

        lda ip
        clc
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
        lda samplecount, x
        clc
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

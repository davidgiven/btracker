; This is the interrupt-driven player itself. It runs on every 100Hz timer tick.

.player_irq_cb
{
    cld
    lda &fc
    pha

    lda #CLOCK_IRQ
    bit SHEILA+SYSTEM_VIA+VIA_IFR
    beq irq_exit

	txa
	pha
	tya
	pha

    ; Zero user VIA.

    lda #&ff
    sta SHEILA+USER_VIA+VIA_T1CH ; low byte copied from latch automatically
    jsr player_routine

    sec
    lda #0
    sbc SHEILA+USER_VIA+VIA_T1CH
    tay

    lsr a
    lsr a
    lsr a
    lsr a
    tax
    lda hex_table, x
    sta &7c00
    tya
    and #&0f
    tax
    lda hex_table, x
    sta &7c01

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
    print ~P%

    ; Adjust timers.

    dec tickcount
    inc ticks

    ; Run the tone handlers.

    {
        ldy #3
    .loop
        jsr tone_generator

        dey
        bpl loop
    }

    ; Updates the sound chip with the current data, after tone processing.

    {
        ldx #3 ; channel
    .loop
        ; Pitch byte(s)

        txa
        lsr a
        ror a
        ror a
        ror a
        sta iw

        lda cpitch, x   ; get pitch byte
        cmp rpitch, x   ; chip already set for this pitch?
        beq do_volume   ; yes, skip write and go straight for volume
        sta rpitch, x   ; update current value
        cpx #3          ; drum track is special
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
        lda cvolume, x  ; get volume byte
        cmp rvolume, x  ; chip already set for this volume?
        beq next        ; nothing to do
        sta rvolume, x  ; update current value
        eor #&0f
        and #&0f
        ora iw
        ora #&90
        jsr poke_sound_chip

    .next
        dex
        bpl loop
        rts

    .drum
        and #&0f
        ora #&e0
        jmp poke
    }
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


; Called with the channel in y.
.tone_generator
{
    lda tone, y
    tax             ; x contains tone number

    lda #0          ; reset pitch and volume accumulators
    sta cpitch, y
    sta cvolume, y

    ; Pitch

    {
        lda ticks
        cmp tonen_pitch, y
        bne no_adjust

        clc
        lda ticks
        adc tone_pitch_period, x
        sta tonen_pitch, y

        ; Modulus add of tonew_pitch + tone_pitch_delta.

        {
            lda tone_pitch_delta, x
            clc
            bmi negative
            sec
            sbc #NUM_PITCHES
        .negative
            adc tonew_pitch, y
            bcs end
            adc #NUM_PITCHES
        .end
            sta tonew_pitch, y
        }
        
    .no_adjust
        clc
        lda cpitch, y
        adc tonew_pitch, y
        sta cpitch, y
    }

    ; Volume

    {
        lda ticks
        cmp tonen_volume, y
        bne adjust_value

        clc
        lda ticks
        adc tone_volume_period, x
        sta tonen_volume, y

        ; Signed saturated addition of unsigned tonew_volume + signed tone_volume_delta.

        clc
        lda tonew_volume, y
        adc tone_volume_delta, x
        bvc no_overflow         ; signed overflow

        lda tone_volume_delta, x
        bmi delta_negative
        lda #15
        jmp no_overflow
    .delta_negative
        lda #lo(-15)
    .no_overflow
        sta tonew_volume, y

    .adjust_value
        clc
        lda cvolume, y
        adc tonew_volume, y
        sta cvolume, y
    }

    ; Saturated add of volume + cvolume to 0..f.

    {
        clc
        lda volume, y
        eor #&80
        adc cvolume, y
        eor #&80
        bvc exit               ; forbid overflow

        lda cvolume, y         ; sign extend delta
        asl a                  ; get sign
        lda #0
        adc #&ff               ; negative -> 0, positive -> &ff
        and #&0f
    .exit
        sta cvolume, y
    }

    ; Modulus add of pitch + cpitch.

    {
        sec
        lda pitch, y
        sbc #NUM_PITCHES
        adc cpitch, y
        bcs end
        adc #NUM_PITCHES
    .end
        sta cpitch, y
    }

    rts

    ; Adds A to the value in cpitch, wrapping around if necessary.

    .modulus_add_to_cpitch
    {
        sec
        sbc #NUM_PITCHES
        adc cpitch, y
        bcs ret
        adc #NUM_PITCHES
    .ret
        sta cpitch, y
        rts
    }
}

; vim: ts=4 sw=4 et ft=asm

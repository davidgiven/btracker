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
    lda SHEILA+USER_VIA+VIA_T1CH
    sta &7c00

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

    ; Pitch

    {
        lda ticks
        cmp tonen_pitch, y
        bne no_adjust

        clc
        lda ticks
        adc tone_pitch_period, x
        sta tonen_pitch, y

        clc
        lda tone_pitch_delta, x
        adc tonew_pitch, y
        cmp #NUM_PITCHES
        blt no_pitch_wrap
        sec
        sbc #NUM_PITCHES
    .no_pitch_wrap
        sta tonew_pitch, y

    .no_adjust
        lda pitch, y
        clc
        adc tonew_pitch, y          ; allow overflow
        sta cpitch, y
    }

    ; Volume

    {
        lda ticks
        cmp tonen_volume, y
        bne no_adjust

        clc
        lda ticks
        adc tone_volume_period, x
        sta tonen_volume, y

        ; Saturated add of tonew_volume + tone_volume_delta to -f..+f.

        clc
        lda tonew_volume, y
        adc tone_volume_delta, x
        {
            bmi negative

            cmp #&10
            blt exit
            lda #&f
            jmp exit
        .negative
            cmp #&f0
            bge exit
            lda #&f0
        .exit
        }
        sta tonew_volume, y

    .no_adjust
        ; Saturated add of volume + tonew_volume to 0..f.

        {
            clc
            lda volume, y
            eor #&80
            adc tonew_volume, y
            eor #&80
            bvc exit               ; forbid overflow

            lda tonew_volume, y    ; sign extend delta
            asl a                  ; get sign
            lda #0
            adc #&ff               ; negative -> 0, positive -> &ff
            and #&0f
        .exit
        }
        sta cvolume, y
        clc
        adc #65
        sta &7c01, y
    }

    rts
}

; vim: ts=4 sw=4 et ft=asm

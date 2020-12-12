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
    ;lda SHEILA+USER_VIA+VIA_T1CL
    ;sta &7c00
    ;lda SHEILA+USER_VIA+VIA_T1CH
    ;sta &7c01

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
    ; The front end uses this to track when the current note has ended.

    dec tickcount

    ; Run the tone handlers.

    {
        ldx #0
    .loop
        txa
        pha
        jsr tone_generator
        pla
        tax
        inc tonet, x

        inx
        cpx #4
        bne loop
    }

    ; Updates the sound chip with the current data, after tone processing.

    {
        ldx #0 ; channel
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
        inx
        cpx #4
        bne loop
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


; Called with the channel in x.
.tone_generator
{
    lda pitch, x
    sta cpitch, x

    lda volume, x
    sta cvolume, x

    rts
}

; vim: ts=4 sw=4 et ft=asm

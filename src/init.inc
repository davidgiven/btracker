; Everything in here is only used for initialisation. Once this routine exits,
; it can be overwritten with data.

{
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

    ; Launch the program proper.

    jmp main
}

; vim: ts=4 sw=4 et ft=asm


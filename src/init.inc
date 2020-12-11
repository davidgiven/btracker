; Everything in here is only used for initialisation. Once this routine exits,
; it can be overwritten with data.

{
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

    lda #4          ; cursor keys
    ldx #1          ; ...make characters
    jsr OSBYTE

    lda #229        ; ESCAPE key
    ldx #1          ; ...makes character
    ldy #0
    jsr OSBYTE

    lda #16         ; ADC channels
    ldx #0          ; ...disabled
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

    ; We use timer 1 of the user VIA as a free-running timer, used to monitor
    ; the speed of the interrupt-driven player. It's zeroed at the beginning of
    ; an interrupt and read at the end.

    lda #&60                     ; disable timer interrupts
    sta SHEILA+USER_VIA+VIA_IER     
    lda #&ff                     ; reset to &ffff on rollover
    sta SHEILA+USER_VIA+VIA_T1LL
    sta SHEILA+USER_VIA+VIA_T1LH

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

    ; Launch the program proper.

    jmp _start

.init_vdu
    equb 22, 7 ; mode 7
.init_vdu_end

.filename
    equs "data", 13
.load_cb
    equw filename
    equw MUSIC_DATA
    equw 0
    equw 0
    equw 0
}

; vim: ts=4 sw=4 et ft=asm


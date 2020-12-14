hi = &F
lo = (-&F) AND 255
input=&70
delta=&71
FOR pass=0 TO 3 STEP 3
P% = &900
[opt pass
	clc
	lda input
	adc delta

    bmi negative
    cmp #hi+1
    bcc exit
    lda #hi
    jmp exit

.negative
	cmp #lo
	bcs exit
	lda #lo
.exit
	sta input
	rts
]:NEXT

FOR X%=-30 TO 30
  FOR Y%=-30 TO 30
    ?input = X%
	?delta = Y%
	CALL &900
	R% = X% + Y%
	IF R% < (-&F) THEN R% = -&F
	IF R% > hi THEN R% = hi
	IF (R% AND 255) <> ?input THEN PRINT; X%;" ";Y%;" ";R%;" "; ?input
  NEXT
NEXT



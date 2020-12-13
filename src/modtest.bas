mod=201
input=&70
delta=&71
FOR pass=0 TO 3 STEP 3
P% = &900
[opt pass
	lda input
	bit delta
	clc
    bmi negative
    sec
    sbc #mod
.negative
    adc delta
    bcs end
    adc #mod
.end
	sta input
	rts
]:NEXT

FOR X%=0 TO 200
  PRINT X%
  FOR Y%=-128 TO 127
    ?input = X%
	?delta = Y%
	CALL &900
	R% = (X% + Y%) MOD mod
	IF R% < 0 THEN R% = R% + mod
	IF R% <> ?input THEN PRINT; X%;" ";Y%;" "; R%;" "; ?input
  NEXT
NEXT


C_0  = 0*36+0
CS_0 = 0*36+3
D_0  = 0*36+6
DS_0 = 0*36+9
E_0  = 0*36+12
F_0  = 0*36+15
FS_0 = 0*36+18
G_0  = 0*36+21
GS_0 = 0*36+24
A_0  = 0*36+27
AS_0 = 0*36+30
B_0  = 0*36+33

C_1  = 1*36+0
CS_1 = 1*36+3
D_1  = 1*36+6
DS_1 = 1*36+9
E_1  = 1*36+12
F_1  = 1*36+15
FS_1 = 1*36+18
G_1  = 1*36+21
GS_1 = 1*36+24
A_1  = 1*36+27
AS_1 = 1*36+30
B_1  = 1*36+33

C_2  = 2*36+0
CS_2 = 2*36+3
D_2  = 2*36+6
DS_2 = 2*36+9
E_2  = 2*36+12
F_2  = 2*36+15
FS_2 = 2*36+18
G_2  = 2*36+21
GS_2 = 2*36+24
A_2  = 2*36+27
AS_2 = 2*36+30
B_2  = 2*36+33

C_3  = 3*36+0
CS_3 = 3*36+3
D_3  = 3*36+6
DS_3 = 3*36+9
E_3  = 3*36+12
F_3  = 3*36+15
FS_3 = 3*36+18
G_3  = 3*36+21
GS_3 = 3*36+24
A_3  = 3*36+27
AS_3 = 3*36+30
B_3  = 3*36+33

BLANK = FIRST_COMMAND + ('B' - 'A')
OFF = FIRST_COMMAND + ('O' - 'A')

clear MUSIC_DATA, &7c00
guard &7c00
org MUSIC_DATA
    ; Header
.sequence
    equb 0
    for i, 1, 127
        equb &ff
    next

.tempo          equb 12
.patternlength  equb &1f
.numpatterns    equb 0
.sequencelength equb 1
.samplespeed    equb 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
.pitchscale     equb 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
.repeatstart    equb 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.repeatend      equb 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0

; Tone information. All zeros.

org TONE_DATA

    for i, 0, 63
        equb 0
    next

; Stolen from https://www.youtube.com/watch?v=gUGlMhhasno

    ; Pattern 0

org PATTERN_DATA + 0*&100
    equb OFF,   &0f, OFF,   &0f, OFF, &0f, OFF, &0f
    equb BLANK, 0,   OFF,   &0e, BLANK, 0, BLANK, 0
    equb G_1,   &0f, G_0,   &0e, BLANK, 0, BLANK, 0
    equb G_1,   &0f, G_0,   &0e, BLANK, 0, BLANK, 0
    equb F_1,   &0f, BLANK, &0e, BLANK, 0, BLANK, 0
    equb BLANK, 0,   BLANK, &0e, BLANK, 0, BLANK, 0
    equb G_1,   &0f, G_0,   &0e, BLANK, 0, BLANK, 0
    equb OFF,   0,   G_0,   &0e, BLANK, 0, BLANK, 0
    equb AS_1,  &0f, AS_0,  &0e, BLANK, 0, BLANK, 0
    equb AS_1,  &0f, G_0,   &0e, BLANK, 0, BLANK, 0
    equb A_1,   &0f, C_1,   &0e, BLANK, 0, BLANK, 0
    equb OFF,   0,   G_0,   &0e, BLANK, 0, BLANK, 0
    equb G_1,   &0f, F_0,   &0e, BLANK, 0, BLANK, 0
    equb BLANK, 0,   G_0,   &0e, BLANK, 0, BLANK, 0
    equb F_1,   &0f, OFF,   &0e, BLANK, 0, BLANK, 0
    equb BLANK, 0,   G_0,   &0e, BLANK, 0, BLANK, 0
    equb D_1,   &0f, AS_0,  &0e, BLANK, 0, BLANK, 0
    equb BLANK, 0,   G_0,   &0e, BLANK, 0, BLANK, 0
    equb OFF,   0,   D_1,   &0e, BLANK, 0, BLANK, 0
    equb BLANK, 0,   G_0,   &0e, BLANK, 0, BLANK, 0
    equb D_1,   &0f, F_0,   &0e, BLANK, 0, BLANK, 0
    equb BLANK, 0,   G_0,   &0e, BLANK, 0, BLANK, 0
    equb OFF,   0,   OFF,   &0e, BLANK, 0, BLANK, 0
    equb BLANK, 0,   G_0,   &0e, BLANK, 0, BLANK, 0
    equb G_1,   &0f, F_0,   &0e, BLANK, 0, BLANK, 0
    equb D_1,   &0f, G_0,   &0e, BLANK, 0, BLANK, 0
    equb BLANK, 0,   G_0,   &0e, BLANK, 0, BLANK, 0
    equb E_1,   &0f, G_0,   &0e, BLANK, 0, BLANK, 0
    equb F_1,   &0f, BLANK, &0e, BLANK, 0, BLANK, 0
    equb F_1,   &0f, G_0,   &0e, BLANK, 0, BLANK, 0
    equb E_1,   &0f, AS_0,  &0e, BLANK, 0, BLANK, 0
    equb BLANK, 0,   G_0,   &0e, BLANK, 0, BLANK, 0

; vim: sw=4 ts=4 et ft=asm


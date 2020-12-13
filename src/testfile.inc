C_0	 = 0*36+0
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

C_1	 = 1*36+0
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

C_2	 = 2*36+0
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

C_3	 = 3*36+0
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
.tempo  equb 12

; Tone information.

.tone_pitch          equb 0,  0, 0, 0, 0, 0, 0, 0
.tone_pitch_delta    equb 1, 1, 1, 0, 0, 0, 0, 0
.tone_pitch_period   equb 1,  2, 3, 0, 0, 0, 0, 0
.tone_volume	     equb 0,  0, 0, 0, 0, 0, 0, 0
.tone_volume_delta   equb 0, 0, 0, 0, 0, 0, 0, 0
.tone_volume_period  equb 10, 0, 0, 0, 0, 0, 0, 0
.tone_tremulo        skip 16
.tone_tremulo_delta  skip 16
.tone_tremulo_period skip 16
.tone_vibrato        skip 16
.tone_vibrato_delta  skip 16
.tone_vibrato_period skip 16

; Stolen from https://www.youtube.com/watch?v=gUGlMhhasno

    ; Pattern 0

org PATTERN_DATA + 0*&100
	equb D_1,   &0f, F_0,   &0e, 0, 0, 0, 0
	equb BLANK, 0,   OFF,   &0e, 0, 0, 0, 0
	equb G_1,   &0f, G_0,   &0e, 0, 0, 0, 0
	equb G_1,   &0f, G_0,   &0e, 0, 0, 0, 0
	equb F_1,   &0f, BLANK, &0e, 0, 0, 0, 0
	equb BLANK, 0,   BLANK, &0e, 0, 0, 0, 0
	equb G_1,   &0f, G_0,   &0e, 0, 0, 0, 0
	equb OFF,   0,   G_0,   &0e, 0, 0, 0, 0
	equb AS_1,  &0f, AS_0,  &0e, 0, 0, 0, 0
	equb AS_1,  &0f, G_0,   &0e, 0, 0, 0, 0
	equb A_1,   &0f, C_1,   &0e, 0, 0, 0, 0
	equb OFF,   0,   G_0,   &0e, 0, 0, 0, 0
	equb G_1,   &0f, F_0,   &0e, 0, 0, 0, 0
	equb BLANK, 0,   G_0,   &0e, 0, 0, 0, 0
	equb F_1,   &0f, OFF,   &0e, 0, 0, 0, 0
	equb BLANK, 0,   G_0,   &0e, 0, 0, 0, 0
	equb D_1,   &0f, AS_0,  &0e, 0, 0, 0, 0
	equb BLANK, 0,   G_0,   &0e, 0, 0, 0, 0
	equb OFF,   0,   D_1,   &0e, 0, 0, 0, 0
	equb BLANK, 0,   G_0,   &0e, 0, 0, 0, 0
	equb D_1,   &0f, F_0,   &0e, 0, 0, 0, 0
	equb BLANK, 0,   G_0,   &0e, 0, 0, 0, 0
	equb OFF,   0,   OFF,   &0e, 0, 0, 0, 0
	equb BLANK, 0,   G_0,   &0e, 0, 0, 0, 0
	equb G_1,   &0f, F_0,   &0e, 0, 0, 0, 0
	equb D_1,   &0f, G_0,   &0e, 0, 0, 0, 0
	equb BLANK, 0,   G_0,   &0e, 0, 0, 0, 0
	equb E_1,   &0f, G_0,   &0e, 0, 0, 0, 0
	equb F_1,   &0f, BLANK, &0e, 0, 0, 0, 0
	equb F_1,   &0f, G_0,   &0e, 0, 0, 0, 0
	equb E_1,   &0f, AS_0,  &0e, 0, 0, 0, 0
	equb BLANK, 0,   G_0,   &0e, 0, 0, 0, 0

save "data", MUSIC_DATA, P%


b-tracker
=========


## What?

b-tracker is a chiptune tracker for the BBC Micro.

The BBC Micro has a rather pathetic SN76489 sound chip, capable of three
channels of not-very-accurate square wave and one channel of what is
delightfully termed 'noise'. These were cheap and cheerful and pretty common in
eight-bit computers of the day. b-tracker allows you to write music for one of
these, _on_ a BBC Micro.

[![Youtube link](http://img.youtube.com/vi/0QoihOxzcAo/0.jpg)](http://www.youtube.com/watch?v=0QoihOxzcAo "b-tracker on Youtube")

Click to play on Youtube. [Or, try it for yourself via jsbeeb
emulation!](https://bbc.godbolt.org/?&disc1=https://cowlark.com/btracker/btracker.ssd)
Press SHIFT+F12 to boot, and then load the file `DEBRIS`.

It features:

- 70 32-step patterns
- 128 patterns per sequence
- three tone channels plus noise channel
- ⅓-semitone pitch resolution
- its own envelope code
  - 16 envelopes
  - 64 individually controllable volume and pitch steps each
  - 200Hz resolution
  - 'graphic' user interface
- will run on stock BBC B or BBC Master (haven't tried a Compact)
- noise channel is tuned to the tone channels

Unlike traditional trackers, which were designed for machines with more memory
(_cough_ Amiga _cough_), b-tracker has 32 steps per pattern, and each note can
be either a note _or_ a command (because that uses half as much space).


## How?

To build, you need beebasm. There's a Makefile with the right command in it.
This will generate both a raw MOS executable called `btrack` and also a
bootable disk image also containing a demo track, a rendering of Captain's
masterpiece, _Space Debris_. The filename is DEBRIS.

Following are brief instructions 

There are three screens, selectable with f1 to f3 (f0 is not used because I
couldn't figure out how to press it in the emulator I used for development).

### File

This allows you to load and save files, as well as clear the workspace for a
new file. Pressing * will give you an OSCLI command prompt. Be careful with
this; b-tracker is using absolutely all the memory, and it's also not very good
about cleaning up its interrupt handler, so don't try to drop back into Basic
or your machine will probably lock up. Press BREAK if you want to exit.

### Pattern

This is the heart of the editor, and allows you to edit patterns (i.e., the
music itself).

- the cursor keys move around the pattern.
- A-G and 0-9 enter data into the pattern. If you're on a note, A-G are
  interpreted as a note name and 0-5 select octave. Otherwise, you're entering
  a hex digit. (Trackers traditionally use hexadecimal. The fact that it's
  easier to draw has nothing to do with things.) To get accidentals, enter a
  note name and then press + or -.
- Shift plus A-Z enter a command. See below for the list of supported commands.
- Shift plus , and . change the song tempo, globally.
- , and . change the pattern length, globally. The maximum is 1f. This doesn't
  change the amount of memory used per pattern (always 256 bytes) but allows
  songs which don't fit the power-of-two pattern size.
- Control plus the up and down cursor keys move between patterns (by pattern
  ID).
- Control plus the left and right cursor keys move between sequence entries.
  You'll be automatically taken to the appropriate pattern.
- ^N creates a new pattern and takes you there. The sequence is left untouched.
- ^S saves the current pattern to the visible slot in the sequence.
- ^A inserts the current pattern _after_ the visible slot in the sequence,
  extending the sequence.
- ^W inserts the current pattern _before_ the visible slot in the sequence,
  extending the sequence.
- ^Q removes the current pattern from the sequence, making the sequence
  shorter. The pattern itself is untouched.
- ^D copies a pattern to another pattern by ID. Be careful with this --- you
  can do huge damage.
- ^1-^4 toggle mute for each channel.
- ^L toggles looping and playback mode. In looping mode, the current pattern is
  played over and over again. In playback mode, the sequence is followed.
- + and - increment and decrement whatever the cursor is on.
- SPACE toggles playback.
- ESCAPE is the panic button and will cancel any notes which are playing
  (except for ones on the cursor row).

Each note is displayed as the note, followed by two hex digits: the first is
the tone number used for the note, and the second is the volume, with 0 being
silent and f being loudest. For commands, these digits contain the command
parameter.

The drum channel doesn't play ordinary notes, instead playing eight different
kinds of noise. Use 0-7 to set these. When on c3 or c3T mode, the pitch of the
noise will be taken from whatever note channel 3 is playing. Unfortunately, the
BBC Micro's sound chip doesn't play the noise channel in tune with the other
tone channels. b-tracker can compensate for this, but only if the volume in
channel 3 is 0 (because otherwise channel 3 will be out of tune with the other
tone channels).

There's a fairly small set of commands currently implemented:

- B: does nothing. This is displayed as `....` in the pattern editor. Any
  existing note on the channel continues to play.
- O: off. Cancels any note being played on the channel. This is displayed as
  '===' in the pattern editor.
- P: sets the channel's pitch delta to the parameter, in ⅓-semitone intervals.
- V: sets the channel's volume to the parameter.
- N: skips to the next pattern in the sequence --- useful if you want just one
  short pattern.
- X: stops playing.

### Tone

This is the envelope editor, allowing you to edit the notes being played. Use
the up and down cursor keys to move between fields; when on the graphs, use
left and right to move. Press + and - to change a value.

The fields are:

- Tone: which tone you're looking at.
- Pitch scale: a multiplication factor for the pitch delta. Useful values are 1
  (so each graph step corresponds to ⅓ of a semitone) and 3 (so each graph step
  corresponds to a semitone). You can use any value here, but most of them
  aren't helpful.
- Sample rate: how many 5ms rendering ticks per graph element.
- Repeat: the start and end of the repeat segment. The note will play until it
  hits the end marker, and then loop back to the start marker, which must be
  smaller than the end marker.

The two graphs show the delta per rendering tick. The pitch can be increased or
decreased, but the volume can only be decreased from the value actually played.

You can use pitch deltas for the noise channel, but the result is probably not
what you're expecting. Instead, you probably want to create two tones: one for
the volume, which is applied to the noise channel, and then one for the pitch,
which is applied to a silent note on channel 3. Then set the noise channel to
`c3` or `c3T`.

To play a sample note, press A-G to set the note and 0-5 to set the octave.

## Why?

I dunno. Nostalgia?


## Why not?

There are bugs, although it seems pretty robust to me.


## License?

Two-clause BSD; see the COPYING file. Go nuts.


## Who?

This program was written by myself, David Given <dg@cowlark.com>; I have a
website at http://cowlark.com. There may or may not be anything interesting
there.

The sample track, _Space Debris_, is a cover of Captain's 1991 work _Space
Debris_. See https://www.scenestream.net/demovibes/song/863/ for more
information.

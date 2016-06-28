How to control Infinity by using the keyboard.
How to add new effects.

Adding New Effects
------------------

- Build with --enable-debug and install
- Run Audacious on console
- Go to interactive mode (see next section) and experiment
- Press key 'm' for saving current effect to disk (will persist amongst Audacious sessions)

Modes
-----

The plugin has two modes. Default is Non-Interactive.

**Non-Interactive Mode**:

States and palettes periodically switches in a random sequence.

If you run Audacity from a terminal the following command keys will display on
terminal when Infinity loads:

  Keys:
  - Space:	change effect
  - Tab:   	toggle full-screen
  - Up/Down:	up/down audacity main volume
  - Left/Right:	reward/forward actual played song, if any
  - z:		previous song
  - x:		play
  - c:		pause
  - v:		stop
  - b:		next song
  - Enter:	switch to interactive mode
  		(works only if infinity was configured with --enable-debug option)
  - F11:   	screenshot
  - F12:   	change palette

**Interactive Mode**:

You can control the effects using keys:

  - F12:    change palette
  - Tab:    toggle full-screen
  - Enter:  switch to non-interactive mode
  - F1-F10: choose transformation vector field
  - a,z:    change curve 1 lighting
  - q,s:    change curve 2 lighting
  - e,r:    change curve 1 amplitude
  - d,f:    change curve 2 amplitude  
  - w:      change curve 2 type
  - m:      persist current effect
  - mouse:  change curve 2 position  
  - F11:    screenshot

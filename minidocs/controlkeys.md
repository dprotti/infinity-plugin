How to control Infinity by using the keyboard.
How to add new effects.

Adding New Effects
------------------

- Build with --enable-debug and install
- Run Audacious on console
- Go to interactive mode (see next sections) and experiment
- Press key 'm' for saving current effect to disk (will persist amongst Audacious sessions)

Modes
-----

The plugin has two modes. Default is Non-Interactive.

**Non-Interactive Mode**:

Built-in effects are used.

Keys:
  - F11:	toggle full-screen
  - Up/Down:	up/down sound volume
  - Left/Right:	reward/forward current song
  - z:		previous song
  - x:		play
  - c:		pause
  - v:		stop
  - b:		next song
  - F12:	change palette
  - Space:	change effect
  - Enter:	switch to interactive mode (only if was compiled with --enable-debug)

**Interactive Mode**:

You can change effects using keys:

  - F1-F10: choose transformation vector field
  - a,z:    change curve 1 lighting
  - q,s:    change curve 2 lighting
  - e,r:    change curve 1 amplitude
  - d,f:    change curve 2 amplitude  
  - w:      change curve 2 type
  - m:      persist current effect
  - mouse:  change curve 2 position  
  - Enter:  switch to non-interactive mode

Persisted effects go to home directory and are not added to your Audacious.
If you want them to be added to your Audacious, append it to system-wide effects
file:

```
cat ~/infinite_states >> {your_prefix}/share/infinity/infinite_states
```

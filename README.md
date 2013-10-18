Infinity Visualization Plugin
-----------------------------

http://infinity-plugin.sf.net

by Julien Carme (c) 2000
  Duilio Protti (c) 2004 - 2011

Infinity is a visualization plugin that generates light effects according to a
playing sound.

To run the plugin, 
-run audacious
-enter the Preferences menu (Ctrl-P)
-choose Plugins on the left
-choose Visualization tab on the right frame
-mark Infinity checkbox
-press Tab if you want to enter Full-Screen mode
-to stop it, press Tab again and disable it.

You don't need to be root to enter the full-screen mode.

There are two different modes in the plugin. Default mode is non-interactive.

-Non-Interactive Mode:

States and palettes are selected randomly, and change sometimes.

The following command keys will be shown when Infinity loads. If you want to
see it, start Audacity from a terminal.

  Keys:
  - Space:	change effect.
  - Tab:   	toggle full-screen.
  - Up/Down:	up/down audacity main volume.
  - Left/Right:	reward/forward actual played song, if any.
  - z:		previous song.
  - x:		play.
  - c:		pause.
  - v:		stop.
  - b:		next song.
  - Enter:	switch to interactive mode.
  		(works only if infinity was configured with --enable-debug option)
  - F11:   	screenshot.
  - F12:   	change palette.

-Interactive Mode:

You can control the effect:

  Keys:

  - F12:    change palette.
  - Tab:    toggle full-screen.
  - Enter:  switch to non-interactive mode.
  - F1-F10: choose transformation vector field
  - a,z:    change curve 1 lighting
  - q,s:    change curve 2 lighting
  - e,r:    change curve 1 amplitude
  - d,f:    change curve 2 amplitude  
  - w:      change curve 2 type
  - mouse:  change curve 2 position  
  - F11:    screenshot

SDL
---

You must have installed SDL 1.0.6 or above to run the plugin.

Known Bugs
----------

Your X system has to be at least in 16 bpp to make this program work. If you
have more than 16 bpp, it should work but may be very slow. The Preferences
cannot be changed while the plugin is enabled.

If you have problems finding locale.h header file, try to configure with
--disable-nls option.

If you found a bug on this program, please take five minutes of your time and
fill a bug report on the project's site <http://infinity-plugin.sf.net>.
The author will work to solve the problem.

Contributions
-------------

This project as of 2011 welcomes people that makes translations and people
to make binary distributions. Translations are easy because of the gettext
system, and because Infinity have just a few translatable strings (and that
will not change, this is a plugin so doesn't have a rich user interface).
Both translations and binary packages are very important because the intended
audience are end users, so we aim to leave things as simple as possible.

If you want to keep track of Infinity development, subscribe to the development
mailing list at:

http://lists.sourceforge.net/mailman/listinfo/infinity-plugin-devel

You don't need to be a project developer to subscribe.

New Features
------------

If you like any new feature on this program, please fill a feature request
or directly submit a patch to SourceForge's project site at:

http://sourceforge.net/projects/infinity-plugin


Duilio J. Protti.


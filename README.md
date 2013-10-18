Infinity Visualization Plugin
-----------------------------

https://github.com/dprotti/infinity-plugin

by
- Julien Carme (c) 2000
- Duilio Protti (c) 2004 - 2013

Infinity is a visualization plugin that generates light effects according to a
playing sound.

Requirements
------------

- Audacious >= 3.2 (http://audacious-media-player.org/)
- SDL >= 1.0.6
- Glib >= 2.8
- Gtk+ >= 2.8

Install
-------

- ./configure --prefix=/usr (or whatever your prefix is)
- make
- sudo make install

Run
---

- start Audacious
- enter menu View -> Visualizations
- mark Infinity
- play your favorite songs

To enter/leave Full-Screen mode press Tab key.

Modes
-----

The plugin has two modes. Default is non-interactive.

- Non-Interactive Mode:

States and palettes periodically switches in a random sequence.

If you run Audacity from a terminal the following command keys will display on
terminal when Infinity loads:

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

- Interactive Mode:

You can control the effects using keys:

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

Known Bugs
----------

As of 0.8.0beta1 the settings window and the about dialog does not work.

If you have problems finding locale.h header file, try to configure with
--disable-nls option.

In Ubuntu systems over AMD64 architectures, try running configure with
--prefix=/usr --libdir=/usr/lib/x86_64-linux-gnu

If you found a bug please report it at
<https://github.com/dprotti/infinity-plugin/issues>.

Old Versions
------------

Releases older than 0.8.0beta1 can be found at Sourceforge: <https://sourceforge.net/projects/infinity-plugin/>

Contributions
-------------

Your help is welcome, either coding, translating or building binary (distro)
packages.

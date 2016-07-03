Infinity
========

Visualization plugin for [Audacious](http://audacious-media-player.org/) music player.

It generates beautiful light effects. Supports full-screen mode, mouse resizing, preferences
saving and player control through keyboard.

**[Go to Downloads](https://github.com/dprotti/infinity-plugin/releases/latest/)**

![Screenshot of Infinity](https://a.fsdn.com/con/app/proj/infinity-plugin/screenshots/Infinity.png)

Requirements
------------

Audacious >= 3.5, Glib >= 2.28, SDL >= 2

**Install deps in Ubuntu**

`sudo apt -y install audacious-dev libsdl2-dev libglib2.0-dev`

Install from tarball
-------

- [Download](https://github.com/dprotti/infinity-plugin/releases/latest/)
- tar xf infinity-plugin-{version}.tar.xz
- ./configure
- make
- sudo make install

Install from repo
-------

- git clone https://github.com/dprotti/infinity-plugin
- ./autogen.sh
- make
- sudo make install

Run
---

Audacious -> View -> Visualizations -> Infinity

Enter / leave full-screen by pressing `F11`.

![Screenshot of Infinity Settings](https://cloud.githubusercontent.com/assets/2084073/16421084/2d45d54a-3d2a-11e6-9919-3d6aa5733743.png "Infinity Settings")

Playing Around
--------------

See [how to add new effects or how to control the plugin with the keyboard](https://github.com/dprotti/infinity-plugin/blob/master/minidocs/controlkeys.md).

Known Bugs
----------

In Ubuntu systems over AMD64 architectures, try configure with
``--prefix=/usr --libdir=/usr/lib/x86_64-linux-gnu``

If you found a bug please report it at
<https://github.com/dprotti/infinity-plugin/issues>.

Authors
-------

- Julien Carme (c) 2000 Original author
- Duilio Protti (C) 2004 - Present : Current maintainer
- CBke (C) 2016 <https://github.com/CBke> Nice patches
- John Lightsey (C) 2004 <john@nixnuts.net> Portability fixes and GPL License compliance
- Jean Delvare (C) 2004 <khali@linux-fr.org> Some nice patches
- Will Tatam (C) 2004 <wtatam@premierit.com> Online source RPMs <http://apt.premierithosting.com/FC2/i386/SRPMS.premierit/>
- Haavard Kvaalen (C) 2000 <havardk@xmms.org> Converted original hand made Makefile to automake/autoconf/libtool
- Chris Lea (C) 2000 <chrislea@luciddesign.com> Made RPMs
- Mitja Horvat (C) 2000 <Mitja.Horvat@hermes.si> Misc optimisations

Old Versions
------------

Releases older than 0.8.0beta1 can be found at Sourceforge: <https://sourceforge.net/projects/infinity-plugin/>

Contributions
-------------

Your feedback or help would be really appreciated.

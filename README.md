Infinity
========

Visualization plugin for [Audacious](http://audacious-media-player.org/) music player.
It generates beautiful light effects. Supports full-screen mode, mouse resizing and preferences
saving.

![Screenshot of Infinity](https://a.fsdn.com/con/app/proj/infinity-plugin/screenshots/Infinity.png)

Requirements
------------

Audacious >= 3.5, Audclient >= 3.5, 1.0.6 <= SDL < 2, Glib >= 2.8, Gtk+ >= 2.8

**Install deps in Ubuntu**

```
sudo apt -y install audacious-dev libaudclient-dev libsdl1.2-dev libglib2.0-dev libgtk2.0-dev
```

Install from tarball
-------

- **[Download](https://github.com/dprotti/infinity-plugin/releases/latest/)**
- ./configure
- make
- sudo make install

Install from repo
-------

- git clone https://github.com/dprotti/infinity-plugin
- ./autogen.sh --prefix=/usr (or whatever your prefix is)
- make
- sudo make install

Run
---

Audacious -> View -> Visualizations -> Infinity

Enter / leave full-screen by pressing `Tab`.

![Screenshot of Infinity Settings](../screenshots/screenshot-settings.png?raw=true "Infinity Settings")

Playing Around
--------------

See [how to add new effects or how to control the plugin with the keyboard](minidocs/controlkeys.md).

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

Your help is welcome either coding, testing or building distro packages.

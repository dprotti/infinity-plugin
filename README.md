Infinity
========

Standalone app for audio visualization in Linux.

Generates charming light effects. Supports full-screen mode and mouse resizing.

![Screenshot of Infinity](https://a.fsdn.com/con/app/proj/infinity-plugin/screenshots/Infinity.png)

Requirements
------------

Pipewire 0.3, Gtk+ 3.x, Glib 2.28.

During building needs Meson, Ninja, pkg-config.

**Install Deps**

Tested in Ubuntu 22 & 24.

`sudo apt-get -y install meson ninja-build pkgconf libglib2.0-dev libgtk-3-dev libpipewire-0.3-dev wireplumber pipewire-pulse libpulse-dev`

Install & Run
-------

- git clone https://github.com/dprotti/infinity-plugin
- cd infinity-plugin
- meson setup build -Db_lto=true
- meson compile -C build
- sudo meson install -C build
- infinity

After the last step you should see a new window titled "Infinity" reacting to
audio played on your desktop.

Troubleshooting
---------------

### Visualization is not reacting to audio

Infinity captures audio from your default output device (speakers/headphones)
via PipeWire. If the visualization runs but ignores desktop audio, work through
these steps.

**1. Confirm PipeWire is the active audio server**

```
pactl info | grep "Server Name"
```

Expected output contains `PulseAudio (on PipeWire ...)`. If it says only
`pulseaudio`, PipeWire is not running. Enable it:

```
sudo apt-get install pipewire-pulse wireplumber
systemctl --user disable pulseaudio.service pulseaudio.socket
systemctl --user mask pulseaudio
systemctl --user enable pipewire pipewire-pulse wireplumber
systemctl --user start pipewire pipewire-pulse wireplumber
```

Log out and back in, then re-run the `pactl info` check.

**2. Confirm a monitor source exists**

```
pactl list sources short | grep monitor
```

You should see at least one line with `monitor` in the name and status
`RUNNING` or `IDLE`. If the list is empty, your ALSA driver may not be
exposing a loopback — check `dmesg | grep snd` for ALSA errors.

**3. Confirm WirePlumber is running**

WirePlumber is the session manager that links Infinity to the monitor source.
Without it the stream stays in `paused` state indefinitely.

```
systemctl --user status wireplumber
```

If it is not active, start it:

```
systemctl --user enable --now wireplumber
```

Then restart Infinity.

Options
---

Enter / leave full-screen by pressing `F11`.

![Screenshot of Infinity Settings](https://cloud.githubusercontent.com/assets/2084073/16421084/2d45d54a-3d2a-11e6-9919-3d6aa5733743.png "Infinity Settings")

Playing Around
--------------

See [how to add new effects or how to control the plugin with the keyboard](https://github.com/dprotti/infinity-plugin/blob/master/minidocs/controlkeys.md).

Authors
-------
- Duilio Protti (C) 2004-2026 Current maintainer
- James Carthew (c) 2026 Modernization: Audacious 4.x support, Qt UI, Meson build, bug fixes
- CBke (C) 2016 <https://github.com/CBke> Nice patches
- John Lightsey (C) 2004 <john@nixnuts.net> Portability fixes and GPL License compliance
- Jean Delvare (C) 2004 <khali@linux-fr.org> Some nice patches
- Will Tatam (C) 2004 <wtatam@premierit.com> Online source RPMs <http://apt.premierithosting.com/FC2/i386/SRPMS.premierit/>
- Haavard Kvaalen (C) 2000 <havardk@xmms.org> Converted original hand made Makefile to automake/autoconf/libtool
- Chris Lea (C) 2000 <chrislea@luciddesign.com> Made RPMs
- Mitja Horvat (C) 2000 <Mitja.Horvat@hermes.si> Misc optimisations
- Julien Carme (c) 2000 Original author

Contributions
-------------

Your feedback or help would be really appreciated.

If you found a bug please report it at
<https://github.com/dprotti/infinity-plugin/issues>.

Old Versions
------------

Can be found at Sourceforge: <https://sourceforge.net/projects/infinity-plugin/>

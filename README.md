![](zytrax_logo.png)

# Zytrax

ZyTrax is an easy to use music sequencer with an interface heavily 
inspired by 90's "tracker" software (most specifically [Impulse
Tracker](https://en.wikipedia.org/wiki/Impulse_Tracker)).

While contemporary software that uses this approach exists, it 
usually has a high entry barrier because it maintains compatibility 
with old formats.

In contrast to this, ZyTrax starts afresh with an user friendly 
approach (no hex numbers, pure plugin-based architecture, inlined 
automation envelopes, smart automations, zoomable patterns and a 
simple pattern/orderlist layout).

![](zytrax.png)

## Usage

Currently, ZyTrax runs only on Windows and supports VST2 plugins via Vestige. 
It compiles and runs on Linux/X11, but no plugin
code exists (If anyone wants to contribute LV2 support, that would be
awesome! I just don't have the time).

You can find a tutorial [here](http://zytrax.org/tutorial/).

## Download

Head over to the [releases](https://github.com/reduz/zytrax/releases)
section.

## Building

You need to download MSys2, and then GTKmm for Windows, instructions
[here](https://wiki.gnome.org/Projects/gtkmm/MSWindows). Make sure to
download Python and Scons too from the package manager.

To build, type:
```
scons
```

To run:
```
cd bin
start zytrax.exe
```

Check the release to see how to package the executable for redistribution
(just replace the .exe file).



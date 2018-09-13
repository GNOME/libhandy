<div align="center">
  <a href="https://source.puri.sm/Librem5/libhandy"><b>Handy</b></a>
  <br>
  
  A Library to help with developing UI for mobile devices
  <br>

  <a href="https://source.puri.sm/Librem5/libhandy/pipelines"><img
       src="https://source.puri.sm/Librem5/libhandy/badges/master/build.svg" /></a>
  <a href="https://source.puri.sm/Librem5/libhandy/coverage"><img
       src="https://source.puri.sm/Librem5/libhandy/badges/master/coverage.svg" /></a>
</div>

---

The aim of The handy library is to help with developing UI for mobile devices
using GTK+/GNOME.

## License

libhandy is licensed under the LGPL-2.1+.

## Build dependencies

To build libhandy you need the following build-deps:

	sudo apt-get -y install gtk-doc-tools libgirepository1.0-dev libgnome-desktop-3-dev libgtk-3-dev meson pkg-config valac

## Building

We use the meson (and thereby Ninja) build system for libhandy.  The quickest
way to get going is to do the following:

	meson . _build
	ninja -C _build
	ninja -C _build install

For build options see [meson_options.txt](./meson_options.txt). E.g. to enable documentation:

     meson . _build -Dgtk_doc=true
     ninja -C _build/ libhandy-doc

## Usage

There's a C example:

     _build/examples/example

and one in Python. When running from the built source tree it
needs several environment varibles so use \_build/run to set them:

     _build/run examples/example.py

### Glade
To be able to use Handy's widgets in the glade interface designer without
installing the library use:

     _build/run glade

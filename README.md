# Handy
[![Pipeline status](https://gitlab.gnome.org/GNOME/libhandy/badges/master/build.svg)](https://gitlab.gnome.org/GNOME/libhandy/commits/master)
[![Code coverage](https://gitlab.gnome.org/GNOME/libhandy/badges/master/coverage.svg)](https://gitlab.gnome.org/GNOME/libhandy/commits/master)

The aim of the Handy library is to help with developing UI for mobile devices
using GTK/GNOME.

## License

libhandy is licensed under the LGPL-2.1+.

## Build dependencies

To build libhandy you need to first install the build-deps defined by [the debian/control file](https://gitlab.gnome.org/GNOME/libhandy/blob/master/debian/control#L6).

If you are running a Debian based distribution, you can easily install all those the dependencies making use of the following command

```sh
sudo apt-get build-dep .
```

## Building

We use the Meson (and thereby Ninja) build system for libhandy. The quickest
way to get going is to do the following:

```sh
meson . _build
ninja -C _build
ninja -C _build install
```

For build options see [meson_options.txt](./meson_options.txt). E.g. to enable documentation:

```sh
meson . _build -Dgtk_doc=true
ninja -C _build libhandy-doc
```

## Usage

There's a C example:

```sh
_build/examples/example
```

and one in Python. When running from the built source tree it
needs several environment variables so use \_build/run to set them:

```sh
_build/run examples/example.py
```

### Glade

To be able to use Handy's widgets in the glade interface designer without
installing the library use:

```sh
_build/run glade
```

## Documentation

The documentation can be found online
[here](https://gnome.pages.gitlab.gnome.org/libhandy). Further examples on how
to use the widgets in your favorite programming language can be found here:

### C
- [gnome-bluetooth](https://gitlab.gnome.org/GNOME/gnome-bluetooth) (HdyColumn)
- [GNOME Settings](https://gitlab.gnome.org/GNOME/gnome-control-center) (HdyActionRow, HdyColumn, HdyLeaflet, HdyTitleBar)
- [GNOME Web](https://gitlab.gnome.org/GNOME/epiphany) (HdyActionRow, HdyColumn, HdyComboRow, HdySearchBar)
- [phosh](https://source.puri.sm/Librem5/phosh) (HdyKeypad)

### Python 3
- [Daty](https://gitlab.gnome.org/World/Daty) (HdyColumn, HdyLeaflet, HdySearchBar, HdyTitleBar)
- [HydraPaper](https://gabmus.gitlab.io/HydraPaper/) (HdyActionRow, HdyHeaderBar, HdyPreferencesGroup, HdyPreferencesPage, HdyPreferencesRow, HdyPreferencesWindow, HdyViewSwitcher)
- [PasswordSafe](https://gitlab.gnome.org/World/PasswordSafe) (HdyColumn)
- [Unifydmin](https://gitlab.com/gabmus/unifydmin) (HdyColumn, HdyLeaflet, HdyTitleBar)

### Rust
- [Fractal](https://gitlab.gnome.org/GNOME/fractal) (HdyColumn, HdyLeaflet)
- [Podcasts](https://gitlab.gnome.org/World/podcasts) (HdyColumn)
- [Shortwave](https://gitlab.gnome.org/World/Shortwave) (HdyActionRow, HdyColumn, HdyDialog, HdyHeaderBar, HdyLeaflet, HdyPaginator, HdyPreferencesGroup, HdyPreferencesPage, HdyPreferencesWindow, HdySearchBar, HdySqueezer, HdyViewSwitcher, HdyViewSwitcherBar)

### Vala
- [GNOME Contacts](https://gitlab.gnome.org/GNOME/gnome-contacts) (HdyLeaflet, HdyHeaderGroup, HdyTitleBar)
- [GNOME Games](https://gitlab.gnome.org/GNOME/gnome-games) (HdyColumn, HdyTitleBar)
- [GNOME Clocks](https://gitlab.gnome.org/GNOME/gnome-clocks) (HdyActionRow, HdyColumn, HdyDialog, HdyHeaderBar, HdySqueezer, HdyViewSwitcher)

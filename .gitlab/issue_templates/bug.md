# What problem did you encounter?

## In what part of libhandy did you experience the problem? Note that multiple boxes may be checked.

 - [ ] build system
 - [ ] documentation
 - [ ] example application
 - [ ] HdyArrows
 - [ ] HdyColumn
 - [ ] HdyDialer
 - [ ] HdyLeaflet
 - [ ] somewhere else (please elaborate)

## What is the actual behaviour?

## What is the expected behaviour?

## How to reproduce?

  Please provide steps to reproduce the issue. If it's a graphical issue please
  attach screenshot.

# Which version did you encounter the bug in?

 - [ ] I compiled it myself. If you compiled libhandy from source please provide the
   git revision via e.g. by running ``git log -1 --pretty=oneline`` and pasting
   the output below.

 - [ ] I used the precompiled Debian package (e.g. by running a prebuilt
   image). Please determine which package you have installed and paste the package status (dpkg -s)

```
$ dpkg -l | grep libhandy
ii  gir1.2-handy-0.0:amd64                0.0.3~203.gbp18952a                     amd64        GObject introspection files for libhandy
ii  libhandy-0.0-0:amd64                  0.0.3~203.gbp18952a                     amd64        Library with GTK widgets for mobile phones
ii  libhandy-0.0-dev:amd64                0.0.3~203.gbp18952a                     amd64        Development files for libhandy

$ dpkg -s libhandy-0.0-0
```

# What hardware are you running libhandy on?

 - [ ] amd64 qemu image
 - [ ] Librem5 devkit
 - [ ] other (please elaborate)

# Releveant logfiles

  Please provide relevant logs with ``G_MESSAGES_DEBUG=all <yourappliation>``


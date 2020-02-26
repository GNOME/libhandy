#!/usr/bin/python3

import gi

gi.require_version('Gtk', '3.0')
from gi.repository import Gtk
gi.require_version('Handy', '1')
from gi.repository import Handy
import sys


window = Gtk.Window(title="Dialer Example with Python")
vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
entry = Gtk.Entry()
column = Handy.Column()
keypad = Handy.Keypad()

column.add(vbox)
vbox.pack_start(entry, True, False, 16)
vbox.pack_start(keypad, True, True, 16)

column.set_maximum_width(200)
keypad.set_entry(entry)
window.connect("destroy", Gtk.main_quit)

window.add(column)
window.show_all()
Gtk.main()

#!/usr/bin/python3

import gi

gi.require_version('Gtk', '3.0')
from gi.repository import Gtk
gi.require_version('Handy', '1')
from gi.repository import Handy
import sys


window = Gtk.Window()
vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
entry = Gtk.Entry()
headerBar = Handy.HeaderBar()
column = Handy.Column()
keypad = Handy.Keypad()

headerBar.props.title = "Keypad Example with Python"
headerBar.set_show_close_button(True)
headerBar.set_decoration_layout(":close")

vbox.add(entry)
vbox.add(keypad)
vbox.set_halign(Gtk.Align.CENTER)
vbox.set_valign(Gtk.Align.CENTER)

keypad.set_entry(entry)

window.connect("destroy", Gtk.main_quit)
window.set_titlebar(headerBar)
window.add(vbox)
window.show_all()
Gtk.main()

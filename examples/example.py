#!/usr/bin/python3

import gi

gi.require_version('Gtk', '3.0')
from gi.repository import Gtk
gi.require_version('Handy', '1')
from gi.repository import Handy
import sys


window = Gtk.Window()
vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
scrollWindow = Gtk.ScrolledWindow()
entry = Gtk.Entry()
headerBar = Handy.HeaderBar()
column = Handy.Column()
keypad = Handy.Keypad()


# set customised window title
headerBar.props.title = "Keypad Example with Python"
headerBar.set_show_close_button(True)
headerBar.set_decoration_layout(":maximize,close")


# column to limit the width of HdyWidget
column.props.linear_growth_width = 400      # allow only widgets to grow and not the margins
column.props.maximum_width = 500
column.add(vbox)


vbox.add(entry)     # widget to show dialed number
vbox.add(keypad)
vbox.set_halign(Gtk.Align.FILL)
vbox.set_valign(Gtk.Align.CENTER)
vbox.props.margin = 18
vbox.props.spacing = 18

keypad.set_entry(entry)     # attach the entry widget

scrollWindow.add(column)
scrollWindow.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)

window.connect("destroy", Gtk.main_quit)    # terminate app with close button
window.set_titlebar(headerBar)
window.add(scrollWindow)
window.show_all()
Gtk.main()

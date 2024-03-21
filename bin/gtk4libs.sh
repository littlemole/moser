#!/bin/bash

rm -rf lib/linux/glib
rm -rf lib/linux/gtk4


mkdir -p lib/linux/glib
mkdir -p lib/linux/gtk4

./build/moser bin/gir/gen3gir.msr GLib > lib/linux/glib/glib.lib
./build/moser bin/gir/gen3gir.msr GObject > lib/linux/glib/gobject.lib
./build/moser bin/gir/gen3gir.msr Gio > lib/linux/glib/gio.lib

./build/moser bin/gir/gen4gir.msr Gdk > lib/linux/gtk4/gdk.lib
./build/moser bin/gir/gen4gir.msr Gtk > lib/linux/gtk4/gtk.lib
./build/moser bin/gir/gen4gir.msr GtkSource > lib/linux/gtk4/gtksource.lib
./build/moser bin/gir/gen4gir.msr WebKit > lib/linux/gtk4/WebKit.lib
./build/moser bin/gir/gen4gir.msr JavaScriptCore > lib/linux/gtk4/jscore.lib

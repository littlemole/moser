#!/bin/bash

rm -rf lib/linux/glib
rm -rf lib/linux/gtk3


mkdir -p lib/linux/glib
mkdir -p lib/linux/gtk3

./build/moser bin/gir/gen3gir.msr GLib > lib/linux/glib/glib.lib
./build/moser bin/gir/gen3gir.msr GObject > lib/linux/glib/gobject.lib
./build/moser bin/gir/gen3gir.msr Gio > lib/linux/glib/gio.lib
./build/moser bin/gir/gen3gir.msr Gdk > lib/linux/gtk3/gdk.lib
./build/moser bin/gir/gen3gir.msr Gtk > lib/linux/gtk3/gtk.lib
./build/moser bin/gir/gen3gir.msr GtkSource > lib/linux/gtk3/gtksource.lib
./build/moser bin/gir/gen3gir.msr WebKit2 > lib/linux/gtk3/webkit2.lib
./build/moser bin/gir/gen3gir.msr JavaScriptCore > lib/linux/gtk3/jscore.lib

# ./build/moser bin/gir/genSoup.msr Soup > lib/linux/soup.lib




# -lpthread : dancingparticles expects us to have linked to this

g++ -I/opt/gnome/include/gtk-1.2 -I/opt/gnome/include/glib-1.2 -I/opt/gnome/include xmmswrapper.cpp -o amK_xmmswrapper -L/opt/gnome/lib -lgdk -lgtk -lpthread -export-dynamic && ./amK_xmmswrapper

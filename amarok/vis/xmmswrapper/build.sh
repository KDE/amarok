# -lpthread : dancingparticles expects us to have linked to this

#mxcl build
g++ -I/opt/gnome/include/gtk-1.2 -I/opt/gnome/include/glib-1.2 -I/opt/gnome/include xmmswrapper.cpp -o amK_xmmswrapper -L/opt/gnome/lib -lxmms -lgdk -lgtk -lpthread -export-dynamic && ./amK_xmmswrapper


#larson build
#g++ -I/usr/include/gtk-1.2 -I/usr/include/glib-1.2 -I/usr/lib/glib/include xmmswrapper.cpp -o amK_xmmswrapper -L/usr/lib -lxmms -lgdk -lgtk -lpthread -export-dynamic -g && ./amK_xmmswrapper libblursk.so

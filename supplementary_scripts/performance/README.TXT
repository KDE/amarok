This script was developed by Federico Mena Quintero for performance measurement in Gnome.
To use it:
Run amarok with strace:
strace -ttt -f -o /tmp/logfile.strace amarok

Run the plotting program:
python plot-timeline.py -o prettygraph.png /tmp/logfile.strace

See his homepage for the source of this script and more information how to use it:
http://www.gnome.org/~federico/hacks/index.html

Additionally, it is recommended to watch his presentation at FOSDEM 2007
http://archive.fosdem.org/2007/media/video


# YOU WILL PROBABLY NEED TO EDIT THIS SCRIPT!
echo
echo "****************"

#g++ -g `sdl-config --libs` base.cpp sdl.cpp bouncyvis.cpp -o amK_scope
g++ -g `sdl-config --libs` all.cpp -c -o libamK_vis.o

g++ -g `sdl-config --libs` sonogram.cpp libamK_vis.o -o amK_scope


#!/usr/bin/env python
# -*- coding: Latin-1 -*-
import user
import os
from Playlist import Playlist



def main():
    stdin = os.popen("kdialog --getsaveurl %s"%(user.home))
    dest = stdin.readline().strip()[5:]
    plist = Playlist()
    print dest
    try:
        f = open(dest, "w")
        f.write(plist.toHtml())
        f.close()
        os.system("kdialog --msgbox 'Amarok-playlist saved to %s'"%(dest))
    except IOError:
        os.system('kdialog --error "Sorry, could not save the playlist to %s"'%(dest))



if __name__ == "__main__":
    main()

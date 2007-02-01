/*
  Copyright (c) 2006  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/


#ifndef MAGNATUNELISTVIEWITEMS_H
#define MAGNATUNELISTVIEWITEMS_H

#include "magnatunedatabasehandler.h"
#include "magnatunetypes.h"

#include <k3listview.h>

/**
A specialized K3ListViewItem that encapsulates a MagnatuneArtist

@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/


class MagnatuneListViewArtistItem : public K3ListViewItem, public MagnatuneArtist
{
public:
    MagnatuneListViewArtistItem(MagnatuneArtist artist, K3ListView * parent);

    ~MagnatuneListViewArtistItem();

    void setOpen( bool o );
    void setup();

};


/**
A specialized K3ListViewItem that encapsulates a MagnatuneAlbum

@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
 */
class MagnatuneListViewAlbumItem : public K3ListViewItem, public MagnatuneAlbum
{
public:

    MagnatuneListViewAlbumItem(MagnatuneAlbum album, K3ListViewItem * parent);
    ~MagnatuneListViewAlbumItem();

    void setOpen( bool o );
    void setup();

};


/**
A specialized K3ListViewItem that encapsulates a MagnatuneTrack

@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
 */
class MagnatuneListViewTrackItem : public K3ListViewItem, public MagnatuneTrack
{
public:

    MagnatuneListViewTrackItem(MagnatuneTrack track, K3ListViewItem * parent);
    ~MagnatuneListViewTrackItem();

};

#endif

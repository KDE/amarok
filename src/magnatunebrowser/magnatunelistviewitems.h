//
// C++ Interface: magnatunelistviewitems
//
// Description: 
//
//
// Author: Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef MAGNATUNELISTVIEWITEMS_H
#define MAGNATUNELISTVIEWITEMS_H


#include "magnatunetypes.h"
#include "magnatunedatabasehandler.h"
#include <qlistview.h>

/**
A specialised QListViewItem that stores the streaming url for a magnatune preview track

@author Mark Kretschmann
*/


class MagnatuneListViewArtistItem : public QListViewItem, public MagnatuneArtist
{
public:
    MagnatuneListViewArtistItem(MagnatuneArtist artist, QListView * parent);

    ~MagnatuneListViewArtistItem();

    void setOpen( bool o );
    void setup();

};

class MagnatuneListViewAlbumItem : public QListViewItem, public MagnatuneAlbum
{
public:

    MagnatuneListViewAlbumItem(MagnatuneAlbum album, QListViewItem * parent);
    ~MagnatuneListViewAlbumItem();

    void setOpen( bool o );
    void setup();

};

class MagnatuneListViewTrackItem : public QListViewItem, public MagnatuneTrack
{
public:

    MagnatuneListViewTrackItem(MagnatuneTrack track, QListViewItem * parent);
    ~MagnatuneListViewTrackItem();

};

#endif

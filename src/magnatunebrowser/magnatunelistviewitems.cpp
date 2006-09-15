//
// C++ Implementation: magnatunelistviewitems
//
// Description: 
//
//
// Author: Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "magnatunelistviewitems.h"


MagnatuneListViewArtistItem::MagnatuneListViewArtistItem( MagnatuneArtist artist, QListView * parent )
: QListViewItem(parent), MagnatuneArtist(artist)
{
   QListViewItem::setText(0, artist.getName());
   setDragEnabled (true);
}

MagnatuneListViewArtistItem::~ MagnatuneListViewArtistItem( )
{
}

void MagnatuneListViewArtistItem::setOpen( bool o )
{

    if ( o && !childCount() ) {
        listView()->setUpdatesEnabled( FALSE );

        MagnatuneAlbumList albums;
        albums = MagnatuneDatabaseHandler::instance()->getAlbumsByArtistId(m_id, "");

        MagnatuneAlbumList::iterator it;
        for ( it = albums.begin(); it != albums.end(); ++it ) {
           new MagnatuneListViewAlbumItem((*it), this);
        }
    }
    QListViewItem::setOpen( o );
    listView()->setUpdatesEnabled( TRUE );
    listView()->repaintContents();
     
}


void MagnatuneListViewArtistItem::setup()
{
    setExpandable( TRUE );
    QListViewItem::setup();
}













MagnatuneListViewAlbumItem::MagnatuneListViewAlbumItem( MagnatuneAlbum album, QListViewItem * parent )
: QListViewItem(parent), MagnatuneAlbum(album)
{
   QListViewItem::setText(0, album.getName());
   setDragEnabled (true);
}

MagnatuneListViewAlbumItem::~ MagnatuneListViewAlbumItem( )
{
}


void MagnatuneListViewAlbumItem::setOpen( bool o )
{

    if ( o && !childCount() ) {
        listView()->setUpdatesEnabled( FALSE );

        MagnatuneTrackList tracks;
        tracks = MagnatuneDatabaseHandler::instance()->getTracksByAlbumId(m_id);

        MagnatuneTrackList::iterator it;
        for ( it = tracks.begin(); it != tracks.end(); ++it ) {
           new MagnatuneListViewTrackItem((*it), this);
        }
    }

    QListViewItem::setOpen( o );
    listView()->repaintContents();
     listView()->setUpdatesEnabled( TRUE );
}

void MagnatuneListViewAlbumItem::setup( )
{
    setExpandable( TRUE );
    QListViewItem::setup();
}













MagnatuneListViewTrackItem::MagnatuneListViewTrackItem( MagnatuneTrack track, QListViewItem * parent )
: QListViewItem(parent), MagnatuneTrack(track)
{
   QListViewItem::setText(0, QString::number(track.getTrackNumber()) + " - " +  track.getName());
   setDragEnabled (true);
}

MagnatuneListViewTrackItem::~ MagnatuneListViewTrackItem( )
{
}







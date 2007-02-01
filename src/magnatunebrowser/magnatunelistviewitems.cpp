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

#include "debug.h"
#include "magnatunelistviewitems.h"

#include <kglobal.h>
#include <kiconloader.h> 
#include <klocale.h>

#include <qdatetime.h>




MagnatuneListViewArtistItem::MagnatuneListViewArtistItem( MagnatuneArtist artist, K3ListView * parent )
: K3ListViewItem( parent ), MagnatuneArtist( artist )
{
    K3ListViewItem::setText( 0, artist.getName() );

    setPixmap(0, KIconLoader::global()->loadIcon( "personal", K3Icon::Toolbar, K3Icon::SizeSmall ) );

    setDragEnabled ( true );
}

MagnatuneListViewArtistItem::~ MagnatuneListViewArtistItem( )
{
}

void MagnatuneListViewArtistItem::setOpen( bool o )
{

    if ( o && !childCount() ) {
        listView()->setUpdatesEnabled( false );

        MagnatuneAlbumList albums;
        albums = MagnatuneDatabaseHandler::instance()->getAlbumsByArtistId( getId(), "" );

        MagnatuneAlbumList::iterator it;
        for ( it = albums.begin(); it != albums.end(); ++it ) {
           new MagnatuneListViewAlbumItem( (*it), this );
        }
    }
    listView()->setUpdatesEnabled( true );
    K3ListViewItem::setOpen( o );
    invalidateHeight();
    listView()->repaintContents();
   
     
}


void MagnatuneListViewArtistItem::setup()
{
    setExpandable( true );
    K3ListViewItem::setup();
}













MagnatuneListViewAlbumItem::MagnatuneListViewAlbumItem( MagnatuneAlbum album, K3ListViewItem * parent )
: K3ListViewItem( parent ), MagnatuneAlbum( album )
{
    K3ListViewItem::setText( 0, album.getName() );
    setDragEnabled( true );

    //setPixmap(0, KIconLoader::global()->loadIcon( "cdrom_unmount", K3Icon::Toolbar, K3Icon::SizeSmall ) );


}

MagnatuneListViewAlbumItem::~ MagnatuneListViewAlbumItem( )
{
}


void MagnatuneListViewAlbumItem::setOpen( bool o )
{

    if ( o && !childCount() ) {
        listView()->setUpdatesEnabled( false );

        MagnatuneTrackList tracks;
        tracks = MagnatuneDatabaseHandler::instance()->getTracksByAlbumId( getId() );

        MagnatuneTrackList::iterator it;
        for ( it = tracks.begin(); it != tracks.end(); ++it ) {
            new MagnatuneListViewTrackItem( (*it), this );
        }
    }

    listView()->setUpdatesEnabled( true );
    K3ListViewItem::setOpen( o );
    invalidateHeight();
    listView()->repaintContents();
    
    
}

void MagnatuneListViewAlbumItem::setup( )
{
    setExpandable( true );
    K3ListViewItem::setup();
}













MagnatuneListViewTrackItem::MagnatuneListViewTrackItem( MagnatuneTrack track, K3ListViewItem * parent )
: K3ListViewItem( parent ), MagnatuneTrack( track )
{
    
    int trackNumber = track.getTrackNumber();
    QString trackNumberString = QString::number( trackNumber );
    if (trackNumber < 10)
        trackNumberString = '0' + trackNumberString;
    
    
    
    K3ListViewItem::setText( 0, trackNumberString + " - " +  track.getName() );
    
    debug() << "track duration: " << QString::number( track.getDuration() ) <<  endl;
    
    QTime duration;
    duration = duration.addSecs(track.getDuration());
    
    if (duration.hour() == 0)
        K3ListViewItem::setText( 1, duration.toString( "m:ss" ) );
    else
        K3ListViewItem::setText( 1, duration.toString( "h:mm:ss" ) );
    
    setDragEnabled( true );

     //setPixmap(0, KIconLoader::global()->loadIcon( "track", K3Icon::Toolbar, K3Icon::SizeSmall ) );
}

MagnatuneListViewTrackItem::~ MagnatuneListViewTrackItem( )
{
}







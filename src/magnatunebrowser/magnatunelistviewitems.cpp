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




MagnatuneListViewArtistItem::MagnatuneListViewArtistItem( MagnatuneArtist artist, KListView * parent )
: KListViewItem( parent ), MagnatuneArtist( artist )
{
    KListViewItem::setText( 0, artist.getName() );

    setPixmap(0, KGlobal::iconLoader()->loadIcon( "personal", KIcon::Toolbar, KIcon::SizeSmall ) );

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
    KListViewItem::setOpen( o );
    invalidateHeight();
    listView()->repaintContents();
   
     
}


void MagnatuneListViewArtistItem::setup()
{
    setExpandable( true );
    KListViewItem::setup();
}













MagnatuneListViewAlbumItem::MagnatuneListViewAlbumItem( MagnatuneAlbum album, KListViewItem * parent )
: KListViewItem( parent ), MagnatuneAlbum( album )
{
    KListViewItem::setText( 0, album.getName() );
    setDragEnabled( true );

    //setPixmap(0, KGlobal::iconLoader()->loadIcon( "cdrom_unmount", KIcon::Toolbar, KIcon::SizeSmall ) );


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
    KListViewItem::setOpen( o );
    invalidateHeight();
    listView()->repaintContents();
    
    
}

void MagnatuneListViewAlbumItem::setup( )
{
    setExpandable( true );
    KListViewItem::setup();
}













MagnatuneListViewTrackItem::MagnatuneListViewTrackItem( MagnatuneTrack track, KListViewItem * parent )
: KListViewItem( parent ), MagnatuneTrack( track )
{
    
    int trackNumber = track.getTrackNumber();
    QString trackNumberString = QString::number( trackNumber );
    if (trackNumber < 10)
        trackNumberString = '0' + trackNumberString;
    
    
    
    KListViewItem::setText( 0, trackNumberString + " - " +  track.getName() );
    
    debug() << "track duration: " << QString::number( track.getDuration() ) <<  endl;
    
    QTime duration;
    duration = duration.addSecs(track.getDuration());
    
    if (duration.hour() == 0)
        KListViewItem::setText( 1, duration.toString( "m:ss" ) );
    else
        KListViewItem::setText( 1, duration.toString( "h:mm:ss" ) );
    
    setDragEnabled( true );

     //setPixmap(0, KGlobal::iconLoader()->loadIcon( "track", KIcon::Toolbar, KIcon::SizeSmall ) );
}

MagnatuneListViewTrackItem::~ MagnatuneListViewTrackItem( )
{
}







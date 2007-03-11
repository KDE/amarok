/*
Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

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


#include "magnatunecontentitem.h"


#include "debug.h"
#include "magnatunedatabasehandler.h"



MagnatuneContentItem::MagnatuneContentItem(MagnatuneArtist artist, QString genre, MagnatuneContentItem *parent )
{
    m_genre = genre;
    m_content.artistValue = new MagnatuneArtist( artist );
    m_type = MAGNATUNE_ARTIST;
    m_parent = parent;
    m_hasPopulatedChildItems = false;
}


MagnatuneContentItem::MagnatuneContentItem( MagnatuneAlbum album, QString genre, MagnatuneContentItem *parent )
{

    m_genre = genre;
    m_content.albumValue = new MagnatuneAlbum ( album );
    m_type = MAGNATUNE_ALBUM;
    m_parent = parent;
    m_hasPopulatedChildItems = false;
}


MagnatuneContentItem::MagnatuneContentItem( MagnatuneTrack track, QString genre, MagnatuneContentItem *parent )
{

    m_genre = genre;
    m_content.trackValue = new MagnatuneTrack (track );
    m_type = MAGNATUNE_TRACK;
    m_parent = parent;
    m_hasPopulatedChildItems = true;
}

MagnatuneContentItem::MagnatuneContentItem( QString genre )
{
    m_genre = genre;
    m_type = MAGNATUNE_ROOT;
    m_parent = 0;
    m_hasPopulatedChildItems = false;
}


 MagnatuneContentItem::~MagnatuneContentItem()
 {
     qDeleteAll(m_childItems);
 }

 MagnatuneContentItem *MagnatuneContentItem::child(int row)
 {

    if ( !m_hasPopulatedChildItems )
        populateChildItems();
    return dynamic_cast<MagnatuneContentItem*>( m_childItems.value( row ) );

 }

 int MagnatuneContentItem::childCount() const
 {
     if ( !m_hasPopulatedChildItems )
         populateChildItems();
     return m_childItems.count();
 }

 int MagnatuneContentItem::columnCount() const
 {
     return 1; //FIXME!!
 }

QVariant MagnatuneContentItem::data(int column) const  //FIXME!!! do We need more columns (for track length and so on...)
{
   switch ( m_type ) {
       case MAGNATUNE_ARTIST:
           return  m_content.artistValue->getName();
       case MAGNATUNE_ALBUM:
           return  m_content.albumValue->getName();
       case MAGNATUNE_TRACK:
           return  m_content.trackValue->getName();
       default:
           return QVariant();
    }

}

int MagnatuneContentItem::row() const
{
    if (m_parent){
        if ( !m_hasPopulatedChildItems )
            populateChildItems();
        return m_parent->getChildItems().indexOf(const_cast<MagnatuneContentItem*>(this));
    }
    return 0;
} 

QList<ServiceModelItemBase*> MagnatuneContentItem::getChildItems() const {
    if ( !m_hasPopulatedChildItems )
        populateChildItems();

    return m_childItems;
}

void MagnatuneContentItem::populateChildItems() const {

    switch ( m_type ) {
       case MAGNATUNE_ROOT: {
           MagnatuneArtistList artists = MagnatuneDatabaseHandler::instance()->getArtistsByGenre( m_genre );
           MagnatuneArtistList::iterator it;
           for ( it = artists.begin(); it != artists.end(); ++it ) {
               m_childItems.append( new MagnatuneContentItem( (*it), m_genre, const_cast<MagnatuneContentItem*>( this ) ) );
           }
           break; }
       case MAGNATUNE_ARTIST: {
           MagnatuneAlbumList albums = MagnatuneDatabaseHandler::instance()->getAlbumsByArtistId( m_content.artistValue->getId(), m_genre );
           MagnatuneAlbumList::iterator it;
           for ( it = albums.begin(); it != albums.end(); ++it ) {
               m_childItems.append( new MagnatuneContentItem( (*it), m_genre, const_cast<MagnatuneContentItem*>( this ) ) );
           }
           break; }
       case MAGNATUNE_ALBUM: {
           MagnatuneTrackList tracks = MagnatuneDatabaseHandler::instance()->getTracksByAlbumId( m_content.albumValue->getId() );
           MagnatuneTrackList::iterator it;
           for ( it = tracks.begin(); it != tracks.end(); ++it ) {
               m_childItems.append( new MagnatuneContentItem( (*it), m_genre,  const_cast<MagnatuneContentItem*>( this ) ) );
           }
           break; }
    }


    m_hasPopulatedChildItems = true;

}

bool MagnatuneContentItem::hasChildren () const {
    if ( m_type != MAGNATUNE_TRACK ) return true;
    return false;
}

contentTypeUnion MagnatuneContentItem::getContentUnion ( ) { return m_content; }

int MagnatuneContentItem::getType()  { return m_type; }

QString MagnatuneContentItem::getUrl() {

    if ( m_type == MAGNATUNE_TRACK ) {
        return m_content.trackValue->getHifiURL();
    } else {
        return QString();
    }
}

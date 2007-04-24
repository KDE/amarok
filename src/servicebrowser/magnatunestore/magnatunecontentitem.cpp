/***************************************************************************
 *   Copyright (c) 2006, 2007                                              *
 *        Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/ 


#include "magnatunecontentitem.h"


#include "debug.h"
#include "magnatunedatabasehandler.h"



MagnatuneContentItem::MagnatuneContentItem(MagnatuneArtist artist, const QString &genre, MagnatuneContentItem *parent )
{
    m_genre = genre;
    m_content.artistValue = new MagnatuneArtist( artist );
    m_type = MAGNATUNE_ARTIST;
    m_parent = parent;
    m_hasPopulatedChildItems = false;
}


MagnatuneContentItem::MagnatuneContentItem( MagnatuneAlbum album, const QString &genre, MagnatuneContentItem *parent )
{

    m_genre = genre;
    m_content.albumValue = new MagnatuneAlbum ( album );
    m_type = MAGNATUNE_ALBUM;
    m_parent = parent;
    m_hasPopulatedChildItems = false;
}


MagnatuneContentItem::MagnatuneContentItem( MagnatuneTrack track, const QString &genre, MagnatuneContentItem *parent )
{

    m_genre = genre;
    m_content.trackValue = new MagnatuneTrack (track );
    m_type = MAGNATUNE_TRACK;
    m_parent = parent;
    m_hasPopulatedChildItems = true;
}

MagnatuneContentItem::MagnatuneContentItem( const QString &genre )
{
    m_genre = genre;
    m_type = MAGNATUNE_ROOT;
    m_parent = 0;
    m_hasPopulatedChildItems = false;
    prePopulate();
    populate(); // need to fill up artist or there will be nothing to show
}


 MagnatuneContentItem::~MagnatuneContentItem()
 {
 }

 MagnatuneContentItem *MagnatuneContentItem::child(int row)
 {

    //if ( !m_hasPopulatedChildItems )
        //populateChildItems();
    return dynamic_cast<MagnatuneContentItem*>( m_childItems.value( row ) );

 }

 int MagnatuneContentItem::childCount() const
 {
     //if ( !m_hasPopulatedChildItems )
         //populateChildItems();
     return m_childItems.count();
 }

 int MagnatuneContentItem::columnCount() const
 {
     return 1; //FIXME!!
 }

QVariant MagnatuneContentItem::data(int column) const  //FIXME!!! do We need more columns (for track length and so on...)
{
   QString leadingZero;

   switch ( m_type ) {
       case MAGNATUNE_ROOT:
           return  "Root node";
       case MAGNATUNE_ARTIST:
           return  m_content.artistValue->getName();
       case MAGNATUNE_ALBUM:
           return  m_content.albumValue->getName();
       case MAGNATUNE_TRACK:
           if (m_content.trackValue->getTrackNumber() < 10)
               leadingZero = "0";



           return leadingZero + QString::number( m_content.trackValue->getTrackNumber() ) + " - " +  m_content.trackValue->getName();
       default:
           return QVariant();
    }

}

int MagnatuneContentItem::row() const
{
    if (m_parent){
        //if ( !m_hasPopulatedChildItems )
            //populateChildItems();
        return m_parent->getChildItems().indexOf(const_cast<MagnatuneContentItem*>(this));
    }
    return 0;
}

QList<ServiceModelItemBase*> MagnatuneContentItem::getChildItems() const {
    if ( !m_hasPopulatedChildItems ) {
        prePopulate();
        populateChildItems();
    }

    return m_childItems;
}

int MagnatuneContentItem::prePopulate() const
{

    int numberOfChildren =0;
    if ( !m_hasPopulatedChildItems ) {
        switch ( m_type ) {
           case MAGNATUNE_ROOT: {
               MagnatuneArtistList artists = MagnatuneDatabaseHandler::instance()->getArtistsByGenre( m_genre );
               numberOfChildren = artists.size();
               MagnatuneArtistList::iterator it;
               for ( it = artists.begin(); it != artists.end(); ++it ) {
                   m_childItems.append( new MagnatuneContentItem( (*it), m_genre, const_cast<MagnatuneContentItem*>( this ) ) );
               }
               break; }
           case MAGNATUNE_ARTIST: {
               MagnatuneAlbumList albums = MagnatuneDatabaseHandler::instance()->getAlbumsByArtistId( m_content.artistValue->getId(), m_genre );
               numberOfChildren = albums.size();
               MagnatuneAlbumList::iterator it;
               for ( it = albums.begin(); it != albums.end(); ++it ) {
                   m_childItems.append( new MagnatuneContentItem( (*it), m_genre, const_cast<MagnatuneContentItem*>( this ) ) );
               }
               break; }
           case MAGNATUNE_ALBUM: {
               MagnatuneTrackList tracks = MagnatuneDatabaseHandler::instance()->getTracksByAlbumId( m_content.albumValue->getId() );
               numberOfChildren = tracks.size();
               MagnatuneTrackList::iterator it;
               for ( it = tracks.begin(); it != tracks.end(); ++it ) {
                   m_childItems.append( new MagnatuneContentItem( (*it), m_genre,  const_cast<MagnatuneContentItem*>( this ) ) );
               }
               break; 
            }
        }

    }


   
    return numberOfChildren;

}

void MagnatuneContentItem::populateChildItems() const {

    while ( !m_prefetchedItems.isEmpty() )
        m_childItems.append( m_prefetchedItems.takeFirst() );
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
        return m_content.trackValue->getURL();
    } else {
        return QString();
    }
}

void MagnatuneContentItem::populate() const
{
     if ( !m_hasPopulatedChildItems )
        populateChildItems();
}




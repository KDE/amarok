/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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


#include "databasedrivencontentitem.h"


#include "debug.h"
#include "databasehandlerbase.h"



DatabaseDrivenContentItem::DatabaseDrivenContentItem( SimpleServiceArtist *artist, const QString &genre, DatabaseDrivenContentItem *parent, DatabaseHandlerBase * dbHandler )
{
    m_genre = genre;
    m_content.artistValue =  artist;
    m_type = SERVICE_ITEM_ARTIST;
    m_parent = parent;
    m_hasPopulatedChildItems = false;
    m_dbHandler = dbHandler;
}


DatabaseDrivenContentItem::DatabaseDrivenContentItem( SimpleServiceAlbum *album, const QString &genre, DatabaseDrivenContentItem *parent, DatabaseHandlerBase * dbHandler )
{

    m_genre = genre;
    m_content.albumValue =  album;
    m_type = SERVICE_ITEM_ALBUM;
    m_parent = parent;
    m_hasPopulatedChildItems = false;
    m_dbHandler = dbHandler;
}


DatabaseDrivenContentItem::DatabaseDrivenContentItem( SimpleServiceTrack *track, const QString &genre, DatabaseDrivenContentItem *parent, DatabaseHandlerBase * dbHandler )
{

    m_genre = genre;
    m_content.trackValue = track;
    m_type = SERVICE_ITEM_TRACK;
    m_parent = parent;
    m_hasPopulatedChildItems = true;
    m_dbHandler = dbHandler;
}

DatabaseDrivenContentItem::DatabaseDrivenContentItem( const QString &genre, DatabaseHandlerBase * dbHandler )
{
    m_genre = genre;
    m_type = SERVICE_ITEM_ROOT;
    m_parent = 0;
    m_hasPopulatedChildItems = false;
    m_dbHandler = dbHandler;
    prePopulate();
    populate(); // need to fill up artist or there will be nothing to show
}


 DatabaseDrivenContentItem::~DatabaseDrivenContentItem()
 {
 }

 DatabaseDrivenContentItem *DatabaseDrivenContentItem::child(int row)
 {

    //if ( !m_hasPopulatedChildItems )
        //populateChildItems();
    return dynamic_cast<DatabaseDrivenContentItem*>( m_childItems.value( row ) );

 }

 int DatabaseDrivenContentItem::childCount() const
 {
     //if ( !m_hasPopulatedChildItems )
         //populateChildItems();
     return m_childItems.count();
 }

 int DatabaseDrivenContentItem::columnCount() const
 {
     return 1; //FIXME!!
 }

QVariant DatabaseDrivenContentItem::data(int column) const  //FIXME!!! do We need more columns (for track length and so on...)
{
   QString leadingZero;

   switch ( m_type ) {
       case SERVICE_ITEM_ROOT:
           return  "Root node";
       case SERVICE_ITEM_ARTIST:
           return  m_content.artistValue->getName();
       case SERVICE_ITEM_ALBUM:
           return  m_content.albumValue->getName();
       case SERVICE_ITEM_TRACK:
           if (m_content.trackValue->getTrackNumber() < 10)
               leadingZero = "0";



           return leadingZero + QString::number( m_content.trackValue->getTrackNumber() ) + " - " +  m_content.trackValue->getName();
       default:
           return QVariant();
    }

}

int DatabaseDrivenContentItem::row() const
{
    if (m_parent){
        //if ( !m_hasPopulatedChildItems )
            //populateChildItems();
        return m_parent->getChildItems().indexOf(const_cast<DatabaseDrivenContentItem*>(this));
    }
    return 0;
}

QList<ServiceModelItemBase*> DatabaseDrivenContentItem::getChildItems() const {
    if ( !m_hasPopulatedChildItems )
        populateChildItems();

    return m_childItems;
}


int DatabaseDrivenContentItem::prePopulate() const
{
 
    int numberOfChildren =0;
    if ( !m_hasPopulatedChildItems ) {

        switch ( m_type ) {
            case SERVICE_ITEM_ROOT: {
                SimpleServiceArtistList artists = m_dbHandler->getArtistsByGenre( m_genre );
                numberOfChildren = artists.size();
                SimpleServiceArtistList::iterator it;
                for ( it = artists.begin(); it != artists.end(); ++it ) {
                   m_prefetchedItems.append( new DatabaseDrivenContentItem( (*it), m_genre, const_cast<DatabaseDrivenContentItem*>( this ), m_dbHandler ) );
                }
                break; }
            case SERVICE_ITEM_ARTIST: {
                SimpleServiceAlbumList albums = m_dbHandler->getAlbumsByArtistId( m_content.artistValue->getId(), m_genre );
                numberOfChildren = albums.size();
                SimpleServiceAlbumList::iterator it;
                for ( it = albums.begin(); it != albums.end(); ++it ) {
                    m_prefetchedItems.append( new DatabaseDrivenContentItem( (*it), m_genre, const_cast<DatabaseDrivenContentItem*>( this ), m_dbHandler  ) );
                }
                break; }
            case SERVICE_ITEM_ALBUM: {
                SimpleServiceTrackList tracks = m_dbHandler->getTracksByAlbumId( m_content.albumValue->getId() );
                numberOfChildren = tracks.size();
                SimpleServiceTrackList::iterator it;
                for ( it = tracks.begin(); it != tracks.end(); ++it ) {
                    m_prefetchedItems.append( new DatabaseDrivenContentItem( (*it), m_genre,  const_cast<DatabaseDrivenContentItem*>( this ), m_dbHandler  ) );
                }
                break;
             }
         }
    }

    return numberOfChildren;
}

void DatabaseDrivenContentItem::populateChildItems() const {


    while ( !m_prefetchedItems.isEmpty() )
        m_childItems.append( m_prefetchedItems.takeFirst() );
    m_hasPopulatedChildItems = true;

}

bool DatabaseDrivenContentItem::hasChildren () const {
    if ( m_type != SERVICE_ITEM_TRACK ) return true;
    return false;
}

contentTypeUnion DatabaseDrivenContentItem::getContentUnion ( ) { return m_content; }

int DatabaseDrivenContentItem::getType()  { return m_type; }

QString DatabaseDrivenContentItem::getUrl() {

    if ( m_type == SERVICE_ITEM_TRACK ) {
        return m_content.trackValue->getURL();
    } else {
        return QString();
    }
}

void DatabaseDrivenContentItem::populate() const
{
     if ( !m_hasPopulatedChildItems )
        populateChildItems();
}




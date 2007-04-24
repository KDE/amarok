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


#include "jamendocontentitem.h"


#include "debug.h"
#include "jamendodatabasehandler.h"



JamendoContentItem::JamendoContentItem(JamendoArtist artist, const QString &genre, JamendoContentItem *parent )
{
    m_genre = genre;
    m_content.artistValue = new JamendoArtist( artist );
    m_type = JAMENDO_ARTIST;
    m_parent = parent;
    m_hasPopulatedChildItems = false;
}


JamendoContentItem::JamendoContentItem( JamendoAlbum album, const QString &genre, JamendoContentItem *parent )
{

    m_genre = genre;
    m_content.albumValue = new JamendoAlbum ( album );
    m_type = JAMENDO_ALBUM;
    m_parent = parent;
    m_hasPopulatedChildItems = false;
}


JamendoContentItem::JamendoContentItem( JamendoTrack track, const QString &genre, JamendoContentItem *parent )
{

    m_genre = genre;
    m_content.trackValue = new JamendoTrack (track );
    m_type = JAMENDO_TRACK;
    m_parent = parent;
    m_hasPopulatedChildItems = true;
}

JamendoContentItem::JamendoContentItem( const QString &genre )
{
    m_genre = genre;
    m_type = JAMENDO_ROOT;
    m_parent = 0;
    m_hasPopulatedChildItems = false;
    prePopulate();
    populate(); // need to fill up artist or there will be nothing to show
}


 JamendoContentItem::~JamendoContentItem()
 {
 }

 JamendoContentItem *JamendoContentItem::child(int row)
 {

    //if ( !m_hasPopulatedChildItems )
        //populateChildItems();
    return dynamic_cast<JamendoContentItem*>( m_childItems.value( row ) );

 }

 int JamendoContentItem::childCount() const
 {
     //if ( !m_hasPopulatedChildItems )
         //populateChildItems();
     return m_childItems.count();
 }

 int JamendoContentItem::columnCount() const
 {
     return 1; //FIXME!!
 }

QVariant JamendoContentItem::data(int column) const  //FIXME!!! do We need more columns (for track length and so on...)
{
   QString leadingZero;

   switch ( m_type ) {
       case JAMENDO_ROOT:
           return  "Root node";
       case JAMENDO_ARTIST:
           return  m_content.artistValue->getName();
       case JAMENDO_ALBUM:
           return  m_content.albumValue->getName();
       case JAMENDO_TRACK:
           if (m_content.trackValue->getTrackNumber() < 10)
               leadingZero = "0";



           return leadingZero + QString::number( m_content.trackValue->getTrackNumber() ) + " - " +  m_content.trackValue->getName();
       default:
           return QVariant();
    }

}

int JamendoContentItem::row() const
{
    if (m_parent){
        //if ( !m_hasPopulatedChildItems )
            //populateChildItems();
        return m_parent->getChildItems().indexOf(const_cast<JamendoContentItem*>(this));
    }
    return 0;
}

QList<ServiceModelItemBase*> JamendoContentItem::getChildItems() const {
    if ( !m_hasPopulatedChildItems )
        populateChildItems();

    return m_childItems;
}


int JamendoContentItem::prePopulate() const
{
 
    int numberOfChildren =0;
    if ( !m_hasPopulatedChildItems ) {

        switch ( m_type ) {
            case JAMENDO_ROOT: {
                JamendoArtistList artists = JamendoDatabaseHandler::instance()->getArtistsByGenre( m_genre );
                numberOfChildren = artists.size();
                JamendoArtistList::iterator it;
                for ( it = artists.begin(); it != artists.end(); ++it ) {
                   m_prefetchedItems.append( new JamendoContentItem( (*it), m_genre, const_cast<JamendoContentItem*>( this ) ) );
                }
                break; }
            case JAMENDO_ARTIST: {
                JamendoAlbumList albums = JamendoDatabaseHandler::instance()->getAlbumsByArtistId( m_content.artistValue->getId(), m_genre );
                numberOfChildren = albums.size();
                JamendoAlbumList::iterator it;
                for ( it = albums.begin(); it != albums.end(); ++it ) {
                    m_prefetchedItems.append( new JamendoContentItem( (*it), m_genre, const_cast<JamendoContentItem*>( this ) ) );
                }
                break; }
            case JAMENDO_ALBUM: {
                JamendoTrackList tracks = JamendoDatabaseHandler::instance()->getTracksByAlbumId( m_content.albumValue->getId() );
                numberOfChildren = tracks.size();
                JamendoTrackList::iterator it;
                for ( it = tracks.begin(); it != tracks.end(); ++it ) {
                    m_prefetchedItems.append( new JamendoContentItem( (*it), m_genre,  const_cast<JamendoContentItem*>( this ) ) );
                }
                break;
             }
         }
    }

    return numberOfChildren;
}

void JamendoContentItem::populateChildItems() const {


    while ( !m_prefetchedItems.isEmpty() )
        m_childItems.append( m_prefetchedItems.takeFirst() );
    m_hasPopulatedChildItems = true;

}

bool JamendoContentItem::hasChildren () const {
    if ( m_type != JAMENDO_TRACK ) return true;
    return false;
}

contentTypeUnion JamendoContentItem::getContentUnion ( ) { return m_content; }

int JamendoContentItem::getType()  { return m_type; }

QString JamendoContentItem::getUrl() {

    if ( m_type == JAMENDO_TRACK ) {
        return m_content.trackValue->getURL();
    } else {
        return QString();
    }
}

void JamendoContentItem::populate() const
{
     if ( !m_hasPopulatedChildItems )
        populateChildItems();
}




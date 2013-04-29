/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
 * Copyright (c) 2013 Alberto Villa <avilla@FreeBSD.org>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "MusicBrainzTagsItem.h"

#include "AmarokMimeData.h"
#include "core/support/Debug.h"
#include "MusicBrainzMeta.h"

#define DEBUG_PREFIX "MusicBrainzTagsItem"

MusicBrainzTagsItem::MusicBrainzTagsItem( MusicBrainzTagsItem *parent,
                                          const Meta::TrackPtr track,
                                          const QVariantMap tags )
    : m_parent( parent )
    , m_track( track )
    , m_data( tags )
    , m_chosen( false )
    , m_dataLock( QReadWriteLock::Recursive )
    , m_parentLock( QReadWriteLock::Recursive )
    , m_childrenLock( QReadWriteLock::Recursive )
{
}

MusicBrainzTagsItem::~MusicBrainzTagsItem()
{
    qDeleteAll( m_childItems );
}

MusicBrainzTagsItem *
MusicBrainzTagsItem::parent() const
{
    QReadLocker lock( &m_parentLock );
    return m_parent;
}

void
MusicBrainzTagsItem::setParent( MusicBrainzTagsItem *parent )
{
    QWriteLocker lock( &m_parentLock );
    m_parent = parent;
}

MusicBrainzTagsItem *
MusicBrainzTagsItem::child( const int row ) const
{
    QReadLocker lock( &m_childrenLock );
    return m_childItems.value( row );
}

void
MusicBrainzTagsItem::appendChild( MusicBrainzTagsItem *newItem )
{
    DEBUG_BLOCK

    if( newItem->track().isNull() )
    {
        delete newItem;
        return;
    }

    if( m_track.isNull() )
    {
        // Root item.
        bool found = false;

        /*
         * A write lock is required, because with a read lock two or more threads
         * referencing the same track could search for a matching item at the same time,
         * fail, and queue up to create a new one, thus resulting in duplicates.
         */
        QWriteLocker lock( &m_childrenLock );
        foreach( MusicBrainzTagsItem *item, m_childItems )
        {
            if( item->track() == newItem->track() )
            {
                found = true;
                if( !newItem->data().isEmpty() )
                    item->appendChild( newItem );
                break;
            }
        }

        if( !found )
        {
            MusicBrainzTagsItem *newChild = new MusicBrainzTagsItem( this, newItem->track() );
            if( !newItem->data().isEmpty() )
                newChild->appendChild( newItem );
            m_childItems.append( newChild );
        }
    }
    else
    {
        if( m_track != newItem->track() )
        {
            debug() << "Trying to insert track data to the wrong tree branch.";
            delete newItem;
            return;
        }

        bool found = false;
        newItem->setParent( this );

        // Already locked in parent call (the same logic applies).
        foreach( MusicBrainzTagsItem *item, m_childItems )
        {
            if( newItem == item )
            {
                found = true;
                // Merge the two matching results.
                debug() << "Track" << newItem->dataValue( MusicBrainz::TRACKID ).toString() << "already in the tree.";
                item->mergeWith( newItem );
                delete newItem;
                break;
            }
        }

        if( !found )
        {
            newItem->dataInsert( MusicBrainz::SIMILARITY,
                                 newItem->dataValue( MusicBrainz::MUSICBRAINZ ).toFloat() +
                                 newItem->dataValue( MusicBrainz::MUSICDNS ).toFloat() );

            QVariantList trackList;
            QVariantList artistList;
            QVariantList releaseList;
            if( newItem->dataContains( MusicBrainz::TRACKID ) )
                trackList.append( newItem->dataValue( MusicBrainz::TRACKID ) );
            if( newItem->dataContains( MusicBrainz::ARTISTID ) )
                artistList.append( newItem->dataValue( MusicBrainz::ARTISTID ) );
            if( newItem->dataContains( MusicBrainz::RELEASEID ) )
                releaseList.append( newItem->dataValue( MusicBrainz::RELEASEID ) );
            newItem->dataInsert( MusicBrainz::TRACKID, trackList );
            newItem->dataInsert( MusicBrainz::ARTISTID, artistList );
            newItem->dataInsert( MusicBrainz::RELEASEID, releaseList );

            m_childItems.append( newItem );
        }
    }
}

void
MusicBrainzTagsItem::mergeWith( MusicBrainzTagsItem *item )
{
    /*
     * The lock is inherited from appendChild(). This method is not supposed to be called
     * elsewhere.
     */

    // Calculate the future score of the result when merged.
    if( !item->dataContains( MusicBrainz::MUSICBRAINZ ) &&
        dataContains( MusicBrainz::MUSICBRAINZ ) )
        item->dataInsert( MusicBrainz::MUSICBRAINZ,
                          dataValue( MusicBrainz::MUSICBRAINZ ) );
    if( !item->dataContains( MusicBrainz::MUSICDNS ) &&
        dataContains( MusicBrainz::MUSICDNS ) )
        item->dataInsert( MusicBrainz::MUSICDNS,
                          dataValue( MusicBrainz::MUSICDNS ) );
    item->dataInsert( MusicBrainz::SIMILARITY,
                      item->dataValue( MusicBrainz::MUSICBRAINZ ).toFloat() +
                      item->dataValue( MusicBrainz::MUSICDNS ).toFloat() );

    QVariantList trackList = dataValue( MusicBrainz::TRACKID ).toList();
    QVariantList artistList = dataValue( MusicBrainz::ARTISTID ).toList();
    QVariantList releaseList = dataValue( MusicBrainz::RELEASEID ).toList();
    if( item->score() > score() )
    {
        // Update the score.
        if( item->dataContains( MusicBrainz::MUSICBRAINZ ) )
            dataInsert( MusicBrainz::MUSICBRAINZ,
                        item->dataValue( MusicBrainz::MUSICBRAINZ ) );
        if( item->dataContains( MusicBrainz::MUSICDNS ) )
            dataInsert( MusicBrainz::MUSICDNS,
                        item->dataValue( MusicBrainz::MUSICDNS ) );
        dataInsert( MusicBrainz::SIMILARITY,
                    item->dataValue( MusicBrainz::SIMILARITY ) );

        if( item->dataContains( MusicBrainz::TRACKID ) )
            trackList.prepend( item->dataValue( MusicBrainz::TRACKID ) );
        if( item->dataContains( MusicBrainz::ARTISTID ) )
            artistList.prepend( item->dataValue( MusicBrainz::ARTISTID ) );
        if( item->dataContains( MusicBrainz::RELEASEID ) )
            releaseList.prepend( item->dataValue( MusicBrainz::RELEASEID ) );
    }
    else
    {
        if( item->dataContains( MusicBrainz::TRACKID ) )
            trackList.append( item->dataValue( MusicBrainz::TRACKID ) );
        if( item->dataContains( MusicBrainz::ARTISTID ) )
            artistList.append( item->dataValue( MusicBrainz::ARTISTID ) );
        if( item->dataContains( MusicBrainz::RELEASEID ) )
            releaseList.append( item->dataValue( MusicBrainz::RELEASEID ) );
    }
    dataInsert( MusicBrainz::TRACKID, trackList );
    dataInsert( MusicBrainz::ARTISTID, artistList );
    dataInsert( MusicBrainz::RELEASEID, releaseList );
}

int
MusicBrainzTagsItem::childCount() const
{
    QReadLocker lock( &m_childrenLock );
    return m_childItems.count();
}

int
MusicBrainzTagsItem::row() const
{
    if( parent() )
    {
        QReadLocker lock( &m_childrenLock );
        return m_parent->m_childItems.indexOf( const_cast<MusicBrainzTagsItem *>( this ) );
    }

    return 0;
}

Meta::TrackPtr
MusicBrainzTagsItem::track() const
{
    QReadLocker lock( &m_dataLock );
    return m_track;
}

float
MusicBrainzTagsItem::score() const
{
    QReadLocker lock( &m_dataLock );
    float score = dataValue( MusicBrainz::SIMILARITY ).toFloat();

    /*
     * Results of fingerprint-only lookup go on bottom as they are weak matches (only
     * their length is compared).
     */
    if( !dataContains( MusicBrainz::MUSICBRAINZ ) )
        score -= 1.0;

    return score;
}

QVariantMap
MusicBrainzTagsItem::data() const
{
    QReadLocker lock( &m_dataLock );
    return m_data;
}

QVariant
MusicBrainzTagsItem::data( const int column ) const
{
    if( m_data.isEmpty() )
    {
        switch( column )
        {
        case 0:
            {
                QString title;
                int trackNumber = m_track->trackNumber();
                if( trackNumber > 0 )
                    title += QString( "%1 - " ).arg( trackNumber );
                title += m_track->prettyName();
                return title;
            }
        case 1:
            return ( !m_track->artist().isNull() )? m_track->artist()->name() : QVariant();
        case 2:
            {
                if( m_track->album().isNull() )
                    return QVariant();
                QString album = m_track->album()->name();
                int discNumber = m_track->discNumber();
                if( discNumber > 0 )
                    album += QString( " (disc %1)" ).arg( discNumber );
                return album;
            }
        case 3:
            return ( !m_track->album().isNull() && m_track->album()->hasAlbumArtist() )?
                   m_track->album()->albumArtist()->name() : QVariant();
        case 4:
            return ( m_track->year()->year() > 0 )? m_track->year()->year() : QVariant();
        }

        return QVariant();
    }

    switch( column )
    {
    case 0:
        {
            QString title;
            QVariant trackNumber = dataValue( Meta::Field::TRACKNUMBER );
            if( trackNumber.toInt() > 0 )
            {
                title += trackNumber.toString();
                int trackCount = dataValue( MusicBrainz::TRACKCOUNT ).toInt();
                if ( trackCount > 0 )
                    title += QString( "/%1" ).arg( trackCount );
                title += " - ";
            }
            title += dataValue( Meta::Field::TITLE ).toString();
            return title;
        }
    case 1:
        return dataValue( Meta::Field::ARTIST );;
    case 2:
        {
            QString album = dataValue( Meta::Field::ALBUM ).toString();
            int discNumber = dataValue( Meta::Field::DISCNUMBER ).toInt();
            if( discNumber > 0 )
                album += QString( " (disc %1)" ).arg( discNumber );
            return album;
        }
    case 3:
        return dataValue( Meta::Field::ALBUMARTIST );
    case 4:
        return dataValue( Meta::Field::YEAR );
    }

    return QVariant();
}

void
MusicBrainzTagsItem::setData( const QVariantMap &tags )
{
    QWriteLocker lock( &m_dataLock );
    m_data = tags;
}

bool
MusicBrainzTagsItem::dataContains( const QString &key ) const
{
    QReadLocker lock( &m_dataLock );
    return m_data.contains( key );
}

QVariant
MusicBrainzTagsItem::dataValue( const QString &key ) const
{
    QReadLocker lock( &m_dataLock );
    if( m_data.contains( key ) )
        return m_data.value( key );

    return QVariant();
}

void
MusicBrainzTagsItem::dataInsert( const QString &key, const QVariant &value )
{
    QWriteLocker lock( &m_dataLock );
    m_data.insert( key, value );
}

bool
MusicBrainzTagsItem::isChosen() const
{
    QReadLocker lock( &m_dataLock );
    if( m_data.isEmpty() )
    {
        foreach( MusicBrainzTagsItem *item, m_childItems )
            if( item->isChosen() )
                return true;
        return false;
    }

    return m_chosen;
}

void
MusicBrainzTagsItem::setChosen( bool chosen )
{
    if( m_data.isEmpty() )
        return;

    QWriteLocker lock( &m_dataLock );
    m_chosen = chosen;
}

MusicBrainzTagsItem *
MusicBrainzTagsItem::chosenItem() const
{
    if( m_data.isEmpty() )
    {
        QReadLocker lock( &m_childrenLock );
        foreach( MusicBrainzTagsItem *item, m_childItems )
            if( item->isChosen() )
                return item;
    }

    return 0;
}

bool
MusicBrainzTagsItem::chooseBestMatch()
{
    if( !m_data.isEmpty() )
        return false;

    QReadLocker lock( &m_childrenLock );
    if( !childCount() || isChosen() )
        return false;

    MusicBrainzTagsItem *bestMatch;
    float maxScore = 0;
    foreach( MusicBrainzTagsItem *item, m_childItems )
    {
        if( item->score() > maxScore )
        {
            bestMatch = item;
            maxScore = item->score();
        }
    }

    bestMatch->setChosen( true );
    return true;
}

bool
MusicBrainzTagsItem::chooseBestMatchFromRelease( const QStringList &releases )
{
    if( !m_data.isEmpty() )
        return false;

    QReadLocker lock( &m_childrenLock );
    if( !childCount() || isChosen() )
        return false;

    MusicBrainzTagsItem *bestMatch = 0;
    float maxScore = 0;
    QSet<QString> idList = releases.toSet();
    foreach( MusicBrainzTagsItem *item, m_childItems )
    {
        /*
         * Match any of the releases referenced by selected entry. This should guarantee
         * that best results are always chosen when available.
         */
        if( item->score() > maxScore &&
            !item->dataValue( MusicBrainz::RELEASEID ).toStringList().toSet().intersect( idList ).isEmpty() )
        {
            bestMatch = item;
            maxScore = item->score();
        }
    }

    if( bestMatch )
    {
        bestMatch->setChosen( true );
        return true;
    }

    return false;
}

void
MusicBrainzTagsItem::clearChoices()
{
    QReadLocker lock( &m_childrenLock );
    if( !parent() )
        foreach( MusicBrainzTagsItem *item, m_childItems )
            item->clearChoices();
    else if( m_data.isEmpty() )
        foreach( MusicBrainzTagsItem *item, m_childItems )
            item->setChosen( false );
}

bool
MusicBrainzTagsItem::operator==( const MusicBrainzTagsItem* item ) const
{
    QReadLocker lock( &m_dataLock );
#define MATCH( k, t ) dataValue( k ).t() == item->dataValue( k ).t()
    /*
     * This is the information shown to the user: he will never be able to
     * distinguish between two tracks with the same information.
     */
    return MATCH( Meta::Field::TITLE, toString ) &&
           MATCH( Meta::Field::ARTIST, toString ) &&
           MATCH( Meta::Field::ALBUM, toString ) &&
           MATCH( Meta::Field::ALBUMARTIST, toString ) &&
           MATCH( Meta::Field::YEAR, toInt ) &&
           MATCH( MusicBrainz::TRACKCOUNT, toInt ) &&
           MATCH( Meta::Field::DISCNUMBER, toInt ) &&
           MATCH( Meta::Field::TRACKNUMBER, toInt );
#undef MATCH
}

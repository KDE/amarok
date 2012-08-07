/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "CollectionProvider.h"

#include "MetaValues.h"
#include "amarokconfig.h"
#include "core/collections/Collection.h"
#include "core/collections/QueryMaker.h"
#include "statsyncing/collection/CollectionTrack.h"

using namespace StatSyncing;

CollectionProvider::CollectionProvider( Collections::Collection *collection )
    : m_coll( collection )
{
    Q_ASSERT( m_coll );
    connect( this, SIGNAL(startArtistSearch()), SLOT(slotStartArtistSearch()) );
    connect( this, SIGNAL(startTrackSearch(QString)), SLOT(slotStartTrackSearch(QString)) );
}

CollectionProvider::~CollectionProvider()
{
}

QString
CollectionProvider::id() const
{
    return m_coll ? m_coll.data()->collectionId() : QString();
}

QString
CollectionProvider::prettyName() const
{
    return m_coll ? m_coll.data()->prettyName() : QString();
}

KIcon
CollectionProvider::icon() const
{
    return m_coll ? m_coll.data()->icon() : KIcon();
}

qint64
CollectionProvider::reliableTrackMetaData() const
{
    return Meta::valTitle | Meta::valArtist | Meta::valAlbum |
           Meta::valComposer | Meta::valYear | Meta::valTrackNr | Meta::valDiscNr;
}

qint64
CollectionProvider::writableTrackStatsData() const
{
    // TODO: this is unreliable and hacky, but serves for now:
    if( id() == "localCollection" )
        return Meta::valRating | Meta::valFirstPlayed | Meta::valLastPlayed | Meta::valPlaycount | Meta::valLabel;
    else
        return Meta::valRating | Meta::valFirstPlayed | Meta::valLastPlayed | Meta::valPlaycount;
}

Provider::Preference
CollectionProvider::defaultPreference()
{
    // currently only Local Collection and iPod one have good syncing capabilities
    if( id() == "localCollection" )
        return YesByDefault;
    if( id().startsWith( "amarok-ipodtrackuid" ) )
        return Ask;
    return NoByDefault;
}

QSet<QString>
CollectionProvider::artists()
{
    if( !m_coll )
        return QSet<QString>();

    m_foundArtists.clear();
    emit startArtistSearch();
    m_queryMakerSemaphore.acquire(); // blocks until slotQueryDone() releases the semaphore
    QSet<QString> ret = m_foundArtists;
    m_foundArtists.clear();  // don't waste memory

    return ret;
}

TrackList
CollectionProvider::artistTracks( const QString &artistName )
{
    if( !m_coll )
        return TrackList();

    m_foundTracks.clear();
    emit startTrackSearch( artistName );
    m_queryMakerSemaphore.acquire(); // blocks until slotQueryDone() releases the semaphore
    TrackList ret = m_foundTracks;
    m_foundTracks.clear();  // don't waste memory

    return ret;
}

void
CollectionProvider::slotStartArtistSearch()
{
    if( !m_coll )
    {
        m_queryMakerSemaphore.release(); // prevent deadlock
        return;
    }

    Collections::QueryMaker *qm = m_coll.data()->queryMaker();
    qm->setAutoDelete( true );
    qm->setQueryType( Collections::QueryMaker::Artist );
    connect( qm, SIGNAL(newResultReady(Meta::ArtistList)),
             SLOT(slotNewResultReady(Meta::ArtistList)) );
    connect( qm, SIGNAL(queryDone()), SLOT(slotQueryDone()) );
    qm->run();
}

void
CollectionProvider::slotStartTrackSearch( QString artistName )
{
    if( !m_coll )
    {
        m_queryMakerSemaphore.release(); // prevent deadlock
        return;
    }

    Collections::QueryMaker *qm = m_coll.data()->queryMaker();
    qm->setAutoDelete( true );
    qm->setQueryType( Collections::QueryMaker::Track );
    qm->addFilter( Meta::valArtist, artistName, true, true );
    connect( qm, SIGNAL(newResultReady(Meta::TrackList)),
             SLOT(slotNewResultReady(Meta::TrackList)) );
    connect( qm, SIGNAL(queryDone()), SLOT(slotQueryDone()) );
    qm->run();
}

void
CollectionProvider::slotNewResultReady( Meta::ArtistList list )
{
    foreach( const Meta::ArtistPtr &artist, list )
    {
        m_foundArtists.insert( artist->name().toLower() );
    }
}

void
CollectionProvider::slotNewResultReady( Meta::TrackList list )
{
    foreach( Meta::TrackPtr track, list )
    {
        m_foundTracks.append( TrackPtr( new CollectionTrack( track ) ) );
    }
}

void
CollectionProvider::slotQueryDone()
{
    m_queryMakerSemaphore.release(); // unblock method in a worker thread
}

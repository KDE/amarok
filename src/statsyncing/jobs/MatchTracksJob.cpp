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

#define DEBUG_PREFIX "StatSyncing"

#include "MatchTracksJob.h"

#include "MetaValues.h"
#include "core/meta/Meta.h"

#include <algorithm>

using namespace StatSyncing;

#undef VERBOSE_DEBUG
#ifdef VERBOSE_DEBUG
#include "core/support/Debug.h"
static void printPerProviderTrackList( const PerProviderTrackList &providerTracks,
                                       const QString *fromArtist = 0L )
{
    for( ProviderPtr provider : providerTracks.keys() )
    {
        if( fromArtist )
            debug() << provider->prettyName() << "tracks from" << *fromArtist;
        else
            debug() << provider->prettyName() << "tracks";
        for( TrackPtr track : providerTracks.value( provider ) )
        {
            debug() << "  " << track->artist() << "-" << track->album() << "-" << track->name();
        }
    }
}

#include "core/meta/support/MetaConstants.h"
static QString comparisonFieldNames( qint64 fields )
{
    QStringList names;
    for( qint64 value = 1; value < Meta::valCustom; value *= 2 )
    {
        if( value & fields )
        {
            names << Meta::i18nForField( value );
        }
    }
    return names.join( ", " );
}

QDebug operator<<( QDebug dbg, const ProviderPtr &provider )
{
    dbg.nospace() << "ProviderPtr(" << provider->prettyName() << ")";
    return dbg.space();
}

QDebug operator<<( QDebug dbg, const TrackPtr &track )
{
    dbg.nospace() << "TrackPtr(" << track->artist() << " - " << track->name() << ")";
    return dbg.space();
}
#endif

qint64 MatchTracksJob::s_comparisonFields( 0 );

qint64
MatchTracksJob::comparisonFields()
{
    return s_comparisonFields;
}

MatchTracksJob::MatchTracksJob( const ProviderPtrList &providers, QObject *parent )
    : QObject(parent)
    , ThreadWeaver::Job()
    , m_abort( false )
    , m_providers( providers )
{
}

ProviderPtrList
MatchTracksJob::providers() const
{
    return m_providers;
}

bool
MatchTracksJob::success() const
{
    return !m_abort;
}

void
MatchTracksJob::abort()
{
    m_abort = true;
}

// work-around macro vs. template argument clash in foreach
typedef QMultiMap<ProviderPtr, QString> ArtistProviders;

void MatchTracksJob::run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread)
{
    Q_UNUSED(self);
    Q_UNUSED(thread);
    const qint64 possibleFields = Meta::valTitle | Meta::valArtist | Meta::valAlbum |
        Meta::valComposer | Meta::valYear | Meta::valTrackNr | Meta::valDiscNr;
    const qint64 requiredFields = Meta::valTitle | Meta::valArtist | Meta::valAlbum;
    s_comparisonFields = possibleFields;

    // map of lowercase artist names to a list of providers that contain it plus their
    // preferred representation of the artist name
    QMap<QString, QMultiMap<ProviderPtr, QString> > providerArtists;
    for( ProviderPtr provider : m_providers )
    {
        QSet<QString> artists = provider->artists();
        for( const QString &artist : artists )
            providerArtists[ artist.toLower() ].insert( provider, artist );
        s_comparisonFields &= provider->reliableTrackMetaData();
    }
    Q_UNUSED( requiredFields ) // silence gcc warning about unused var in non-debug build
    Q_ASSERT( ( s_comparisonFields & requiredFields ) == requiredFields );
    Q_EMIT totalSteps( providerArtists.size() );
#ifdef VERBOSE_DEBUG
    debug() << "Matching using:" << comparisonFieldNames( s_comparisonFields ).toLocal8Bit().constData();
#endif

    for( const ArtistProviders &artistProviders : providerArtists )
    {
        if( m_abort )
            break;
        matchTracksFromArtist( artistProviders );
        Q_EMIT incrementProgress();
    }
    Q_EMIT endProgressOperation( this );

#ifdef VERBOSE_DEBUG
    debug();
    int tupleCount = m_matchedTuples.count();
    debug() << "Found" << tupleCount << "tuples of matched tracks from multiple collections";
    for( ProviderPtr provider : m_providers )
    {
        const TrackList uniqueList = m_uniqueTracks.value( provider );
        const TrackList excludedList = m_excludedTracks.value( provider );
        debug() << provider->prettyName() << "has" << uniqueList.count() << "unique tracks +"
                << excludedList.count() << "duplicate tracks +" << m_matchedTrackCounts[ provider ]
                << " matched =" << uniqueList.count() + excludedList.count() + m_matchedTrackCounts[ provider ];
    }
#endif
}

void MatchTracksJob::defaultBegin(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    Q_EMIT started(self);
    ThreadWeaver::Job::defaultBegin(self, thread);
}

void MatchTracksJob::defaultEnd(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    ThreadWeaver::Job::defaultEnd(self, thread);
    if (!self->success()) {
        Q_EMIT failed(self);
    }
    Q_EMIT done(self);
}

void
MatchTracksJob::matchTracksFromArtist( const QMultiMap<ProviderPtr, QString> &providerArtists )
{
#ifdef VERBOSE_DEBUG
    DEBUG_BLOCK
    debug() << "providerArtists:" << providerArtists;
#endif
    PerProviderTrackList providerTracks;
    for( ProviderPtr provider : providerArtists.uniqueKeys() )
    {
        TrackList trackList;
        for( const QString &artist : providerArtists.values( provider ) )
            trackList << provider->artistTracks( artist );
        if( trackList.isEmpty() )
            continue;  // don't add empty lists to providerTracks
        // the sorting is important and makes our matching algorithm work
        std::sort( trackList.begin(), trackList.end(), trackDelegatePtrLessThan<MatchTracksJob> );

        scanForScrobblableTracks( trackList );
        providerTracks[ provider ] = trackList;
    }

#ifdef VERBOSE_DEBUG
    debug() << "providerTracks:" << providerTracks;
    QScopedPointer<Debug::Block> debugBlockPointer;
    if( providerTracks.keys().count() > 1 )
    {
        debugBlockPointer.reset( new Debug::Block( __PRETTY_FUNCTION__ ) );
        printPerProviderTrackList( providerTracks );
    }
#endif

    // if only one (or less) non-empty provider is left, we're done
    while( providerTracks.keys().count() > 1 )
    {
        TrackPtr firstTrack = findSmallestTrack( providerTracks );
        PerProviderTrackList equalTracks = takeTracksEqualTo( firstTrack, providerTracks );
        Q_ASSERT( !equalTracks.isEmpty() );

        // optimization: continue early if there's only one provider left
        if( equalTracks.keys().count() <= 1 )
        {
            ProviderPtr provider = equalTracks.keys().first();
            m_uniqueTracks[ provider ].append( equalTracks[ provider ] );
            continue;
        }

#ifdef VERBOSE_DEBUG
        debug();
        debug() << "First track:" << firstTrack->artist() << "-" << firstTrack->album() << "-" << firstTrack->name();
        debug() << "Tracks no greater than first track:";
        printPerProviderTrackList( equalTracks );
#endif

        TrackTuple matchedTuple;
        for( ProviderPtr provider : equalTracks.keys() )
        {
            int listSize = equalTracks[ provider ].size();
            Q_ASSERT( listSize >= 1 );
            if( listSize == 1 )
                matchedTuple.insert( provider, equalTracks[ provider ].at( 0 ) );
            else
                m_excludedTracks[ provider ].append( equalTracks[ provider ] );
        }

        if( matchedTuple.count() > 1 )
            // good, we've found track that matches!
            addMatchedTuple( matchedTuple );
        else if( matchedTuple.count() == 1 )
        {
            // only one provider
            ProviderPtr provider = matchedTuple.provider( 0 );
            m_uniqueTracks[ provider ].append( matchedTuple.track( provider ) );
        }
    }

    if( !providerTracks.isEmpty() ) // some tracks from one provider left
    {
        ProviderPtr provider = providerTracks.keys().first();
        m_uniqueTracks[ provider ].append( providerTracks[ provider ] );
    }
}

TrackPtr
MatchTracksJob::findSmallestTrack( const PerProviderTrackList &providerTracks )
{
    TrackPtr smallest;
    for( const TrackList &list : providerTracks )
    {
        if( !smallest || list.first()->lessThan( *smallest, s_comparisonFields ) )
            smallest = list.first();
    }
    Q_ASSERT( smallest );
    return smallest;
}

PerProviderTrackList
MatchTracksJob::takeTracksEqualTo( const TrackPtr &track,
                                   PerProviderTrackList &providerTracks )
{
    PerProviderTrackList ret;
    for( ProviderPtr provider : providerTracks.keys() )
    {
        while( !providerTracks[ provider ].isEmpty() &&
               track->equals( *providerTracks[ provider ].first(), s_comparisonFields ) )
        {
            ret[ provider ].append( providerTracks[ provider ].takeFirst() );
        }
        if( providerTracks[ provider ].isEmpty() )
            providerTracks.remove( provider );
    }
    return ret;
}

void
MatchTracksJob::addMatchedTuple( const TrackTuple &tuple )
{
    m_matchedTuples.append( tuple );
    for( ProviderPtr provider : tuple.providers() )
    {
        m_matchedTrackCounts[ provider ]++;
    }
}

void
MatchTracksJob::scanForScrobblableTracks( const TrackList &trackList )
{
    for( const TrackPtr &track : trackList )
    {
        // ScrobblingServices take Meta::Track, ensure there is an underlying one
        if( track->recentPlayCount() > 0 && track->metaTrack() )
            m_tracksToScrobble << track;
    }
}

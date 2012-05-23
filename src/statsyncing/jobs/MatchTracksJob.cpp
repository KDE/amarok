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

#include "MatchTracksJob.h"

#include "MetaValues.h"
#include "core/support/Debug.h"

using namespace StatSyncing;

//BEGIN debugging
static void printPerProviderTrackList( const PerProviderTrackList &providerTracks,
                                       const QString *fromArtist = 0L )
{
    foreach( const TrackDelegateProvider *provider, providerTracks.keys() )
    {
        if( fromArtist )
            debug() << provider->prettyName() << "tracks from" << *fromArtist;
        else
            debug() << provider->prettyName() << "tracks";
        foreach( TrackDelegatePtr track, providerTracks.value( provider ) )
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
//END debugging

qint64 MatchTracksJob::s_comparisonFields( 0 );

qint64
MatchTracksJob::comparisonFields()
{
    return s_comparisonFields;
}

MatchTracksJob::MatchTracksJob( QList<TrackDelegateProvider *> providers,
                                QObject *parent )
    : Job( parent )
    , m_providers( providers )
{
}

void MatchTracksJob::run()
{
    DEBUG_BLOCK
    const qint64 possibleFields = Meta::valTitle | Meta::valArtist | Meta::valAlbum |
        Meta::valComposer | Meta::valYear | Meta::valTrackNr | Meta::valDiscNr;
    const qint64 requiredFields = Meta::valTitle | Meta::valArtist | Meta::valAlbum;
    s_comparisonFields = possibleFields;

    QSet<QString> allArtists;
    // remember which providers contain particular artists (optimisation)
    QMap<QString, QSet<TrackDelegateProvider *> > artistProviders;
    foreach( TrackDelegateProvider *provider, m_providers )
    {
        QSet<QString> artists = provider->artists();
        foreach( const QString &artist, artists )
            artistProviders[ artist ].insert( provider );
        allArtists.unite( artists );
        s_comparisonFields &= provider->reliableTrackMetaData();
    }
    Q_ASSERT( ( s_comparisonFields & requiredFields ) == requiredFields );
    debug() << "Matching using:" << comparisonFieldNames( s_comparisonFields ).toLocal8Bit().constData();

    foreach( const QString &artist, allArtists )
    {
        matchTracksFromArtist( artist, artistProviders.value( artist ) );
    }

    //BEGIN debugging
    debug();
    int tupleCount = m_matchedTuples.count();
    debug() << "Found" << tupleCount << "tuples of matched tracks from multiple collections";
    foreach( const TrackDelegateProvider *provider, m_providers )
    {
        const TrackDelegateList uniqueList = m_uniqueTracks.value( provider );
        const TrackDelegateList excludedList = m_excludedTracks.value( provider );

//         if( uniqueList.count() <= 100 )
//             foreach( const TrackDelegatePtr &track, uniqueList )
//                 debug() << "  " << track->artist() << "-" << track->album() << "-" << track->name();
//         if( excludedList.count() <= 100 )
//             foreach( const TrackDelegatePtr &track, excludedList )
//                 debug() << "  " << track->artist() << "-" << track->album() << "-" << track->name();

        debug() << provider->prettyName() << "has" << uniqueList.count() << "unique tracks +"
                << excludedList.count() << "duplicate tracks +" << m_matchedTrackCounts[ provider ]
                << " matched =" << uniqueList.count() + excludedList.count() + m_matchedTrackCounts[ provider ];
    }
    //END debugging
}

void
MatchTracksJob::matchTracksFromArtist( const QString &artist,
                                       const QSet<TrackDelegateProvider *> &artistProviders )
{
    PerProviderTrackList providerTracks;
    foreach( TrackDelegateProvider *provider, artistProviders )
    {
        if( !artistProviders.contains( provider ) )
            continue;  // optimisation: don't query providers without this artist
        TrackDelegateList trackList = provider->artistTracks( artist );
        if( trackList.isEmpty() )
            continue;  // don't add empty lists to providerTracks
        // the sorting is important and makes our matching algorithm work
        qSort( trackList.begin(), trackList.end(), trackDelegatePtrLessThan<MatchTracksJob> );
        providerTracks[ provider ] = trackList;
    }

    //BEGIN debugging
    QScopedPointer<Debug::Block> debugBlockPointer;
    if( providerTracks.keys().count() > 1 )
    {
        debugBlockPointer.reset( new Debug::Block( __PRETTY_FUNCTION__ ) );
        printPerProviderTrackList( providerTracks );
    }
    //END debugging

    // if only one (or less) non-empty provider is left, we're done
    while( providerTracks.keys().count() > 1 )
    {
        TrackDelegatePtr firstTrack = findSmallestTrack( providerTracks );
        PerProviderTrackList equalTracks = takeTracksEqualTo( firstTrack, providerTracks );
        Q_ASSERT( !equalTracks.isEmpty() );

        // optimisation: continue early if there's only one provider left
        if( equalTracks.keys().count() <= 1 )
        {
            const TrackDelegateProvider *provider = equalTracks.keys().first();
            m_uniqueTracks[ provider ].append( equalTracks[ provider ] );
            continue;
        }

        //BEGIN debugging
        debug();
        debug() << "First track:" << firstTrack->artist() << "-" << firstTrack->album() << "-" << firstTrack->name();
        debug() << "Tracks no greater than first track:";
        printPerProviderTrackList( equalTracks );
        //END debugging

        TrackTuple matchedTuple;
        foreach( const TrackDelegateProvider *provider, equalTracks.keys() )
        {
            int listSize = equalTracks[ provider ].size();
            Q_ASSERT( listSize >= 1 );
            if( listSize == 1 )
                matchedTuple.insert( provider, equalTracks[ provider ].at( 0 ) );
            else
                m_excludedTracks[ provider ].append( equalTracks[ provider ] );
        }

        if( matchedTuple.size() > 1 )
            // good, we've found track that match!
            addMatchedTuple( matchedTuple );
        else if( matchedTuple.size() == 1 )
        {
            // only one provider
            const TrackDelegateProvider *provider = matchedTuple.keys().first();
            m_uniqueTracks[ provider ].append( matchedTuple[ provider ] );
        }
    }

    if( !providerTracks.isEmpty() ) // some tracks from one provider left
    {
        const TrackDelegateProvider *provider = providerTracks.keys().first();
        m_uniqueTracks[ provider ].append( providerTracks[ provider ] );
    }
}

TrackDelegatePtr
MatchTracksJob::findSmallestTrack( const PerProviderTrackList &providerTracks )
{
    TrackDelegatePtr smallest;
    foreach( const TrackDelegateList &list, providerTracks )
    {
        if( !smallest || list.first()->lessThan( *smallest, s_comparisonFields ) )
            smallest = list.first();
    }
    Q_ASSERT( smallest );
    return smallest;
}

PerProviderTrackList
MatchTracksJob::takeTracksEqualTo( const TrackDelegatePtr &track,
                                   PerProviderTrackList &providerTracks )
{
    PerProviderTrackList ret;
    foreach( const TrackDelegateProvider *provider, providerTracks.keys() )
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
    foreach( const TrackDelegateProvider *provider, tuple.keys() )
    {
        m_matchedTrackCounts[ provider ]++;
    }
}

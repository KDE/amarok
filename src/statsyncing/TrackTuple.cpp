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

#include "TrackTuple.h"

#include "MetaValues.h"
#include "core/support/Debug.h"
#include "statsyncing/Options.h"
#include "statsyncing/Provider.h"

Q_DECLARE_METATYPE(QSet<QString>)

using namespace StatSyncing;

const QList<qint64> TrackTuple::s_fields = QList<qint64>() << Meta::valRating
    << Meta::valFirstPlayed << Meta::valLastPlayed << Meta::valPlaycount << Meta::valLabel;

TrackTuple::TrackTuple()
{
}

void
TrackTuple::insert( ProviderPtr provider, const TrackPtr &track )
{
    m_map.insert( provider, track );
}

ProviderPtrList
TrackTuple::providers() const
{
    return m_map.keys();
}

ProviderPtr
TrackTuple::provider( int i ) const
{
    return m_map.keys().value( i );
}

TrackPtr
TrackTuple::track( const ProviderPtr &provider ) const
{
    Q_ASSERT( m_map.contains( provider ) );
    return m_map.value( provider );
}

int
TrackTuple::count() const
{
    return m_map.count();
}

bool
TrackTuple::isEmpty() const
{
    return m_map.isEmpty();
}

bool
TrackTuple::fieldUpdated( qint64 field, const Options &options, ProviderPtr provider ) const
{
    if( isEmpty() ||
        !(options.syncedFields() & field) ||
        ( provider && !m_map.contains( provider ) ) ||
        ( provider && !(provider->writableTrackStatsData() & field) ) )
    {
        return false;
    }

    switch( field )
    {
        case Meta::valRating:
        {
            int rating = syncedRating( options );
            if( rating < 0 )
                return false; // unresolved conflict, not going to write that
            if( provider )
                return track( provider )->rating() != rating;

            for( const ProviderPtr &prov : m_map.keys() )
            {
                if( !(prov->writableTrackStatsData() & field ) )
                    continue; // this provider doesn't even know how to write this field
                if( track( prov )->rating() != rating )
                    return true;
            }
            return false;
        }

        case Meta::valFirstPlayed:
        {
            QDateTime firstPlayed = syncedFirstPlayed( options );
            if( provider )
                return track( provider )->firstPlayed() != firstPlayed;

            for( const ProviderPtr &prov : m_map.keys() )
            {
                if( !(prov->writableTrackStatsData() & field ) )
                    continue; // this provider doesn't even know how to write this field
                if( track( prov )->firstPlayed() != firstPlayed )
                    return true;
            }
            return false;
        }

        case Meta::valLastPlayed:
        {
            QDateTime lastPlayed = syncedLastPlayed( options );
            if( provider )
                return track( provider )->lastPlayed() != lastPlayed;

            for( const ProviderPtr &prov : m_map.keys() )
            {
                if( !(prov->writableTrackStatsData() & field ) )
                    continue; // this provider doesn't even know how to write this field
                if( track( prov )->lastPlayed() != lastPlayed )
                    return true;
            }
            return false;
        }

        case Meta::valPlaycount:
        {
            int playcount = syncedPlaycount( options );
            if( provider )
                return track( provider )->playCount() != playcount;

            for( const ProviderPtr &prov : m_map.keys() )
            {
                if( !(prov->writableTrackStatsData() & field ) )
                    continue; // this provider doesn't even know how to write this field
                if( track( prov )->playCount() != playcount )
                    return true;
            }
            return false;
        }

        case Meta::valLabel:
        {
            bool hasConflict = true;
            QSet<QString> labels = syncedLabels( options, m_labelProviders, hasConflict );
            if( hasConflict )
                return false; // unresolved conflict, not going to write that
            if( provider )
                return track( provider )->labels() - options.excludedLabels() != labels;

            for( const ProviderPtr &prov : m_map.keys() )
            {
                if( !(prov->writableTrackStatsData() & field ) )
                    continue; // this provider doesn't even know how to write this field
                if( track( prov )->labels() - options.excludedLabels() != labels )
                    return true;
            }
            return false;
        }
    }
    return false;
}

bool
TrackTuple::hasUpdate( const Options &options ) const
{
    for( qint64 field : s_fields )
    {
        if( fieldUpdated( field, options ) )
            return true;
    }
    return false;
}

bool
TrackTuple::fieldHasConflict( qint64 field, const Options& options, bool includeResolved ) const
{
    switch( field )
    {
        case Meta::valRating:
            // we must disregard currently selected rating provider for includeResolved = true
            return syncedRating( options, includeResolved ? ProviderPtr() : m_ratingProvider ) < 0;
        case Meta::valLabel:
        {
            bool hasConflict = false;
            // we must disregard currently selected label providers for includeResolved = true
            syncedLabels( options, includeResolved ? ProviderPtrSet() : m_labelProviders , hasConflict );
            return hasConflict;
        }
    }
    return false;
}

bool
TrackTuple::hasConflict( const Options &options ) const
{
    return fieldHasConflict( Meta::valRating, options )
        || fieldHasConflict( Meta::valLabel, options );
}

ProviderPtr
TrackTuple::ratingProvider() const
{
    return m_ratingProvider;
}

void
TrackTuple::setRatingProvider( const ProviderPtr &provider )
{
    if( !provider || m_map.contains( provider ) )
        m_ratingProvider = provider;
}

ProviderPtrSet
TrackTuple::labelProviders() const
{
    return m_labelProviders;
}

void
TrackTuple::setLabelProviders( const ProviderPtrSet &providers )
{
    m_labelProviders.clear();
    foreach( const ProviderPtr &provider, providers )
    {
        if( m_map.contains( provider ) )
            m_labelProviders.insert( provider );
    }
}

int
TrackTuple::syncedRating( const Options &options ) const
{
    return syncedRating( options, m_ratingProvider );
}

int
TrackTuple::syncedRating( const Options &options, ProviderPtr ratingProvider ) const
{
    if( isEmpty() || !(options.syncedFields() & Meta::valRating) )
        return 0;
    if( ratingProvider ) // a provider has been chosen
        return track( ratingProvider )->rating();

    // look for conflict:
    int candidate = -1; // rating candidate
    QMapIterator<ProviderPtr, TrackPtr> it( m_map );
    while( it.hasNext() )
    {
        it.next();
        int rating = it.value()->rating();

        // take rating candidate only from rated tracks or from rating-writable collections
        bool canWriteRating = it.key()->writableTrackStatsData() & Meta::valRating;
        if( candidate < 0 )
        {
            if( rating > 0 || canWriteRating )
                candidate = rating;
            continue; // nothing to do in this loop iteration in either case
        }

        if( rating <= 0 && !canWriteRating )
            // skip unrated songs from colls with not-writable rating
            continue;

        if( rating != candidate )
            return -1;
    }
    // if candidate == -1, it means there are no colls with writable or non-zero rating
    return qMax( 0, candidate );
}

QDateTime
TrackTuple::syncedFirstPlayed( const Options &options ) const
{
    QDateTime first;
    if( isEmpty() || !(options.syncedFields() & Meta::valFirstPlayed) )
        return first;
    for( TrackPtr track : m_map )
    {
        QDateTime trackFirstPlayed = track->firstPlayed();
        if( !trackFirstPlayed.isValid() )
            continue;
        if( !first.isValid() || trackFirstPlayed < first )
            first = trackFirstPlayed;
    }
    return first;
}

QDateTime
TrackTuple::syncedLastPlayed( const Options &options ) const
{
    QDateTime last;
    if( isEmpty() || !(options.syncedFields() & Meta::valLastPlayed) )
        return last;
    for( TrackPtr track : m_map )
    {
        QDateTime trackLastPlayed = track->lastPlayed();
        if( !trackLastPlayed.isValid() )
            continue;
        if( !last.isValid() || trackLastPlayed > last )
            last = trackLastPlayed;
    }
    return last;
}

int
TrackTuple::syncedPlaycount( const Options &options ) const
{
    if( isEmpty() || !(options.syncedFields() & Meta::valPlaycount) )
        return 0;
    int max = 0;
    int sumRecent = 0;
    for( TrackPtr track : m_map )
    {
        int recent = track->recentPlayCount();
        sumRecent += recent;
        max = qMax( max, track->playCount() - recent );
    }
    return max + sumRecent;
}

QSet<QString>
TrackTuple::syncedLabels( const Options &options ) const
{
    bool dummy = false;
    return syncedLabels( options, m_labelProviders, dummy );
}

QSet<QString>
TrackTuple::syncedLabels( const Options &options, const ProviderPtrSet &labelProviders, bool &hasConflict ) const
{
    hasConflict = false;
    QSet<QString> labelsCandidate;
    if( isEmpty() || !(options.syncedFields() & Meta::valLabel) )
        return labelsCandidate;
    if( !labelProviders.isEmpty() ) // providers have been chosen
    {
        foreach( const ProviderPtr &provider, labelProviders )
            labelsCandidate |= track( provider )->labels();
        return labelsCandidate - options.excludedLabels();
    }

    // look for conflict:
    bool labelsCandidateAlreadySet = false;
    QMapIterator<ProviderPtr, TrackPtr> it( m_map );
    while( it.hasNext() )
    {
        it.next();
        QSet<QString> labels = it.value()->labels() - options.excludedLabels();

        // take labels candidate only from labelled tracks or from label-writable collections
        bool canWriteLabels = it.key()->writableTrackStatsData() & Meta::valLabel;
        if( !labelsCandidateAlreadySet )
        {
            if( !labels.isEmpty() || canWriteLabels )
            {
                labelsCandidate = labels;
                labelsCandidateAlreadySet = true;
            }
            continue; // nothing to do in this loop iteration in either case
        }

        if( labels.isEmpty() && !canWriteLabels )
            // skip unlabelled songs from colls with not-writable labels
            continue;

        if( labels != labelsCandidate )
        {
            hasConflict = true;
            return QSet<QString>();
        }
    }
    return labelsCandidate;
}

ProviderPtrSet
TrackTuple::synchronize( const Options &options ) const
{
    ProviderPtrSet updatedProviders;
    foreach( qint64 field, s_fields )
    {
        // catches if field should not be at all updated (either no change or not in options )
        if( !fieldUpdated( field, options ) )
            continue;

        QVariant synced;
        switch( field )
        {
            case Meta::valRating:
                synced = syncedRating( options ); break;
            case Meta::valFirstPlayed:
                synced = syncedFirstPlayed( options ); break;
            case Meta::valLastPlayed:
                synced = syncedLastPlayed( options ); break;
            case Meta::valPlaycount:
                synced = syncedPlaycount( options ); break;
            case Meta::valLabel:
                synced.setValue<QSet<QString> >( syncedLabels( options ) ); break;
            default:
                warning() << __PRETTY_FUNCTION__ << "unhandled first switch";
        }

        QMapIterator<ProviderPtr, TrackPtr> it( m_map );
        while( it.hasNext() )
        {
            it.next();
            ProviderPtr provider = it.key();
            // we have special case for playcount because it needs to we written even if
            // apparently unchanged to reset possible nonzero recentPlayCount
            if( field != Meta::valPlaycount && !fieldUpdated( field, options, provider ) )
                continue; // nothing to do for this field and provider

            updatedProviders.insert( provider );
            TrackPtr track = it.value();
            switch( field )
            {
                case Meta::valRating:
                    track->setRating( synced.toInt() ); break;
                case Meta::valFirstPlayed:
                    track->setFirstPlayed( synced.toDateTime() ); break;
                case Meta::valLastPlayed:
                    track->setLastPlayed( synced.toDateTime() ); break;
                case Meta::valPlaycount:
                    track->setPlayCount( synced.toInt() ); break;
                case Meta::valLabel:
                {
                    QSet<QString> desiredLabels = synced.value<QSet<QString> >();
                    /* add back blacklisted labels; we say we don't touch them, so we
                     * should neither add them (handled in syncedLabels()) nor remove them
                     * (handled here)
                     */
                    desiredLabels |= track->labels() & options.excludedLabels();
                    track->setLabels( desiredLabels );
                    break;
                }
                default:
                    warning() << __PRETTY_FUNCTION__ << "unhandled second switch";
            }
        }
    }

    for( const ProviderPtr &provider : updatedProviders )
        track( provider )->commit();
    return updatedProviders;
}

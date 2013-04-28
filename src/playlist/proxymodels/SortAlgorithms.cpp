/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2010 Nanno Langstraat <langstr@gmail.com>                              *
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#include "SortAlgorithms.h"

#include "core/meta/Meta.h"
#include "core/meta/Statistics.h"
#include "core/support/Debug.h"
#include "playlist/proxymodels/AbstractModel.h"

#include <QDateTime>

namespace Playlist
{

void
multilevelLessThan::setSortScheme( const SortScheme & scheme )
{
    m_scheme = scheme;
    m_randomSalt = qrand();    //! Do a different random sort order every time.
}

bool
multilevelLessThan::operator()( const QAbstractItemModel* sourceModel,
                                int sourceModelRowA, int sourceModelRowB ) const
{
    // Handle "Last Played" as a special case because the time since last played is not
    // reported as an int in the data columns. Handle Title, Album, Artist as special
    // cases with Meta::Base::sortableName(). This is necessary in order to have the same
    // sort order policy regarding "The" in both the playlist and the collection browser.
    QSet< Playlist::Column > specialCases;
    specialCases << Playlist::LastPlayed << Playlist::Title << Playlist::Album
                 << Playlist::Artist << Playlist::AlbumArtist;

    foreach( const SortLevel &level, m_scheme )
    {
        const bool inverted = ( level.order() == Qt::DescendingOrder );
        const Playlist::Column currentCategory = level.category();

        if( currentCategory == Playlist::Shuffle )
        {
            long randomSeqnumA = constantRandomSeqnumForRow( sourceModel, sourceModelRowA );
            long randomSeqnumB = constantRandomSeqnumForRow( sourceModel, sourceModelRowB );

            // '!=' is the XOR operation; it simply negates the result if 'inverted'
            // is true. It isn't necessarry to do it this way, although later on it will
            // ease figuring out what's actually being returned.
            return ( randomSeqnumA < randomSeqnumB ) != inverted;
        }

        const QModelIndex indexA = sourceModel->index( sourceModelRowA, currentCategory );
        const QModelIndex indexB = sourceModel->index( sourceModelRowB, currentCategory );

        const Meta::TrackPtr trackA = indexA.data( TrackRole ).value<Meta::TrackPtr>();
        const Meta::TrackPtr trackB = indexB.data( TrackRole ).value<Meta::TrackPtr>();

        if( trackA && trackB && specialCases.contains( currentCategory ) )
        {
            switch( currentCategory )
            {
                case Playlist::LastPlayed:
                {
                    const QDateTime lastPlayedA = trackA->statistics()->lastPlayed();
                    const QDateTime lastPlayedB = trackB->statistics()->lastPlayed();

                    // The track with higher lastPlayed value was played more recently
                    if( lastPlayedA != lastPlayedB )
                        return ( lastPlayedA > lastPlayedB ) != inverted;

                    break;
                }
                case Playlist::Title:
                {
                    const int compareResult = compareBySortableName( trackA, trackB );
                    if( compareResult != 0 )
                        return ( compareResult < 0 ) != inverted;

                    break;
                }
                case Playlist::Album:
                {
                    const int compareResult
                            = compareBySortableName( trackA->album(), trackB->album() );

                    if( compareResult != 0 )
                        return ( compareResult < 0 ) != inverted;

                    // Fall through to sorting by album artist if albums have same name
                }
                case Playlist::AlbumArtist:
                {
                    const Meta::ArtistPtr artistA =
                            (trackA->album() ? trackA->album()->albumArtist() : Meta::ArtistPtr());

                    const Meta::ArtistPtr artistB =
                            (trackB->album() ? trackB->album()->albumArtist() : Meta::ArtistPtr());

                    const int compareResult = compareBySortableName( artistA, artistB );
                    if( compareResult != 0 )
                        return ( compareResult < 0 ) != inverted;

                    break;
                }
                case Playlist::Artist:
                {
                    const int compareResult
                            = compareBySortableName( trackA->artist(), trackB->artist() );

                    if( compareResult != 0 )
                        return ( compareResult < 0 ) != inverted;

                    break;
                }
                default:
                    warning() << "One of the cases in specialCases set has not received special treatment!";
                    break;
            }
        }
        else // either it's not a special case, or we don't have means (TrackPtrs) to handle it
        {
            const QVariant dataA = indexA.data( Qt::DisplayRole );
            const QVariant dataB = indexB.data( Qt::DisplayRole );

            if( level.isString() )
            {
                const int compareResult =
                        dataA.toString().compare(dataB.toString(),
                                                 Qt::CaseInsensitive);
                if( compareResult != 0 )
                    return ( compareResult < 0 ) != inverted;
            }
            else if( level.isFloat() )
            {
                if( dataA.toDouble() != dataB.toDouble() )
                    return ( dataA.toDouble() < dataB.toDouble() ) != inverted;
            }
            else // if it's neither a string nor a float ==> it's an integer
            {
                if( dataA.toInt() != dataB.toInt() )
                    return ( dataA.toInt() < dataB.toInt() ) != inverted;
            }
        }
    }

    // Tie breaker: order by row number
    return ( sourceModelRowA < sourceModelRowB );
}

template<typename T>
int
multilevelLessThan::compareBySortableName( const KSharedPtr<T> &left,
                                           const KSharedPtr<T> &right ) const
{
    if( !left && right )
        return -1;
    else if( left && !right )
        return 1;
    else if( left && right )
        return left->sortableName().compare( right->sortableName(),
                                             Qt::CaseInsensitive );
    return 0;
}

// If the 'qrand()' save+restore ever turns out to be a performance bottleneck: try
// switching to 'jrand48()', which has no common random pool and therefore doesn't have
// to be saved+restored.
//
// I chose qrand() because I'm not sure about 'jrand48()' portability.
typedef uint randomSeedType;    // For multilevelLessThan::constantRandomSeqnumForRow() 'qsrand()'

long
multilevelLessThan::constantRandomSeqnumForRow( const QAbstractItemModel* sourceModel, int sourceModelRow ) const
{
    randomSeedType randomSeedForItem;
    unsigned char *randomSeedForItem_bytes = (unsigned char*)( &randomSeedForItem );
    memset( randomSeedForItem_bytes, 0x00, sizeof(randomSeedType) );


    // Use the playlist item id as the fixed key for the random generator.
    //   This has all the properties we need:
    //     - unique
    //     - stable
    //
    //   Note that we do *NOT* assume the playlist item ids to be random. That happens
    //   to be the case in Amarok v2.3, but we work just as well if an item id is e.g.
    //   a C pointer or a linear number.
    //
    QModelIndex index = sourceModel->index( sourceModelRow, 0 );
    quint64 id = index.data( UniqueIdRole ).value<quint64>();

    const unsigned char *key = (const unsigned char *)( &id );
    unsigned int keyLen = sizeof(id);


    // Don't make any assumptions about the structure of the item key: treat it as bytes.
    for ( unsigned int i = 0; i < keyLen; i++ )
        randomSeedForItem_bytes[ i % sizeof(randomSeedType) ] ^= key[ i ];


    // Mix in our salt, to get a different sort order from run to run.
    const unsigned char *salt = (const unsigned char *)( &m_randomSalt );
    for ( unsigned int i = 0; i < sizeof(m_randomSalt); i++ )
        randomSeedForItem_bytes[ i % sizeof(randomSeedType) ] ^= salt[ i ];


    // Generate the fixed sequence number based on the fixed key:

    //   1. Save current non-predictable randomness.
    int globalSeed = qrand();

    //   2. (Ab)use the random generator as a hash function.
    qsrand( randomSeedForItem );    // Ensure we get the same random number for a given item every time
    long randomSeqnum = qrand();    // qrand() actually returns 'int'; use 'long' to allow switch to 'jrand48()'.

    //   3. Restore non-predictability for the rest of Amarok.
    qsrand( globalSeed );


    return randomSeqnum;
}

}   //namespace Playlist

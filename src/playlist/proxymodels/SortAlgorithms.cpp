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

    for( const SortLevel &level : m_scheme )
    {
        const bool inverted = ( level.order() == Qt::DescendingOrder );
        const Playlist::Column currentCategory = level.category();

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
                    //
                    // '!=' is the XOR operation; it simply negates the result if 'inverted'
                    // is true. It isn't necessary to do it this way, although later on it will
                    // ease figuring out what's actually being returned.
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
                    Q_FALLTHROUGH();
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
multilevelLessThan::compareBySortableName( const AmarokSharedPointer<T> &left,
                                           const AmarokSharedPointer<T> &right ) const
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

}   //namespace Playlist

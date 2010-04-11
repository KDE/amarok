/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2010 Nanno Langstraat <langstr@gmail.com>                              *
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

#include "AbstractModel.h"

namespace Playlist
{

void
multilevelLessThan::setSortScheme( const SortScheme & scheme )
{
    m_scheme = scheme;
    m_randomSalt = qrand();    //! Do a different random sort order every time.
}

bool
multilevelLessThan::operator()( const QAbstractItemModel* sourceModel, int sourceModelRowA, int sourceModelRowB ) const
{
    bool decided = false;
    bool verdict = false;    // Guaranteed to be overwritten

    for( int i = 0; i < m_scheme.length(); i++ )
    {
        int currentCategory = m_scheme.level( i ).category();  //see enum Column in PlaylistDefines.h

        if( currentCategory == -1 ) //random
        {
            long randomSeqnumA = constantRandomSeqnumForRow( sourceModel, sourceModelRowA );
            long randomSeqnumB = constantRandomSeqnumForRow( sourceModel, sourceModelRowB );

            if( randomSeqnumA < randomSeqnumB )
            {   decided = true;  verdict = true;   }
            else if( randomSeqnumA > randomSeqnumB )
            {   decided = true;  verdict = false;   }
        }
        else
        {
            QModelIndex indexA = sourceModel->index( sourceModelRowA, currentCategory );
            QModelIndex indexB = sourceModel->index( sourceModelRowB, currentCategory );

            QVariant dataA = indexA.data();
            QVariant dataB = indexB.data();

            //Handle "Last Played" as a special case because the time since last played is not
            //reported as an int in the data columns.
            //Also, the verdicts are inverted because I answer to the question about the time
            //since the track was played by comparing the absolute time when the track was last
            //played.
            if( m_scheme.level( i ).category() == Playlist::LastPlayed )
            {
                Meta::TrackPtr trackA = indexA.data( TrackRole ).value<Meta::TrackPtr>();
                Meta::TrackPtr trackB = indexB.data( TrackRole ).value<Meta::TrackPtr>();

                if( trackA->lastPlayed() < trackB->lastPlayed() )
                {   decided = true;  verdict = false;   }
                else if( trackA->lastPlayed() > trackB->lastPlayed() )
                {   decided = true;  verdict = true;   }
            }

            //And now the comparison logic for ordinary columns.
            else if( m_scheme.level( i ).isString() )
            {
                if( dataA.toString().toLower() < dataB.toString().toLower() )
                {   decided = true;  verdict = true;   }
                else if( dataA.toString().toLower() > dataB.toString().toLower() )
                {   decided = true;  verdict = false;   }
            }
            else if( m_scheme.level( i ).isFloat() )
            {
                if( dataA.toDouble() < dataB.toDouble() )
                {   decided = true;  verdict = true;   }
                else if( dataA.toDouble() > dataB.toDouble() )
                {   decided = true;  verdict = false;   }
            }
            else //if it's not a string ==> it's a number
            {
                if( dataA.toInt() < dataB.toInt() )
                {   decided = true;  verdict = true;   }
                else if( dataA.toInt() > dataB.toInt() )
                {   decided = true;  verdict = false;   }
            }
        }

        if( m_scheme.level( i ).order() == Qt::DescendingOrder )
            verdict = ( ! verdict );    // Reverse sort order

        if ( decided )
            break;
    }

    if ( ! decided )
        verdict = (sourceModelRowA < sourceModelRowB);    // Tie breaker: order by row number

    return verdict;
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

/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

bool
multilevelLessThan::operator()( int sourceModelRowA, int sourceModelRowB )
{
    bool decided = false;
    bool verdict = false;    // Guaranteed to be overwritten

    for( int i = 0; i < m_scheme.length(); i++ )
    {
        int currentCategory = m_scheme.level( i ).category();  //see enum Column in PlaylistDefines.h

        if( currentCategory == -1 ) //random
        {
            long randomSeqnumA = constantRandomSeqnumForRow( sourceModelRowA );
            long randomSeqnumB = constantRandomSeqnumForRow( sourceModelRowB );

            if( randomSeqnumA < randomSeqnumB )
            {   decided = true;  verdict = true;   }
            else if( randomSeqnumA > randomSeqnumB )
            {   decided = true;  verdict = false;   }
        }
        else
        {
            QModelIndex indexA = m_sourceModel->index( sourceModelRowA, currentCategory );
            QModelIndex indexB = m_sourceModel->index( sourceModelRowB, currentCategory );

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

long
multilevelLessThan::constantRandomSeqnumForRow(int sourceRow)
{
    // If the 'seed = qrand(); qsrand( seed )' save+restore ever turns out to be a
    // performance bottleneck: try switching to 'jrand48()', which has no common
    // random pool and therefore doesn't have to be saved+restored.
    int seed = qrand();

    qsrand( sourceRow ^ m_randomSalt );    // Ensure we get the same random number for a given row every time
    long randomSeqnum = qrand();    // qrand() is int; long to allow switch to 'jrand48()'.

    qsrand( seed );    // Restore non-predictability for the rest of Amarok

    return randomSeqnum;
}

}   //namespace Playlist

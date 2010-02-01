/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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
    quint8 verdict = 0;  //0 = false  1 = true  2 = nextIteration
    for( int i = 0; i < m_scheme.length(); i++ )
    {
        int currentCategory = m_scheme.level( i ).category();  //see enum Column in PlaylistDefines.h
        if( currentCategory == -1 ) //random
            return static_cast<bool>( qrand() % 2 );
        QVariant dataA = m_sourceModel->index( sourceModelRowA, currentCategory ).data();
        QVariant dataB = m_sourceModel->index( sourceModelRowB, currentCategory ).data();

        //Handle "Last Played" as a special case because the time since last played is not
        //reported as an int in the data columns.
        //Also, the verdicts are inverted because I answer to the question about the time
        //since the track was played by comparing the absolute time when the track was last
        //played.
        if( m_scheme.level( i ).category() == Playlist::LastPlayed )
        {
            Meta::TrackPtr trackA = dynamic_cast< AbstractModel * >( m_sourceModel )->trackAt( sourceModelRowA );
            Meta::TrackPtr trackB = dynamic_cast< AbstractModel * >( m_sourceModel )->trackAt( sourceModelRowB );
            if( trackA->lastPlayed() < trackB->lastPlayed() )
                verdict = 0;
            else if( trackA->lastPlayed() > trackB->lastPlayed() )
                verdict = 1;
            else
                verdict = 2;
        }

        //And now the comparison logic for ordinary columns.
        else if( m_scheme.level( i ).isString() )
        {
            if( dataA.toString().toLower() < dataB.toString().toLower() )
                verdict = 1;
            else if( dataA.toString().toLower() > dataB.toString().toLower() )
                verdict = 0;
            else
                verdict = 2;
        }
        else if( m_scheme.level( i ).isFloat() )
        {
            if( dataA.toDouble() < dataB.toDouble() )
                verdict = 1;
            else if( dataA.toDouble() > dataB.toDouble() )
                verdict = 0;
            else
                verdict = 2;
        }
        else //if it's not a string ==> it's a number
        {
            if( dataA.toInt() < dataB.toInt() )
                verdict = 1;
            else if( dataA.toInt() > dataB.toInt() )
                verdict = 0;
            else
                verdict = 2;
        }
        if( verdict != 2 )
        {
            if( m_scheme.level( i ).order() == Qt::DescendingOrder )
                verdict = verdict ? 0 : 1;
            break;
        }
        else
        {
            if( m_scheme.level( i ).order() == Qt::DescendingOrder )
                verdict = 0;
        }
    }
    if( verdict == 2 )
    {
        if( m_scheme.level( m_scheme.length() - 1 ).order() == Qt::DescendingOrder )
            verdict = 1;
        else
            verdict = 0;
    }
    return static_cast<bool>( verdict );
}

}   //namespace Playlist

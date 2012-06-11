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

#include "SynchronizeTracksJob.h"

#include "core/support/Debug.h"
#include "statsyncing/TrackTuple.h"

using namespace StatSyncing;

static const int denom = 20;
static const int fuzz = denom / 2;

SynchronizeTracksJob::SynchronizeTracksJob( const QList<TrackTuple> &tuples,
                                            const Options &options, QObject *parent )
    : Job( parent )
    , m_abort( false )
    , m_tuples( tuples )
    , m_options( options )
{
}

void
SynchronizeTracksJob::abort()
{
    m_abort = true;
}

void
SynchronizeTracksJob::run()
{
    DEBUG_BLOCK
    emit totalSteps( ( m_tuples.size() + fuzz ) / denom );

    int i = 0;
    foreach( TrackTuple tuple, m_tuples )
    {
        if( m_abort )
            break;

        // no point in checking for hasUpdate() here, synchronize() is witty enough
        tuple.synchronize( m_options );
        if( ( i + fuzz ) % denom == 0 )
            emit incrementProgress();
    }

    emit endProgressOperation( this );
}

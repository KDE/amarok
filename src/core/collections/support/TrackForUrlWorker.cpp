/****************************************************************************************
 * Copyright (c) 2009 Casey Link <unnamedrambler@gmail.com>                             *
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

#include "TrackForUrlWorker.h"

Amarok::TrackForUrlWorker::TrackForUrlWorker ( const KUrl &url ) : ThreadWeaver::Job(), mUrl ( url )
{
    connect ( this, SIGNAL ( done ( ThreadWeaver::Job* ) ), SLOT ( completeJob() ) );
}

Amarok::TrackForUrlWorker::TrackForUrlWorker( const QString &url ) : ThreadWeaver::Job(), mUrl ( KUrl ( url ) )
{
    connect ( this, SIGNAL ( done ( ThreadWeaver::Job* ) ), SLOT ( completeJob() ) );
}

Amarok::TrackForUrlWorker::~TrackForUrlWorker()
{}

void
Amarok::TrackForUrlWorker::completeJob()
{
    emit ( finishedLookup ( mTrack ) );
    deleteLater();
}

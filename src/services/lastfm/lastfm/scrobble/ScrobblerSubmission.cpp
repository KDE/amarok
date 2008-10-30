/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "ScrobblerSubmission.h"
#include "ScrobbleCache.h"
#include "Scrobbler.h"
#include "Scrobble.h"


void
ScrobblerSubmission::setTracks( const QList<Track>& tracks )
{
    m_tracks = tracks;
    // submit in chronological order
    qSort( m_tracks.begin(), m_tracks.end() );
}


void
ScrobblerSubmission::submitNextBatch()
{
    if (hasPendingRequests())
		// the tracks cannot be submitted at this time
		// if a parent Scrobbler exists, it will submit another batch
		// when the current one is done
		return;

    m_batch.clear(); //yep first
    m_data.clear();

    if (m_tracks.isEmpty())
        return;

    bool portable = false;
    for (int i = 0; i < 50 && !m_tracks.isEmpty(); ++i)
    {
        Scrobble t = m_tracks.takeFirst();
        
        QByteArray const N = QByteArray::number( i );
        #define e( x ) QUrl::toPercentEncoding( x )
        m_data += "&a[" + N + "]=" + e(t.artist()) +
                  "&t[" + N + "]=" + e(t.title()) +
                  "&i[" + N + "]=" + QByteArray::number( t.timestamp().toTime_t() ) +
                  "&o[" + N + "]=" + t.sourceString() +
                  "&r[" + N + "]=" + t.ratingCharacter() +
                  "&l[" + N + "]=" + QByteArray::number( t.duration() ) +
                  "&b[" + N + "]=" + e(t.album()) +
                  "&n[" + N + "]=" + QByteArray::number( t.trackNumber() ) +
                  "&m[" + N + "]=" + e(t.mbid());
        #undef e

        if (t.source() == Track::MediaDevice)
            portable = true;

        m_batch += t;
    }

    if (portable)
        m_data += "&portable=1";

    request();
}

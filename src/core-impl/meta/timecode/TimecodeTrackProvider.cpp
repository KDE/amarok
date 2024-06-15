/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#include "TimecodeTrackProvider.h"

#include "TimecodeMeta.h"

#include "QRegularExpression"

TimecodeTrackProvider::TimecodeTrackProvider()
{
}


TimecodeTrackProvider::~TimecodeTrackProvider()
{
}

bool TimecodeTrackProvider::possiblyContainsTrack( const QUrl &url ) const
{
    return url.url().contains( QRegularExpression(":\\d+-\\d+$") );
}

Meta::TrackPtr TimecodeTrackProvider::trackForUrl( const QUrl &url )
{
    QString urlString = url.url();

    QRegularExpression rx;
    rx.setPattern( "^(.+):(\\d+)-(\\d+)$" );
    QRegularExpressionMatch rmatch = rx.match( urlString );
    if( rmatch.hasMatch() )
    {
        QString baseUrlString = rmatch.captured(1);
        int start = rmatch.captured(2).toInt();
        int end = rmatch.captured(3).toInt();

        Meta::TimecodeTrack * track = new Meta::TimecodeTrack( "TimecodeTrack", QUrl( baseUrlString ), start, end );
        return Meta::TrackPtr( track );
    }
    return Meta::TrackPtr();
}



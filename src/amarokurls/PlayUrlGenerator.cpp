/***************************************************************************
*   Copyright (c) 2009  Casey Link <unnamedrambler@gmail.com>             *
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
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
***************************************************************************/

#include "PlayUrlGenerator.h"

#include "AmarokUrl.h"
#include "Debug.h"
#include "EngineController.h"
#include "MetaUtility.h"

PlayUrlGenerator::PlayUrlGenerator()
{
}


PlayUrlGenerator::~PlayUrlGenerator()
{
}

AmarokUrl PlayUrlGenerator::createCurrentTrackBookmark()
{
    Meta::TrackPtr track = The::engineController()->currentTrack();
    int seconds = The::engineController()->trackPosition();
    return createTrackBookmark( track, seconds );
}

AmarokUrl PlayUrlGenerator::createTrackBookmark( Meta::TrackPtr track, int seconds )
{
    AmarokUrl url;
    if( !track )
        return url;
    url.setCommand ( "play" );
    QString track_url = track->playableUrl().toEncoded().toBase64();
    url.appendArg ( track_url );
    url.appendArg ( QString::number ( seconds ) );
    url.setName( track->prettyName() + " - " + Meta::secToPrettyTime( seconds ) );
    debug() << "concocted url: " << url.url();
    return url;
}

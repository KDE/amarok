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

#include "PlayUrlGenerator.h"

#include "AmarokUrl.h"
#include "AmarokUrlHandler.h"
#include "Debug.h"
#include "EngineController.h"
#include "MetaUtility.h"

PlayUrlGenerator * PlayUrlGenerator::s_instance = 0;

PlayUrlGenerator * PlayUrlGenerator::instance()
{
    if( s_instance == 0)
        s_instance = new PlayUrlGenerator();

    return s_instance;
}

PlayUrlGenerator::PlayUrlGenerator()
{
}


PlayUrlGenerator::~PlayUrlGenerator()
{
    The::amarokUrlHandler()->unRegisterGenerator( this );
}

AmarokUrl
PlayUrlGenerator::createCurrentTrackBookmark()
{
    Meta::TrackPtr track = The::engineController()->currentTrack();
    const int seconds = The::engineController()->trackPosition();

    return createTrackBookmark( track, seconds );
}

AmarokUrl
PlayUrlGenerator::createTrackBookmark( Meta::TrackPtr track, int seconds, QString name )
{
    DEBUG_BLOCK
    AmarokUrl url;
    if( !track )
        return url;

    const QString trackUrl = track->playableUrl().toEncoded().toBase64();
    url.setCommand( "play" );
    url.setPath( trackUrl );
    url.appendArg( "pos", QString::number( seconds ) );

    if( name.isEmpty() )
        url.setName( track->prettyName() + " - " + Meta::secToPrettyTime( seconds ) );
    else
        url.setName( name + " - " + Meta::secToPrettyTime( seconds ) );

    debug() << "concocted url: " << url.url();
    debug() << "pos: " << seconds;
    return url;
}

QString
PlayUrlGenerator::description()
{
    return i18n( "Bookmark Track Position" );
}

AmarokUrl
PlayUrlGenerator::createUrl()
{
    return createCurrentTrackBookmark();
}


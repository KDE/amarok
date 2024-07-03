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
#include "core/meta/Meta.h"
#include "core/meta/support/MetaUtility.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "BookmarkModel.h"

#include <KLocalizedString>

PlayUrlGenerator * PlayUrlGenerator::s_instance = nullptr;

PlayUrlGenerator * PlayUrlGenerator::instance()
{
    if( s_instance == nullptr)
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
    const qint64 miliseconds = The::engineController()->trackPositionMs();

    return createTrackBookmark( track, miliseconds );
}

AmarokUrl
PlayUrlGenerator::createTrackBookmark( Meta::TrackPtr track, qint64 miliseconds, const QString &name )
{
    DEBUG_BLOCK

    const int seconds = miliseconds / 1000;
    const qreal accurateSeconds = (qreal) miliseconds / 1000.0;
    QString secondsString = QString::number( accurateSeconds );

    AmarokUrl url;
    if( !track )
        return url;

    const QString trackUrl = QLatin1String( track->playableUrl().toEncoded().toBase64() );
    url.setCommand( QStringLiteral("play") );
    url.setPath( trackUrl );
    url.setArg( QStringLiteral("pos"), secondsString );

    if( name.isEmpty() )
        url.setName( track->prettyName() + QStringLiteral(" - ") + Meta::secToPrettyTime( seconds ) );
    else
        url.setName( name + QStringLiteral(" - ") + Meta::secToPrettyTime( seconds ) );

    debug() << "concocted url: " << url.url();
    debug() << "pos: " << accurateSeconds;
    return url;
}

void
PlayUrlGenerator::moveTrackBookmark( Meta::TrackPtr track, qint64 newMiliseconds, const QString &name )
{
    qreal seconds = qreal ( newMiliseconds ) / 1000;
    QString trackPosition;
    trackPosition.setNum( seconds );

    QString trackName = track->prettyName();
    QString newName = ( trackName + QStringLiteral(" - ") + Meta::msToPrettyTime( newMiliseconds ) );

    BookmarkModel::instance()->setBookmarkArg( name, QStringLiteral("pos"), trackPosition );
    BookmarkModel::instance()->renameBookmark( name, newName );
}

QString
PlayUrlGenerator::description()
{
    return i18n( "Bookmark Track Position" );
}

QIcon PlayUrlGenerator::icon()
{
    return QIcon::fromTheme( QStringLiteral("x-media-podcast-amarok") );
}

AmarokUrl
PlayUrlGenerator::createUrl()
{
    return createCurrentTrackBookmark();
}


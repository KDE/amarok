/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "BookmarkMetaActions.h"

#include "EngineController.h"
#include "SvgHandler.h"
#include "amarokurls/AmarokUrlHandler.h"
#include "amarokurls/BookmarkModel.h"
#include "core/meta/Meta.h"
#include "core-impl/capabilities/timecode/TimecodeWriteCapability.h"
#include "widgets/ProgressWidget.h"

#include <QIcon>
#include <KLocalizedString>

BookmarkAlbumAction::BookmarkAlbumAction( QObject *parent, const Meta::AlbumPtr &album )
    : QAction( i18n( "Bookmark this Album" ), parent )
    , m_album( album )
{
    connect( this, &BookmarkAlbumAction::triggered, this, &BookmarkAlbumAction::slotTriggered );
    setIcon( QIcon::fromTheme(QStringLiteral("bookmark-new")) );
    setProperty( "popupdropper_svg_id", QStringLiteral("lastfm") );
}

void
BookmarkAlbumAction::slotTriggered()
{
    The::amarokUrlHandler()->bookmarkAlbum( m_album );
}


BookmarkArtistAction::BookmarkArtistAction( QObject *parent, const Meta::ArtistPtr &artist )
    : QAction( i18n( "Bookmark this Artist" ), parent )
    , m_artist( artist )
{
    connect( this, &BookmarkArtistAction::triggered, this, &BookmarkArtistAction::slotTriggered );
    setIcon( QIcon::fromTheme(QStringLiteral("bookmark-new")) );
    setProperty( "popupdropper_svg_id", QStringLiteral("lastfm") );
}

void
BookmarkArtistAction::slotTriggered()
{
    The::amarokUrlHandler()->bookmarkArtist( m_artist );
}

BookmarkCurrentTrackPositionAction::BookmarkCurrentTrackPositionAction( QObject * parent )
    : QAction( i18n( "Add Position Marker" ), parent )
{
    connect( this, &BookmarkCurrentTrackPositionAction::triggered, this, &BookmarkCurrentTrackPositionAction::slotTriggered );
    setIcon( QIcon::fromTheme(QStringLiteral("flag-amarok")) );
}

void
BookmarkCurrentTrackPositionAction::slotTriggered()
{
    DEBUG_BLOCK

    Meta::TrackPtr track = The::engineController()->currentTrack();
    const qint64 miliseconds = The::engineController()->trackPositionMs();

    if ( track && track->has<Capabilities::TimecodeWriteCapability>() )
    {
        debug() << " has WriteTimecode  ";
        QScopedPointer<Capabilities::TimecodeWriteCapability> tcw( track->create<Capabilities::TimecodeWriteCapability>() );
        tcw->writeTimecode( miliseconds );
    }
}




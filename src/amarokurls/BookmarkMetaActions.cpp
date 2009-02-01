/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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

#include "BookmarkMetaActions.h"
#include "AmarokUrlHandler.h"
#include "SvgHandler.h"

#include "BookmarkModel.h"
#include "EngineController.h"
#include "meta/capabilities/TimecodeWriteCapability.h"
#include "PlayUrlRunner.h"
#include "PlayUrlGenerator.h"
#include "ProgressSlider.h"

#include <KIcon>
#include <KLocale>

BookmarkAlbumAction::BookmarkAlbumAction( QObject *parent, Meta::AlbumPtr album )
    : PopupDropperAction( i18n( "Bookmark this Album" ), parent )
    , m_album( album )
{
    connect( this, SIGNAL( triggered( bool ) ), SLOT( slotTriggered() ) );
    setIcon( KIcon("bookmark-new") );
    setRenderer( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ) );
    setElementId( "lastfm" );
}

void BookmarkAlbumAction::slotTriggered()
{
    The::amarokUrlHandler()->bookmarkAlbum( m_album );
}


BookmarkArtistAction::BookmarkArtistAction( QObject *parent, Meta::ArtistPtr artist )
    : PopupDropperAction( i18n( "Bookmark this Artist" ), parent )
    , m_artist( artist )
{
    connect( this, SIGNAL( triggered( bool ) ), SLOT( slotTriggered() ) );
    setIcon( KIcon("bookmark-new") );
    setRenderer( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ) );
    setElementId( "lastfm" );
}

void BookmarkArtistAction::slotTriggered()
{
    The::amarokUrlHandler()->bookmarkArtist( m_artist );
}

BookmarkCurrentTrackPositionAction::BookmarkCurrentTrackPositionAction( QObject * parent )
    : PopupDropperAction( i18n( "Bookmark Position" ), parent )
{
    connect( this, SIGNAL( triggered( bool ) ), SLOT( slotTriggered() ) );
    setIcon( KIcon("flag-amarok") );
}

void
BookmarkCurrentTrackPositionAction::slotTriggered()
{
    DEBUG_BLOCK
    PlayUrlGenerator urlGenerator;
    Meta::TrackPtr track = The::engineController()->currentTrack();
    int seconds = The::engineController()->trackPosition();
    if ( track && track->hasCapabilityInterface( Meta::Capability::WriteTimecode ) )
    {
        debug() << " has WriteTimecode  ";
        Meta::TimecodeWriteCapability *tcw = track->as<Meta::TimecodeWriteCapability>();
        tcw->writeTimecode( seconds );
    }
}

#include "BookmarkMetaActions.moc"



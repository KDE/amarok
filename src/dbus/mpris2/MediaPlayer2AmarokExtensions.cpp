/***********************************************************************
 * Copyright 2010  Canonical Ltd
 *   (author: Aurelien Gateau <aurelien.gateau@canonical.com>)
 * Copyright 2012  Alex Merry <alex.merry@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************/

#include "MediaPlayer2AmarokExtensions.h"

#include "core/support/Debug.h"
#include "EngineController.h"
#include "playlist/PlaylistActions.h"

using namespace Amarok;

MediaPlayer2AmarokExtensions::MediaPlayer2AmarokExtensions(QObject* parent)
    : DBusAbstractAdaptor(parent)
{
    connect( The::engineController(), &EngineController::muteStateChanged,
             this, &MediaPlayer2AmarokExtensions::mutedChanged );
}

MediaPlayer2AmarokExtensions::~MediaPlayer2AmarokExtensions()
{
}

bool MediaPlayer2AmarokExtensions::Muted() const
{
    return The::engineController()->isMuted();

}

void MediaPlayer2AmarokExtensions::setMuted( bool muted )
{
    The::engineController()->setMuted( muted );
}

void MediaPlayer2AmarokExtensions::StopAfterCurrent()
{
    The::playlistActions()->stopAfterPlayingTrack();
}

void MediaPlayer2AmarokExtensions::AdjustVolume( double increaseBy )
{
    int volume = The::engineController()->volume() + (increaseBy * 100);
    if (volume < 0)
        volume = 0;
    if (volume > 100)
        volume = 100;
    The::engineController()->setVolume( volume );
}

void MediaPlayer2AmarokExtensions::mutedChanged( bool newValue )
{
    signalPropertyChange( QStringLiteral("Muted"), newValue );
}


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

#include "PlayUrlRunner.h"

#include "Debug.h"
#include "AmarokUrl.h"
#include "AmarokUrlHandler.h"
#include "collection/CollectionManager.h"
#include "EngineController.h"
#include "playlist/PlaylistController.h"

PlayUrlRunner::PlayUrlRunner() : AmarokUrlRunnerBase()
{
}

PlayUrlRunner::~PlayUrlRunner()
{
    The::amarokUrlHandler()->unRegisterRunner ( this );
}

bool PlayUrlRunner::run ( AmarokUrl url )
{
    DEBUG_BLOCK
    if( url.numberOfArgs() == 0)
        return false;

    QString track_url = QUrl::fromPercentEncoding( url.arg(0).toUtf8() );
    debug() << "decoded url: " << track_url;
    int pos = url.arg(1).toInt() * 1000;
    debug() << "seek pos: " << pos;
    Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( track_url );
    if( !track )
        return false;

//     The::playlistController()->insertOptioned( track, Playlist::AppendAndPlay );
    The::engineController()->play(track, pos);
//     The::engineController()->seek(pos);
    return true;
}

QString PlayUrlRunner::command() const
{
    return "play";
}

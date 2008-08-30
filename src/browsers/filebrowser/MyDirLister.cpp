/*
    Copyright (c) 2008 Dan Meltzer <hydrogen@notyetimplemented.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "MyDirLister.h"

#include "Debug.h"
#include "PlaylistManager.h"


MyDirLister::~MyDirLister()
{
    DEBUG_BLOCK
}

bool
MyDirLister::matchesFilter( const KFileItem &item ) const
{
    if( item.isHidden() )
        return false;

    return
        item.isDir() ||
        EngineController::canDecode( item.url() ) || 
        item.url().protocol() == "audiocd" ||
        PlaylistManager::isPlaylist( item.url() ) ||
        item.name().endsWith( ".mp3", Qt::CaseInsensitive ) || //for now this is less confusing for the user
        item.name().endsWith( ".aa", Qt::CaseInsensitive ) ||  //for adding to iPod
        item.name().endsWith( ".mp4", Qt::CaseInsensitive ) || //for adding to iPod
        item.name().endsWith( ".m4v", Qt::CaseInsensitive ) || //for adding to iPod
        item.name().endsWith( ".m4b", Qt::CaseInsensitive );   //for adding to iPod
}

#include "MyDirLister.moc"

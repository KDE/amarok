/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "PlaylistManager.h"
#include "TheInstances.h"

PlaylistManager * PlaylistManager::s_instance = 0;

PlaylistManager*
The::playlistManager()
{
    return PlaylistManager::instance();
}

PlaylistManager::PlaylistManager()
{}

PlaylistManager::~PlaylistManager()
{}

PlaylistManager *
PlaylistManager::instance()
{
    if ( s_instance == 0 )
        s_instance = new PlaylistManager();

    return s_instance;
}

void
PlaylistManager::addProvider( int playlistCategory, PlaylistProvider * provider )
{
    m_map.insert( playlistCategory, provider );
}

void
PlaylistManager::addCustomProvider( int customCategory, PlaylistProvider * provider )
{
    m_map.insert( customCategory, provider );
    if ( !m_customCategories.contains( customCategory ) )
    {
        m_customCategories << customCategory;
        //notify PlaylistBrowser of new custom category.
    }
}

Meta::PlaylistList
PlaylistManager::playlistsOfCategory( int playlistCategory )
{
    QList<PlaylistProvider *> providers = m_map.values( playlistCategory );
    QListIterator<PlaylistProvider *> i( providers );

    Meta::PlaylistList list;
    while ( i.hasNext() )
        list << i.next()->playlists();

    return list;
}

#include "PlaylistManager.moc"

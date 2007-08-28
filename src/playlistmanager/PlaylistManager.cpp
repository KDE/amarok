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

#include "debug.h"

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
PlaylistManager::addProvider( PlaylistProvider * provider, PlaylistCategory category )
{
    DEBUG_BLOCK
    m_map.insert( category, provider );
    connect( provider, SIGNAL(updated()), SLOT(slotUpdated( /*PlaylistProvider **/ )) );
    emit(updated());
}

void
PlaylistManager::addCustomProvider( PlaylistProvider * provider, int customCategory )
{
    m_map.insert( customCategory, provider );
    if ( !m_customCategories.contains( customCategory ) )
    {
        m_customCategories << customCategory;
        //notify PlaylistBrowser of new custom category.
    }
    connect( provider, SIGNAL(updated()), SLOT(slotUpdated( /*PlaylistProvider **/ )) );
    emit(updated());
}


void
PlaylistManager::slotUpdated( /*PlaylistProvider * provider*/ )
{
    DEBUG_BLOCK
    emit(updated());
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

PlaylistProvider *
PlaylistManager::playlistProvider(int category, QString name)
{
    QList<PlaylistProvider *> providers( m_map.values( category ) );

    QListIterator<PlaylistProvider *> i(providers);
    while( i.hasNext() )
    {
        PlaylistProvider * p = static_cast<PlaylistProvider *>( i.next() );
        if( p->prettyName() == name )
            return p;
    }

    return 0;
}

#include "PlaylistManager.moc"

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

#include "ContextStatusBar.h"
#include "PlaylistManager.h"
#include "TheInstances.h"
#include "debug.h"

#include <kio/jobclasses.h>
#include <kio/job.h>
#include <KLocale>

PlaylistManager * PlaylistManager::s_instance = 0;

PlaylistManager*
The::playlistManager()
{
    return PlaylistManager::instance();
}

bool
PlaylistManager::isPlaylist( const KUrl & path )
{
    const QString ext = Amarok::extension( path.fileName() );

    if( ext == "m3u" ) return true;
    if( ext == "pls" ) return true;
    if( ext == "ram" ) return true;
    if( ext == "smil") return true;
    if( ext == "asx" || ext == "wax" ) return true;
    if( ext == "xml" ) return true;
    if( ext == "xspf" ) return true;

    return false;
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
PlaylistManager::addProvider( PlaylistProvider * provider, int category )
{
    DEBUG_BLOCK

    bool newCategory = false;
    if( !m_map.uniqueKeys().contains( category ) )
            newCategory = true;

    m_map.insert( category, provider );
    connect( provider, SIGNAL(updated()), SLOT(slotUpdated( /*PlaylistProvider **/ )) );

    if( newCategory )
        emit( categoryAdded( category ) );
}

int
PlaylistManager::registerCustomCategory( const QString & name )
{
    int typeNumber = Custom + m_customCategories.size() + 1;

    //TODO: find the name in the configfile, might have been registered before.
    m_customCategories[typeNumber] = name;
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

void
PlaylistManager::downloadPlaylist( const KUrl & path, Meta::PlaylistPtr playlist )
{
    DEBUG_BLOCK

    KIO::StoredTransferJob * downloadJob =  KIO::storedGet( path );

    m_downloadJobMap[downloadJob] = playlist;

    connect( downloadJob, SIGNAL( result( KJob * ) ),
             this, SLOT( downloadComplete( KJob * ) ) );

    Amarok::ContextStatusBar::instance()->newProgressOperation( downloadJob )
            .setDescription( i18n( "Downloading Playlist" ) );
}

void
PlaylistManager::downloadComplete( KJob * job )
{
    DEBUG_BLOCK

    if ( !job->error() == 0 )
    {
        //TODO: error handling here
        return ;
    }

    Meta::PlaylistPtr playlist = m_downloadJobMap.take( job );

    QString contents = static_cast<KIO::StoredTransferJob *>(job)->data();
    QTextStream stream;
    stream.setString( &contents );

    playlist->load( stream );

}

QString PlaylistManager::typeName(int playlistCategory)
{
    switch( playlistCategory )
    {
        case CurrentPlaylist: return i18n("Current Playlist");
        case UserPlaylist: return i18n("My Playlists");
        case PodcastChannel: return i18n("Podcasts");
        case Dynamic: return i18n("Dynamic Playlists");
        case SmartPlaylist: return i18n("Smart Playlist");
    }
    //if control reaches here playlistCategory is either invalid or a custom category
    if( m_customCategories.contains( playlistCategory ) )
        return m_customCategories[playlistCategory];
    else
        //note: this shouldn't happen so I'm not translating it to facilitate bug reports
        return QString("!!!Invalid Playlist Category!!!");
}

#include "PlaylistManager.moc"

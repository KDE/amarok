/****************************************************************************************
 * Copyright (c) 2010 Bart Cerneels <bart.cerneels@kde.org>                             *
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
#include "UmsPodcastProvider.h"

#include <KMimeType>

#include <QDirIterator>
#include <QObject>

using namespace Meta;

UmsPodcastProvider::UmsPodcastProvider( UmsHandler *handler, QString scanDirectory )
        : m_handler( handler )
        , m_scanDirectory( scanDirectory )
{
}

UmsPodcastProvider::~UmsPodcastProvider()
{

}

bool
UmsPodcastProvider::possiblyContainsTrack( const KUrl &url ) const
{
    return false;
}

TrackPtr
UmsPodcastProvider::trackForUrl( const KUrl &url )
{
    return TrackPtr();
}

void
UmsPodcastProvider::addPodcast( const KUrl &url )
{
}

PodcastChannelPtr
UmsPodcastProvider::addChannel( PodcastChannelPtr channel )
{
    return PodcastChannelPtr();
}

PodcastEpisodePtr
UmsPodcastProvider::addEpisode( Meta::PodcastEpisodePtr episode )
{
    return PodcastEpisodePtr();
}

PodcastChannelList
UmsPodcastProvider::channels()
{
    return PodcastChannelList();
}

void
UmsPodcastProvider::removeSubscription( Meta::PodcastChannelPtr channel )
{
}

void
UmsPodcastProvider::configureProvider()
{
}

void
UmsPodcastProvider::configureChannel( Meta::PodcastChannelPtr channel )
{
}

QString
UmsPodcastProvider::prettyName() const
{
    return i18nc( "Podcasts on a media device", "Podcasts on %1" )
            .arg( m_handler->prettyName() );
}

KIcon
UmsPodcastProvider::icon() const
{
    return KIcon("drive-removable-media-usb-pendrive");
}

PlaylistList
UmsPodcastProvider::playlists()
{
    PlaylistList playlists;
    foreach( PodcastChannelPtr channel, channels() )
        playlists << PlaylistPtr::dynamicCast( channel );
}

QList<QAction *>
UmsPodcastProvider::episodeActions( Meta::PodcastEpisodeList )
{
    return QList<QAction *>();
}

QList<QAction *>
UmsPodcastProvider::channelActions( Meta::PodcastChannelList )
{
    return QList<QAction *>();
}

QList<QAction *>
UmsPodcastProvider::playlistActions( Meta::PlaylistPtr playlist )
{
    Q_UNUSED( playlist )
    return QList<QAction *>();
}

QList<QAction *>
UmsPodcastProvider::trackActions( Meta::PlaylistPtr playlist,                                              int trackIndex )
{
    Q_UNUSED( playlist)
    Q_UNUSED( trackIndex )
    return QList<QAction *>();
}

void
UmsPodcastProvider::completePodcastDownloads()
{

}

void
UmsPodcastProvider::updateAll() //slot
{
}

void
UmsPodcastProvider::update( Meta::PodcastChannelPtr channel ) //slot
{

}

void
UmsPodcastProvider::downloadEpisode( Meta::PodcastEpisodePtr episode ) //slot
{

}

void
UmsPodcastProvider::deleteDownloadedEpisode( Meta::PodcastEpisodePtr episode ) //slot
{

}

void
UmsPodcastProvider::slotUpdated() //slot
{

}

void
UmsPodcastProvider::scan()
{
    if( m_scanDirectory.isEmpty() )
        return;
    m_dirList.clear();
    QDirIterator it( m_scanDirectory, QDirIterator::Subdirectories );
    while( it.hasNext() )
        addPath( it.next() );
}

int
UmsPodcastProvider::addPath( const QString &path )
{
    DEBUG_BLOCK
    int acc = 0;
    KMimeType::Ptr mime = KMimeType::findByFileContent( path, &acc );
    if( !mime || mime->name() == KMimeType::defaultMimeType() )
    {
        debug() << "Trying again with findByPath:" ;
        mime = KMimeType::findByPath( path, 0, true, &acc );
        if( mime->name() == KMimeType::defaultMimeType() )
            return 0;
    }
    debug() << "Got type: " << mime->name() << "For file: " << path << ", with accuracy: " << acc;

    QFileInfo info( path );
    if( info.isDir() )
    {
        if( m_dirList.contains( path ) )
            return 0;
        m_dirList << info.canonicalPath();
        return 1;
    }
    else if( info.isFile() )
    {
        foreach( const QString &mimetype, m_handler->mimetypes() )
        {
            if( mime->is( mimetype ) )
            {
                addFile( MetaFile::TrackPtr( new MetaFile::Track(
                        KUrl( info.canonicalFilePath() ) ) ) );
                return 2;
            }
        }
    }

    return 0;
}

void
UmsPodcastProvider::addFile( MetaFile::TrackPtr metafileTrack )
{
    //wrap the track into a proxy and parse it's album to use as channel title
}

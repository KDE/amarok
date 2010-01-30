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
#include "Debug.h"

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
    UmsPodcastChannelPtr umsChannel = UmsPodcastChannelPtr(
            new UmsPodcastChannel( channel, this ) );
    m_umsChannels << umsChannel;
    return PodcastChannelPtr::dynamicCast( umsChannel );
}

PodcastEpisodePtr
UmsPodcastProvider::addEpisode( PodcastEpisodePtr episode )
{
    return PodcastEpisodePtr();
}

PodcastChannelList
UmsPodcastProvider::channels()
{
    return UmsPodcastChannel::toPodcastChannelList( m_umsChannels );
}

void
UmsPodcastProvider::removeSubscription( PodcastChannelPtr channel )
{
    UmsPodcastChannelPtr umsChannel = UmsPodcastChannelPtr::dynamicCast( channel );
    if( umsChannel.isNull() )
    {
        error() << "trying to remove a podcast channel of the wrong type";
        return;
    }

    if( !m_umsChannels.contains( umsChannel ) )
    {
        error() << "trying to remove a podcast channel that is not in the list";
        return;
    }

    m_umsChannels.removeAll( umsChannel );
}

void
UmsPodcastProvider::configureProvider()
{
}

void
UmsPodcastProvider::configureChannel( PodcastChannelPtr channel )
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
    foreach( UmsPodcastChannelPtr channel, m_umsChannels )
        playlists << PlaylistPtr::dynamicCast( channel );
    debug() << "there are " << playlists.count() << " channels";
    return playlists;
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
    debug() << "scan directory for podcasts: " << m_scanDirectory;
    QDirIterator it( m_scanDirectory, QDirIterator::Subdirectories );
    while( it.hasNext() )
        addPath( it.next() );
}

int
UmsPodcastProvider::addPath( const QString &path )
{
    DEBUG_BLOCK
    int acc = 0;
    debug() << path;
    KMimeType::Ptr mime = KMimeType::findByFileContent( path, &acc );
    if( !mime || mime->name() == KMimeType::defaultMimeType() )
    {
        debug() << "Trying again with findByPath:" ;
        mime = KMimeType::findByPath( path, 0, true, &acc );
        if( mime->name() == KMimeType::defaultMimeType() )
            return 0;
    }
    debug() << "Got type: " << mime->name() << ", with accuracy: " << acc;

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
    DEBUG_BLOCK
    debug() << metafileTrack->playableUrl().url();
    debug() << "album: " << metafileTrack->album()->name();
    debug() << "title: " << metafileTrack->name();
    if( metafileTrack->album()->name().isEmpty() )
    {
        debug() << "Can't figure out channel without album tag.";
        return;
    }

    if( metafileTrack->name().isEmpty() )
    {
        debug() << "Can not use a track without a title.";
        return;
    }

    //see if there is already a UmsPodcastEpisode for this track
    UmsPodcastChannelPtr channel;
    UmsPodcastEpisodePtr episode;

    foreach( UmsPodcastChannelPtr c, m_umsChannels )
    {
        if( c->name() == metafileTrack->album()->name() )
        {
            channel = c;
            break;
        }
    }

    if( channel )
    {
        foreach( UmsPodcastEpisodePtr e, channel->umsEpisodes() )
        {
            if( e->title() == metafileTrack->name() )
            {
                episode = e;
                break;
            }
        }
    }
    else
    {
        debug() << "there is no channel for this episode yet";
        channel = UmsPodcastChannelPtr( new UmsPodcastChannel( this ) );
        channel->setTitle( metafileTrack->album()->name() );
        m_umsChannels << channel;
        emit( updated() );
    }

    if( episode.isNull() )
    {
        debug() << "this episode was not found in an existing channel";
        episode = UmsPodcastEpisodePtr( new UmsPodcastEpisode( channel ) );
        episode->setLocalFile( metafileTrack );

        channel->addUmsEpisode( episode );
    }

    episode->setLocalFile( metafileTrack );
}

/****************************************************************************************
 * Copyright (c) 2009 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "core/podcasts/PodcastImageFetcher.h"

#include "core/support/Debug.h"

#include <QCryptographicHash>
#include <QNetworkConfigurationManager>

#include <KIO/Job>

PodcastImageFetcher::PodcastImageFetcher()
{
}

void
PodcastImageFetcher::addChannel( Podcasts::PodcastChannelPtr channel )
{
    DEBUG_BLOCK
    if( channel->imageUrl().isEmpty() )
    {
        debug() << channel->title() << " does not have an imageUrl";
        return;
    }

    if( hasCachedImage( channel ) )
    {
        debug() << "using cached image for " << channel->title();
        QString imagePath = cachedImagePath( channel ).toLocalFile();
        QImage image( imagePath );
        if( image.isNull() )
            error() << "could not load pixmap from " << imagePath;
        else
            channel->setImage( image );
        return;
    }

    if( m_channels.contains( channel ) )
    {
        debug() << "Channel already queued:" << channel->title();
        return;
    }

    if( m_jobChannelMap.values().contains( channel ) )
    {
        debug() << "Copy job already running for channel:" << channel->title();
        return;
    }

    debug() << "Adding " << channel->title() << " to fetch queue";
    m_channels.append( channel );
}

void
PodcastImageFetcher::addEpisode( Podcasts::PodcastEpisodePtr episode )
{
    Q_UNUSED( episode );
}

QUrl
PodcastImageFetcher::cachedImagePath( Podcasts::PodcastChannelPtr channel )
{
    return cachedImagePath( channel.data() );
}

QUrl
PodcastImageFetcher::cachedImagePath( Podcasts::PodcastChannel *channel )
{
    QUrl imagePath = channel->saveLocation();
    if( imagePath.isEmpty() || !imagePath.isLocalFile() )
        imagePath = QUrl::fromLocalFile( Amarok::saveLocation( "podcasts" ) );
    QCryptographicHash md5( QCryptographicHash::Md5 );
    md5.addData( channel->url().url().toLocal8Bit() );
    QString extension = Amarok::extension( channel->imageUrl().fileName() );
    imagePath = imagePath.adjusted( QUrl::StripTrailingSlash );
    imagePath.setPath( imagePath.path() + QLatin1Char('/') + ( md5.result().toHex() + '.' + extension ) );
    return imagePath;
}

bool
PodcastImageFetcher::hasCachedImage( Podcasts::PodcastChannelPtr channel )
{
    DEBUG_BLOCK
    return QFile( PodcastImageFetcher::cachedImagePath(
            Podcasts::PodcastChannelPtr::dynamicCast( channel ) ).toLocalFile() ).exists();
}

void
PodcastImageFetcher::run()
{
    if( m_channels.isEmpty() && m_episodes.isEmpty()
        && m_jobChannelMap.isEmpty() && m_jobEpisodeMap.isEmpty() )
    {
        //nothing to do
        emit( done( this ) );
        return;
    }

    QNetworkConfigurationManager mgr;
    if( !mgr.isOnline() )
    {
        debug() << "QNetworkConfigurationManager reports we are not online, canceling podcast image download";
        emit( done( this ) );
        //TODO: schedule another run after Solid reports we are online again
        return;
    }

    foreach( Podcasts::PodcastChannelPtr channel, m_channels )
    {
        QUrl cachedPath = cachedImagePath( channel );
        KIO::mkdir( cachedPath.adjusted(QUrl::RemoveFilename|QUrl::StripTrailingSlash) );
        KIO::FileCopyJob *job = KIO::file_copy( channel->imageUrl(), cachedPath,
                                -1, KIO::HideProgressInfo | KIO::Overwrite );
        //remove channel from the todo list
        m_channels.removeAll( channel );
        m_jobChannelMap.insert( job, channel );
        connect( job, &KIO::FileCopyJob::finished, this, &PodcastImageFetcher::slotDownloadFinished );
    }

    //TODO: episodes
}

void
PodcastImageFetcher::slotDownloadFinished( KJob *job )
{
    DEBUG_BLOCK

    //QMap::take() also removes the entry from the map.
    Podcasts::PodcastChannelPtr channel = m_jobChannelMap.take( job );
    if( channel.isNull() )
    {
        error() << "got null PodcastChannelPtr " << __FILE__ << ":" << __LINE__;
        return;
    }

    if( job->error() )
    {
        error() << "downloading podcast image " << job->errorString();
    }
    else
    {
        QString imagePath = cachedImagePath( channel ).toLocalFile();
        QImage image( imagePath );
        if( image.isNull() )
            error() << "could not load pixmap from " << imagePath;
        else
            channel->setImage( image );
    }

    //call run again to start the next batch of transfers.
    run();
}

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

#include "PodcastImageFetcher.h"

#include "Debug.h"

#include <KIO/Job>
#include <KMD5>
#include <Solid/Networking>

PodcastImageFetcher::PodcastImageFetcher()
{
}

void
PodcastImageFetcher::addChannel( Meta::PodcastChannelPtr channel )
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
        QPixmap pixmap( imagePath );
        if( pixmap.isNull() )
            error() << "could not load pixmap from " << imagePath;
        else
            channel->setImage( pixmap );
        return;
    }

    debug() << "Adding " << channel->title() << " to fetch queue";
    m_channels.append( channel );
}

void
PodcastImageFetcher::addEpisode( Meta::PodcastEpisodePtr episode )
{
    Q_UNUSED( episode );
}

KUrl
PodcastImageFetcher::cachedImagePath( Meta::PodcastChannelPtr channel )
{
    KUrl imagePath = channel->saveLocation();
    if( imagePath.isEmpty() )
        imagePath = Amarok::saveLocation( "podcasts" );
    KMD5 md5( channel->url().url().toLocal8Bit() );
    QString extension = Amarok::extension( channel->imageUrl().fileName() );
    imagePath.addPath( md5.hexDigest() + "." + extension );
    debug() << "imagePath for " << channel->title() << " " << imagePath;
    return imagePath.toLocalFile();
}

bool
PodcastImageFetcher::hasCachedImage( Meta::PodcastChannelPtr channel )
{
    DEBUG_BLOCK
    return QFile( PodcastImageFetcher::cachedImagePath(
            Meta::PodcastChannelPtr::dynamicCast( channel ) ).toLocalFile() ).exists();
}

void
PodcastImageFetcher::run()
{
    DEBUG_BLOCK
    if( m_channels.isEmpty() && m_episodes.isEmpty() )
    {
        //nothing to do
        emit( done( this ) );
        return;
    }

    if( Solid::Networking::status() != Solid::Networking::Connected
        && Solid::Networking::status() != Solid::Networking::Unknown )
    {
        debug() << "Solid reports we are not online, canceling podcast image download";
        emit( done( this ) );
        return;
    }

    foreach( Meta::PodcastChannelPtr channel, m_channels )
    {
        debug() << "Download image from " << channel->imageUrl();
        KUrl cachedPath = cachedImagePath( channel );
        KIO::mkdir( cachedPath.directory() );
        KIO::FileCopyJob *job = KIO::file_copy( channel->imageUrl(), cachedPath,
                                -1, KIO::HideProgressInfo | KIO::Overwrite );
        //remove channel from the todo list
        m_channels.removeAll( channel );
        m_jobChannelMap.insert( job, channel );
        connect( job, SIGNAL( finished( KJob * ) ), SLOT( slotDownloadFinished( KJob * ) ) );
    }

    //TODO: episodes
}

void
PodcastImageFetcher::slotDownloadFinished( KJob *job )
{
    DEBUG_BLOCK

    //QMap::take() also removes the entry from the map.
    Meta::PodcastChannelPtr channel = m_jobChannelMap.take( job );
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
        QPixmap pixmap( imagePath );
        if( pixmap.isNull() )
            error() << "could not load pixmap from " << imagePath;
        else
            channel->setImage( pixmap );
    }

    //call run again to start the next batch of transfers.
    run();
}

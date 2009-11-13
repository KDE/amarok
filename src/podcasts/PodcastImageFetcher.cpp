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

PodcastImageFetcher::PodcastImageFetcher()
{
}

void
PodcastImageFetcher::addChannel( Meta::PodcastChannelPtr channel )
{
    if( channel->imageUrl().isEmpty() )
        return;

    m_channels << channel;
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
    imagePath.addPath( md5.hexDigest() + ".png" );
    debug() << "imagePath for " << channel->title() << " " << imagePath;
    return imagePath.toLocalFile();
}

void
PodcastImageFetcher::run()
{
    DEBUG_BLOCK
    if( m_jobChannelMap.isEmpty() && m_jobEpisodeMap.isEmpty() )
    {
        //nothing to do
        emit( done( this ) );
        return;
    }

    foreach( Meta::PodcastChannelPtr channel, m_channels )
    {
        debug() << "Download image from " << channel->imageUrl();
        KIO::FileCopyJob *job =
                KIO::file_copy( channel->imageUrl(), cachedImagePath( channel ),
                                -1, KIO::HideProgressInfo );
        m_jobChannelMap.insert( job, channel );
        connect( job, SIGNAL( finished( KJob * ) ), SLOT( slotDownloadFinished( KJob * ) ) );
    }

    //TODO: episodes
}

void
PodcastImageFetcher::slotDownloadFinished( KJob *job )
{
    DEBUG_BLOCK

    if( job->error() )
    {
        error() << "downloading podcast image " << job->errorString();
    }
    else if( m_jobChannelMap.contains( job ) )
    {
        Meta::PodcastChannelPtr channel = m_jobChannelMap.value( job );
        QPixmap pixmap( cachedImagePath( channel ).toLocalFile() );
        channel->setImage( pixmap );
    }

    //call run again to start the next batch of transfers.
    run();
}

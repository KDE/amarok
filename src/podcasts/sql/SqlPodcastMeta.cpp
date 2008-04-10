/* This file is part of the KDE project
   Copyright (C) 20078 Bart Cerneels <bart.cerneels@kde.org>

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

#include "SqlPodcastMeta.h"
#include "SqlPodcastProvider.h"
#include "SqlStorage.h"

Meta::SqlPodcastEpisode::SqlPodcastEpisode( const QStringList &result )
    : Meta::PodcastEpisode()
    , m_batchUpdate( false )
{
    QStringList::ConstIterator iter = result.constBegin();
    m_id = (*(iter++)).toInt();
    m_url = KUrl( *(iter++) );
    m_sqlChannel = SqlPodcastChannelPtr::dynamicCast(
        SqlPodcastProvider::instance()->podcastChannelForId( (*(iter++)).toInt() ) );
    m_localUrl = KUrl( *(iter++) );
    m_guid = *(iter++);
    m_title = *(iter++);
    m_subtitle = *(iter++);
    m_sequenceNumber = (*(iter++)).toInt();
    m_description = *(iter++);
    m_mimeType = *(iter++);
    m_pubDate = *(iter++);
    m_duration = (*(iter++)).toInt();
    m_size = (*(iter++)).toInt();
    m_isNew = SqlPodcastProvider::instance()->sqlStorage()->boolTrue() == (*(iter++));

    Q_ASSERT_X( iter == result.constEnd(), "SqlPodcastEpisode( PodcastCollection*, QStringList )", "number of expected fields did not match number of actual fields" );
}

Meta::SqlPodcastEpisode::SqlPodcastEpisode( Meta::PodcastEpisodePtr episode )
    : Meta::PodcastEpisode()
{
    m_url = episode->url();
    m_sqlChannel = SqlPodcastChannelPtr::dynamicCast( episode->channel() );
    m_localUrl = episode->localUrl();
    m_guid = episode->guid();
    m_title = episode->title();
}

Meta::SqlPodcastChannel::SqlPodcastChannel( const QStringList &result )
    : Meta::PodcastChannel()
{
    QStringList::ConstIterator iter = result.constBegin();
    m_id = (*(iter++)).toInt();
    m_url = KUrl( *(iter++) );
    m_title = *(iter++);
    m_link = *(iter++);
    QString imageUrl = *(iter++);
    m_description = *(iter++);
    m_copyright = *(iter++);
    m_directory = KUrl( *(iter++) );
    m_labels = QStringList( *(iter++) );
    m_autoScan = SqlPodcastProvider::instance()->sqlStorage()->boolTrue() == *(iter++);
    m_fetchType = (*(iter++)).toInt() == DownloadWhenAvailable ?
        DownloadWhenAvailable : StreamOrDownloadOnDemand;
    m_autoTransfer = SqlPodcastProvider::instance()->sqlStorage()->boolTrue() == *(iter++);
    m_hasPurge = SqlPodcastProvider::instance()->sqlStorage()->boolTrue() == *(iter++);
    m_purgeCount = (*(iter++)).toInt();
}

Meta::SqlPodcastChannel::SqlPodcastChannel( PodcastChannelPtr channel )
    : Meta::PodcastChannel()
{
    m_url = channel->url();
    m_title = channel->title();
    m_link = channel->webLink();
    m_description = channel->description();
    m_copyright = channel->copyright();
    m_labels = channel->labels();
}

#include "SqlPodcastMeta.moc"

/* This file is part of the KDE project
   Copyright (C) 2008 Bart Cerneels <bart.cerneels@kde.org>

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

#include "CollectionManager.h"
#include "Debug.h"
#include "SqlPodcastProvider.h"
#include "SqlStorage.h"

#include <QDate>

Meta::SqlPodcastEpisode::SqlPodcastEpisode( const QStringList &result, Meta::SqlPodcastChannelPtr sqlChannel )
    : Meta::PodcastEpisode( Meta::PodcastChannelPtr::staticCast( sqlChannel ) )
    , m_batchUpdate( false )
    , m_sqlChannel( sqlChannel )
{
    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();
    QStringList::ConstIterator iter = result.constBegin();
    m_dbId = (*(iter++)).toInt();
    m_url = KUrl( *(iter++) );
    int channelId = (*(iter++)).toInt();
    Q_UNUSED( channelId );
    m_localUrl = KUrl( *(iter++) );
    m_guid = *(iter++);
    m_title = *(iter++);
    m_subtitle = *(iter++);
    m_sequenceNumber = (*(iter++)).toInt();
    m_description = *(iter++);
    m_mimeType = *(iter++);
    m_pubDate = *(iter++);
    m_duration = (*(iter++)).toInt();
    m_fileSize = (*(iter++)).toInt();
    m_isNew = sqlStorage->boolTrue() == (*(iter++));

    Q_ASSERT_X( iter == result.constEnd(), "SqlPodcastEpisode( PodcastCollection*, QStringList )", "number of expected fields did not match number of actual fields" );
}

Meta::SqlPodcastEpisode::SqlPodcastEpisode( Meta::PodcastEpisodePtr episode )
    : Meta::PodcastEpisode()
{
    m_url = KUrl( episode->uidUrl() );
    m_sqlChannel = SqlPodcastChannelPtr::dynamicCast( episode->channel() );
    setChannel( episode->channel() );

    if ( !m_sqlChannel ) {
        debug() << "invalid m_sqlChannel";
        debug() <<  episode->channel()->title();
    }

    m_localUrl = episode->localUrl();
    m_title = episode->title();
    m_guid = episode->guid();

    //commit to the database
    updateInDb();
}

Meta::SqlPodcastEpisode::~SqlPodcastEpisode()
{
    updateInDb();
}

void
Meta::SqlPodcastEpisode::updateInDb()
{
    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();

    QString boolTrue = sqlStorage->boolTrue();
    QString boolFalse = sqlStorage->boolFalse();
    #define escape(x) sqlStorage->escape(x)
    QString insert = "INSERT INTO podcastepisodes("
    "url,channel,localurl,guid,title,subtitle,sequencenumber,description,"
    "mimetype,pubdate,duration,filesize,isnew) "
    "VALUES ( '%1','%2','%3','%4','%5','%6',%7,'%8','%9','%10',%11,%12,%13 );";

    QString update = "UPDATE podcastepisodes "
    "SET url='%1',channel=%2,localurl='%3',guid='%4',title='%5',subtitle='%6',"
    "sequencenumber=%7,description='%8',mimetype='%9',pubdate='%10',"
    "duration=%11,filesize=%12,isnew=%13 WHERE id=%14;";
    //if we don't have a database ID yet we should insert
    QString command = m_dbId ? update : insert;

    command = command.arg( escape(m_url.url()) );
    command = command.arg( m_sqlChannel->dbId() );
    command = command.arg( escape(m_localUrl.url()) );
    command = command.arg( escape(m_guid) );
    command = command.arg( escape(m_title) );
    command = command.arg( escape(m_subtitle) );
    command = command.arg( QString::number(m_sequenceNumber) );
    command = command.arg( escape(m_description) );
    command = command.arg( escape(m_mimeType) );
    command = command.arg( escape(m_pubDate) );
    command = command.arg( QString::number(m_duration) );
    command = command.arg( QString::number(m_fileSize) );
    command = command.arg( m_isNew ? boolTrue : boolFalse );

    if( m_dbId )
        sqlStorage->query( command.arg( m_dbId ) );
    else
        m_dbId = sqlStorage->insert( command, "podcastepisodes" );
}

Meta::SqlPodcastChannel::SqlPodcastChannel( const QStringList &result )
    : Meta::PodcastChannel()
{
    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();
    QStringList::ConstIterator iter = result.constBegin();
    m_dbId = (*(iter++)).toInt();
    m_url = KUrl( *(iter++) );
    m_title = *(iter++);
    m_webLink = *(iter++);
    QString imageUrl = *(iter++);
    m_description = *(iter++);
    m_copyright = *(iter++);
    m_directory = KUrl( *(iter++) );
    m_labels = QStringList( *(iter++) );
    m_subscribeDate = QDate::fromString( *(iter++) );
    m_autoScan = sqlStorage->boolTrue() == *(iter++);
    m_fetchType = (*(iter++)).toInt() == DownloadWhenAvailable ? DownloadWhenAvailable : StreamOrDownloadOnDemand;
    m_purge = sqlStorage->boolTrue() == *(iter++);
    m_purgeCount = (*(iter++)).toInt();
    loadEpisodes();
}

Meta::SqlPodcastChannel::SqlPodcastChannel( PodcastChannelPtr channel )
    : Meta::PodcastChannel()
    , m_dbId( 0 )
{
    m_url = channel->url();
    m_title = channel->title();
    m_webLink = channel->webLink();
    m_description = channel->description();
    m_copyright = channel->copyright();
    m_labels = channel->labels();
    m_subscribeDate = channel->subscribeDate();

    //Default Settings
    m_directory = KUrl( Amarok::saveLocation("podcasts") );
    m_directory.addPath( m_title );
    m_autoScan = true;
    m_fetchType = StreamOrDownloadOnDemand;
    m_purge = false;
    m_purgeCount = 10;

    updateInDb();

    m_episodes = channel->episodes();

    foreach ( Meta::PodcastEpisodePtr episode, channel->episodes() ) {
        episode->setChannel( PodcastChannelPtr( this ) );
        SqlPodcastEpisode * sqlEpisode = new SqlPodcastEpisode( episode );

        m_episodes << PodcastEpisodePtr( sqlEpisode );
        m_sqlEpisodes << SqlPodcastEpisodePtr( sqlEpisode );
    }
}

Meta::SqlPodcastChannel::~SqlPodcastChannel()
{
    updateInDb();
}

void
Meta::SqlPodcastChannel::updateInDb()
{
    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();

    QString boolTrue = sqlStorage->boolTrue();
    QString boolFalse = sqlStorage->boolFalse();
    #define escape(x) sqlStorage->escape(x)
    QString insert = "INSERT INTO podcastchannels("
    "url,title,weblink,image,description,copyright,directory,labels,"
    "subscribedate,autoscan,fetchtype,haspurge,purgecount) "
    "VALUES ( '%1','%2','%3','%4','%5','%6','%7','%8','%9',%10,%11,%12,%13 );";

    QString update = "UPDATE podcastchannels SET url='%1',title='%2'"
    ",weblink='%3',image='%4',description='%5',copyright='%6',directory='%7'"
    ",labels='%8',subscribedate='%9',autoscan=%10,fetchtype=%11,haspurge=%12,"
    "purgecount=%13 WHERE id=%14;";
    //if we don't have a database ID yet we should insert;
    QString command = m_dbId ? update : insert;

    command = command.arg( escape(m_url.url()) );
    command = command.arg( escape(m_title) );
    command = command.arg( escape(m_webLink.url()) );
    //TODO:m_image.url()
    command = command.arg( escape(QString("")) ); //image
    command = command.arg( escape(m_description) );
    command = command.arg( escape(m_copyright) );
    command = command.arg( escape(m_directory.url()) );
    //TODO: QStringList -> comma separated QString
    QString labels = QString("");
    command = command.arg( escape(labels) );
    command = command.arg( escape(m_subscribeDate.toString()) );
    command = command.arg( m_autoScan ? boolTrue : boolFalse );
    command = command.arg( QString::number(m_fetchType) );
    command = command.arg( m_purge ? boolTrue : boolFalse );
    command = command.arg( QString::number(m_purgeCount) );

    if( m_dbId )
        sqlStorage->query( command.arg( m_dbId ) );
    else
        m_dbId = sqlStorage->insert( command, "podcastchannels" );
}

void
Meta::SqlPodcastChannel::loadEpisodes()
{
    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();

    QStringList results = sqlStorage->query( QString("SELECT id, url, channel, localurl, guid, title, subtitle, sequencenumber, description, mimetype, pubdate, duration, filesize, isnew FROM podcastepisodes WHERE channel = %1;").arg( m_dbId ) );

    int rowLength = 14;
    for(int i=0; i < results.size(); i+=rowLength)
    {
        QStringList episodesResult = results.mid( i, rowLength );
        SqlPodcastEpisode *sqlEpisode = new SqlPodcastEpisode( episodesResult, SqlPodcastChannelPtr( this ) );
        m_sqlEpisodes << SqlPodcastEpisodePtr( sqlEpisode );
        m_episodes << PodcastEpisodePtr( sqlEpisode );
    }
}

#include "SqlPodcastMeta.moc"

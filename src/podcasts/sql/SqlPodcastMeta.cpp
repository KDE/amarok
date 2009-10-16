/****************************************************************************************
 * Copyright (c) 2008 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "SqlPodcastMeta.h"

#include "amarokurls/BookmarkMetaActions.h"
#include "amarokurls/PlayUrlRunner.h"
#include "CollectionManager.h"
#include "Debug.h"
#include "meta/capabilities/CurrentTrackActionsCapability.h"
#include "meta/capabilities/TimecodeLoadCapability.h"
#include "meta/capabilities/TimecodeWriteCapability.h"
#include "SqlPodcastProvider.h"
#include "SqlStorage.h"

#include <QDate>
#include <QFile>

class TimecodeWriteCapabilityPodcastImpl : public Meta::TimecodeWriteCapability
{
    public:
        TimecodeWriteCapabilityPodcastImpl( Meta::PodcastEpisode *episode )
            : Meta::TimecodeWriteCapability()
            , m_episode( episode )
        {}

    virtual bool writeTimecode ( int seconds )
    {
        DEBUG_BLOCK
        return Meta::TimecodeWriteCapability::writeTimecode( seconds,
                Meta::TrackPtr::dynamicCast( m_episode ) );
    }

    virtual bool writeAutoTimecode ( int seconds )
    {
        DEBUG_BLOCK
        return Meta::TimecodeWriteCapability::writeAutoTimecode( seconds,
                Meta::TrackPtr::dynamicCast( m_episode ) );
    }

    private:
        Meta::PodcastEpisodePtr m_episode;
};

class TimecodeLoadCapabilityPodcastImpl : public Meta::TimecodeLoadCapability
{
    public:
        TimecodeLoadCapabilityPodcastImpl( Meta::PodcastEpisode *episode )
        : Meta::TimecodeLoadCapability()
        , m_episode( episode )
        {
            DEBUG_BLOCK
            debug() << "episode: " << m_episode->name();
        }

        virtual bool hasTimecodes()
        {
            if ( loadTimecodes().size() > 0 )
                return true;
            return false;
        }

        virtual BookmarkList loadTimecodes()
        {
            DEBUG_BLOCK
            if ( m_episode && m_episode->playableUrl().isValid() )
            {
                BookmarkList list = PlayUrlRunner::bookmarksFromUrl( m_episode->playableUrl() );
                return list;
            }
            else
                return BookmarkList();
        }

    private:
        Meta::PodcastEpisodePtr m_episode;
};

Meta::SqlPodcastEpisode::SqlPodcastEpisode( const QStringList &result, Meta::SqlPodcastChannelPtr sqlChannel )
    : Meta::PodcastEpisode( Meta::PodcastChannelPtr::staticCast( sqlChannel ) )
    , m_batchUpdate( false )
    , m_channel( sqlChannel )
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
    m_pubDate = QDateTime::fromString( *(iter++), Qt::ISODate );
    m_duration = (*(iter++)).toInt();
    m_fileSize = (*(iter++)).toInt();
    m_isNew = sqlStorage->boolTrue() == (*(iter++));

    Q_ASSERT_X( iter == result.constEnd(), "SqlPodcastEpisode( PodcastCollection*, QStringList )", "number of expected fields did not match number of actual fields" );

    if( !m_localUrl.isEmpty() && QFileInfo( m_localUrl.toLocalFile() ).exists() )
    {
        m_localFile = new MetaFile::Track( m_localUrl );
    }
}

Meta::SqlPodcastEpisode::SqlPodcastEpisode( Meta::PodcastEpisodePtr episode )
    : Meta::PodcastEpisode()
    , m_dbId( 0 )
{
    m_url = KUrl( episode->uidUrl() );
    m_channel = SqlPodcastChannelPtr::dynamicCast( episode->channel() );

    if ( !m_channel && episode->channel()) {
        debug() << "BUG: creating SqlEpisode but not an sqlChannel!!!";
        debug() <<  episode->channel()->title();
    }

    m_localUrl = episode->localUrl();
    m_title = episode->title();
    m_guid = episode->guid();
    m_pubDate = episode->pubDate();

    //commit to the database
    updateInDb();

    if( !m_localUrl.isEmpty() && QFileInfo( m_localUrl.toLocalFile() ).exists() )
    {
        m_localFile = new MetaFile::Track( m_localUrl );
    }
}

Meta::SqlPodcastEpisode::~SqlPodcastEpisode()
{
}

void
Meta::SqlPodcastEpisode::setLocalUrl( const KUrl &url )
{
    m_localUrl = url;

    if( m_localUrl.isEmpty() )
    {
        if( !m_localFile.isNull() )
        {
            m_localFile.clear();
            return;
        }
    }
    else
    {
        //if we had a local file previously it will get deleted by the KSharedPtr.
        m_localFile = new MetaFile::Track( m_localUrl );
        return;
    }
}

qint64
Meta::SqlPodcastEpisode::length() const
{
    //if downloaded get the duration from the file, else use the value read from the feed
    if( m_localFile.isNull() )
        return m_duration * 1000;

    return m_localFile->length();
}

bool
Meta::SqlPodcastEpisode::hasCapabilityInterface( Meta::Capability::Type type ) const
{
    switch( type )
    {
        case Meta::Capability::CurrentTrackActions:
        case Meta::Capability::WriteTimecode:
        case Meta::Capability::LoadTimecode:
            //only downloaded episodes can be position marked
//            return !localUrl().isEmpty();
            return true;
            //TODO: downloaded episodes can be edited
        case Meta::Capability::Editable:
            return isEditable();

        default:
            return false;
    }
}

Meta::Capability*
Meta::SqlPodcastEpisode::createCapabilityInterface( Meta::Capability::Type type )
{
    switch( type )
    {
        case Meta::Capability::CurrentTrackActions:
        {
            QList< QAction * > actions;
            QAction* flag = new BookmarkCurrentTrackPositionAction( 0 );
            actions << flag;
            return new Meta::CurrentTrackActionsCapability( actions );
        }
        case Meta::Capability::WriteTimecode:
            return new TimecodeWriteCapabilityPodcastImpl( this );
        case Meta::Capability::LoadTimecode:
            return new TimecodeLoadCapabilityPodcastImpl( this );
        case Meta::Capability::Editable:
            if( !m_localFile.isNull() )
                return m_localFile->createCapabilityInterface( type );
        default:
            return 0;
    }
}

bool
Meta::SqlPodcastEpisode::isEditable() const
{
     if( m_localFile.isNull() )
         return false;

     return m_localFile->isEditable();
}

QString
Meta::SqlPodcastEpisode::name() const
{
    if( m_localFile.isNull() )
        return m_title;

    return m_localFile->name();
}

QString
Meta::SqlPodcastEpisode::prettyName() const
{
    /*for now just do the same as name, but in the future we might want to used a cleaned
      up string using some sort of regex tag rewrite for podcasts. decapitateString on
      steroides. */
    return name();
}

void
Meta::SqlPodcastEpisode::setTitle( const QString &title )
{
    if( !m_localFile.isNull() )
    {
        m_localFile->setTitle( title );
    }

    m_title = title;
}

Meta::AlbumPtr
Meta::SqlPodcastEpisode::album() const
{
    if( m_localFile.isNull() )
        return m_albumPtr;

    return m_localFile->album();
}

Meta::ArtistPtr
Meta::SqlPodcastEpisode::artist() const
{
    if( m_localFile.isNull() )
        return m_artistPtr;

    return m_localFile->artist();
}

Meta::ComposerPtr
Meta::SqlPodcastEpisode::composer() const
{
    if( m_localFile.isNull() )
        return m_composerPtr;

    return m_localFile->composer();
}

Meta::GenrePtr
Meta::SqlPodcastEpisode::genre() const
{
    if( m_localFile.isNull() )
        return m_genrePtr;

    return m_localFile->genre();
}

Meta::YearPtr
Meta::SqlPodcastEpisode::year() const
{
    if( m_localFile.isNull() )
        return m_yearPtr;

    return m_localFile->year();
}

void
Meta::SqlPodcastEpisode::beginMetaDataUpdate()
{
    if( !m_localFile.isNull() && m_localFile->isEditable() )
        m_localFile->beginMetaDataUpdate();
}

void
Meta::SqlPodcastEpisode::endMetaDataUpdate()
{
    if( !m_localFile.isNull() && m_localFile->isEditable() )
        m_localFile->endMetaDataUpdate();
}
void
Meta::SqlPodcastEpisode::abortMetaDataUpdate()
{
    if( !m_localFile.isNull() && m_localFile->isEditable() )
        m_localFile->abortMetaDataUpdate();
}

void
Meta::SqlPodcastEpisode::updateInDb()
{
    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();

    QString boolTrue = sqlStorage->boolTrue();
    QString boolFalse = sqlStorage->boolFalse();
    #define escape(x) sqlStorage->escape(x)
    QString command;
    QTextStream stream( &command );
    if( m_dbId )
    {
        stream << "UPDATE podcastepisodes ";
        stream << "SET url='";
        stream << escape(m_url.url());
        stream << "', channel=";
        stream << m_channel->dbId();
        stream << ", localurl='";
        stream << escape(m_localUrl.url());
        stream << "', guid='";
        stream << escape(m_guid);
        stream << "', title='";
        stream << escape(m_title);
        stream << "', subtitle='";
        stream << escape(m_subtitle);
        stream << "', sequencenumber=";
        stream << m_sequenceNumber;
        stream << ", description='";
        stream << escape(m_description);
        stream << "', mimetype='";
        stream << escape(m_mimeType);
        stream << "', pubdate='";
        stream << escape(m_pubDate.toString(Qt::ISODate));
        stream << "', duration=";
        stream << m_duration;
        stream << ", filesize=";
        stream << m_fileSize;
        stream << ", isnew=";
        stream << (m_isNew ? boolTrue : boolFalse);
        stream << " WHERE id=";
        stream << m_dbId;
        stream << ";";
        sqlStorage->query( command );
    }
    else
    {
        stream << "INSERT INTO podcastepisodes (";
        stream << "url,channel,localurl,guid,title,subtitle,sequencenumber,description,";
        stream << "mimetype,pubdate,duration,filesize,isnew) ";
        stream << "VALUES ( '";
        stream << escape(m_url.url()) << "', ";
        stream << m_channel->dbId() << ", '";
        stream << escape(m_localUrl.url()) << "', '";
        stream << escape(m_guid) << "', '";
        stream << escape(m_title) << "', '";
        stream << escape(m_subtitle) << "', ";
        stream << m_sequenceNumber << ", '";
        stream << escape(m_description) << "', '";
        stream << escape(m_mimeType) << "', '";
        stream << escape(m_pubDate.toString(Qt::ISODate)) << "', ";
        stream << m_duration << ", ";
        stream << m_fileSize << ", ";
        stream << (m_isNew ? boolTrue : boolFalse);
        stream << ");";
        m_dbId = sqlStorage->insert( command, "podcastepisodes" );
    }
}

void
Meta::SqlPodcastEpisode::deleteFromDb()
{
    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();
    sqlStorage->query(
        QString( "DELETE FROM podcastepisodes WHERE id = %1;" ).arg( dbId() ) );
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
    m_directory.addPath( Amarok::vfatPath( m_title ) );
    m_autoScan = true;
    m_fetchType = StreamOrDownloadOnDemand;
    m_purge = false;
    m_purgeCount = 10;

    updateInDb();

    foreach ( Meta::PodcastEpisodePtr episode, channel->episodes() ) {
        episode->setChannel( PodcastChannelPtr( this ) );
        SqlPodcastEpisode * sqlEpisode = new SqlPodcastEpisode( episode );

        m_episodes << SqlPodcastEpisodePtr( sqlEpisode );
    }
}

Meta::SqlPodcastChannel::~SqlPodcastChannel()
{
    m_episodes.clear();
}

Meta::TrackList
Meta::SqlPodcastChannel::sqlEpisodesToTracks( Meta::SqlPodcastEpisodeList episodes )
{
    Meta::TrackList tracks;
    foreach( Meta::SqlPodcastEpisodePtr sqlEpisode, episodes )
        tracks << Meta::TrackPtr::dynamicCast( sqlEpisode );

    return tracks;
}

Meta::PodcastEpisodeList
Meta::SqlPodcastChannel::sqlEpisodesToPodcastEpisodes( SqlPodcastEpisodeList episodes )
{
    Meta::PodcastEpisodeList sqlEpisodes;
    foreach( Meta::SqlPodcastEpisodePtr sqlEpisode, episodes )
        sqlEpisodes << Meta::PodcastEpisodePtr::dynamicCast( sqlEpisode );

    return sqlEpisodes;
}

Meta::PodcastEpisodeList
Meta::SqlPodcastChannel::episodes()
{
    return sqlEpisodesToPodcastEpisodes( m_episodes );
}

Meta::PodcastEpisodePtr
Meta::SqlPodcastChannel::addEpisode( PodcastEpisodePtr episode )
{
    DEBUG_BLOCK
    debug() << "adding episode " << episode->title() << " to sqlchannel " << title();
    SqlPodcastEpisodePtr sqlEpisode = SqlPodcastEpisodePtr( new SqlPodcastEpisode( episode ) );

    //episodes are sorted on pubDate high to low
    SqlPodcastEpisodeList::iterator i;
    for( i = m_episodes.begin() ; i != m_episodes.end() ; ++i )
    {
        if( sqlEpisode->pubDate() > (*i)->pubDate() )
        {
            m_episodes.insert( i, sqlEpisode );
            break;
        }
    }

    //insert in case the list is empty or at the end of the list
    if( i == m_episodes.end() )
        m_episodes << sqlEpisode;


    return PodcastEpisodePtr::dynamicCast( sqlEpisode );
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

    command = command.arg( escape(m_url.url()) ); //%1
    command = command.arg( escape(m_title) ); //%2
    command = command.arg( escape(m_webLink.url()) ); //%3
    //TODO:m_image.url()
    command = command.arg( escape(QString("")) ); //image  //%4
    command = command.arg( escape(m_description) ); //%5
    command = command.arg( escape(m_copyright) ); //%6
    command = command.arg( escape(m_directory.url()) ); //%7
    //TODO: QStringList -> comma separated QString
    QString labels = QString("");
    command = command.arg( escape(labels) ); //%8
    command = command.arg( escape(m_subscribeDate.toString()) ); //%9
    command = command.arg( m_autoScan ? boolTrue : boolFalse ); //%10
    command = command.arg( QString::number(m_fetchType) ); //%11
    command = command.arg( m_purge ? boolTrue : boolFalse ); //%12
    command = command.arg( QString::number(m_purgeCount) ); //%13

    if( m_dbId )
        sqlStorage->query( command.arg( m_dbId ) );
    else
        m_dbId = sqlStorage->insert( command, "podcastchannels" );
}

void
Meta::SqlPodcastChannel::deleteFromDb()
{
    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();
    foreach( Meta::SqlPodcastEpisodePtr sqlEpisode, m_episodes )
    {
       sqlEpisode->deleteFromDb();
       m_episodes.removeOne( sqlEpisode );
    }

    sqlStorage->query(
        QString( "DELETE FROM podcastchannels WHERE id = %1;" ).arg( dbId() ) );
}

void
Meta::SqlPodcastChannel::loadEpisodes()
{
    m_episodes.clear();

    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();

    //if purge is enabled is true we limit the number of results
    QString command;
    if( hasPurge() )
    {
        command = QString( "SELECT id, url, channel, localurl, guid, "
        "title, subtitle, sequencenumber, description, mimetype, pubdate, "
        "duration, filesize, isnew FROM podcastepisodes WHERE channel = %1 "
        "ORDER BY pubdate DESC LIMIT " + QString::number( purgeCount() ) + ';' );
    }
    else
    {
        command = QString( "SELECT id, url, channel, localurl, guid, "
            "title, subtitle, sequencenumber, description, mimetype, pubdate, "
            "duration, filesize, isnew FROM podcastepisodes WHERE channel = %1 "
            "ORDER BY pubdate DESC;" );
    }

    QStringList results = sqlStorage->query( command.arg( m_dbId ) );

    int rowLength = 14;
    for(int i=0; i < results.size(); i+=rowLength)
    {
        QStringList episodesResult = results.mid( i, rowLength );
        SqlPodcastEpisode *episode = new SqlPodcastEpisode( episodesResult, SqlPodcastChannelPtr( this ) );
        m_episodes << SqlPodcastEpisodePtr( episode );
    }
}


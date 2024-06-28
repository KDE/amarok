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
#include "core/capabilities/ActionsCapability.h"
#include <core/storage/SqlStorage.h>
#include "core/meta/TrackEditor.h"
#include "core/support/Debug.h"
#include "core-impl/capabilities/timecode/TimecodeLoadCapability.h"
#include "core-impl/capabilities/timecode/TimecodeWriteCapability.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/storage/StorageManager.h"
#include "core-impl/meta/proxy/MetaProxy.h"
#include "core-impl/meta/file/FileTrackProvider.h"
#include "core-impl/podcasts/sql/SqlPodcastProvider.h"

#include <QDate>
#include <QFile>

using namespace Podcasts;

static FileTrackProvider myFileTrackProvider; // we need it to be available for lookups

class TimecodeWriteCapabilityPodcastImpl : public Capabilities::TimecodeWriteCapability
{
    public:
        TimecodeWriteCapabilityPodcastImpl( Podcasts::PodcastEpisode *episode )
            : Capabilities::TimecodeWriteCapability()
            , m_episode( episode )
        {}

    bool writeTimecode ( qint64 miliseconds ) override
    {
        DEBUG_BLOCK
        return Capabilities::TimecodeWriteCapability::writeTimecode( miliseconds,
                Meta::TrackPtr::dynamicCast( m_episode ) );
    }

    bool writeAutoTimecode ( qint64 miliseconds ) override
    {
        DEBUG_BLOCK
        return Capabilities::TimecodeWriteCapability::writeAutoTimecode( miliseconds,
                Meta::TrackPtr::dynamicCast( m_episode ) );
    }

    private:
        Podcasts::PodcastEpisodePtr m_episode;
};

class TimecodeLoadCapabilityPodcastImpl : public Capabilities::TimecodeLoadCapability
{
    public:
        TimecodeLoadCapabilityPodcastImpl( Podcasts::PodcastEpisode *episode )
        : Capabilities::TimecodeLoadCapability()
        , m_episode( episode )
        {
            DEBUG_BLOCK
            debug() << "episode: " << m_episode->name();
        }

        bool hasTimecodes() override
        {
            if ( loadTimecodes().size() > 0 )
                return true;
            return false;
        }

        BookmarkList loadTimecodes() override
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
        Podcasts::PodcastEpisodePtr m_episode;
};

Meta::TrackList
SqlPodcastEpisode::toTrackList( Podcasts::SqlPodcastEpisodeList episodes )
{
    Meta::TrackList tracks;
    for( SqlPodcastEpisodePtr sqlEpisode : episodes )
        tracks << Meta::TrackPtr::dynamicCast( sqlEpisode );

    return tracks;
}

Podcasts::PodcastEpisodeList
SqlPodcastEpisode::toPodcastEpisodeList( SqlPodcastEpisodeList episodes )
{
    Podcasts::PodcastEpisodeList sqlEpisodes;
    for( SqlPodcastEpisodePtr sqlEpisode : episodes )
        sqlEpisodes << Podcasts::PodcastEpisodePtr::dynamicCast( sqlEpisode );

    return sqlEpisodes;
}

SqlPodcastEpisode::SqlPodcastEpisode( const QStringList &result, const SqlPodcastChannelPtr &sqlChannel )
    : Podcasts::PodcastEpisode( Podcasts::PodcastChannelPtr::staticCast( sqlChannel ) )
    , m_channel( sqlChannel )
{
    auto sqlStorage = StorageManager::instance()->sqlStorage();
    QStringList::ConstIterator iter = result.constBegin();
    m_dbId = (*(iter++)).toInt();
    m_url = QUrl( *(iter++) );
    int channelId = (*(iter++)).toInt();
    Q_UNUSED( channelId );
    m_localUrl = QUrl( *(iter++) );
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
    m_isKeep = sqlStorage->boolTrue() == (*(iter++));

    Q_ASSERT_X( iter == result.constEnd(), "SqlPodcastEpisode( PodcastCollection*, QStringList )", "number of expected fields did not match number of actual fields" );

    setupLocalFile();
}

//TODO: why do PodcastMetaCommon and PodcastEpisode not have an appropriate copy constructor?
SqlPodcastEpisode::SqlPodcastEpisode( Podcasts::PodcastEpisodePtr episode )
    : Podcasts::PodcastEpisode()
    , m_dbId( 0 )
    , m_isKeep( false )
{
    m_channel = SqlPodcastChannelPtr::dynamicCast( episode->channel() );

    if( !m_channel && episode->channel() )
    {
        debug() << "BUG: creating SqlEpisode but not an sqlChannel!!!";
        debug() <<  episode->channel()->title();
        debug() <<  m_channel->title();
    }

    // PodcastMetaCommon
    m_title = episode->title();
    m_description = episode->description();
    m_keywords = episode->keywords();
    m_subtitle = episode->subtitle();
    m_summary = episode->summary();
    m_author = episode->author();

    // PodcastEpisode
    m_guid = episode->guid();
    m_url = QUrl( episode->uidUrl() );
    m_localUrl = episode->localUrl();
    m_mimeType = episode->mimeType();
    m_pubDate = episode->pubDate();
    m_duration = episode->duration();
    m_fileSize = episode->filesize();
    m_sequenceNumber = episode->sequenceNumber();
    m_isNew = episode->isNew();

    // The album, artist, composer, genre and year fields
    // contain proxy objects with internal references to this.
    // These proxies are created by Podcasts::PodcastEpisode(), so
    // these fields don't have to be set here.

    //commit to the database
    updateInDb();
    setupLocalFile();
}

SqlPodcastEpisode::SqlPodcastEpisode( const PodcastChannelPtr &channel, Podcasts::PodcastEpisodePtr episode )
    : Podcasts::PodcastEpisode()
    , m_dbId( 0 )
    , m_isKeep( false )
{
    m_channel = SqlPodcastChannelPtr::dynamicCast( channel );

    if( !m_channel && episode->channel() )
    {
        debug() << "BUG: creating SqlEpisode but not an sqlChannel!!!";
        debug() <<  episode->channel()->title();
        debug() <<  m_channel->title();
    }

    // PodcastMetaCommon
    m_title = episode->title();
    m_description = episode->description();
    m_keywords = episode->keywords();
    m_subtitle = episode->subtitle();
    m_summary = episode->summary();
    m_author = episode->author();

    // PodcastEpisode
    m_guid = episode->guid();
    m_url = QUrl( episode->uidUrl() );
    m_localUrl = episode->localUrl();
    m_mimeType = episode->mimeType();
    m_pubDate = episode->pubDate();
    m_duration = episode->duration();
    m_fileSize = episode->filesize();
    m_sequenceNumber = episode->sequenceNumber();
    m_isNew = episode->isNew();

    // The album, artist, composer, genre and year fields
    // contain proxy objects with internal references to this.
    // These proxies are created by Podcasts::PodcastEpisode(), so
    // these fields don't have to be set here.

    //commit to the database
    updateInDb();
    setupLocalFile();
}

void
SqlPodcastEpisode::setupLocalFile()
{
    if( m_localUrl.isEmpty() || !QFileInfo( m_localUrl.toLocalFile() ).exists() )
        return;

    MetaProxy::TrackPtr proxyTrack( new MetaProxy::Track( m_localUrl, MetaProxy::Track::ManualLookup ) );
    m_localFile = Meta::TrackPtr( proxyTrack.data() ); // avoid static_cast
    /* following won't write to actual file, because MetaProxy::Track hasn't yet looked
     * up the underlying track. It will just set some cached values. */
    writeTagsToFile();
    proxyTrack->lookupTrack( &myFileTrackProvider );
}

SqlPodcastEpisode::~SqlPodcastEpisode()
{
}

void
SqlPodcastEpisode::setNew( bool isNew )
{
    PodcastEpisode::setNew( isNew );
    updateInDb();
}

void SqlPodcastEpisode::setKeep( bool isKeep )
{
    m_isKeep = isKeep;
    updateInDb();
}

void
SqlPodcastEpisode::setLocalUrl( const QUrl &url )
{
    m_localUrl = url;
    updateInDb();

    if( m_localUrl.isEmpty() && !m_localFile.isNull() )
    {
        m_localFile.clear();
        notifyObservers();
    }
    else
    {
        //if we had a local file previously it should get deleted by the AmarokSharedPointer.
        m_localFile = new MetaFile::Track( m_localUrl );
        if( m_channel->writeTags() )
            writeTagsToFile();
    }
}

qint64
SqlPodcastEpisode::length() const
{
    //if downloaded get the duration from the file, else use the value read from the feed
    if( m_localFile.isNull() )
        return m_duration * 1000;

    return m_localFile->length();
}

bool
SqlPodcastEpisode::hasCapabilityInterface( Capabilities::Capability::Type type ) const
{
    switch( type )
    {
        case Capabilities::Capability::Actions:
        case Capabilities::Capability::WriteTimecode:
        case Capabilities::Capability::LoadTimecode:
            //only downloaded episodes can be position marked
//            return !localUrl().isEmpty();
            return true;
        default:
            return false;
    }
}

Capabilities::Capability*
SqlPodcastEpisode::createCapabilityInterface( Capabilities::Capability::Type type )
{
    switch( type )
    {
        case Capabilities::Capability::Actions:
        {
            QList< QAction * > actions;
            actions << new BookmarkCurrentTrackPositionAction( nullptr );
            return new Capabilities::ActionsCapability( actions );
        }
        case Capabilities::Capability::WriteTimecode:
            return new TimecodeWriteCapabilityPodcastImpl( this );
        case Capabilities::Capability::LoadTimecode:
            return new TimecodeLoadCapabilityPodcastImpl( this );
        default:
            return nullptr;
    }
}

void
SqlPodcastEpisode::finishedPlaying( double playedFraction )
{
    if( length() <= 0 || playedFraction >= 0.1 )
        setNew( false );

    PodcastEpisode::finishedPlaying( playedFraction );
}

QString
SqlPodcastEpisode::name() const
{
    if( m_localFile.isNull() )
        return m_title;

    return m_localFile->name();
}

QString
SqlPodcastEpisode::prettyName() const
{
    /*for now just do the same as name, but in the future we might want to used a cleaned
      up string using some sort of regex tag rewrite for podcasts. decapitateString on
      steroides. */
    return name();
}

void
SqlPodcastEpisode::setTitle( const QString &title )
{
    m_title = title;

    Meta::TrackEditorPtr ec = m_localFile ? m_localFile->editor() : Meta::TrackEditorPtr();
    if( ec  )
        ec->setTitle( title );
}

Meta::ArtistPtr
SqlPodcastEpisode::artist() const
{
    if( m_localFile.isNull() )
        return m_artistPtr;

    return m_localFile->artist();
}

Meta::ComposerPtr
SqlPodcastEpisode::composer() const
{
    if( m_localFile.isNull() )
        return m_composerPtr;

    return m_localFile->composer();
}

Meta::GenrePtr
SqlPodcastEpisode::genre() const
{
    if( m_localFile.isNull() )
        return m_genrePtr;

    return m_localFile->genre();
}

Meta::YearPtr
SqlPodcastEpisode::year() const
{
    if( m_localFile.isNull() )
        return m_yearPtr;

    return m_localFile->year();
}

Meta::TrackEditorPtr
SqlPodcastEpisode::editor()
{
    if( m_localFile )
        return m_localFile->editor();
    else
        return Meta::TrackEditorPtr();
}

bool
SqlPodcastEpisode::writeTagsToFile()
{
    if( !m_localFile )
        return false;

    Meta::TrackEditorPtr ec = m_localFile->editor();
    if( !ec )
        return false;

    debug() << "writing tags for podcast episode " << title() << "to " << m_localUrl.url();
    ec->beginUpdate();
    ec->setTitle( m_title );
    ec->setAlbum( m_channel->title() );
    ec->setArtist( m_channel->author() );
    ec->setGenre( i18n( "Podcast" ) );
    ec->setYear( m_pubDate.date().year() );
    ec->endUpdate();

    notifyObservers();
    return true;
}

void
SqlPodcastEpisode::updateInDb()
{
    auto sqlStorage = StorageManager::instance()->sqlStorage();

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
        stream << (isNew() ? boolTrue : boolFalse);
        stream << ", iskeep=";
        stream << (isKeep() ? boolTrue : boolFalse);
        stream << " WHERE id=";
        stream << m_dbId;
        stream << ";";
        sqlStorage->query( command );
    }
    else
    {
        stream << "INSERT INTO podcastepisodes (";
        stream << "url,channel,localurl,guid,title,subtitle,sequencenumber,description,";
        stream << "mimetype,pubdate,duration,filesize,isnew,iskeep) ";
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
        stream << (isNew() ? boolTrue : boolFalse) << ", ";
        stream << (isKeep() ? boolTrue : boolFalse);
        stream << ");";
        m_dbId = sqlStorage->insert( command, QStringLiteral("podcastepisodes") );
    }
}

void
SqlPodcastEpisode::deleteFromDb()
{
    auto sqlStorage = StorageManager::instance()->sqlStorage();
    sqlStorage->query(
        QStringLiteral( "DELETE FROM podcastepisodes WHERE id = %1;" ).arg( dbId() ) );
}

Playlists::PlaylistPtr
SqlPodcastChannel::toPlaylistPtr( const SqlPodcastChannelPtr &sqlChannel )
{
    Playlists::PlaylistPtr playlist = Playlists::PlaylistPtr::dynamicCast( sqlChannel );
    return playlist;
}

SqlPodcastChannelPtr
SqlPodcastChannel::fromPlaylistPtr( const Playlists::PlaylistPtr &playlist )
{
    SqlPodcastChannelPtr sqlChannel = SqlPodcastChannelPtr::dynamicCast( playlist );
    return sqlChannel;
}

SqlPodcastChannel::SqlPodcastChannel( SqlPodcastProvider *provider,
                                            const QStringList &result )
    : Podcasts::PodcastChannel()
    , m_episodesLoaded( false )
    , m_trackCacheIsValid( false )
    , m_provider( provider )
{
    auto sqlStorage = StorageManager::instance()->sqlStorage();
    QStringList::ConstIterator iter = result.constBegin();
    m_dbId = (*(iter++)).toInt();
    m_url = QUrl( *(iter++) );
    m_title = *(iter++);
    m_webLink = QUrl::fromUserInput(*(iter++));
    m_imageUrl = QUrl::fromUserInput(*(iter++));
    m_description = *(iter++);
    m_copyright = *(iter++);
    m_directory = QUrl( *(iter++) );
    m_labels = QStringList( QString( *(iter++) ).split( QLatin1Char(','), Qt::SkipEmptyParts ) );
    m_subscribeDate = QDate::fromString( *(iter++) );
    m_autoScan = sqlStorage->boolTrue() == *(iter++);
    m_fetchType = (*(iter++)).toInt() == DownloadWhenAvailable ? DownloadWhenAvailable : StreamOrDownloadOnDemand;
    m_purge = sqlStorage->boolTrue() == *(iter++);
    m_purgeCount = (*(iter++)).toInt();
    m_writeTags = sqlStorage->boolTrue() == *(iter++);
    m_filenameLayout = *(iter++);
}

SqlPodcastChannel::SqlPodcastChannel( Podcasts::SqlPodcastProvider *provider,
                                            Podcasts::PodcastChannelPtr channel )
    : Podcasts::PodcastChannel()
    , m_dbId( 0 )
    , m_trackCacheIsValid( false )
    , m_provider( provider )
    , m_filenameLayout( QStringLiteral("%default%") )
{
    // PodcastMetaCommon
    m_title = channel->title();
    m_description = channel->description();
    m_keywords = channel->keywords();
    m_subtitle = channel->subtitle();
    m_summary = channel->summary();
    m_author = channel->author();

    // PodcastChannel
    m_url = channel->url();
    m_webLink = channel->webLink();
    m_imageUrl = channel->imageUrl();
    m_labels = channel->labels();
    m_subscribeDate = channel->subscribeDate();
    m_copyright = channel->copyright();

    if( channel->hasImage() )
        m_image = channel->image();

    //Default Settings

    m_directory = QUrl( m_provider->baseDownloadDir() );
    m_directory = m_directory.adjusted(QUrl::StripTrailingSlash);
    m_directory.setPath( QDir::toNativeSeparators(m_directory.path() + QLatin1Char('/') + Amarok::vfatPath( m_title )) );

    m_autoScan = true;
    m_fetchType = StreamOrDownloadOnDemand;
    m_purge = false;
    m_purgeCount = 10;
    m_writeTags = true;

    updateInDb();

    for( Podcasts::PodcastEpisodePtr episode : channel->episodes() )
    {
        episode->setChannel( PodcastChannelPtr( this ) );
        SqlPodcastEpisode *sqlEpisode = new SqlPodcastEpisode( episode );

        m_episodes << SqlPodcastEpisodePtr( sqlEpisode );
    }
    m_episodesLoaded = true;
}

int
SqlPodcastChannel::trackCount() const
{
    if( m_episodesLoaded )
        return m_episodes.count();
    else
        return -1;
}

void
SqlPodcastChannel::triggerTrackLoad()
{
    if( !m_episodesLoaded )
        loadEpisodes();
    notifyObserversTracksLoaded();
}

Playlists::PlaylistProvider *
SqlPodcastChannel::provider() const
{
    return dynamic_cast<Playlists::PlaylistProvider *>( m_provider );
}

QStringList
SqlPodcastChannel::groups()
{
    return m_labels;
}

void
SqlPodcastChannel::setGroups( const QStringList &groups )
{
    m_labels = groups;
}

QUrl
SqlPodcastChannel::uidUrl() const
{
    return QUrl( QStringLiteral( "amarok-sqlpodcastuid://%1").arg( m_dbId ) );
}

SqlPodcastChannel::~SqlPodcastChannel()
{
    m_episodes.clear();
}

void
SqlPodcastChannel::setTitle( const QString &title )
{
    /* also change the savelocation if a title is not set yet.
       This is a special condition that can happen when first fetching a podcast feed */
    if( m_title.isEmpty() )
    {
        m_directory = m_directory.adjusted(QUrl::StripTrailingSlash);
        m_directory.setPath( QDir::toNativeSeparators(m_directory.path() + QLatin1Char('/') + Amarok::vfatPath( title )) );
    }
    m_title = title;
}

Podcasts::PodcastEpisodeList
SqlPodcastChannel::episodes() const
{
    return SqlPodcastEpisode::toPodcastEpisodeList( m_episodes );
}

void
SqlPodcastChannel::setImage( const QImage &image )
{
    DEBUG_BLOCK

    m_image = image;
}

void
SqlPodcastChannel::setImageUrl( const QUrl &imageUrl )
{
    DEBUG_BLOCK
    debug() << imageUrl;
    m_imageUrl = imageUrl;

    if( imageUrl.isLocalFile() )
    {
        m_image = QImage( imageUrl.path() );
        return;
    }

    debug() << "Image is remote, handled by podcastImageFetcher.";
}

Podcasts::PodcastEpisodePtr
SqlPodcastChannel::addEpisode(const PodcastEpisodePtr &episode )
{
    if( !m_provider )
        return PodcastEpisodePtr();

    QUrl checkUrl;
    //searched in the database for guid or enclosure url
    if( !episode->guid().isEmpty() )
        checkUrl = QUrl::fromUserInput(episode->guid());
    else if( !episode->uidUrl().isEmpty() )
        checkUrl = QUrl::fromUserInput(episode->uidUrl());
    else
        return PodcastEpisodePtr(); //noting to check for

    if( m_provider->possiblyContainsTrack( checkUrl ) )
        return PodcastEpisodePtr::dynamicCast( m_provider->trackForUrl( QUrl::fromUserInput(episode->guid()) ) );

    //force episodes load.
    if( !m_episodesLoaded )
        loadEpisodes();

    SqlPodcastEpisodePtr sqlEpisode;

    if (SqlPodcastEpisodePtr::dynamicCast( episode ))
        sqlEpisode = SqlPodcastEpisodePtr( new SqlPodcastEpisode( episode ) );
    else
        sqlEpisode = SqlPodcastEpisodePtr( new SqlPodcastEpisode( PodcastChannelPtr(this) , episode ) );


    //episodes are sorted on pubDate high to low
    int i;
    for( i = 0; i < m_episodes.count() ; i++ )
    {
        if( sqlEpisode->pubDate() > m_episodes[i]->pubDate() )
        {
            m_episodes.insert( i, sqlEpisode );
            break;
        }
    }

    //insert in case the list is empty or at the end of the list
    if( i == m_episodes.count() )
        m_episodes << sqlEpisode;

    notifyObserversTrackAdded( Meta::TrackPtr::dynamicCast( sqlEpisode ), i );

    applyPurge();
    m_trackCacheIsValid = false;
    return PodcastEpisodePtr::dynamicCast( sqlEpisode );
}

void
SqlPodcastChannel::applyPurge()
{
    DEBUG_BLOCK
    if( !hasPurge() )
        return;

    if( m_episodes.count() > purgeCount() )
    {
        int purgeIndex = 0;

        foreach( SqlPodcastEpisodePtr episode, m_episodes )
        {
            if ( !episode->isKeep() )
            {
                if( purgeIndex >= purgeCount() )
                {
                    m_provider->deleteDownloadedEpisode( episode );
                    m_episodes.removeOne( episode );
                }
                else
                    purgeIndex++;
            }
        }
        m_trackCacheIsValid = false;
    }
}

void
SqlPodcastChannel::updateInDb()
{
    auto sqlStorage = StorageManager::instance()->sqlStorage();

    QString boolTrue = sqlStorage->boolTrue();
    QString boolFalse = sqlStorage->boolFalse();
    #define escape(x) sqlStorage->escape(x)
    QString command;
    QTextStream stream( &command );
    if( m_dbId )
    {
        stream << "UPDATE podcastchannels ";
        stream << "SET url='";
        stream << escape(m_url.url());
        stream << "', title='";
        stream << escape(m_title);
        stream << "', weblink='";
        stream << escape(m_webLink.url());
        stream << "', image='";
        stream << escape(m_imageUrl.url());
        stream << "', description='";
        stream << escape(m_description);
        stream << "', copyright='";
        stream << escape(m_copyright);
        stream << "', directory='";
        stream << escape(m_directory.url());
        stream << "', labels='";
        stream << escape(m_labels.join( QLatin1Char(',') ));
        stream << "', subscribedate='";
        stream << escape(m_subscribeDate.toString());
        stream << "', autoscan=";
        stream << (m_autoScan ? boolTrue : boolFalse);
        stream << ", fetchtype=";
        stream << QString::number(m_fetchType);
        stream << ", haspurge=";
        stream << (m_purge ? boolTrue : boolFalse);
        stream << ", purgecount=";
        stream << QString::number(m_purgeCount);
        stream << ", writetags=";
        stream << (m_writeTags ? boolTrue : boolFalse);
        stream << ", filenamelayout='";
        stream << escape(m_filenameLayout);
        stream << "' WHERE id=";
        stream << m_dbId;
        stream << ";";
        debug() << command;
        sqlStorage->query( command );
    }
    else
    {
        stream << "INSERT INTO podcastchannels(";
        stream << "url,title,weblink,image,description,copyright,directory,labels,";
        stream << "subscribedate,autoscan,fetchtype,haspurge,purgecount,writetags,filenamelayout) ";
        stream << "VALUES ( '";
        stream << escape(m_url.url()) << "', '";
        stream << escape(m_title) << "', '";
        stream << escape(m_webLink.url()) << "', '";
        stream << escape(m_imageUrl.url()) << "', '";
        stream << escape(m_description) << "', '";
        stream << escape(m_copyright) << "', '";
        stream << escape(m_directory.url()) << "', '";
        stream << escape(m_labels.join( QLatin1Char(',') )) << "', '";
        stream << escape(m_subscribeDate.toString()) << "', ";
        stream << (m_autoScan ? boolTrue : boolFalse) << ", ";
        stream << QString::number(m_fetchType) << ", ";
        stream << (m_purge ? boolTrue : boolFalse) << ", ";
        stream << QString::number(m_purgeCount) << ", ";
        stream << (m_writeTags ? boolTrue : boolFalse) << ", '";
        stream << escape(m_filenameLayout);
        stream << "');";
        debug() << command;
        m_dbId = sqlStorage->insert( command, QStringLiteral("podcastchannels") );
    }
}

void
SqlPodcastChannel::deleteFromDb()
{
    auto sqlStorage = StorageManager::instance()->sqlStorage();
    foreach( SqlPodcastEpisodePtr sqlEpisode, m_episodes )
    {
       sqlEpisode->deleteFromDb();
       m_episodes.removeOne( sqlEpisode );
    }
    m_trackCacheIsValid = false;

    sqlStorage->query(
        QStringLiteral( "DELETE FROM podcastchannels WHERE id = %1;" ).arg( dbId() ) );
}

void
SqlPodcastChannel::loadEpisodes()
{
    m_episodes.clear();

    auto sqlStorage = StorageManager::instance()->sqlStorage();

    //If purge is enabled we must limit the number of results
    QString command;

    int rowLength = 15;

    //If purge is enabled we must limit the number of results, though there are some files
    //the user want to be shown even if there is no more slot
    if( hasPurge() )
    {
        command = QStringLiteral( "(SELECT id, url, channel, localurl, guid, "
                           "title, subtitle, sequencenumber, description, mimetype, pubdate, "
                           "duration, filesize, isnew, iskeep FROM podcastepisodes WHERE channel = %1 "
                           "AND iskeep IS FALSE ORDER BY pubdate DESC LIMIT ") + QString::number( purgeCount() ) + QStringLiteral(") "
                           "UNION "
                           "(SELECT id, url, channel, localurl, guid, "
                           "title, subtitle, sequencenumber, description, mimetype, pubdate, "
                           "duration, filesize, isnew, iskeep FROM podcastepisodes WHERE channel = %1 "
                           "AND iskeep IS TRUE) "
                           "ORDER BY pubdate DESC;"
                           );
    }
    else
    {
        command = QStringLiteral( "SELECT id, url, channel, localurl, guid, "
                           "title, subtitle, sequencenumber, description, mimetype, pubdate, "
                           "duration, filesize, isnew, iskeep FROM podcastepisodes WHERE channel = %1 "
                           "ORDER BY pubdate DESC;"
                           );
    }

    QStringList results = sqlStorage->query( command.arg( m_dbId ) );

    for( int i = 0; i < results.size(); i += rowLength )
    {
        QStringList episodesResult = results.mid( i, rowLength );
        SqlPodcastEpisodePtr sqlEpisode = SqlPodcastEpisodePtr(
                                              new SqlPodcastEpisode(
                                                  episodesResult,
                                                  SqlPodcastChannelPtr( this ) ) );
        m_episodes << sqlEpisode;
    }

    m_episodesLoaded = true;
    m_trackCacheIsValid = false;
}

Meta::TrackList
Podcasts::SqlPodcastChannel::tracks()
{
    if ( !m_trackCacheIsValid ) {
        m_episodesAsTracksCache = Podcasts::SqlPodcastEpisode::toTrackList( m_episodes );
        m_trackCacheIsValid = true;
    }
    return m_episodesAsTracksCache;
}

void
Podcasts::SqlPodcastChannel::syncTrackStatus(int position, const Meta::TrackPtr &otherTrack )
{
    Q_UNUSED( position );

    Podcasts::PodcastEpisodePtr master =
            Podcasts::PodcastEpisodePtr::dynamicCast( otherTrack );

    if ( master )
    {
        this->setName( master->channel()->name() );
        this->setTitle( master->channel()->title() );
        this->setUrl( master->channel()->url() );
    }
}

void
Podcasts::SqlPodcastChannel::addTrack( const Meta::TrackPtr &track, int position )
{
    Q_UNUSED( position );

    addEpisode( Podcasts::PodcastEpisodePtr::dynamicCast( track ) );
    notifyObserversTrackAdded( track, position );
}

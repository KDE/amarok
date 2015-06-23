/****************************************************************************************
 * Copyright (c) 2007-2009 Bart Cerneels <bart.cerneels@kde.org>                        *
 * Copyright (c) 2009 Frank Meerkoetter <frank@meerkoetter.org>                         *
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

#include "SqlPodcastProvider.h"

#include "MainWindow.h"
#include "OpmlWriter.h"
#include "SvgHandler.h"
#include "QStringx.h"
#include "browsers/playlistbrowser/PodcastModel.h"
#include "context/popupdropper/libpud/PopupDropper.h"
#include "context/popupdropper/libpud/PopupDropperItem.h"
#include <core/storage/SqlStorage.h>
#include "core/interfaces/Logger.h"
#include "core/podcasts/PodcastImageFetcher.h"
#include "core/podcasts/PodcastReader.h"
#include "core/support/Amarok.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "core-impl/storage/StorageManager.h"
#include "core-impl/podcasts/sql/PodcastSettingsDialog.h"
#include "playlistmanager/sql/SqlPlaylistGroup.h"

#include "ui_SqlPodcastProviderSettingsWidget.h"

#include <KCodecs>
#include <KFileDialog>
#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/Job>
#include <KIO/NetAccess>
#include <KLocale>
#include <KProgressDialog>
#include <KStandardDirs>
#include <QUrl>
#include <Solid/Networking>

#include <QAction>
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QMap>
#include <QTimer>

using namespace Podcasts;

static const int PODCAST_DB_VERSION = 6;
static const QString key( "AMAROK_PODCAST" );
static const QString PODCAST_TMP_POSTFIX( ".tmp" );

SqlPodcastProvider::SqlPodcastProvider()
        : m_updateTimer( new QTimer( this ) )
        , m_updatingChannels( 0 )
        , m_completedDownloads( 0 )
        , m_providerSettingsDialog( 0 )
        , m_providerSettingsWidget( 0 )
        , m_configureChannelAction( 0 )
        , m_deleteAction( 0 )
        , m_downloadAction( 0 )
        , m_keepAction( 0 )
        , m_removeAction( 0 )
        , m_updateAction( 0 )
        , m_writeTagsAction( 0 )
        , m_podcastImageFetcher( 0 )
{
    connect( m_updateTimer, SIGNAL(timeout()), SLOT(autoUpdate()) );

    SqlStorage *sqlStorage = StorageManager::instance()->sqlStorage();

    if( !sqlStorage )
    {
        error() << "Could not get a SqlStorage instance";
        return;
    }

    m_autoUpdateInterval = Amarok::config( "Podcasts" )
                           .readEntry( "AutoUpdate Interval", 30 );
    m_maxConcurrentDownloads = Amarok::config( "Podcasts" )
                               .readEntry( "Maximum Simultaneous Downloads", 4 );
    m_maxConcurrentUpdates = Amarok::config( "Podcasts" )
                             .readEntry( "Maximum Simultaneous Updates", 4 );
    m_baseDownloadDir = QUrl::fromUserInput( Amarok::config( "Podcasts" ).readEntry( "Base Download Directory",
                                                           Amarok::saveLocation( "podcasts" ) ) );

    QStringList values;

    values = sqlStorage->query(
                 QString( "SELECT version FROM admin WHERE component = '%1';" )
                    .arg( sqlStorage->escape( key ) )
             );
    if( values.isEmpty() )
    {
        debug() << "creating Podcast Tables";
        createTables();
        sqlStorage->query( "INSERT INTO admin(component,version) "
                           "VALUES('" + key + "',"
                           + QString::number( PODCAST_DB_VERSION ) + ");" );
    }
    else
    {
        int version = values.first().toInt();
        if( version == PODCAST_DB_VERSION )
            loadPodcasts();
        else
            updateDatabase( version /*from*/, PODCAST_DB_VERSION /*to*/ );

        startTimer();
    }
}

void
SqlPodcastProvider::startTimer()
{
    if( !m_autoUpdateInterval )
        return; //timer is disabled

    if( m_updateTimer->isActive() &&
        m_updateTimer->interval() == ( m_autoUpdateInterval * 1000 * 60 ) )
        return; //already started with correct interval

    //and only start if at least one channel has autoscan enabled
    foreach( Podcasts::SqlPodcastChannelPtr channel, m_channels )
    {
        if( channel->autoScan() )
        {
            m_updateTimer->start( 1000 * 60 * m_autoUpdateInterval );
            return;
        }
    }
}

SqlPodcastProvider::~SqlPodcastProvider()
{
    foreach( Podcasts::SqlPodcastChannelPtr channel, m_channels )
    {
        channel->updateInDb();
        foreach( Podcasts::SqlPodcastEpisodePtr episode, channel->sqlEpisodes() )
            episode->updateInDb();
    }
    m_channels.clear();

    Amarok::config( "Podcasts" )
        .writeEntry( "AutoUpdate Interval", m_autoUpdateInterval );
    Amarok::config( "Podcasts" )
        .writeEntry( "Maximum Simultaneous Downloads", m_maxConcurrentDownloads );
    Amarok::config( "Podcasts" )
        .writeEntry( "Maximum Simultaneous Updates", m_maxConcurrentUpdates );
}

void
SqlPodcastProvider::loadPodcasts()
{
    m_channels.clear();
    SqlStorage *sqlStorage = StorageManager::instance()->sqlStorage();
    if( !sqlStorage )
        return;

    QStringList results = sqlStorage->query( "SELECT id, url, title, weblink, image"
        ", description, copyright, directory, labels, subscribedate, autoscan, fetchtype"
        ", haspurge, purgecount, writetags, filenamelayout FROM podcastchannels;" );

    int rowLength = 16;
    for( int i = 0; i < results.size(); i += rowLength )
    {
        QStringList channelResult = results.mid( i, rowLength );
        SqlPodcastChannelPtr channel =
                SqlPodcastChannelPtr( new SqlPodcastChannel( this, channelResult ) );
        if( channel->image().isNull() )
            fetchImage( channel );

        m_channels << channel;
    }
    if( m_podcastImageFetcher )
        m_podcastImageFetcher->run();
    emit updated();
}

SqlPodcastEpisodePtr
SqlPodcastProvider::sqlEpisodeForString( const QString &string )
{
    if( string.isEmpty() )
        return SqlPodcastEpisodePtr();

    SqlStorage *sqlStorage = StorageManager::instance()->sqlStorage();
    if( !sqlStorage )
        return SqlPodcastEpisodePtr();

    QString command = "SELECT id, url, channel, localurl, guid, "
            "title, subtitle, sequencenumber, description, mimetype, pubdate, "
            "duration, filesize, isnew, iskeep FROM podcastepisodes "
            "WHERE guid='%1' OR url='%1' OR localurl='%1' ORDER BY id DESC;";
    command = command.arg( sqlStorage->escape( string ) );
    QStringList dbResult = sqlStorage->query( command );

    if( dbResult.isEmpty() )
        return SqlPodcastEpisodePtr();

    int episodeId = dbResult[0].toInt();
    int channelId = dbResult[2].toInt();
    bool found = false;
    Podcasts::SqlPodcastChannelPtr channel;
    foreach( channel, m_channels )
    {
        if( channel->dbId() == channelId )
        {
            found = true;
            break;
        }
    }

    if( !found )
    {
        error() << QString( "There is a track in the database with url/guid=%1 (%2) "
                            "but there is no channel with dbId=%3 in our list!" )
                .arg( string ).arg( episodeId ).arg( channelId );
        return SqlPodcastEpisodePtr();
    }

    Podcasts::SqlPodcastEpisodePtr episode;
    foreach( episode, channel->sqlEpisodes() )
        if( episode->dbId() == episodeId )
            return episode;

    //The episode was found in the database but it's channel didn't have it in it's list.
    //That probably is because it's beyond the purgecount limit or the tracks were not loaded yet.
    return SqlPodcastEpisodePtr( new SqlPodcastEpisode( dbResult.mid( 0, 15 ), channel ) );
}

bool
SqlPodcastProvider::possiblyContainsTrack( const QUrl &url ) const
{
    SqlStorage *sqlStorage = StorageManager::instance()->sqlStorage();
    if( !sqlStorage )
        return false;

    QString command = "SELECT id FROM podcastepisodes WHERE guid='%1' OR url='%1' "
                      "OR localurl='%1';";
    command = command.arg( sqlStorage->escape( url.url() ) );

    QStringList dbResult = sqlStorage->query( command );
    return !dbResult.isEmpty();
}

Meta::TrackPtr
SqlPodcastProvider::trackForUrl( const QUrl &url )
{
    if( url.isEmpty() )
        return Meta::TrackPtr();

    SqlPodcastEpisodePtr episode = sqlEpisodeForString( url.url() );

    return Meta::TrackPtr::dynamicCast( episode );
}

Playlists::PlaylistList
SqlPodcastProvider::playlists()
{
    Playlists::PlaylistList playlistList;

    QListIterator<Podcasts::SqlPodcastChannelPtr> i( m_channels );
    while( i.hasNext() )
    {
        playlistList << Playlists::PlaylistPtr::staticCast( i.next() );
    }
    return playlistList;
}

QActionList
SqlPodcastProvider::providerActions()
{
    if( m_providerActions.isEmpty() )
    {
        QAction *updateAllAction = new QAction( QIcon::fromTheme( "view-refresh-amarok" ),
                i18n( "&Update All Channels" ), this );
        updateAllAction->setProperty( "popupdropper_svg_id", "update" );
        connect( updateAllAction, SIGNAL(triggered()), this, SLOT(updateAll()) );
        m_providerActions << updateAllAction;

        QAction *configureAction = new QAction( QIcon::fromTheme( "configure" ),
                i18n( "&Configure General Settings" ), this );
        configureAction->setProperty( "popupdropper_svg_id", "configure" );
        connect( configureAction, SIGNAL(triggered()), this, SLOT(slotConfigureProvider()) );
        m_providerActions << configureAction;

        QAction *exportOpmlAction = new QAction( QIcon::fromTheme( "document-export" ),
                i18n( "&Export subscriptions to OPML file" ), this );
        connect( exportOpmlAction, SIGNAL(triggered()), SLOT(slotExportOpml()) );
        m_providerActions << exportOpmlAction;
    }

    return m_providerActions;
}

QActionList
SqlPodcastProvider::playlistActions( const Playlists::PlaylistList &playlists )
{
    QActionList actions;
    SqlPodcastChannelList sqlChannels;
    foreach( const Playlists::PlaylistPtr &playlist, playlists )
    {
        SqlPodcastChannelPtr sqlChannel = SqlPodcastChannel::fromPlaylistPtr( playlist );
        if( sqlChannel )
            sqlChannels << sqlChannel;
    }

    if( sqlChannels.isEmpty() )
        return actions;

    //TODO: add export OPML action for selected playlists only. Use the QAction::data() trick.
    if( m_configureChannelAction == 0 )
    {
        m_configureChannelAction = new QAction( QIcon::fromTheme( "configure" ), i18n( "&Configure" ), this );
        m_configureChannelAction->setProperty( "popupdropper_svg_id", "configure" );
        connect( m_configureChannelAction, SIGNAL(triggered()), SLOT(slotConfigureChannel()) );
    }
    //only one channel can be configured at a time.
    if( sqlChannels.count() == 1 )
    {
        m_configureChannelAction->setData( QVariant::fromValue( sqlChannels.first() ) );
        actions << m_configureChannelAction;
    }

    if( m_removeAction == 0 )
    {
        m_removeAction = new QAction( QIcon::fromTheme( "news-unsubscribe" ), i18n( "&Remove Subscription" ), this );
        m_removeAction->setProperty( "popupdropper_svg_id", "remove" );
        connect( m_removeAction, SIGNAL(triggered()), SLOT(slotRemoveChannels()) );
    }
    m_removeAction->setData( QVariant::fromValue( sqlChannels ) );
    actions << m_removeAction;

    if( m_updateAction == 0 )
    {
        m_updateAction = new QAction( QIcon::fromTheme( "view-refresh-amarok" ), i18n( "&Update Channel" ), this );
        m_updateAction->setProperty( "popupdropper_svg_id", "update" );
        connect( m_updateAction, SIGNAL(triggered()), SLOT(slotUpdateChannels()) );
    }
    m_updateAction->setData( QVariant::fromValue( sqlChannels ) );
    actions << m_updateAction;

    return actions;
}

QActionList
SqlPodcastProvider::trackActions( const QMultiHash<Playlists::PlaylistPtr, int> &playlistTracks )
{
    SqlPodcastEpisodeList episodes;
    foreach( const Playlists::PlaylistPtr &playlist, playlistTracks.uniqueKeys() )
    {
        SqlPodcastChannelPtr sqlChannel = SqlPodcastChannel::fromPlaylistPtr( playlist );
        if( !sqlChannel )
            continue;

        SqlPodcastEpisodeList channelEpisodes = sqlChannel->sqlEpisodes();
        QList<int> trackPositions = playlistTracks.values( playlist );
        qSort( trackPositions );
        foreach( int trackPosition, trackPositions )
        {
            if( trackPosition >= 0 && trackPosition < channelEpisodes.count() )
                episodes << channelEpisodes.at( trackPosition );
        }
    }

    QActionList actions;
    if( episodes.isEmpty() )
        return actions;

    if( m_downloadAction == 0 )
    {
        m_downloadAction = new QAction( QIcon::fromTheme( "go-down" ), i18n( "&Download Episode" ), this );
        m_downloadAction->setProperty( "popupdropper_svg_id", "download" );
        connect( m_downloadAction, SIGNAL(triggered()), SLOT(slotDownloadEpisodes()) );
    }

    if( m_deleteAction == 0 )
    {
        m_deleteAction = new QAction( QIcon::fromTheme( "edit-delete" ),
            i18n( "&Delete Downloaded Episode" ), this );
        m_deleteAction->setProperty( "popupdropper_svg_id", "delete" );
        m_deleteAction->setObjectName( "deleteAction" );
        connect( m_deleteAction, SIGNAL(triggered()), SLOT(slotDeleteDownloadedEpisodes()) );
    }

    if( m_writeTagsAction == 0 )
    {
        m_writeTagsAction = new QAction( QIcon::fromTheme( "media-track-edit-amarok" ),
            i18n( "&Write Feed Information to File" ), this );
        m_writeTagsAction->setProperty( "popupdropper_svg_id", "edit" );
        connect( m_writeTagsAction, SIGNAL(triggered()), SLOT(slotWriteTagsToFiles()) );
    }

    if( m_keepAction == 0 )
    {
        m_keepAction = new QAction( QIcon::fromTheme( "podcast-amarok" ),
                i18n( "&Keep downloaded file" ), this );
        m_keepAction->setToolTip( i18n( "Toggle the \"keep\" downloaded file status of "
                "this podcast episode. Downloaded files with this status wouldn't be "
                "deleted even if we apply a purge." ) );
        m_keepAction->setProperty( "popupdropper_svg_id", "keep" );
        m_keepAction->setCheckable( true );
        connect( m_keepAction, SIGNAL(triggered(bool)), SLOT(slotSetKeep()) );
    }

    SqlPodcastEpisodeList remoteEpisodes;
    SqlPodcastEpisodeList keptDownloadedEpisodes, unkeptDownloadedEpisodes;
    foreach( const SqlPodcastEpisodePtr &episode, episodes )
    {
        if( episode->localUrl().isEmpty() )
            remoteEpisodes << episode;
        else
        {
            if( episode->isKeep() )
                keptDownloadedEpisodes << episode;
            else
                unkeptDownloadedEpisodes << episode;
        }
    }

    if( !remoteEpisodes.isEmpty() )
    {
        m_downloadAction->setData( QVariant::fromValue( remoteEpisodes ) );
        actions << m_downloadAction;
    }
    if( !( keptDownloadedEpisodes + unkeptDownloadedEpisodes ).isEmpty() )
    {
        m_deleteAction->setData( QVariant::fromValue( keptDownloadedEpisodes + unkeptDownloadedEpisodes ) );
        actions << m_deleteAction;

        m_keepAction->setChecked( unkeptDownloadedEpisodes.isEmpty() );
        m_keepAction->setData( QVariant::fromValue( keptDownloadedEpisodes + unkeptDownloadedEpisodes ) );
        actions << m_keepAction;
    }

    return actions;
}

Podcasts::PodcastEpisodePtr
SqlPodcastProvider::episodeForGuid( const QString &guid )
{
    return PodcastEpisodePtr::dynamicCast( sqlEpisodeForString( guid ) );
}

void
SqlPodcastProvider::addPodcast( const QUrl &url )
{
    QUrl kurl = QUrl( url );
    debug() << "importing " << kurl.url();

    SqlStorage *sqlStorage = StorageManager::instance()->sqlStorage();
    if( !sqlStorage )
        return;

    QString command = "SELECT title FROM podcastchannels WHERE url='%1';";
    command = command.arg( sqlStorage->escape( kurl.url() ) );

    QStringList dbResult = sqlStorage->query( command );
    if( !dbResult.isEmpty() )
    {
        //Already subscribed to this Channel
        //notify the user.
        Amarok::Components::logger()->longMessage(
                    i18n( "Already subscribed to %1.", dbResult.first() ), Amarok::Logger::Error );
    }
    else
    {
        subscribe( kurl );
    }
}

void
SqlPodcastProvider::updateAll()
{
    foreach( Podcasts::SqlPodcastChannelPtr channel, m_channels )
        updateSqlChannel( channel );
}

void
SqlPodcastProvider::subscribe( const QUrl &url )
{
    if( !url.isValid() )
        return;

    if( m_updatingChannels >= m_maxConcurrentUpdates )
    {
        debug() << QString( "Maximum concurrent updates (%1) reached. "
                            "Queueing \"%2\" for subscribing." )
                        .arg( m_maxConcurrentUpdates )
                        .arg( url.url() );
        m_subscribeQueue << url;
        return;
    }

    PodcastReader *podcastReader = new PodcastReader( this );
    connect( podcastReader, SIGNAL(finished(PodcastReader*)),
             SLOT(slotReadResult(PodcastReader*)) );
    connect( podcastReader, SIGNAL(statusBarSorryMessage(QString)),
            this, SLOT(slotStatusBarSorryMessage(QString)) );
    connect( podcastReader,
        SIGNAL(statusBarNewProgressOperation( KIO::TransferJob *, const QString &,
                                              Podcasts::PodcastReader* )),
            SLOT(slotStatusBarNewProgressOperation( KIO::TransferJob *, const QString &,
                                                    Podcasts::PodcastReader* ))
           );

    m_updatingChannels++;
    podcastReader->read( url );
}

Podcasts::PodcastChannelPtr
SqlPodcastProvider::addChannel( Podcasts::PodcastChannelPtr channel )
{
    Podcasts::SqlPodcastChannelPtr sqlChannel =
            SqlPodcastChannelPtr( new Podcasts::SqlPodcastChannel( this, channel ) );
    m_channels << sqlChannel;

    if( sqlChannel->episodes().count() == 0 )
        updateSqlChannel( sqlChannel );

    emit playlistAdded( Playlists::PlaylistPtr( sqlChannel.data() ) );
    return PodcastChannelPtr( sqlChannel.data() );
}

Podcasts::PodcastEpisodePtr
SqlPodcastProvider::addEpisode( Podcasts::PodcastEpisodePtr episode )
{
    Podcasts::SqlPodcastEpisodePtr sqlEpisode =
            Podcasts::SqlPodcastEpisodePtr::dynamicCast( episode );
    if( sqlEpisode.isNull() )
        return Podcasts::PodcastEpisodePtr();

    if( sqlEpisode->channel().isNull() )
    {
        debug() << "channel is null";
        return Podcasts::PodcastEpisodePtr();
    }

    if( sqlEpisode->channel()->fetchType() == Podcasts::PodcastChannel::DownloadWhenAvailable )
        downloadEpisode( sqlEpisode );
    return Podcasts::PodcastEpisodePtr::dynamicCast( sqlEpisode );
}

Podcasts::PodcastChannelList
SqlPodcastProvider::channels()
{
    PodcastChannelList list;
    QListIterator<SqlPodcastChannelPtr> i( m_channels );
    while( i.hasNext() )
    {
        list << PodcastChannelPtr::dynamicCast( i.next() );
    }
    return list;
}

void
SqlPodcastProvider::removeSubscription( Podcasts::SqlPodcastChannelPtr sqlChannel )
{
    debug() << "Deleting channel " << sqlChannel->title();
    sqlChannel->deleteFromDb();

    m_channels.removeOne( sqlChannel );

    //HACK: because of a database "leak" in the past we have orphan data in the tables.
    //Remove it when we know it's supposed to be empty.
    if( m_channels.isEmpty() )
    {
        SqlStorage *sqlStorage = StorageManager::instance()->sqlStorage();
        if( !sqlStorage )
            return;
        debug() << "Unsubscribed from last channel, cleaning out the podcastepisodes table.";
        sqlStorage->query( "DELETE FROM podcastepisodes WHERE 1;" );
    }

    emit playlistRemoved( Playlists::PlaylistPtr::dynamicCast( sqlChannel ) );
}

void
SqlPodcastProvider::configureProvider()
{
    m_providerSettingsDialog = new KDialog( The::mainWindow() );
    QWidget *settingsWidget = new QWidget( m_providerSettingsDialog );
    m_providerSettingsDialog->setObjectName( "SqlPodcastProviderSettings" );
    Ui::SqlPodcastProviderSettingsWidget settings;
    m_providerSettingsWidget = &settings;
    settings.setupUi( settingsWidget );

    settings.m_baseDirUrl->setMode( KFile::Directory );
    settings.m_baseDirUrl->setUrl( m_baseDownloadDir );

    settings.m_autoUpdateInterval->setValue( m_autoUpdateInterval );
    settings.m_autoUpdateInterval->setPrefix(
            i18nc( "prefix to 'x minutes'", "every " ) );
    settings.m_autoUpdateInterval->setSuffix( ki18np( " minute", " minutes" ) );

    m_providerSettingsDialog->setButtons( KDialog::Ok | KDialog::Cancel | KDialog::Apply );
    m_providerSettingsDialog->setMainWidget( settingsWidget );

    connect( settings.m_baseDirUrl, SIGNAL(textChanged(QString)), SLOT(slotConfigChanged()) );
    connect( settings.m_autoUpdateInterval, SIGNAL(valueChanged(int)),
             SLOT(slotConfigChanged()) );

    m_providerSettingsDialog->setWindowTitle( i18n( "Configure Local Podcasts" ) );
    m_providerSettingsDialog->enableButtonApply( false );

    if( m_providerSettingsDialog->exec() == QDialog::Accepted )
    {
        m_autoUpdateInterval = settings.m_autoUpdateInterval->value();
        if( m_autoUpdateInterval )
            startTimer();
        else
            m_updateTimer->stop();
        QUrl adjustedNewPath = settings.m_baseDirUrl->url();
        adjustedNewPath = adjustedNewPath.adjusted(QUrl::StripTrailingSlash);

        if( adjustedNewPath != m_baseDownloadDir )
        {
            m_baseDownloadDir = adjustedNewPath;
            Amarok::config( "Podcasts" ).writeEntry( "Base Download Directory", m_baseDownloadDir );
            if( !m_channels.isEmpty() )
            {
                //TODO: check if there actually are downloaded episodes
                KDialog moveAllDialog;
                moveAllDialog.setCaption( i18n( "Move Podcasts" ) );

                KVBox *vbox = new KVBox( &moveAllDialog );

                QString question( i18n( "Do you want to move all downloaded episodes to the "
                                       "new location?") );
                QLabel *label = new QLabel( question, vbox );
                label->setWordWrap( true );
                label->setMaximumWidth( 400 );

                moveAllDialog.setMainWidget( vbox );
                moveAllDialog.setButtons( KDialog::Yes | KDialog::No );

                if( moveAllDialog.exec() == KDialog::Yes )
                {
                    foreach( SqlPodcastChannelPtr sqlChannel, m_channels )
                    {
                        QUrl oldSaveLocation = sqlChannel->saveLocation();
                        QUrl newSaveLocation = m_baseDownloadDir;
                        newSaveLocation = newSaveLocation.adjusted(QUrl::StripTrailingSlash);
                        newSaveLocation.setPath(newSaveLocation.path() + '/' + ( oldSaveLocation.fileName() ));
                        sqlChannel->setSaveLocation( newSaveLocation );
                        debug() << newSaveLocation.path();
                        moveDownloadedEpisodes( sqlChannel );

                        if( !QDir().rmdir( oldSaveLocation.toLocalFile() ) )
                                debug() << "Could not remove old directory "
                                        << oldSaveLocation.toLocalFile();
                    }
                }
            }
        }
    }

    delete m_providerSettingsDialog;
    m_providerSettingsDialog = 0;
    m_providerSettingsWidget = 0;
}

void
SqlPodcastProvider::slotConfigChanged()
{
    if( !m_providerSettingsWidget )
        return;

    if( m_providerSettingsWidget->m_autoUpdateInterval->value() != m_autoUpdateInterval
        || m_providerSettingsWidget->m_baseDirUrl->url() != m_baseDownloadDir )
    {
        m_providerSettingsDialog->enableButtonApply( true );
    }
}

void
SqlPodcastProvider::slotExportOpml()
{
    QList<OpmlOutline *> rootOutlines;
    QMap<QString,QString> headerData;
    //TODO: set header data such as date

    //TODO: folder outline support
    foreach( SqlPodcastChannelPtr channel, m_channels )
    {
        OpmlOutline *channelOutline = new OpmlOutline();
        #define addAttr( k, v ) channelOutline->addAttribute( k, v )
        addAttr( "text", channel->title() );
        addAttr( "type", "rss" );
        addAttr( "xmlUrl", channel->url().url() );
        rootOutlines << channelOutline;
    }

    //TODO: add checkbox as widget to filedialog to include podcast settings.
    KFileDialog fileDialog( QUrl("kfiledialog:///podcast/amarok_podcasts.opml"), "*.opml",
                            The::mainWindow() );
    fileDialog.setMode( KFile::File );
    fileDialog.setWindowTitle( i18n( "Select file for OPML export") );
    if( fileDialog.exec() != KDialog::Accepted )
        return;

    QUrl filePath = fileDialog.selectedUrl();

    QFile *opmlFile = new QFile( filePath.toLocalFile(), this );
    if( !opmlFile->open( QIODevice::WriteOnly | QIODevice::Truncate ) )
    {
        error() << "could not open OPML file " << filePath.url();
        return;
    }
    OpmlWriter *opmlWriter = new OpmlWriter( rootOutlines, headerData, opmlFile );
    connect( opmlWriter, SIGNAL(result(int)), SLOT(slotOpmlWriterDone(int)) );
    opmlWriter->run();
}

void
SqlPodcastProvider::slotOpmlWriterDone( int result )
{
    Q_UNUSED( result )

    OpmlWriter *writer = qobject_cast<OpmlWriter *>( QObject::sender() );
    Q_ASSERT( writer );
    writer->device()->close();
    delete writer;
}

void
SqlPodcastProvider::configureChannel( Podcasts::SqlPodcastChannelPtr sqlChannel )
{
    if( !sqlChannel )
        return;

    QUrl oldUrl = sqlChannel->url();
    QUrl oldSaveLocation = sqlChannel->saveLocation();
    bool oldHasPurge = sqlChannel->hasPurge();
    int oldPurgeCount = sqlChannel->purgeCount();
    bool oldAutoScan = sqlChannel->autoScan();

    PodcastSettingsDialog dialog( sqlChannel, The::mainWindow() );
    dialog.configure();

    sqlChannel->updateInDb();

    if( ( oldHasPurge && !sqlChannel->hasPurge() )
        || ( oldPurgeCount < sqlChannel->purgeCount() ) )
    {
        /* changed from purge to no-purge or increase purge count:
        we need to reload all episodes from the database. */
        sqlChannel->loadEpisodes();
    }
    else
        sqlChannel->applyPurge();

    emit updated();

    if( oldSaveLocation != sqlChannel->saveLocation() )
    {
        moveDownloadedEpisodes( sqlChannel );
        if( !QDir().rmdir( oldSaveLocation.toLocalFile() ) )
            debug() << "Could not remove old directory " << oldSaveLocation.toLocalFile();
    }

    //if the url changed force an update.
    if( oldUrl != sqlChannel->url() )
        updateSqlChannel( sqlChannel );

    //start autoscan in case it wasn't already
    if( sqlChannel->autoScan() && !oldAutoScan )
        startTimer();
}

void
SqlPodcastProvider::deleteDownloadedEpisodes( Podcasts::SqlPodcastEpisodeList &episodes )
{
    foreach( Podcasts::SqlPodcastEpisodePtr episode, episodes )
        deleteDownloadedEpisode( episode );
}

void
SqlPodcastProvider::moveDownloadedEpisodes( Podcasts::SqlPodcastChannelPtr sqlChannel )
{
    debug() << QString( "We need to move downloaded episodes of \"%1\" to %2" )
            .arg( sqlChannel->title() )
            .arg( sqlChannel->saveLocation().toDisplayString() );

    QList<QUrl> filesToMove;
    foreach( Podcasts::SqlPodcastEpisodePtr episode, sqlChannel->sqlEpisodes() )
    {
        if( !episode->localUrl().isEmpty() )
        {
            QUrl newLocation = sqlChannel->saveLocation();
            QDir dir( newLocation.toLocalFile() );
            dir.mkpath( "." );

            newLocation = newLocation.adjusted(QUrl::StripTrailingSlash);
            newLocation.setPath(newLocation.path() + '/' + ( episode->localUrl().fileName() ));
            debug() << "Moving from " << episode->localUrl() << " to " << newLocation;
            KIO::Job *moveJob = KIO::move( episode->localUrl(), newLocation,
                                           KIO::HideProgressInfo );
            //wait until job is finished.
            if( KIO::NetAccess::synchronousRun( moveJob, The::mainWindow() ) )
                episode->setLocalUrl( newLocation );
        }
    }
}

void
SqlPodcastProvider::slotDeleteDownloadedEpisodes()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == 0 )
        return;
    Podcasts::SqlPodcastEpisodeList episodes = action->data().value<Podcasts::SqlPodcastEpisodeList>();
    deleteDownloadedEpisodes( episodes );
}

void
SqlPodcastProvider::slotDownloadEpisodes()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == 0 )
        return;
    Podcasts::SqlPodcastEpisodeList episodes = action->data().value<Podcasts::SqlPodcastEpisodeList>();

    foreach( Podcasts::SqlPodcastEpisodePtr episode, episodes )
        downloadEpisode( episode );
}

void
SqlPodcastProvider::slotSetKeep()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == 0 )
        return;

    Podcasts::SqlPodcastEpisodeList episodes = action->data().value<Podcasts::SqlPodcastEpisodeList>();

    foreach( Podcasts::SqlPodcastEpisodePtr episode, episodes )
        episode->setKeep( action->isChecked() );
}

QPair<bool, bool>
SqlPodcastProvider::confirmUnsubscribe( Podcasts::SqlPodcastChannelPtr channel )
{
    KDialog unsubscribeDialog;
    unsubscribeDialog.setCaption( i18n( "Unsubscribe" ) );

    KVBox *vbox = new KVBox( &unsubscribeDialog );

    QString question( i18n( "Do you really want to unsubscribe from \"%1\"?", channel->title() ) );
    QLabel *label = new QLabel( question, vbox );
    label->setWordWrap( true );
    label->setMaximumWidth( 400 );

    QCheckBox *deleteMediaCheckBox = new QCheckBox( i18n( "Delete downloaded episodes" ), vbox );
    unsubscribeDialog.setMainWidget( vbox );
    unsubscribeDialog.setButtons( KDialog::Ok | KDialog::Cancel );
    
    QPair<bool, bool> result;
    result.first = unsubscribeDialog.exec() == QDialog::Accepted;
    result.second = deleteMediaCheckBox->isChecked();
    return result;
}

void
SqlPodcastProvider::slotRemoveChannels()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == 0 )
        return;

    Podcasts::SqlPodcastChannelList channels = action->data().value<Podcasts::SqlPodcastChannelList>();

    foreach( Podcasts::SqlPodcastChannelPtr channel, channels )
    {
        QPair<bool, bool> result = confirmUnsubscribe( channel );        
        if( result.first )
        {
            debug() << "unsubscribing " << channel->title();
            if( result.second )
            {
                debug() << "removing all episodes";
                Podcasts::SqlPodcastEpisodeList sqlEpisodes = channel->sqlEpisodes();
                deleteDownloadedEpisodes( sqlEpisodes );
            }
            removeSubscription( channel );
        }
    }
}

void
SqlPodcastProvider::slotUpdateChannels()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
        if( action == 0 )
            return;
    Podcasts::SqlPodcastChannelList channels = action->data().value<Podcasts::SqlPodcastChannelList>();

    foreach( Podcasts::SqlPodcastChannelPtr channel, channels )
            updateSqlChannel( channel );
}

void
SqlPodcastProvider::slotDownloadProgress( KJob *job, unsigned long percent )
{
    Q_UNUSED( job );
    Q_UNUSED( percent );

    unsigned int totalDownloadPercentage = 0;
    foreach( const KJob *jobKey, m_downloadJobMap.keys() )
        totalDownloadPercentage += jobKey->percent();

    //keep the completed jobs in mind as well.
    totalDownloadPercentage += m_completedDownloads * 100;

    emit totalPodcastDownloadProgress(
        totalDownloadPercentage / ( m_downloadJobMap.count() + m_completedDownloads ) );
}

void
SqlPodcastProvider::slotWriteTagsToFiles()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == 0 )
        return;

    Podcasts::SqlPodcastEpisodeList episodes = action->data().value<Podcasts::SqlPodcastEpisodeList>();
    foreach( Podcasts::SqlPodcastEpisodePtr episode, episodes )
        episode->writeTagsToFile();
}

void
SqlPodcastProvider::slotConfigureChannel()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == 0 )
        return;

    Podcasts::SqlPodcastChannelPtr podcastChannel = action->data().value<Podcasts::SqlPodcastChannelPtr>();
    if( !podcastChannel.isNull() )
        configureChannel( podcastChannel );
}

void
SqlPodcastProvider::deleteDownloadedEpisode( Podcasts::SqlPodcastEpisodePtr episode )
{
    if( !episode || episode->localUrl().isEmpty() )
        return;

    debug() << "deleting " << episode->title();
    KIO::del( episode->localUrl(), KIO::HideProgressInfo );

    episode->setLocalUrl( QUrl() );

    emit episodeDeleted( Podcasts::PodcastEpisodePtr::dynamicCast( episode ) );
}

Podcasts::SqlPodcastChannelPtr
SqlPodcastProvider::podcastChannelForId( int podcastChannelId )
{
    QListIterator<Podcasts::SqlPodcastChannelPtr> i( m_channels );
    while( i.hasNext() )
    {
        int id = i.next()->dbId();
        if( id == podcastChannelId )
            return i.previous();
    }
    return Podcasts::SqlPodcastChannelPtr();
}

void
SqlPodcastProvider::completePodcastDownloads()
{
    //check to see if there are still downloads in progress
    if( !m_downloadJobMap.isEmpty() )
    {
        debug() << QString( "There are still %1 podcast download jobs running!" )
                .arg( m_downloadJobMap.count() );
        KProgressDialog progressDialog( The::mainWindow(),
                                i18n( "Waiting for Podcast Downloads to Finish" ),
                                i18np( "There is still a podcast download in progress",
                                       "There are still %1 podcast downloads in progress",
                                       m_downloadJobMap.count() )
                                      );
        progressDialog.setButtonText( i18n("Cancel Download and Quit.") );

        m_completedDownloads = 0;
        foreach( KJob *job, m_downloadJobMap.keys() )
        {
            connect( job, SIGNAL(percent(KJob*,ulong)),
                    SLOT(slotDownloadProgress(KJob*,ulong))
                   );
        }
        connect( this, SIGNAL(totalPodcastDownloadProgress(int)),
                 progressDialog.progressBar(), SLOT(setValue(int)) );
        int result = progressDialog.exec();
        if( result == QDialog::Rejected )
        {
            foreach( KJob *job, m_downloadJobMap.keys() )
            {
                job->kill();
            }
        }
    }
}

void
SqlPodcastProvider::autoUpdate()
{
    if( Solid::Networking::status() != Solid::Networking::Connected
            && Solid::Networking::status() != Solid::Networking::Unknown )
    {
        debug() << "Solid reports we are not online, canceling podcast auto-update";
        return;
    }

    foreach( Podcasts::SqlPodcastChannelPtr channel, m_channels )
    {
        if( channel->autoScan() )
            updateSqlChannel( channel );
    }
}

void
SqlPodcastProvider::updateSqlChannel( Podcasts::SqlPodcastChannelPtr channel )
{
    if( channel.isNull() )
        return;
    if( m_updatingChannels >= m_maxConcurrentUpdates )
    {
        debug() << QString( "Maximum concurrent updates (%1) reached. "
                            "Queueing \"%2\" for download." )
                .arg( m_maxConcurrentUpdates )
                .arg( channel->title() );
        m_updateQueue << channel;
        return;
    }

    PodcastReader *podcastReader = new PodcastReader( this );

    connect( podcastReader, SIGNAL(finished(PodcastReader*)),
             SLOT(slotReadResult(PodcastReader*)) );
    connect( podcastReader, SIGNAL(statusBarSorryMessage(QString)),
            this, SLOT(slotStatusBarSorryMessage(QString)) );
    connect( podcastReader, SIGNAL(statusBarNewProgressOperation(KIO::TransferJob*,QString,Podcasts::PodcastReader*)),
                this, SLOT(slotStatusBarNewProgressOperation(KIO::TransferJob*,QString,Podcasts::PodcastReader*)) );
    
    m_updatingChannels++;
    podcastReader->update( Podcasts::PodcastChannelPtr::dynamicCast( channel ) );
}

void
SqlPodcastProvider::slotReadResult( Podcasts::PodcastReader *podcastReader )
{
    if( podcastReader->error() != QXmlStreamReader::NoError )
    {
        debug() << podcastReader->errorString();
        Amarok::Components::logger()->longMessage( podcastReader->errorString(),
                                                   Amarok::Logger::Error );
    }
    debug() << "Finished updating: " << podcastReader->url();
    --m_updatingChannels;
    debug() << "Updating counter reached " << m_updatingChannels;

    Podcasts::SqlPodcastChannelPtr channel =
            Podcasts::SqlPodcastChannelPtr::dynamicCast( podcastReader->channel() );

    if( channel.isNull() )
    {
        error() << "Could not cast to SqlPodcastChannel " << __FILE__ << ":" << __LINE__;
        return;
    }

    if( channel->image().isNull() )
    {
        fetchImage( channel );
    }

    channel->updateInDb();

    podcastReader->deleteLater();

    //first we work through the list of new subscriptions
    if( !m_subscribeQueue.isEmpty() )
    {
        subscribe( m_subscribeQueue.takeFirst() );
    }
    else if( !m_updateQueue.isEmpty() )
    {
        updateSqlChannel( m_updateQueue.takeFirst() );
    }
    else if( m_updatingChannels == 0 )
    {
        //TODO: start downloading episodes here.
        if( m_podcastImageFetcher )
            m_podcastImageFetcher->run();
    }
}

void
SqlPodcastProvider::slotStatusBarNewProgressOperation( KIO::TransferJob * job,
                                                       const QString &description,
                                                       Podcasts::PodcastReader* reader )
{
    Amarok::Components::logger()->newProgressOperation( job, description, reader, SLOT(slotAbort()) );
}

void
SqlPodcastProvider::downloadEpisode( Podcasts::SqlPodcastEpisodePtr sqlEpisode )
{
    if( sqlEpisode.isNull() )
    {
        error() << "SqlPodcastProvider::downloadEpisode(  Podcasts::SqlPodcastEpisodePtr sqlEpisode ) was called for a non-SqlPodcastEpisode";
        return;
    }

    foreach( struct PodcastEpisodeDownload download, m_downloadJobMap )
    {
        if( download.episode == sqlEpisode )
        {
            debug() << "already downloading " << sqlEpisode->uidUrl();
            return;
        }
    }

    if( m_downloadJobMap.size() >= m_maxConcurrentDownloads )
    {
        debug() << QString( "Maximum concurrent downloads (%1) reached. "
                            "Queueing \"%2\" for download." )
                .arg( m_maxConcurrentDownloads )
                .arg( sqlEpisode->title() );
        //put into a FIFO which is used in downloadResult() to start a new download
        m_downloadQueue << sqlEpisode;
        return;
    }

    KIO::TransferJob *transferJob =
            KIO::get( QUrl::fromUserInput(sqlEpisode->uidUrl()), KIO::Reload, KIO::HideProgressInfo );


    QFile *tmpFile = createTmpFile( sqlEpisode );
    struct PodcastEpisodeDownload download = { sqlEpisode,
                                               tmpFile,
    /* Unless a redirect happens the filename from the enclosure is used. This is a potential source
       of filename conflicts in downloadResult() */
                                               QUrl( sqlEpisode->uidUrl() ).fileName(),
                                               false
                                             };
    m_downloadJobMap.insert( transferJob, download );

    if( tmpFile->exists() )
    {
        qint64 offset = tmpFile->size();
        debug() << "temporary file exists, resume download from offset " << offset;
        QMap<QString, QString> resumeData;
        resumeData.insert( "resume", QString::number( offset ) );
        transferJob->addMetaData( resumeData );
    }

    if( !tmpFile->open( QIODevice::WriteOnly | QIODevice::Append ) )
    {
        Amarok::Components::logger()->longMessage( i18n( "Unable to save podcast episode file to %1",
                                             tmpFile->fileName() ) );
        delete tmpFile;
        return;
    }

    debug() << "starting download for " << sqlEpisode->title()
            << " url: " << sqlEpisode->prettyUrl();
    Amarok::Components::logger()->newProgressOperation( transferJob
                                                        , sqlEpisode->title().isEmpty()
                                                        ? i18n( "Downloading Podcast Media" )
                                                        : i18n( "Downloading Podcast \"%1\""
                                                                , sqlEpisode->title() ),
                                                        transferJob,
                                                        SLOT(kill())
                                                      );

    connect( transferJob, SIGNAL(data(KIO::Job*,QByteArray)),
             SLOT(addData(KIO::Job*,QByteArray)) );
    //need to connect to finished instead of result because it's always emitted.
    //We need to cleanup after a download is cancled regardless of the argument in
    //KJob::kill()
    connect( transferJob, SIGNAL(finished(KJob*)),
             SLOT(downloadResult(KJob*)) );
    connect( transferJob, SIGNAL(redirection(KIO::Job*,QUrl)),
             SLOT(redirected(KIO::Job*,QUrl)) );
}

void
SqlPodcastProvider::downloadEpisode( Podcasts::PodcastEpisodePtr episode )
{
    downloadEpisode( SqlPodcastEpisodePtr::dynamicCast( episode ) );
}

void
SqlPodcastProvider::cleanupDownload( KJob *job, bool downloadFailed )
{
    struct PodcastEpisodeDownload download = m_downloadJobMap.value( job );
    QFile *tmpFile = download.tmpFile;

    if( downloadFailed && tmpFile )
    {
        debug() << "deleting temporary podcast file: " << tmpFile->fileName();
        tmpFile->remove();
    }
    m_downloadJobMap.remove( job );

    delete tmpFile;
}

QFile *
SqlPodcastProvider::createTmpFile( Podcasts::SqlPodcastEpisodePtr sqlEpisode )
{
    if( sqlEpisode.isNull() )
    {
        error() << "sqlEpisodePtr is NULL after download";
        return 0;
    }
    Podcasts::SqlPodcastChannelPtr sqlChannel =
            Podcasts::SqlPodcastChannelPtr::dynamicCast( sqlEpisode->channel() );
    if( sqlChannel.isNull() )
    {
        error() << "sqlChannelPtr is NULL after download";
        return 0;
    }

    QDir dir( sqlChannel->saveLocation().toLocalFile() );
    dir.mkpath( "." );  // ensure that the path is there
    //TODO: what if result is false?

    QUrl localUrl = QUrl::fromLocalFile( dir.absolutePath() );
    QString tempName;
    if( !sqlEpisode->guid().isEmpty() )
        tempName = QUrl::toPercentEncoding( sqlEpisode->guid() );
    else
        tempName = QUrl::toPercentEncoding( sqlEpisode->uidUrl() );

    QString tempNameMd5( KMD5( tempName.toUtf8() ).hexDigest() );

    localUrl = localUrl.adjusted(QUrl::StripTrailingSlash);
    localUrl.setPath(localUrl.path() + '/' + ( tempNameMd5 + PODCAST_TMP_POSTFIX ));

    return new QFile( localUrl.toLocalFile() );
}

bool
SqlPodcastProvider::checkEnclosureLocallyAvailable( KIO::Job *job )
{
    struct PodcastEpisodeDownload download = m_downloadJobMap.value( job );
    Podcasts::SqlPodcastEpisodePtr sqlEpisode = download.episode;
    if( sqlEpisode.isNull() )
    {
        error() << "sqlEpisodePtr is NULL after download";
        return false;
    }
    Podcasts::SqlPodcastChannelPtr sqlChannel =
            Podcasts::SqlPodcastChannelPtr::dynamicCast( sqlEpisode->channel() );
    if( sqlChannel.isNull() )
    {
        error() << "sqlChannelPtr is NULL after download";
        return false;
    }

    QString fileName = sqlChannel->saveLocation().adjusted(QUrl::StripTrailingSlash).toLocalFile();
    fileName += download.fileName;
    debug() << "checking " << fileName;
    QFileInfo fileInfo( fileName );
    if( !fileInfo.exists() )
        return false;

    debug() << fileName << " already exists, no need to redownload";
    // NOTE: we need to emit because the KJobProgressBar relies on it to clean up
    job->kill( KJob::EmitResult );
    sqlEpisode->setLocalUrl( QUrl::fromLocalFile(fileName) );
    //TODO: repaint icons, probably with signal metadataUpdate()
    return true;
}

void
SqlPodcastProvider::addData( KIO::Job *job, const QByteArray &data )
{
    if( !data.size() )
    {
        return; // EOF
    }

    struct PodcastEpisodeDownload &download = m_downloadJobMap[job];

    // NOTE: if there is a tmpfile we are already downloading, no need to
    // checkEnclosureLocallyAvailable() on every data chunk. performance optimization.
    if( !download.finalNameReady )
    {
        download.finalNameReady = true;
        if( checkEnclosureLocallyAvailable( job ) )
            return;
    }

    if( download.tmpFile->write( data ) == -1 )
    {
        error() << "write error for " << download.tmpFile->fileName() << ": "
                << download.tmpFile->errorString();
        job->kill();
    }
}

void
SqlPodcastProvider::deleteDownloadedEpisode( Podcasts::PodcastEpisodePtr episode )
{
    deleteDownloadedEpisode( SqlPodcastEpisodePtr::dynamicCast( episode ) );
}

void
SqlPodcastProvider::slotStatusBarSorryMessage( const QString &message )
{
    Amarok::Components::logger()->longMessage( message, Amarok::Logger::Error );
}

void
SqlPodcastProvider::downloadResult( KJob *job )
{
    struct PodcastEpisodeDownload download = m_downloadJobMap.value( job );
    QFile *tmpFile = download.tmpFile;
    bool downloadFailed = false;

    if( job->error() )
    {
        // NOTE: prevents empty error notifications from popping up
        // in the statusbar when the user cancels a download
        if( job->error() != KJob::KilledJobError )
        {
            Amarok::Components::logger()->longMessage( job->errorText() );
        }
        error() << "Unable to retrieve podcast media. KIO Error: " << job->errorText();
        error() << "keeping temporary file for download restart";
        downloadFailed = false;
    }
    else
    {
        Podcasts::SqlPodcastEpisodePtr sqlEpisode = download.episode;
        if( sqlEpisode.isNull() )
        {
            error() << "sqlEpisodePtr is NULL after download";
            cleanupDownload( job, true );
            return;
        }
        Podcasts::SqlPodcastChannelPtr sqlChannel =
            Podcasts::SqlPodcastChannelPtr::dynamicCast( sqlEpisode->channel() );
        if( sqlChannel.isNull() )
        {
            error() << "sqlChannelPtr is NULL after download";
            cleanupDownload( job, true );
            return;
        }

        Amarok::QStringx filenameLayout = Amarok::QStringx( sqlChannel->filenameLayout() );
        QMap<QString,QString> layoutmap;
        QString sequenceNumber;

        if( sqlEpisode->artist() )
            layoutmap.insert( "artist", sqlEpisode->artist()->prettyName() );

        layoutmap.insert( "title", sqlEpisode->title() );

        if( sqlEpisode->genre() )
            layoutmap.insert( "genre", sqlEpisode->genre()->prettyName() );

        if( sqlEpisode->year() )
            layoutmap.insert( "year", sqlEpisode->year()->prettyName() );

        if( sqlEpisode->composer() )
            layoutmap.insert( "composer", sqlEpisode->composer()->prettyName() );

        layoutmap.insert( "pubdate", sqlEpisode->pubDate().toString() );

        sequenceNumber.sprintf( "%.6d", sqlEpisode->sequenceNumber() );
        layoutmap.insert( "number", sequenceNumber );

        if( sqlEpisode->album() )
            layoutmap.insert( "album", sqlEpisode->album()->prettyName() );

        if( !filenameLayout.isEmpty() &&
                Amarok::QStringx::compare( filenameLayout, "%default%", Qt::CaseInsensitive ) )
        {
            filenameLayout = filenameLayout.namedArgs( layoutmap );
            //add the file extension to the filename
            filenameLayout.append( QString( "." ) );
            filenameLayout.append( sqlEpisode->type() );
            download.fileName = QString( filenameLayout );
        }

        QString finalName = sqlChannel->saveLocation().adjusted(QUrl::StripTrailingSlash).toLocalFile()
                            + download.fileName;
        if( tmpFile->rename( finalName ) )
        {
            debug() << "successfully written Podcast Episode " << sqlEpisode->title()
                    << " to " << finalName;
            sqlEpisode->setLocalUrl( QUrl::fromLocalFile(finalName) );

            if( sqlChannel->writeTags() )
                sqlEpisode->writeTagsToFile();
            //TODO: force a redraw of the view so the icon can be updated in the PlaylistBrowser

            emit episodeDownloaded( Podcasts::PodcastEpisodePtr::dynamicCast( sqlEpisode ) );
        }
        else
        {
            Amarok::Components::logger()->longMessage( i18n( "Unable to save podcast episode file to %1",
                                                 finalName ) );
            downloadFailed = true;
        }
    }

    //remove it from the jobmap
    m_completedDownloads++;
    cleanupDownload( job, downloadFailed );

    //start a new download. We just finished one so there is at least one slot free.
    if( !m_downloadQueue.isEmpty() )
        downloadEpisode( m_downloadQueue.takeFirst() );
}

void
SqlPodcastProvider::redirected( KIO::Job *job, const QUrl &redirectedUrl )
{
    debug() << "redirecting to " << redirectedUrl << ". filename: "
            << redirectedUrl.fileName();
    m_downloadJobMap[job].fileName = redirectedUrl.fileName();
}

void
SqlPodcastProvider::createTables() const
{
    SqlStorage *sqlStorage = StorageManager::instance()->sqlStorage();
    if( !sqlStorage )
        return;

    sqlStorage->query( QString( "CREATE TABLE podcastchannels ("
                                "id " + sqlStorage->idType() +
                                ",url " + sqlStorage->longTextColumnType() +
                                ",title " + sqlStorage->longTextColumnType() +
                                ",weblink " + sqlStorage->longTextColumnType() +
                                ",image " + sqlStorage->longTextColumnType() +
                                ",description " + sqlStorage->longTextColumnType() +
                                ",copyright "  + sqlStorage->textColumnType() +
                                ",directory "  + sqlStorage->textColumnType() +
                                ",labels " + sqlStorage->textColumnType() +
                                ",subscribedate " + sqlStorage->textColumnType() +
                                ",autoscan BOOL, fetchtype INTEGER"
                                ",haspurge BOOL, purgecount INTEGER"
                                ",writetags BOOL, filenamelayout VARCHAR(1024) ) ENGINE = MyISAM;" ) );

    sqlStorage->query( QString( "CREATE TABLE podcastepisodes ("
                                "id " + sqlStorage->idType() +
                                ",url " + sqlStorage->longTextColumnType() +
                                ",channel INTEGER"
                                ",localurl " + sqlStorage->longTextColumnType() +
                                ",guid " + sqlStorage->exactTextColumnType() +
                                ",title " + sqlStorage->longTextColumnType() +
                                ",subtitle " + sqlStorage->longTextColumnType() +
                                ",sequencenumber INTEGER" +
                                ",description " + sqlStorage->longTextColumnType() +
                                ",mimetype "  + sqlStorage->textColumnType() +
                                ",pubdate "  + sqlStorage->textColumnType() +
                                ",duration INTEGER"
                                ",filesize INTEGER"
                                ",isnew BOOL"
                                ",iskeep BOOL) ENGINE = MyISAM;" ) );

    sqlStorage->query( "CREATE FULLTEXT INDEX url_podchannel ON podcastchannels( url );" );
    sqlStorage->query( "CREATE FULLTEXT INDEX url_podepisode ON podcastepisodes( url );" );
    sqlStorage->query(
            "CREATE FULLTEXT INDEX localurl_podepisode ON podcastepisodes( localurl );" );
}

void
SqlPodcastProvider::updateDatabase( int fromVersion, int toVersion )
{
    debug() << QString( "Updating Podcast tables from version %1 to version %2" )
            .arg( fromVersion ).arg( toVersion );

    SqlStorage *sqlStorage = StorageManager::instance()->sqlStorage();
    if( !sqlStorage )
        return;
#define escape(x) sqlStorage->escape(x)

    if( fromVersion == 1 && toVersion == 2 )
    {
        QString updateChannelQuery = QString( "ALTER TABLE podcastchannels"
                                              " ADD subscribedate " + sqlStorage->textColumnType() + ';' );

        sqlStorage->query( updateChannelQuery );

        QString setDateQuery = QString(
                "UPDATE podcastchannels SET subscribedate='%1' WHERE subscribedate='';" )
                .arg( escape( QDate::currentDate().toString() ) );
        sqlStorage->query( setDateQuery );
    }
    else if( fromVersion < 3 && toVersion == 3 )
    {
        sqlStorage->query( QString( "CREATE TABLE podcastchannels_temp ("
                                    "id " + sqlStorage->idType() +
                                    ",url " + sqlStorage->exactTextColumnType() + " UNIQUE"
                                    ",title " + sqlStorage->textColumnType() +
                                    ",weblink " + sqlStorage->exactTextColumnType() +
                                    ",image " + sqlStorage->exactTextColumnType() +
                                    ",description " + sqlStorage->longTextColumnType() +
                                    ",copyright "  + sqlStorage->textColumnType() +
                                    ",directory "  + sqlStorage->textColumnType() +
                                    ",labels " + sqlStorage->textColumnType() +
                                    ",subscribedate " + sqlStorage->textColumnType() +
                                    ",autoscan BOOL, fetchtype INTEGER"
                                    ",haspurge BOOL, purgecount INTEGER ) ENGINE = MyISAM;" ) );

        sqlStorage->query( QString( "CREATE TABLE podcastepisodes_temp ("
                                    "id " + sqlStorage->idType() +
                                    ",url " + sqlStorage->exactTextColumnType() + " UNIQUE"
                                    ",channel INTEGER"
                                    ",localurl " + sqlStorage->exactTextColumnType() +
                                    ",guid " + sqlStorage->exactTextColumnType() +
                                    ",title " + sqlStorage->textColumnType() +
                                    ",subtitle " + sqlStorage->textColumnType() +
                                    ",sequencenumber INTEGER" +
                                    ",description " + sqlStorage->longTextColumnType() +
                                    ",mimetype "  + sqlStorage->textColumnType() +
                                    ",pubdate "  + sqlStorage->textColumnType() +
                                    ",duration INTEGER"
                                    ",filesize INTEGER"
                                    ",isnew BOOL"
                                    ",iskeep BOOL) ENGINE = MyISAM;" ) );

        sqlStorage->query( "INSERT INTO podcastchannels_temp SELECT * FROM podcastchannels;" );
        sqlStorage->query( "INSERT INTO podcastepisodes_temp SELECT * FROM podcastepisodes;" );

        sqlStorage->query( "DROP TABLE podcastchannels;" );
        sqlStorage->query( "DROP TABLE podcastepisodes;" );

        createTables();

        sqlStorage->query( "INSERT INTO podcastchannels SELECT * FROM podcastchannels_temp;" );
        sqlStorage->query( "INSERT INTO podcastepisodes SELECT * FROM podcastepisodes_temp;" );

        sqlStorage->query( "DROP TABLE podcastchannels_temp;" );
        sqlStorage->query( "DROP TABLE podcastepisodes_temp;" );
    }

    if( fromVersion < 4 && toVersion == 4 )
    {
        QString updateChannelQuery = QString( "ALTER TABLE podcastchannels"
                                              " ADD writetags BOOL;" );
        sqlStorage->query( updateChannelQuery );
        QString setWriteTagsQuery = QString( "UPDATE podcastchannels SET writetags=" +
                                             sqlStorage->boolTrue() +
                                             " WHERE 1;" );
        sqlStorage->query( setWriteTagsQuery );
    }

    if( fromVersion < 5 && toVersion == 5 )
    {
        QString updateChannelQuery = QString ( "ALTER TABLE podcastchannels"
                                               " ADD filenamelayout VARCHAR(1024);" );
        sqlStorage->query( updateChannelQuery );
        QString setWriteTagsQuery = QString( "UPDATE podcastchannels SET filenamelayout='%default%'" );
        sqlStorage->query( setWriteTagsQuery );
    }

    if( fromVersion < 6 && toVersion == 6 )
    {
        QString updateEpisodeQuery = QString ( "ALTER TABLE podcastepisodes"
                                               " ADD iskeep BOOL;" );
        sqlStorage->query( updateEpisodeQuery );
        QString setIsKeepQuery = QString( "UPDATE podcastepisodes SET iskeep=FALSE;" );
        sqlStorage->query( setIsKeepQuery );
    }

    QString updateAdmin = QString( "UPDATE admin SET version=%1 WHERE component='%2';" );
    sqlStorage->query( updateAdmin.arg( toVersion ).arg( escape( key ) ) );

    loadPodcasts();
}

void
SqlPodcastProvider::fetchImage( SqlPodcastChannelPtr channel )
{
    if( m_podcastImageFetcher == 0 )
    {
        m_podcastImageFetcher = new PodcastImageFetcher();
        connect( m_podcastImageFetcher,
                 SIGNAL(imageReady(Podcasts::PodcastChannelPtr,QImage)),
                 SLOT(channelImageReady(Podcasts::PodcastChannelPtr,QImage))
               );
        connect( m_podcastImageFetcher,
                 SIGNAL(done(PodcastImageFetcher*)),
                 SLOT(podcastImageFetcherDone(PodcastImageFetcher*))
               );
    }

    m_podcastImageFetcher->addChannel( PodcastChannelPtr::dynamicCast( channel ) );
}

void
SqlPodcastProvider::channelImageReady( Podcasts::PodcastChannelPtr channel, QImage image )
{
    if( image.isNull() )
        return;

    channel->setImage( image );
}

void
SqlPodcastProvider::podcastImageFetcherDone( PodcastImageFetcher *fetcher )
{
    fetcher->deleteLater();
    m_podcastImageFetcher = 0;
}

void
SqlPodcastProvider::slotConfigureProvider()
{
    configureProvider();
}


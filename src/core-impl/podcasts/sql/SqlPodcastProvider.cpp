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
// #include "context/popupdropper/libpud/PopupDropper.h"
// #include "context/popupdropper/libpud/PopupDropperItem.h"
#include <core/storage/SqlStorage.h>
#include "core/logger/Logger.h"
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
#include <KFileWidget>
#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/Job>
#include <KLocalizedString>

#include <QAction>
#include <QCheckBox>
#include <QCryptographicHash>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QMap>
#include <QMessageBox>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QNetworkConfigurationManager>
#endif
#include <QProgressDialog>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

#include <algorithm>

using namespace Podcasts;

static const int PODCAST_DB_VERSION = 6;
static const QString key( QStringLiteral("AMAROK_PODCAST") );
static const QString PODCAST_TMP_POSTFIX( QStringLiteral(".tmp") );

SqlPodcastProvider::SqlPodcastProvider()
        : m_updateTimer( new QTimer( this ) )
        , m_updatingChannels( 0 )
        , m_completedDownloads( 0 )
        , m_providerSettingsDialog( nullptr )
        , m_providerSettingsWidget( nullptr )
        , m_configureChannelAction( nullptr )
        , m_deleteAction( nullptr )
        , m_downloadAction( nullptr )
        , m_keepAction( nullptr )
        , m_removeAction( nullptr )
        , m_updateAction( nullptr )
        , m_writeTagsAction( nullptr )
        , m_podcastImageFetcher( nullptr )
{
    connect( m_updateTimer, &QTimer::timeout, this, &SqlPodcastProvider::autoUpdate );

    auto sqlStorage = StorageManager::instance()->sqlStorage();

    if( !sqlStorage )
    {
        error() << "Could not get a SqlStorage instance";
        return;
    }

    m_autoUpdateInterval = Amarok::config( QStringLiteral("Podcasts") )
                           .readEntry( "AutoUpdate Interval", 30 );
    m_maxConcurrentDownloads = Amarok::config( QStringLiteral("Podcasts") )
                               .readEntry( "Maximum Simultaneous Downloads", 4 );
    m_maxConcurrentUpdates = Amarok::config( QStringLiteral("Podcasts") )
                             .readEntry( "Maximum Simultaneous Updates", 4 );
    m_baseDownloadDir = QUrl::fromUserInput( Amarok::config( QStringLiteral("Podcasts") ).readEntry( "Base Download Directory",
                                                           Amarok::saveLocation( QStringLiteral("podcasts") ) ) );

    QStringList values;

    values = sqlStorage->query(
                 QStringLiteral( "SELECT version FROM admin WHERE component = '%1';" )
                    .arg( sqlStorage->escape( key ) )
             );
    if( values.isEmpty() )
    {
        debug() << "creating Podcast Tables";
        createTables();
        sqlStorage->query( QStringLiteral("INSERT INTO admin(component,version) "
                           "VALUES('") + key + QStringLiteral("',")
                           + QString::number( PODCAST_DB_VERSION ) + QStringLiteral(");") );
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
    for( Podcasts::SqlPodcastChannelPtr channel : m_channels )
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
    for( Podcasts::SqlPodcastChannelPtr channel : m_channels )
    {
        channel->updateInDb();
        for( Podcasts::SqlPodcastEpisodePtr episode : channel->sqlEpisodes() )
            episode->updateInDb();
    }
    m_channels.clear();

    Amarok::config( QStringLiteral("Podcasts") )
        .writeEntry( "AutoUpdate Interval", m_autoUpdateInterval );
    Amarok::config( QStringLiteral("Podcasts") )
        .writeEntry( "Maximum Simultaneous Downloads", m_maxConcurrentDownloads );
    Amarok::config( QStringLiteral("Podcasts") )
        .writeEntry( "Maximum Simultaneous Updates", m_maxConcurrentUpdates );
}

void
SqlPodcastProvider::loadPodcasts()
{
    m_channels.clear();
    auto sqlStorage = StorageManager::instance()->sqlStorage();
    if( !sqlStorage )
        return;

    QStringList results = sqlStorage->query( QStringLiteral( "SELECT id, url, title, weblink, image"
        ", description, copyright, directory, labels, subscribedate, autoscan, fetchtype"
        ", haspurge, purgecount, writetags, filenamelayout FROM podcastchannels;") );

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
    Q_EMIT updated();
}

SqlPodcastEpisodePtr
SqlPodcastProvider::sqlEpisodeForString( const QString &string )
{
    if( string.isEmpty() )
        return SqlPodcastEpisodePtr();

    auto sqlStorage = StorageManager::instance()->sqlStorage();
    if( !sqlStorage )
        return SqlPodcastEpisodePtr();

    QString command = QStringLiteral("SELECT id, url, channel, localurl, guid, "
            "title, subtitle, sequencenumber, description, mimetype, pubdate, "
            "duration, filesize, isnew, iskeep FROM podcastepisodes "
            "WHERE guid='%1' OR url='%1' OR localurl='%1' ORDER BY id DESC;");
    command = command.arg( sqlStorage->escape( string ) );
    QStringList dbResult = sqlStorage->query( command );

    if( dbResult.isEmpty() )
        return SqlPodcastEpisodePtr();

    int episodeId = dbResult[0].toInt();
    int channelId = dbResult[2].toInt();

    Podcasts::SqlPodcastChannelPtr channel;
    for( const auto &ch : m_channels )
    {
        if( ch->dbId() == channelId )
        {
            channel = ch;
            break;
        }
    }

    if( channel.isNull() )
    {
        error() << QStringLiteral( "There is a track in the database with url/guid=%1 (%2) "
                            "but there is no channel with dbId=%3 in our list!" )
                .arg( string ).arg( episodeId ).arg( channelId );
        return SqlPodcastEpisodePtr();
    }

    for( auto episode : channel->sqlEpisodes() )
        if( episode->dbId() == episodeId )
            return episode;

    //The episode was found in the database but it's channel didn't have it in it's list.
    //That probably is because it's beyond the purgecount limit or the tracks were not loaded yet.
    return SqlPodcastEpisodePtr( new SqlPodcastEpisode( dbResult.mid( 0, 15 ), channel ) );
}

bool
SqlPodcastProvider::possiblyContainsTrack( const QUrl &url ) const
{
    auto sqlStorage = StorageManager::instance()->sqlStorage();
    if( !sqlStorage )
        return false;

    QString command = QStringLiteral("SELECT id FROM podcastepisodes WHERE guid='%1' OR url='%1' "
                                     "OR localurl='%1';");
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
        QAction *updateAllAction = new QAction( QIcon::fromTheme( QStringLiteral("view-refresh-amarok") ),
                i18n( "&Update All Channels" ), this );
        updateAllAction->setProperty( "popupdropper_svg_id", QStringLiteral("update") );
        connect( updateAllAction, &QAction::triggered, this, &SqlPodcastProvider::updateAll );
        m_providerActions << updateAllAction;

        QAction *configureAction = new QAction( QIcon::fromTheme( QStringLiteral("configure") ),
                i18n( "&Configure General Settings" ), this );
        configureAction->setProperty( "popupdropper_svg_id", QStringLiteral("configure") );
        connect( configureAction, &QAction::triggered, this, &SqlPodcastProvider::slotConfigureProvider );
        m_providerActions << configureAction;

        QAction *exportOpmlAction = new QAction( QIcon::fromTheme( QStringLiteral("document-export") ),
                i18n( "&Export subscriptions to OPML file" ), this );
        connect( exportOpmlAction, &QAction::triggered, this, &SqlPodcastProvider::slotExportOpml );
        m_providerActions << exportOpmlAction;
    }

    return m_providerActions;
}

QActionList
SqlPodcastProvider::playlistActions( const Playlists::PlaylistList &playlists )
{
    QActionList actions;
    SqlPodcastChannelList sqlChannels;
    for( const Playlists::PlaylistPtr &playlist : playlists )
    {
        SqlPodcastChannelPtr sqlChannel = SqlPodcastChannel::fromPlaylistPtr( playlist );
        if( sqlChannel )
            sqlChannels << sqlChannel;
    }

    if( sqlChannels.isEmpty() )
        return actions;

    //TODO: add export OPML action for selected playlists only. Use the QAction::data() trick.
    if( m_configureChannelAction == nullptr )
    {
        m_configureChannelAction = new QAction( QIcon::fromTheme( QStringLiteral("configure") ), i18n( "&Configure" ), this );
        m_configureChannelAction->setProperty( "popupdropper_svg_id", QStringLiteral("configure") );
        connect( m_configureChannelAction, &QAction::triggered, this, &SqlPodcastProvider::slotConfigureChannel );
    }
    //only one channel can be configured at a time.
    if( sqlChannels.count() == 1 )
    {
        m_configureChannelAction->setData( QVariant::fromValue( sqlChannels.first() ) );
        actions << m_configureChannelAction;
    }

    if( m_removeAction == nullptr )
    {
        m_removeAction = new QAction( QIcon::fromTheme( QStringLiteral("news-unsubscribe") ), i18n( "&Remove Subscription" ), this );
        m_removeAction->setProperty( "popupdropper_svg_id", QStringLiteral("remove") );
        connect( m_removeAction, &QAction::triggered, this, &SqlPodcastProvider::slotRemoveChannels );
    }
    m_removeAction->setData( QVariant::fromValue( sqlChannels ) );
    actions << m_removeAction;

    if( m_updateAction == nullptr )
    {
        m_updateAction = new QAction( QIcon::fromTheme( QStringLiteral("view-refresh-amarok") ), i18n( "&Update Channel" ), this );
        m_updateAction->setProperty( "popupdropper_svg_id", QStringLiteral("update") );
        connect( m_updateAction, &QAction::triggered, this, &SqlPodcastProvider::slotUpdateChannels );
    }
    m_updateAction->setData( QVariant::fromValue( sqlChannels ) );
    actions << m_updateAction;

    return actions;
}

QActionList
SqlPodcastProvider::trackActions( const QMultiHash<Playlists::PlaylistPtr, int> &playlistTracks )
{
    SqlPodcastEpisodeList episodes;
    for( const Playlists::PlaylistPtr &playlist : playlistTracks.uniqueKeys() )
    {
        SqlPodcastChannelPtr sqlChannel = SqlPodcastChannel::fromPlaylistPtr( playlist );
        if( !sqlChannel )
            continue;

        SqlPodcastEpisodeList channelEpisodes = sqlChannel->sqlEpisodes();
        QList<int> trackPositions = playlistTracks.values( playlist );
        std::sort( trackPositions.begin(), trackPositions.end() );
        for( int trackPosition : trackPositions )
        {
            if( trackPosition >= 0 && trackPosition < channelEpisodes.count() )
                episodes << channelEpisodes.at( trackPosition );
        }
    }

    QActionList actions;
    if( episodes.isEmpty() )
        return actions;

    if( m_downloadAction == nullptr )
    {
        m_downloadAction = new QAction( QIcon::fromTheme( QStringLiteral("go-down") ), i18n( "&Download Episode" ), this );
        m_downloadAction->setProperty( "popupdropper_svg_id", QStringLiteral("download") );
        connect( m_downloadAction, &QAction::triggered, this, &SqlPodcastProvider::slotDownloadEpisodes );
    }

    if( m_deleteAction == nullptr )
    {
        m_deleteAction = new QAction( QIcon::fromTheme( QStringLiteral("edit-delete") ),
            i18n( "&Delete Downloaded Episode" ), this );
        m_deleteAction->setProperty( "popupdropper_svg_id", QStringLiteral("delete") );
        m_deleteAction->setObjectName( QStringLiteral("deleteAction") );
        connect( m_deleteAction, &QAction::triggered, this, &SqlPodcastProvider::slotDeleteDownloadedEpisodes );
    }

    if( m_writeTagsAction == nullptr )
    {
        m_writeTagsAction = new QAction( QIcon::fromTheme( QStringLiteral("media-track-edit-amarok") ),
            i18n( "&Write Feed Information to File" ), this );
        m_writeTagsAction->setProperty( "popupdropper_svg_id", QStringLiteral("edit") );
        connect( m_writeTagsAction, &QAction::triggered, this, &SqlPodcastProvider::slotWriteTagsToFiles );
    }

    if( m_keepAction == nullptr )
    {
        m_keepAction = new QAction( QIcon::fromTheme( QStringLiteral("podcast-amarok") ),
                i18n( "&Keep downloaded file" ), this );
        m_keepAction->setToolTip( i18n( "Toggle the \"keep\" downloaded file status of "
                "this podcast episode. Downloaded files with this status wouldn't be "
                "deleted even if we apply a purge." ) );
        m_keepAction->setProperty( "popupdropper_svg_id", QStringLiteral("keep") );
        m_keepAction->setCheckable( true );
        connect( m_keepAction, &QAction::triggered, this, &SqlPodcastProvider::slotSetKeep );
    }

    SqlPodcastEpisodeList remoteEpisodes;
    SqlPodcastEpisodeList keptDownloadedEpisodes, unkeptDownloadedEpisodes;
    for( const SqlPodcastEpisodePtr &episode : episodes )
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

    auto sqlStorage = StorageManager::instance()->sqlStorage();
    if( !sqlStorage )
        return;

    QString command = QStringLiteral("SELECT title FROM podcastchannels WHERE url='%1';");
    command = command.arg( sqlStorage->escape( kurl.url() ) );

    QStringList dbResult = sqlStorage->query( command );
    if( !dbResult.isEmpty() )
    {
        //Already subscribed to this Channel
        //notify the user.
        Amarok::Logger::longMessage(
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
    for( Podcasts::SqlPodcastChannelPtr channel : m_channels )
        updateSqlChannel( channel );
}

void
SqlPodcastProvider::subscribe( const QUrl &url )
{
    if( !url.isValid() )
        return;

    if( m_updatingChannels >= m_maxConcurrentUpdates )
    {
        debug() << QStringLiteral( "Maximum concurrent updates (%1) reached. "
                                   "Queueing \"%2\" for subscribing." )
                        .arg( m_maxConcurrentUpdates )
                        .arg( url.url() );
        m_subscribeQueue << url;
        return;
    }

    PodcastReader *podcastReader = new PodcastReader( this );
    connect( podcastReader, &PodcastReader::finished,
             this, &SqlPodcastProvider::slotReadResult );
    connect( podcastReader, &PodcastReader::statusBarErrorMessage,
             this, &SqlPodcastProvider::slotStatusBarErrorMessage );
    connect( podcastReader, &PodcastReader::statusBarNewProgressOperation,
             this, &SqlPodcastProvider::slotStatusBarNewProgressOperation );

    m_updatingChannels++;
    podcastReader->read( url );
}

Podcasts::PodcastChannelPtr
SqlPodcastProvider::addChannel(const PodcastChannelPtr &channel )
{
    Podcasts::SqlPodcastChannelPtr sqlChannel =
            SqlPodcastChannelPtr( new Podcasts::SqlPodcastChannel( this, channel ) );
    m_channels << sqlChannel;

    if( sqlChannel->episodes().isEmpty() )
        updateSqlChannel( sqlChannel );

    Q_EMIT playlistAdded( Playlists::PlaylistPtr( sqlChannel.data() ) );
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
        auto sqlStorage = StorageManager::instance()->sqlStorage();
        if( !sqlStorage )
            return;
        debug() << "Unsubscribed from last channel, cleaning out the podcastepisodes table.";
        sqlStorage->query( QStringLiteral("DELETE FROM podcastepisodes WHERE 1;") );
    }

    Q_EMIT playlistRemoved( Playlists::PlaylistPtr::dynamicCast( sqlChannel ) );
}

void
SqlPodcastProvider::configureProvider()
{
    m_providerSettingsDialog = new QDialog( The::mainWindow() );
    QWidget *settingsWidget = new QWidget( m_providerSettingsDialog );
    m_providerSettingsDialog->setObjectName( QStringLiteral("SqlPodcastProviderSettings") );
    Ui::SqlPodcastProviderSettingsWidget settings;
    m_providerSettingsWidget = &settings;
    settings.setupUi( settingsWidget );

    settings.m_baseDirUrl->setMode( KFile::Directory );
    settings.m_baseDirUrl->setUrl( m_baseDownloadDir );

    settings.m_autoUpdateInterval->setValue( m_autoUpdateInterval );
    settings.m_autoUpdateInterval->setPrefix(
            i18nc( "prefix to 'x minutes'", "every " ) );
    settings.m_autoUpdateInterval->setSuffix( i18np( " minute", " minutes", settings.m_autoUpdateInterval->value() ) );

    auto buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, m_providerSettingsDialog );

    connect( settings.m_baseDirUrl, &KUrlRequester::textChanged, this, &SqlPodcastProvider::slotConfigChanged );
    connect( settings.m_autoUpdateInterval, QOverload<int>::of(&QSpinBox::valueChanged),
             this, &SqlPodcastProvider::slotConfigChanged );

    m_providerSettingsDialog->setWindowTitle( i18n( "Configure Local Podcasts" ) );
    buttonBox->button( QDialogButtonBox::Apply )->setEnabled( false );

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
            Amarok::config( QStringLiteral("Podcasts") ).writeEntry( "Base Download Directory", m_baseDownloadDir );
            if( !m_channels.isEmpty() )
            {
                //TODO: check if there actually are downloaded episodes
                auto button = QMessageBox::question( The::mainWindow(),
                                                     i18n( "Move Podcasts" ),
                                                     i18n( "Do you want to move all downloaded episodes to the new location?") );

                if( button == QMessageBox::Yes )
                {
                    for( SqlPodcastChannelPtr sqlChannel : m_channels )
                    {
                        QUrl oldSaveLocation = sqlChannel->saveLocation();
                        QUrl newSaveLocation = m_baseDownloadDir;
                        newSaveLocation = newSaveLocation.adjusted(QUrl::StripTrailingSlash);
                        newSaveLocation.setPath(newSaveLocation.path() + QLatin1Char('/') + ( oldSaveLocation.fileName() ));
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
    m_providerSettingsDialog = nullptr;
    m_providerSettingsWidget = nullptr;
}

void
SqlPodcastProvider::slotConfigChanged()
{
    if( !m_providerSettingsWidget )
        return;

    if( m_providerSettingsWidget->m_autoUpdateInterval->value() != m_autoUpdateInterval
        || m_providerSettingsWidget->m_baseDirUrl->url() != m_baseDownloadDir )
    {
        auto buttonBox = m_providerSettingsDialog->findChild<QDialogButtonBox*>();
        buttonBox->button( QDialogButtonBox::Apply )->setEnabled( true );
    }
}

void
SqlPodcastProvider::slotExportOpml()
{
    QList<OpmlOutline *> rootOutlines;
    QMap<QString,QString> headerData;
    //TODO: set header data such as date

    //TODO: folder outline support
    for( SqlPodcastChannelPtr channel : m_channels )
    {
        OpmlOutline *channelOutline = new OpmlOutline();
        #define addAttr( k, v ) channelOutline->addAttribute( k, v )
        addAttr( QStringLiteral("text"), channel->title() );
        addAttr( QStringLiteral("type"), QStringLiteral("rss") );
        addAttr( QStringLiteral("xmlUrl"), channel->url().url() );
        rootOutlines << channelOutline;
    }

    //TODO: add checkbox as widget to filedialog to include podcast settings.
    QFileDialog fileDialog;
    fileDialog.restoreState( Amarok::config( QStringLiteral("amarok-podcast-export-dialog") ).readEntry( "state", QByteArray() ) );

    fileDialog.setMimeTypeFilters( QStringList( QStringLiteral( "*.opml" ) ) );
    fileDialog.setAcceptMode( QFileDialog::AcceptSave );
    fileDialog.setFileMode( QFileDialog::AnyFile );
    fileDialog.setWindowTitle( i18n( "Select file for OPML export") );

    if( fileDialog.exec() != QDialog::Accepted )
        return;

    QString filePath = fileDialog.selectedFiles().value( 0 );

    QFile *opmlFile = new QFile( filePath, this );
    if( !opmlFile->open( QIODevice::WriteOnly | QIODevice::Truncate ) )
    {
        error() << "could not open OPML file " << filePath;
        return;
    }
    OpmlWriter *opmlWriter = new OpmlWriter( rootOutlines, headerData, opmlFile );
    connect( opmlWriter, &OpmlWriter::result, this, &SqlPodcastProvider::slotOpmlWriterDone );
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

    Q_EMIT updated();

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
    for( Podcasts::SqlPodcastEpisodePtr episode : episodes )
        deleteDownloadedEpisode( episode );
}

void
SqlPodcastProvider::moveDownloadedEpisodes( Podcasts::SqlPodcastChannelPtr sqlChannel )
{
    debug() << QStringLiteral( "We need to move downloaded episodes of \"%1\" to %2" )
            .arg( sqlChannel->title(),
                  sqlChannel->saveLocation().toDisplayString() );

    for( Podcasts::SqlPodcastEpisodePtr episode : sqlChannel->sqlEpisodes() )
    {
        if( !episode->localUrl().isEmpty() )
        {
            QUrl newLocation = sqlChannel->saveLocation();
            QDir dir( newLocation.toLocalFile() );
            dir.mkpath( QStringLiteral(".") );

            newLocation = newLocation.adjusted(QUrl::StripTrailingSlash);
            newLocation.setPath(newLocation.path() + QLatin1Char('/') + ( episode->localUrl().fileName() ));
            debug() << "Moving from " << episode->localUrl() << " to " << newLocation;
            KIO::Job *moveJob = KIO::move( episode->localUrl(), newLocation,
                                           KIO::HideProgressInfo );
            //wait until job is finished.
            if( moveJob->exec() )
                episode->setLocalUrl( newLocation );
        }
    }
}

void
SqlPodcastProvider::slotDeleteDownloadedEpisodes()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == nullptr )
        return;
    Podcasts::SqlPodcastEpisodeList episodes = action->data().value<Podcasts::SqlPodcastEpisodeList>();
    deleteDownloadedEpisodes( episodes );
}

void
SqlPodcastProvider::slotDownloadEpisodes()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == nullptr )
        return;
    Podcasts::SqlPodcastEpisodeList episodes = action->data().value<Podcasts::SqlPodcastEpisodeList>();

    for( Podcasts::SqlPodcastEpisodePtr episode : episodes )
        downloadEpisode( episode );
}

void
SqlPodcastProvider::slotSetKeep()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == nullptr )
        return;

    Podcasts::SqlPodcastEpisodeList episodes = action->data().value<Podcasts::SqlPodcastEpisodeList>();

    for( Podcasts::SqlPodcastEpisodePtr episode : episodes )
        episode->setKeep( action->isChecked() );
}

QPair<bool, bool>
SqlPodcastProvider::confirmUnsubscribe( Podcasts::SqlPodcastChannelPtr channel )
{
    QMessageBox unsubscribeDialog;
    unsubscribeDialog.setText( i18n( "Do you really want to unsubscribe from \"%1\"?", channel->title() ) );
    unsubscribeDialog.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );

    QCheckBox *deleteMediaCheckBox = new QCheckBox( i18n( "Delete downloaded episodes" ), nullptr );
    unsubscribeDialog.setCheckBox( deleteMediaCheckBox );
    
    QPair<bool, bool> result;
    result.first = unsubscribeDialog.exec() == QMessageBox::Ok;
    result.second = deleteMediaCheckBox->isChecked();
    return result;
}

void
SqlPodcastProvider::slotRemoveChannels()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == nullptr )
        return;

    Podcasts::SqlPodcastChannelList channels = action->data().value<Podcasts::SqlPodcastChannelList>();

    for( Podcasts::SqlPodcastChannelPtr channel : channels )
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
        if( action == nullptr )
            return;
    Podcasts::SqlPodcastChannelList channels = action->data().value<Podcasts::SqlPodcastChannelList>();

    for( Podcasts::SqlPodcastChannelPtr channel : channels )
            updateSqlChannel( channel );
}

void
SqlPodcastProvider::slotDownloadProgress( KJob *job, unsigned long percent )
{
    Q_UNUSED( job );
    Q_UNUSED( percent );

    unsigned int totalDownloadPercentage = 0;
    for( const KJob *jobKey : m_downloadJobMap.keys() )
        totalDownloadPercentage += jobKey->percent();

    //keep the completed jobs in mind as well.
    totalDownloadPercentage += m_completedDownloads * 100;

    Q_EMIT totalPodcastDownloadProgress(
        totalDownloadPercentage / ( m_downloadJobMap.count() + m_completedDownloads ) );
}

void
SqlPodcastProvider::slotWriteTagsToFiles()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == nullptr )
        return;

    Podcasts::SqlPodcastEpisodeList episodes = action->data().value<Podcasts::SqlPodcastEpisodeList>();
    for( Podcasts::SqlPodcastEpisodePtr episode : episodes )
        episode->writeTagsToFile();
}

void
SqlPodcastProvider::slotConfigureChannel()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == nullptr )
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

    Q_EMIT episodeDeleted( Podcasts::PodcastEpisodePtr::dynamicCast( episode ) );
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
        debug() << QStringLiteral( "There are still %1 podcast download jobs running!" )
                .arg( m_downloadJobMap.count() );
        QProgressDialog progressDialog( i18np( "There is still a podcast download in progress",
                                        "There are still %1 podcast downloads in progress",
                                        m_downloadJobMap.count() ),
                                        i18n("Cancel Download and Quit."),
                                        0, m_downloadJobMap.size(), The::mainWindow()
                                      );
        progressDialog.setValue( 0 );
        m_completedDownloads = 0;
        for( KJob *job : m_downloadJobMap.keys() )
        {
            connect( job, SIGNAL(percent(KJob*,ulong)),
                     this, SLOT(slotDownloadProgress(KJob*,ulong))
                   );
        }
        connect( this, &SqlPodcastProvider::totalPodcastDownloadProgress,
                 &progressDialog, &QProgressDialog::setValue );
        int result = progressDialog.exec();
        if( result == QDialog::Rejected )
        {
            for( KJob *job : m_downloadJobMap.keys() )
            {
                job->kill();
            }
        }
    }
}

void
SqlPodcastProvider::autoUpdate()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QNetworkConfigurationManager mgr;
    if( !mgr.isOnline() )
    {
        debug() << "Solid reports we are not online, canceling podcast auto-update";
        return;
    }
#endif

    for( Podcasts::SqlPodcastChannelPtr channel : m_channels )
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
        debug() << QStringLiteral( "Maximum concurrent updates (%1) reached. "
                                   "Queueing \"%2\" for download." )
                .arg( m_maxConcurrentUpdates )
                .arg( channel->title() );
        m_updateQueue << channel;
        return;
    }

    PodcastReader *podcastReader = new PodcastReader( this );

    connect( podcastReader, &PodcastReader::finished,
             this, &SqlPodcastProvider::slotReadResult );
    connect( podcastReader, &PodcastReader::statusBarErrorMessage,
             this, &SqlPodcastProvider::slotStatusBarErrorMessage );
    connect( podcastReader, &PodcastReader::statusBarNewProgressOperation,
             this, &SqlPodcastProvider::slotStatusBarNewProgressOperation );
    
    m_updatingChannels++;
    podcastReader->update( Podcasts::PodcastChannelPtr::dynamicCast( channel ) );
}

void
SqlPodcastProvider::slotReadResult( Podcasts::PodcastReader *podcastReader )
{
    if( podcastReader->error() != QXmlStreamReader::NoError )
    {
        debug() << podcastReader->errorString();
        Amarok::Logger::longMessage( podcastReader->errorString(),
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
    Amarok::Logger::newProgressOperation( job, description, reader, &Podcasts::PodcastReader::slotAbort );
}

void
SqlPodcastProvider::downloadEpisode( Podcasts::SqlPodcastEpisodePtr sqlEpisode )
{
    if( sqlEpisode.isNull() )
    {
        error() << "SqlPodcastProvider::downloadEpisode(  Podcasts::SqlPodcastEpisodePtr sqlEpisode ) was called for a non-SqlPodcastEpisode";
        return;
    }

    for( struct PodcastEpisodeDownload download : m_downloadJobMap )
    {
        if( download.episode == sqlEpisode )
        {
            debug() << "already downloading " << sqlEpisode->uidUrl();
            return;
        }
    }

    if( m_downloadJobMap.size() >= m_maxConcurrentDownloads )
    {
        debug() << QStringLiteral( "Maximum concurrent downloads (%1) reached. "
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
        resumeData.insert( QStringLiteral("resume"), QString::number( offset ) );
        transferJob->addMetaData( resumeData );
    }

    if( !tmpFile->open( QIODevice::WriteOnly | QIODevice::Append ) )
    {
        Amarok::Logger::longMessage( i18n( "Unable to save podcast episode file to %1",
                                             tmpFile->fileName() ) );
        delete tmpFile;
        return;
    }

    debug() << "starting download for " << sqlEpisode->title()
            << " url: " << sqlEpisode->prettyUrl();
    Amarok::Logger::newProgressOperation( transferJob
                                                        , sqlEpisode->title().isEmpty()
                                                        ? i18n( "Downloading Podcast Media" )
                                                        : i18n( "Downloading Podcast \"%1\""
                                                                , sqlEpisode->title() ),
                                                        transferJob,
                                                        &KIO::TransferJob::kill,
                                                        Qt::AutoConnection,
                                                        KJob::Quietly
                                                      );

    connect( transferJob, &KIO::TransferJob::data,
             this, &SqlPodcastProvider::addData );
    //need to connect to finished instead of result because it's always emitted.
    //We need to cleanup after a download is canceled regardless of the argument in
    //KJob::kill()
    connect( transferJob, &KIO::TransferJob::finished,
             this, &SqlPodcastProvider::downloadResult );
    connect( transferJob, &KIO::TransferJob::redirection,
             this, &SqlPodcastProvider::redirected );
}

void
SqlPodcastProvider::downloadEpisode( const Podcasts::PodcastEpisodePtr &episode )
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
        return nullptr;
    }
    Podcasts::SqlPodcastChannelPtr sqlChannel =
            Podcasts::SqlPodcastChannelPtr::dynamicCast( sqlEpisode->channel() );
    if( sqlChannel.isNull() )
    {
        error() << "sqlChannelPtr is NULL after download";
        return nullptr;
    }

    QDir dir( sqlChannel->saveLocation().toLocalFile() );
    dir.mkpath( QStringLiteral(".") );  // ensure that the path is there
    //TODO: what if result is false?

    QUrl localUrl = QUrl::fromLocalFile( dir.absolutePath() );
    QByteArray tempName;
    if( !sqlEpisode->guid().isEmpty() )
        tempName = QUrl::toPercentEncoding( sqlEpisode->guid() );
    else
        tempName = QUrl::toPercentEncoding( sqlEpisode->uidUrl() );

    QLatin1String tempNameMd5( QCryptographicHash::hash( tempName, QCryptographicHash::Md5 ).toHex() );

    localUrl = localUrl.adjusted(QUrl::StripTrailingSlash);
    localUrl.setPath(localUrl.path() + QLatin1Char('/') + ( tempNameMd5 + PODCAST_TMP_POSTFIX ));

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

    QString fileName = sqlChannel->saveLocation().adjusted(QUrl::StripTrailingSlash).toLocalFile()
                       + QLatin1Char('/')
                       + download.fileName;
    debug() << "checking " << fileName;
    QFileInfo fileInfo( fileName );
    if( !fileInfo.exists() )
        return false;

    debug() << fileName << " already exists, no need to redownload";
    // NOTE: we need to Q_EMIT because the KJobProgressBar relies on it to clean up
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
SqlPodcastProvider::deleteDownloadedEpisode( const Podcasts::PodcastEpisodePtr &episode )
{
    deleteDownloadedEpisode( SqlPodcastEpisodePtr::dynamicCast( episode ) );
}

void
SqlPodcastProvider::slotStatusBarErrorMessage( const QString &message )
{
    Amarok::Logger::longMessage( message, Amarok::Logger::Error );
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
            Amarok::Logger::longMessage( job->errorText() );
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
            layoutmap.insert( QStringLiteral("artist"), sqlEpisode->artist()->prettyName() );

        layoutmap.insert( QStringLiteral("title"), sqlEpisode->title() );

        if( sqlEpisode->genre() )
            layoutmap.insert( QStringLiteral("genre"), sqlEpisode->genre()->prettyName() );

        if( sqlEpisode->year() )
            layoutmap.insert( QStringLiteral("year"), sqlEpisode->year()->prettyName() );

        if( sqlEpisode->composer() )
            layoutmap.insert( QStringLiteral("composer"), sqlEpisode->composer()->prettyName() );

        layoutmap.insert( QStringLiteral("pubdate"), sqlEpisode->pubDate().toString() );

        sequenceNumber.asprintf( "%.6d", sqlEpisode->sequenceNumber() );
        layoutmap.insert( QStringLiteral("number"), sequenceNumber );

        if( sqlEpisode->album() )
            layoutmap.insert( QStringLiteral("album"), sqlEpisode->album()->prettyName() );

        if( !filenameLayout.isEmpty() &&
                Amarok::QStringx::compare( filenameLayout, QStringLiteral("%default%"), Qt::CaseInsensitive ) )
        {
            filenameLayout = Amarok::QStringx(filenameLayout.namedArgs( layoutmap ));
            //add the file extension to the filename
            filenameLayout.append( QStringLiteral( "." ) );
            filenameLayout.append( sqlEpisode->type() );
            download.fileName = QString( filenameLayout );
        }

        QString finalName = sqlChannel->saveLocation().adjusted(QUrl::StripTrailingSlash).toLocalFile()
                            + QLatin1Char('/')
                            + download.fileName;
        if( tmpFile->rename( finalName ) )
        {
            debug() << "successfully written Podcast Episode " << sqlEpisode->title()
                    << " to " << finalName;
            sqlEpisode->setLocalUrl( QUrl::fromLocalFile(finalName) );

            if( sqlChannel->writeTags() )
                sqlEpisode->writeTagsToFile();
            //TODO: force a redraw of the view so the icon can be updated in the PlaylistBrowser

            Q_EMIT episodeDownloaded( Podcasts::PodcastEpisodePtr::dynamicCast( sqlEpisode ) );
        }
        else
        {
            Amarok::Logger::longMessage( i18n( "Unable to save podcast episode file to %1",
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
    auto sqlStorage = StorageManager::instance()->sqlStorage();
    if( !sqlStorage )
        return;

    sqlStorage->query( QStringLiteral( "CREATE TABLE podcastchannels ("
                                "id ") + sqlStorage->idType() +
                                QStringLiteral(",url ") + sqlStorage->longTextColumnType() +
                                QStringLiteral(",title ") + sqlStorage->longTextColumnType() +
                                QStringLiteral(",weblink ") + sqlStorage->longTextColumnType() +
                                QStringLiteral(",image ") + sqlStorage->longTextColumnType() +
                                QStringLiteral(",description ") + sqlStorage->longTextColumnType() +
                                QStringLiteral(",copyright ")  + sqlStorage->textColumnType() +
                                QStringLiteral(",directory ")  + sqlStorage->textColumnType() +
                                QStringLiteral(",labels ") + sqlStorage->textColumnType() +
                                QStringLiteral(",subscribedate ") + sqlStorage->textColumnType() +
                                QStringLiteral(",autoscan BOOL, fetchtype INTEGER"
                                ",haspurge BOOL, purgecount INTEGER"
                                ",writetags BOOL, filenamelayout VARCHAR(1024) ) ENGINE = MyISAM;" ) );

    sqlStorage->query( QStringLiteral( "CREATE TABLE podcastepisodes ("
                                "id ") + sqlStorage->idType() +
                                QStringLiteral(",url ") + sqlStorage->longTextColumnType() +
                                QStringLiteral(",channel INTEGER"
                                ",localurl ") + sqlStorage->longTextColumnType() +
                                QStringLiteral(",guid ") + sqlStorage->exactTextColumnType() +
                                QStringLiteral(",title ") + sqlStorage->longTextColumnType() +
                                QStringLiteral(",subtitle ") + sqlStorage->longTextColumnType() +
                                QStringLiteral(",sequencenumber INTEGER"
                                ",description ") + sqlStorage->longTextColumnType() +
                                QStringLiteral(",mimetype ")  + sqlStorage->textColumnType() +
                                QStringLiteral(",pubdate ")  + sqlStorage->textColumnType() +
                                QStringLiteral(",duration INTEGER"
                                ",filesize INTEGER"
                                ",isnew BOOL"
                                ",iskeep BOOL) ENGINE = MyISAM;" ) );

    sqlStorage->query( QStringLiteral("CREATE FULLTEXT INDEX url_podchannel ON podcastchannels( url );") );
    sqlStorage->query( QStringLiteral("CREATE FULLTEXT INDEX url_podepisode ON podcastepisodes( url );") );
    sqlStorage->query(
            QStringLiteral("CREATE FULLTEXT INDEX localurl_podepisode ON podcastepisodes( localurl );") );
}

void
SqlPodcastProvider::updateDatabase( int fromVersion, int toVersion )
{
    debug() << QStringLiteral( "Updating Podcast tables from version %1 to version %2" )
            .arg( fromVersion ).arg( toVersion );

    auto sqlStorage = StorageManager::instance()->sqlStorage();
    if( !sqlStorage )
        return;
#define escape(x) sqlStorage->escape(x)

    if( fromVersion == 1 && toVersion == 2 )
    {
        QString updateChannelQuery = QStringLiteral( "ALTER TABLE podcastchannels"
                                              " ADD subscribedate ") + sqlStorage->textColumnType() + QLatin1Char(';');

        sqlStorage->query( updateChannelQuery );

        QString setDateQuery = QStringLiteral(
                "UPDATE podcastchannels SET subscribedate='%1' WHERE subscribedate='';" )
                .arg( escape( QDate::currentDate().toString() ) );
        sqlStorage->query( setDateQuery );
    }
    else if( fromVersion < 3 && toVersion == 3 )
    {
        sqlStorage->query( QStringLiteral( "CREATE TABLE podcastchannels_temp ("
                                    "id ") + sqlStorage->idType() +
                                    QStringLiteral(",url ") + sqlStorage->exactTextColumnType() + QStringLiteral(" UNIQUE"
                                    ",title ") + sqlStorage->textColumnType() +
                                    QStringLiteral(",weblink ") + sqlStorage->exactTextColumnType() +
                                    QStringLiteral(",image ") + sqlStorage->exactTextColumnType() +
                                    QStringLiteral(",description ") + sqlStorage->longTextColumnType() +
                                    QStringLiteral(",copyright ")  + sqlStorage->textColumnType() +
                                    QStringLiteral(",directory ")  + sqlStorage->textColumnType() +
                                    QStringLiteral(",labels ") + sqlStorage->textColumnType() +
                                    QStringLiteral(",subscribedate ") + sqlStorage->textColumnType() +
                                    QStringLiteral(",autoscan BOOL, fetchtype INTEGER"
                                    ",haspurge BOOL, purgecount INTEGER ) ENGINE = MyISAM;" ) );

        sqlStorage->query( QStringLiteral( "CREATE TABLE podcastepisodes_temp ("
                                    "id ") + sqlStorage->idType() +
                                    QStringLiteral(",url ") + sqlStorage->exactTextColumnType() + QStringLiteral(" UNIQUE"
                                    ",channel INTEGER"
                                    ",localurl ") + sqlStorage->exactTextColumnType() +
                                    QStringLiteral(",guid ") + sqlStorage->exactTextColumnType() +
                                    QStringLiteral(",title ") + sqlStorage->textColumnType() +
                                    QStringLiteral(",subtitle ") + sqlStorage->textColumnType() +
                                    QStringLiteral(",sequencenumber INTEGER"
                                    ",description ") + sqlStorage->longTextColumnType() +
                                    QStringLiteral(",mimetype ")  + sqlStorage->textColumnType() +
                                    QStringLiteral(",pubdate ")  + sqlStorage->textColumnType() +
                                    QStringLiteral(",duration INTEGER"
                                    ",filesize INTEGER"
                                    ",isnew BOOL"
                                    ",iskeep BOOL) ENGINE = MyISAM;" ) );

        sqlStorage->query( QStringLiteral("INSERT INTO podcastchannels_temp SELECT * FROM podcastchannels;") );
        sqlStorage->query( QStringLiteral("INSERT INTO podcastepisodes_temp SELECT * FROM podcastepisodes;") );

        sqlStorage->query( QStringLiteral("DROP TABLE podcastchannels;") );
        sqlStorage->query( QStringLiteral("DROP TABLE podcastepisodes;") );

        createTables();

        sqlStorage->query( QStringLiteral("INSERT INTO podcastchannels SELECT * FROM podcastchannels_temp;") );
        sqlStorage->query( QStringLiteral("INSERT INTO podcastepisodes SELECT * FROM podcastepisodes_temp;") );

        sqlStorage->query( QStringLiteral("DROP TABLE podcastchannels_temp;") );
        sqlStorage->query( QStringLiteral("DROP TABLE podcastepisodes_temp;") );
    }

    if( fromVersion < 4 && toVersion == 4 )
    {
        QString updateChannelQuery = QStringLiteral( "ALTER TABLE podcastchannels"
                                              " ADD writetags BOOL;" );
        sqlStorage->query( updateChannelQuery );
        QString setWriteTagsQuery = QStringLiteral( "UPDATE podcastchannels SET writetags=") +
                                             sqlStorage->boolTrue() +
                                             QStringLiteral(" WHERE 1;" );
        sqlStorage->query( setWriteTagsQuery );
    }

    if( fromVersion < 5 && toVersion == 5 )
    {
        QString updateChannelQuery = QStringLiteral ( "ALTER TABLE podcastchannels"
                                               " ADD filenamelayout VARCHAR(1024);" );
        sqlStorage->query( updateChannelQuery );
        QString setWriteTagsQuery = QStringLiteral( "UPDATE podcastchannels SET filenamelayout='%default%'" );
        sqlStorage->query( setWriteTagsQuery );
    }

    if( fromVersion < 6 && toVersion == 6 )
    {
        QString updateEpisodeQuery = QStringLiteral ( "ALTER TABLE podcastepisodes"
                                               " ADD iskeep BOOL;" );
        sqlStorage->query( updateEpisodeQuery );
        QString setIsKeepQuery = QStringLiteral( "UPDATE podcastepisodes SET iskeep=FALSE;" );
        sqlStorage->query( setIsKeepQuery );
    }

    QString updateAdmin = QStringLiteral( "UPDATE admin SET version=%1 WHERE component='%2';" );
    sqlStorage->query( updateAdmin.arg( toVersion ).arg( escape( key ) ) );

    loadPodcasts();
}

void
SqlPodcastProvider::fetchImage( const SqlPodcastChannelPtr &channel )
{
    if( m_podcastImageFetcher == nullptr )
    {
        m_podcastImageFetcher = new PodcastImageFetcher();
        connect( m_podcastImageFetcher, &PodcastImageFetcher::channelImageReady,
                 this, &SqlPodcastProvider::channelImageReady );
                 connect( m_podcastImageFetcher,&PodcastImageFetcher::done,
                 this, &SqlPodcastProvider::podcastImageFetcherDone );
    }

    m_podcastImageFetcher->addChannel( PodcastChannelPtr::dynamicCast( channel ) );
}

void
SqlPodcastProvider::channelImageReady( Podcasts::PodcastChannelPtr channel, const QImage &image )
{
    if( image.isNull() )
        return;

    channel->setImage( image );
}

void
SqlPodcastProvider::podcastImageFetcherDone( PodcastImageFetcher *fetcher )
{
    fetcher->deleteLater();
    m_podcastImageFetcher = nullptr;
}

void
SqlPodcastProvider::slotConfigureProvider()
{
    configureProvider();
}


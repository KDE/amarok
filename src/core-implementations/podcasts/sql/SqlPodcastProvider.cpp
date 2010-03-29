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
#include <kprogressdialog.h>

#include "core/support/Amarok.h"
#include "CollectionManager.h"
#include "context/popupdropper/libpud/PopupDropperItem.h"
#include "context/popupdropper/libpud/PopupDropper.h"
#include "core/support/Debug.h"
#include "core/podcasts/PodcastImageFetcher.h"
#include "PodcastModel.h"
#include "core/podcasts/PodcastReader.h"
#include "PodcastSettingsDialog.h"
#include "playlistmanager/sql/SqlPlaylistGroup.h"
#include "SqlStorage.h"
#include "statusbar/StatusBar.h"
#include "SvgHandler.h"

#include "ui_SqlPodcastProviderSettingsWidget.h"

#include <KLocale>
#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/Job>
#include <KIO/NetAccess>
#include <KStandardDirs>
#include <KUrl>
#include <Solid/Networking>

#include <QAction>
#include <QFile>
#include <QDir>
#include <QTimer>
#include <QCheckBox>

using namespace Podcasts;

static const int PODCAST_DB_VERSION = 4;
static const QString key( "AMAROK_PODCAST" );
static const QString PODCAST_TMP_POSTFIX( ".tmp" );

SqlPodcastProvider::SqlPodcastProvider()
        : m_updateTimer( new QTimer( this ) )
        , m_updatingChannels( 0 )
        , m_completedDownloads( 0 )
        , m_providerSettingsDialog( 0 )
        , m_configureChannelAction( 0 )
        , m_deleteAction( 0 )
        , m_downloadAction( 0 )
        , m_removeAction( 0 )
        , m_renameAction( 0 )
        , m_updateAction( 0 )
        , m_writeTagsAction( 0 )
        , m_podcastImageFetcher( 0 )
{
    connect( m_updateTimer, SIGNAL( timeout() ), SLOT( autoUpdate() ) );

    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();

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
    m_baseDownloadDir = Amarok::config( "Podcasts" ).readEntry( "Base Downlaod Directory",
                                                           Amarok::saveLocation( "podcasts" ) );

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

        bool startAutoRefreshTimer = false;
        foreach( Podcasts::SqlPodcastChannelPtr channel, m_channels )
        {
            startAutoRefreshTimer = channel->autoScan();
        }
        if( startAutoRefreshTimer )
        {
            float interval = 1.0;
            m_updateTimer->start( interval * 1000 * 60 * m_autoUpdateInterval );
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
    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();
    if( !sqlStorage )
        return;

    QStringList results = sqlStorage->query( "SELECT id, url, title, weblink, image"
        ", description, copyright, directory, labels, subscribedate, autoscan, fetchtype"
        ", haspurge, purgecount, writetags FROM podcastchannels;" );

    int rowLength = 15;
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
    emit( updated() );
}

SqlPodcastEpisodePtr
SqlPodcastProvider::sqlEpisodeForString( const QString &string )
{
    if( string.isEmpty() )
        return SqlPodcastEpisodePtr();

    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();
    if( !sqlStorage )
        return SqlPodcastEpisodePtr();

    QString command = "SELECT id, url, channel, localurl, guid, "
            "title, subtitle, sequencenumber, description, mimetype, pubdate, "
            "duration, filesize, isnew FROM podcastepisodes "
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
    //That probably is because it's beyond the purgecount limit.
    return SqlPodcastEpisodePtr( new SqlPodcastEpisode( dbResult.mid( 0, 14 ), channel ) );
}

bool
SqlPodcastProvider::possiblyContainsTrack( const KUrl &url ) const
{
    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();
    if( !sqlStorage )
        return false;

    QString command = "SELECT title FROM podcastepisodes WHERE guid='%1' OR url='%1' "
                      "OR localurl='%1';";
    command = command.arg( sqlStorage->escape( url.url() ) );

    QStringList dbResult = sqlStorage->query( command );
    return !dbResult.isEmpty();
}

Meta::TrackPtr
SqlPodcastProvider::trackForUrl( const KUrl &url )
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

QList<QAction *>
SqlPodcastProvider::providerActions()
{
    if( m_providerActions.isEmpty() )
    {
        QAction *updateAllAction = new QAction( KIcon( "view-refresh-amarok" ),
                                         i18n( "&Update All Channels" ),
                                         this
                                       );
        updateAllAction->setProperty( "popupdropper_svg_id", "update" );
        connect( updateAllAction, SIGNAL( triggered() ), this, SLOT( updateAll() ) );
        m_providerActions << updateAllAction;

        QAction *configureAction = new QAction( KIcon( "configure" ),
            i18n( "&Configure General Settings" ),
            this
        );
        configureAction->setProperty( "popupdropper_svg_id", "configure" );
        connect( configureAction, SIGNAL( triggered() ), this, SLOT( slotConfigureProvider() ) );
        m_providerActions << configureAction;
    }

    return m_providerActions;
}

QList<QAction *>
SqlPodcastProvider::playlistActions( Playlists::PlaylistPtr playlist )
{
    QList<QAction *> actions;

    Podcasts::SqlPodcastChannelPtr sqlChannel = Podcasts::SqlPodcastChannel::fromPlaylistPtr( playlist );
    if( sqlChannel.isNull() )
        return actions;

    if( m_configureChannelAction == 0 )
    {
        m_configureChannelAction = new QAction(
            KIcon( "configure" ),
            i18n( "&Configure" ),
            this
        );
        m_configureChannelAction->setProperty( "popupdropper_svg_id", "configure" );
        connect( m_configureChannelAction, SIGNAL( triggered() ), SLOT( slotConfigureChannel() ) );
    }

    //only one channel can be configured at a time.
    if( m_configureChannelAction->data().isNull() )
        m_configureChannelAction->setData( QVariant::fromValue( sqlChannel ) );

    actions << m_configureChannelAction;

    Podcasts::SqlPodcastChannelList actionChannels;
    if( m_removeAction == 0 )
    {
        m_removeAction = new QAction(
            KIcon( "news-unsubscribe" ),
            i18n( "&Remove Subscription" ),
            this
        );
        m_removeAction->setProperty( "popupdropper_svg_id", "remove" );
        connect( m_removeAction, SIGNAL( triggered() ), SLOT( slotRemoveChannels() ) );
    }
    else
    {
        actionChannels = m_removeAction->data().value<Podcasts::SqlPodcastChannelList>();
    }

    actionChannels << sqlChannel;
    m_removeAction->setData( QVariant::fromValue( actionChannels ) );

    actions << m_removeAction;

    actionChannels.clear();
    if( m_updateAction == 0 )
    {
        m_updateAction = new QAction(
            KIcon( "view-refresh-amarok" ),
            i18n( "&Update Channel" ),
            this
        );
        m_updateAction->setProperty( "popupdropper_svg_id", "update" );
        connect( m_updateAction, SIGNAL( triggered() ), SLOT( slotUpdateChannels() ) );
    }
    else
    {
        actionChannels = m_updateAction->data().value<Podcasts::SqlPodcastChannelList>();
    }

    actionChannels << sqlChannel;
    m_updateAction->setData( QVariant::fromValue( actionChannels ) );

    actions << m_updateAction;

    return actions;
}

QList<QAction *>
SqlPodcastProvider::trackActions( Playlists::PlaylistPtr playlist, int trackIndex )
{
    QList<QAction *> actions;
    Podcasts::SqlPodcastChannelPtr sqlChannel = Podcasts::SqlPodcastChannel::fromPlaylistPtr( playlist );
    if( sqlChannel.isNull() )
        return actions;

    Podcasts::SqlPodcastEpisodeList sqlEpisodes = sqlChannel->sqlEpisodes();
    if( trackIndex >= sqlEpisodes.count() )
        return actions;

    Podcasts::SqlPodcastEpisodePtr sqlEpisode = sqlEpisodes.at( trackIndex );
    if( sqlEpisode.isNull() )
        return actions;

    if( m_deleteAction == 0 )
    {
        m_deleteAction = new QAction(
            KIcon( "edit-delete" ),
            i18n( "&Delete Downloaded Episode" ),
            this
        );
        m_deleteAction->setProperty( "popupdropper_svg_id", "delete" );
        connect( m_deleteAction, SIGNAL( triggered() ), SLOT( slotDeleteDownloadedEpisodes() ) );
    }

    if( m_writeTagsAction == 0 )
    {
        m_writeTagsAction = new QAction(
            KIcon( "media-track-edit-amarok" ),
            i18n( "&Write Feed Information to File" ),
            this
        );
        m_writeTagsAction->setProperty( "popupdropper_svg_id", "edit" );
        connect( m_writeTagsAction, SIGNAL( triggered() ), SLOT( slotWriteTagsToFiles() ) );
    }

    Podcasts::SqlPodcastEpisodeList actionEpisodes;
    if( !sqlEpisode->localUrl().isEmpty() )
    {
        actionEpisodes = m_deleteAction->data().value<Podcasts::SqlPodcastEpisodeList>();
        actionEpisodes << sqlEpisode;
        m_deleteAction->setData( QVariant::fromValue( actionEpisodes ) );
        //these lists are the same anyway
        m_writeTagsAction->setData( QVariant::fromValue( actionEpisodes ) );
        actions << m_deleteAction;
        actions << m_writeTagsAction;
    }
    else
    {
        if( m_downloadAction == 0 )
        {
            m_downloadAction = new QAction(
                KIcon( "go-down" ),
                i18n( "&Download Episode" ),
                this
            );
            m_downloadAction->setProperty( "popupdropper_svg_id", "download" );
            connect( m_downloadAction, SIGNAL( triggered() ), SLOT( slotDownloadEpisodes() ) );
        }
        else
        {
            actionEpisodes = m_downloadAction->data().value<Podcasts::SqlPodcastEpisodeList>();
        }
        actionEpisodes << sqlEpisode;
        m_downloadAction->setData( QVariant::fromValue( actionEpisodes ) );
        actions << m_downloadAction;
    }

    return actions;
}

Podcasts::PodcastEpisodePtr
SqlPodcastProvider::episodeForGuid( const QString &guid )
{
    return PodcastEpisodePtr::dynamicCast( sqlEpisodeForString( guid ) );
}

void
SqlPodcastProvider::addPodcast( const KUrl &url )
{
    KUrl kurl = KUrl( url );
    debug() << "importing " << kurl.url();

    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();
    if( !sqlStorage )
        return;

    QString command = "SELECT title FROM podcastchannels WHERE url='%1';";
    command = command.arg( sqlStorage->escape( kurl.url() ) );

    QStringList dbResult = sqlStorage->query( command );
    if( !dbResult.isEmpty() )
    {
        //Already subscribed to this Channel
        //notify the user.
        The::statusBar()->longMessage(
            i18n( "Already subscribed to %1.", dbResult.first() )
            , StatusBar::Error );
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
SqlPodcastProvider::subscribe( const KUrl &url )
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

    PodcastReader * podcastReader = new PodcastReader( this );
    connect( podcastReader, SIGNAL( finished( PodcastReader * ) ),
             SLOT( slotReadResult( PodcastReader * ) ) );
    connect( podcastReader, SIGNAL( statusBarSorryMessage( const QString & ) ),
            this, SLOT( slotStatusBarSorryMessage( const QString & ) ) );
    connect( podcastReader, SIGNAL( statusBarNewProgressOperation( KIO::TransferJob *, const QString &, Podcasts::PodcastReader* ) ),
            this, SLOT( slotStatusBarNewProgressOperation( KIO::TransferJob *, const QString &, Podcasts::PodcastReader* ) ) );

    m_updatingChannels++;
    podcastReader->read( url );
}

Podcasts::PodcastChannelPtr
SqlPodcastProvider::addChannel( Podcasts::PodcastChannelPtr channel )
{
    Podcasts::SqlPodcastChannel *sqlChannel = new Podcasts::SqlPodcastChannel( this, channel );
    m_channels << SqlPodcastChannelPtr( sqlChannel );
    return Podcasts::PodcastChannelPtr( sqlChannel );
}

Podcasts::PodcastEpisodePtr
SqlPodcastProvider::addEpisode( Podcasts::PodcastEpisodePtr episode )
{
    DEBUG_BLOCK
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
        SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();
        if( !sqlStorage )
            return;
        debug() << "Unsubscribed from last channel, cleaning out the podcastepisodes table.";
        sqlStorage->query( "DELETE FROM podcastepisodes WHERE 1;" );
    }

    emit updated();
}

void
SqlPodcastProvider::configureProvider()
{
    DEBUG_BLOCK
    m_providerSettingsDialog = new KDialog( The::mainWindow() );
    QWidget *settingsWidget = new QWidget( m_providerSettingsDialog );
    Ui::SqlPodcastProviderSettingsWidget settings;
    settings.setupUi( settingsWidget );

    settings.m_baseDirUrl->setMode( KFile::Directory );
    settings.m_baseDirUrl->setUrl( m_baseDownloadDir );

    settings.m_autoUpdateInterval->setValue( m_autoUpdateInterval );
    settings.m_autoUpdateInterval->setSuffix(ki18np(" minute", " minutes"));

    m_providerSettingsDialog->setButtons( KDialog::Ok | KDialog::Cancel | KDialog::Apply );
    m_providerSettingsDialog->setMainWidget( settingsWidget );

    connect( settings.m_baseDirUrl, SIGNAL( textChanged(QString) ), SLOT( slotConfigChanged() ) );
    connect( settings.m_autoUpdateInterval, SIGNAL( valueChanged(int) ),
             SLOT( slotConfigChanged() ) );

    m_providerSettingsDialog->setWindowTitle( i18n( "Configure Local Podcasts" ) );
    m_providerSettingsDialog->enableButtonApply( false );

    if( m_providerSettingsDialog->exec() == QDialog::Accepted )
    {
        debug() << "accepted";

        //TODO: apply
    }

    delete m_providerSettingsDialog;
    m_providerSettingsDialog = 0;

}

void
SqlPodcastProvider::slotConfigChanged()
{
    Ui::SqlPodcastProviderSettingsWidget *settings =
            dynamic_cast<Ui::SqlPodcastProviderSettingsWidget *>(
                    m_providerSettingsDialog->mainWidget() );

    if( !settings )
        return;

    if( settings->m_autoUpdateInterval->value() != m_autoUpdateInterval
        || settings->m_baseDirUrl->url() != m_baseDownloadDir )
    {
        m_providerSettingsDialog->enableButtonApply( true );
    }
}

void
SqlPodcastProvider::configureChannel( Podcasts::SqlPodcastChannelPtr sqlChannel )
{
    if( !sqlChannel )
        return;

    KUrl oldUrl = sqlChannel->url();
    KUrl oldSaveLocation = sqlChannel->saveLocation();
    bool oldHasPurge = sqlChannel->hasPurge();
    int oldPurgeCount = sqlChannel->purgeCount();

    PodcastSettingsDialog dialog( sqlChannel, The::mainWindow() );
    dialog.configure();

    sqlChannel->updateInDb();

    if( sqlChannel->hasPurge() )
    {
        int toPurge = sqlChannel->purgeCount();
        if( !oldHasPurge || ( oldPurgeCount != toPurge && toPurge > 0 ) )
        {
            debug() << "purge to " << toPurge << " newest episodes for "
                    << sqlChannel->title();
            foreach( Podcasts::SqlPodcastEpisodePtr episode, sqlChannel->sqlEpisodes() )
            {
                if( --toPurge < 0 )
                    if( !episode->localUrl().isEmpty() )
                        deleteDownloadedEpisode( episode );
            }
            sqlChannel->loadEpisodes();
            emit( updated() );
        }
    }
    else if( oldHasPurge )
    {
        /* changed from purge to no-purge:
        we need to reload all episodes from the database. */
        sqlChannel->loadEpisodes();
        emit( updated() );
    }

    if( oldSaveLocation != sqlChannel->saveLocation() )
    {
        debug() << QString( "We need to move downloaded episodes of \"%1\" to %2" )
                .arg( sqlChannel->title() )
                .arg( sqlChannel->saveLocation().prettyUrl() );

        KUrl::List filesToMove;
        foreach( Podcasts::SqlPodcastEpisodePtr episode, sqlChannel->sqlEpisodes() )
        {
            if( !episode->localUrl().isEmpty() )
            {
                KUrl newLocation = sqlChannel->saveLocation();
                QDir dir( newLocation.toLocalFile() );
                dir.mkpath( "." );

                newLocation.addPath( episode->localUrl().fileName() );
                debug() << "Moving from " << episode->localUrl() << " to " << newLocation;
                KIO::Job *moveJob = KIO::move( episode->localUrl(), newLocation,
                                               KIO::HideProgressInfo );
                //wait until job is finished.
                if( KIO::NetAccess::synchronousRun( moveJob, The::mainWindow() ) )
                    episode->setLocalUrl( newLocation );
            }
        }

        if( !QDir().rmdir( oldSaveLocation.toLocalFile() ) )
            debug() << "Could not remove old directory " << oldSaveLocation.toLocalFile();
    }

    //if the url changed force an update.
    if( oldUrl != sqlChannel->url() )
        updateSqlChannel( sqlChannel );
}

QList<QAction *>
SqlPodcastProvider::episodeActions( Podcasts::PodcastEpisodeList episodes )
{
    QList< QAction * > actions;
    if( episodes.isEmpty() )
        return actions;

    Podcasts::PodcastEpisodeList actionEpisodes;
    if( m_deleteAction == 0 )
    {
        m_deleteAction = new QAction(
            KIcon( "edit-delete" ),
            i18n( "&Delete Downloaded Episode" ),
            this
        );
        m_deleteAction->setProperty( "popupdropper_svg_id", "delete" );
        connect( m_deleteAction, SIGNAL( triggered() ), SLOT( slotDeleteDownloadedEpisodes() ) );
    }


    actionEpisodes.clear();
    if( m_writeTagsAction == 0 )
    {
        m_writeTagsAction = new QAction(
            KIcon( "media-track-edit-amarok" ),
            i18n( "&Write Feed Information to File" ),
            this
        );
        m_writeTagsAction->setProperty( "popupdropper_svg_id", "edit" );
        connect( m_writeTagsAction, SIGNAL( triggered() ), SLOT( slotWriteTagsToFiles() ) );
    }

    bool hasDownloaded = false;
    foreach( Podcasts::PodcastEpisodePtr episode, episodes )
    {
        Podcasts::SqlPodcastEpisodePtr sqlEpisode
                = Podcasts::SqlPodcastEpisodePtr::dynamicCast( episode );
        if( sqlEpisode.isNull() )
            break;

        if( !sqlEpisode->localUrl().isEmpty() )
        {
            hasDownloaded = true;
            break;
        }
    }
    if( hasDownloaded )
    {
        Podcasts::PodcastEpisodeList actionEpisodes = m_deleteAction->data().value<Podcasts::PodcastEpisodeList>();
        actionEpisodes << episodes;
        m_deleteAction->setData( QVariant::fromValue( actionEpisodes ) );
        //these lists are the same anyway
        m_writeTagsAction->setData( QVariant::fromValue( actionEpisodes ) );
        actions << m_deleteAction;
        actions << m_writeTagsAction;
    }
    else
    {
        if( m_downloadAction == 0 )
        {
            m_downloadAction = new QAction(
                KIcon( "go-down" ),
                i18n( "&Download Episode" ),
                this
            );
            m_downloadAction->setProperty( "popupdropper_svg_id", "download" );
            connect( m_downloadAction, SIGNAL( triggered() ), SLOT( slotDownloadEpisodes() ) );
        }
        else
        {
            actionEpisodes = m_downloadAction->data().value<Podcasts::PodcastEpisodeList>();
        }
        actionEpisodes << episodes;
        m_downloadAction->setData( QVariant::fromValue( actionEpisodes ) );
        actions << m_downloadAction;
    }

    return actions;
}

QList<QAction *>
SqlPodcastProvider::channelActions( Podcasts::PodcastChannelList channels )
{
    QList< QAction * > actions;

    if( channels.isEmpty() )
        return actions;

    if( m_configureChannelAction == 0 )
    {
        m_configureChannelAction = new QAction(
            KIcon( "configure" ),
            i18n( "&Configure" ),
            this
        );
        m_configureChannelAction->setProperty( "popupdropper_svg_id", "configure" );
        connect( m_configureChannelAction, SIGNAL( triggered() ), SLOT( slotConfigureChannel() ) );
    }

    //only one playlist can be renamed at a time.
    if( m_configureChannelAction->data().isNull() )
        m_configureChannelAction->setData( QVariant::fromValue( channels.first() ) );

    actions << m_configureChannelAction;

    Podcasts::PodcastChannelList actionChannels;
    if( m_removeAction == 0 )
    {
        m_removeAction = new QAction(
            KIcon( "news-unsubscribe" ),
            i18n( "&Remove Subscription" ),
            this
        );
        m_removeAction->setProperty( "popupdropper_svg_id", "remove" );
        connect( m_removeAction, SIGNAL( triggered() ), SLOT( slotRemoveChannels() ) );
    }
    else
    {
        actionChannels = m_removeAction->data().value<Podcasts::PodcastChannelList>();
    }

    actionChannels << channels;
    m_removeAction->setData( QVariant::fromValue( actionChannels ) );

    actions << m_removeAction;

    actionChannels.clear();
    if( m_updateAction == 0 )
    {
        m_updateAction = new QAction(
            KIcon( "view-refresh-amarok" ),
            i18n( "&Update Channel" ),
            this
        );
        m_updateAction->setProperty( "popupdropper_svg_id", "update" );
        connect( m_updateAction, SIGNAL( triggered() ), SLOT( slotUpdateChannels() ) );
    }
    else
    {
        actionChannels = m_updateAction->data().value<Podcasts::PodcastChannelList>();
    }

    actionChannels << channels;
    m_updateAction->setData( QVariant::fromValue( actionChannels ) );

    actions << m_updateAction;

    return actions;
}

void
SqlPodcastProvider::deleteDownloadedEpisodes( Podcasts::SqlPodcastEpisodeList &episodes )
{
    foreach( Podcasts::SqlPodcastEpisodePtr episode, episodes )
        deleteDownloadedEpisode( episode );
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

QPair<bool, bool>
SqlPodcastProvider::confirmUnsubscribe( Podcasts::SqlPodcastChannelPtr channel )
{
    KDialog unsubscribeDialog;
    unsubscribeDialog.setCaption( i18n( "Unsubscribe" ) );

    KVBox *vbox = new KVBox( &unsubscribeDialog );

    QString question( i18n( "Do you really want to unsubscribe from ") + "\"" );
    question += channel->title();
    question += "\"?";
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

    bool removedSomething = false;
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
                removedSomething = true;
            }
            removeSubscription( channel );
            removedSomething = true;
        }
    }
    if( removedSomething )
        emit updated();
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

    episode->setLocalUrl( KUrl() );
    emit( updated() );
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
            connect( job, SIGNAL( percent( KJob *, unsigned long ) ),
                    SLOT( slotDownloadProgress( KJob *, unsigned long ) )
                   );
        }
        connect( this, SIGNAL( totalPodcastDownloadProgress( int ) ),
                 progressDialog.progressBar(), SLOT( setValue( int ) ) );
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
    DEBUG_BLOCK
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

    connect( podcastReader, SIGNAL( finished( PodcastReader * ) ),
             SLOT( slotReadResult( PodcastReader * ) ) );
    connect( podcastReader, SIGNAL( statusBarMessage( const QString & ) ),
            this, SLOT( slotStatusBarSorryMessage( const QString & ) ) );
    connect( podcastReader, SIGNAL( statusBarNewProgressOperation( KIO::TransferJob *, const QString &, Podcasts::PodcastReader* ) ),
                this, SLOT( slotStatusBarNewProgressOperation( KIO::TransferJob *, const QString &, Podcasts::PodcastReader* ) ) );
    
    m_updatingChannels++;
    podcastReader->update( Podcasts::PodcastChannelPtr::dynamicCast( channel ) );
}

void
SqlPodcastProvider::slotReadResult( Podcasts::PodcastReader *podcastReader )
{
    DEBUG_BLOCK
    if( podcastReader->error() != QXmlStreamReader::NoError )
    {
        debug() << podcastReader->errorString();
        The::statusBar()->longMessage( podcastReader->errorString(), StatusBar::Error );
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

    emit( updated() );

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
    The::statusBar()->newProgressOperation( job, description )
            ->setAbortSlot( reader, SLOT( slotAbort() ) );
}

void
SqlPodcastProvider::downloadEpisode( Podcasts::SqlPodcastEpisodePtr sqlEpisode )
{
    if( sqlEpisode.isNull() )
    {
        error() << "SqlPodcastProvider::downloadEpisode(  Podcasts::SqlPodcastEpisodePtr sqlEpisode ) was called for a non-SqlPodcastEpisode";
        return;
    }

    if( m_downloadJobMap.values().contains( sqlEpisode ) )
    {
        debug() << "already downloading " << sqlEpisode->uidUrl();
        return;
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
            KIO::get( sqlEpisode->uidUrl(), KIO::Reload, KIO::HideProgressInfo );

    m_downloadJobMap[transferJob] = sqlEpisode;
    m_fileNameMap[transferJob] = KUrl( sqlEpisode->uidUrl() ).fileName();

    debug() << "starting download for " << sqlEpisode->title()
            << " url: " << sqlEpisode->prettyUrl();
    The::statusBar()->newProgressOperation( transferJob
                                            , sqlEpisode->title().isEmpty()
                                            ? i18n( "Downloading Podcast Media" )
                                            : i18n( "Downloading Podcast \"%1\""
                                                    , sqlEpisode->title() )
                                          )->setAbortSlot( transferJob, SLOT( kill() ) );

    connect( transferJob, SIGNAL( data( KIO::Job *, const QByteArray & ) ),
             SLOT( addData( KIO::Job *, const QByteArray & ) ) );
    //need to connect to finished instead of result because it's always emited.
    //We need to cleanup after a download is cancled regardless of the argument in
    //KJob::kill()
    connect( transferJob, SIGNAL( finished( KJob * ) ),
             SLOT( downloadResult( KJob * ) ) );
    connect( transferJob, SIGNAL( redirection( KIO::Job *, const KUrl& ) ),
             SLOT( redirected( KIO::Job *, const KUrl& ) ) );
}

void
SqlPodcastProvider::downloadEpisode( Podcasts::PodcastEpisodePtr episode )
{
    downloadEpisode( SqlPodcastEpisodePtr::dynamicCast( episode ) );
}

void
SqlPodcastProvider::cleanupDownload( KJob *job, bool downloadFailed )
{
    DEBUG_BLOCK

    QFile *tmpFile = m_tmpFileMap.value( job );

    if( downloadFailed && tmpFile )
    {
        debug() << "deleting temporary podcast file: " << tmpFile->fileName();
        tmpFile->remove();
    }
    m_downloadJobMap.remove( job );
    m_fileNameMap.remove( job );
    m_tmpFileMap.remove( job );

    delete tmpFile;
}

QFile*
SqlPodcastProvider::createTmpFile( KJob *job )
{
    DEBUG_BLOCK

    Podcasts::SqlPodcastEpisodePtr sqlEpisode = m_downloadJobMap.value( job );
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

    QDir dir( sqlChannel->saveLocation().path() );
    dir.mkpath( "." );  // ensure that the path is there

    KUrl localUrl = KUrl::fromPath( dir.absolutePath() );
    QString tmpFileName = m_fileNameMap.value( job );
    localUrl.addPath( tmpFileName + PODCAST_TMP_POSTFIX );

    QFile *tmpFile = new QFile( localUrl.path() );
    if( tmpFile->open( QIODevice::WriteOnly ) )
    {
        debug() << "podcast tmpfile created: " << localUrl.path();
        return tmpFile;
    }
    else
    {
        The::statusBar()->longMessage( i18n( "Unable to save podcast episode file to %1",
                                             localUrl.prettyUrl() ) );
        delete tmpFile;
        return 0;
    }
}

bool
SqlPodcastProvider::checkEnclosureLocallyAvailable( KIO::Job *job )
{
    Podcasts::SqlPodcastEpisodePtr sqlEpisode = m_downloadJobMap.value( job );
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

    QString fileName = sqlChannel->saveLocation().path(KUrl::AddTrailingSlash);
    fileName += m_fileNameMap.value( job );
    debug() << "checking " << fileName;
    QFileInfo fileInfo( fileName );
    if( !fileInfo.exists() )
        return false;

    debug() << fileName << " already exists, no need to redownload";
    // NOTE: we need to emit because the KJobProgressBar relies on it to clean up
    job->kill( KJob::EmitResult );
    sqlEpisode->setLocalUrl( fileName );
    emit( updated() );  // repaint icons
    return true;
}

void
SqlPodcastProvider::addData( KIO::Job *job, const QByteArray &data )
{
    if( !data.size() )
    {
        return; // EOF
    }

    QFile *tmpFile = m_tmpFileMap.value( job );

    // NOTE: if there is a tmpfile we are already downloading, no need to
    // checkEnclosureLocallyAvailable() on every data chunk. performance optimization.
    if ( !tmpFile && checkEnclosureLocallyAvailable( job ) )
    {
        return;
    }

    if( !tmpFile )
    {
        tmpFile = createTmpFile( job );
        if( !tmpFile )
        {
            debug() << "failed to create tmpfile for podcast download";
            job->kill();
            return;
        }
        m_tmpFileMap[job] = tmpFile;
    }

    if( tmpFile->write( data ) == -1 )
    {
        error() << "write error for " << tmpFile->fileName() << ": " <<
        tmpFile->errorString();
        job->kill();
    }
}

void
SqlPodcastProvider::deleteDownloadedEpisode( Podcasts::PodcastEpisodePtr episode )
{
    DEBUG_BLOCK
    deleteDownloadedEpisode( SqlPodcastEpisodePtr::dynamicCast( episode ) );
}

void
SqlPodcastProvider::slotStatusBarSorryMessage( const QString &message )
{
    The::statusBar()->longMessage( message, StatusBar::Sorry );
}

void
SqlPodcastProvider::slotUpdated()
{
    emit updated();
}

void
SqlPodcastProvider::downloadResult( KJob *job )
{
    QFile *tmpFile = m_tmpFileMap.value( job );
    bool downloadFailed = false;

    if( job->error() )
    {
        // NOTE: prevents empty error notifications from popping up
        // in the statusbar when the user cancels a download
        if( job->error() != KJob::KilledJobError )
        {
            The::statusBar()->longMessage( job->errorText() );
        }
        debug() << "Unable to retrieve podcast media. KIO Error: " << job->errorText();
        downloadFailed = true;
    }
    else if( !m_downloadJobMap.contains( job ) )
    {
        warning() << "Download is finished for a job that was not added to m_downloadJobMap. Waah?";
        downloadFailed = true;
    }
    else
    {
        Podcasts::SqlPodcastEpisodePtr sqlEpisode = m_downloadJobMap.value( job );
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

        QString finalName = tmpFile->fileName();
        finalName.chop( PODCAST_TMP_POSTFIX.length() );
        if( tmpFile->rename( finalName ) )
        {
            debug() << "successfully written Podcast Episode " << sqlEpisode->title()
                    << " to " << finalName;
            sqlEpisode->setLocalUrl( finalName );

            if( sqlChannel->writeTags() )
                sqlEpisode->writeTagsToFile();
            //force an update so the icon can be updated in the PlaylistBrowser
            emit( updated() );
        }
        else
        {
            The::statusBar()->longMessage( i18n( "Unable to save podcast episode file to %1",
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
SqlPodcastProvider::redirected( KIO::Job *job, const KUrl & redirectedUrl )
{
    debug() << "redirecting to " << redirectedUrl << ". filename: "
            << redirectedUrl.fileName();
    m_fileNameMap[job] = redirectedUrl.fileName();
}

void
SqlPodcastProvider::createTables() const
{
    DEBUG_BLOCK

    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();
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
                                ",writetags BOOL ) ENGINE = MyISAM;" ) );

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
                                ",isnew BOOL ) ENGINE = MyISAM;" ) );

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

    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();
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
                                    ",isnew BOOL ) ENGINE = MyISAM;" ) );

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


    QString updateAdmin = QString( "UPDATE admin SET version=%1 WHERE component='%2';" );
    sqlStorage->query( updateAdmin.arg( toVersion ).arg( escape( key ) ) );

    loadPodcasts();
}

void
SqlPodcastProvider::fetchImage( SqlPodcastChannelPtr channel )
{
    DEBUG_BLOCK

    if( m_podcastImageFetcher == 0 )
    {
        m_podcastImageFetcher = new PodcastImageFetcher();
        connect( m_podcastImageFetcher,
                 SIGNAL( imageReady( Podcasts::PodcastChannelPtr, QPixmap ) ),
                 SLOT( channelImageReady( Podcasts::PodcastChannelPtr, QPixmap ) )
               );
        connect( m_podcastImageFetcher,
                 SIGNAL( done( PodcastImageFetcher * ) ),
                 SLOT( podcastImageFetcherDone( PodcastImageFetcher * ) )
               );
    }

    m_podcastImageFetcher->addChannel( PodcastChannelPtr::dynamicCast( channel ) );
}

void
SqlPodcastProvider::channelImageReady( Podcasts::PodcastChannelPtr channel, QPixmap pixmap )
{
    DEBUG_BLOCK
    debug() << "channel: " << channel->title();
    if( pixmap.isNull() )
        return;

    channel->setImage( pixmap );
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

#include "SqlPodcastProvider.moc"

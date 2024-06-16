/****************************************************************************************
 * Copyright (c) 2010 Bart Cerneels <bart.cerneels@kde.org>                             *
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
#include "UmsPodcastProvider.h"
#include "core/support/Debug.h"

#include <KConfigGroup>
#include <KIO/DeleteJob>
#include <KIO/FileCopyJob>
#include <KIO/Job>

#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDirIterator>
#include <QLabel>
#include <QListWidget>
#include <QObject>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMimeDatabase>
#include <QMimeType>

#include <algorithm>

using namespace Podcasts;

UmsPodcastProvider::UmsPodcastProvider( const QUrl &scanDirectory )
        : m_scanDirectory( scanDirectory )
        , m_deleteEpisodeAction( nullptr )
        , m_deleteChannelAction( nullptr )
{

}

UmsPodcastProvider::~UmsPodcastProvider()
{

}

bool
UmsPodcastProvider::possiblyContainsTrack( const QUrl &url ) const
{
    Q_UNUSED( url )
    return false;
}

Meta::TrackPtr
UmsPodcastProvider::trackForUrl( const QUrl &url )
{
    Q_UNUSED( url )
    return Meta::TrackPtr();
}

PodcastEpisodePtr
UmsPodcastProvider::episodeForGuid( const QString &guid )
{
    Q_UNUSED( guid )
    return PodcastEpisodePtr();
}

void
UmsPodcastProvider::addPodcast( const QUrl &url )
{
    Q_UNUSED( url );
}

PodcastChannelPtr
UmsPodcastProvider::addChannel( const PodcastChannelPtr &channel )
{
    UmsPodcastChannelPtr umsChannel = UmsPodcastChannelPtr(
            new UmsPodcastChannel( channel, this ) );
    m_umsChannels << umsChannel;

    Q_EMIT playlistAdded( Playlists::PlaylistPtr( umsChannel.data() ) );
    return PodcastChannelPtr( umsChannel.data() );
}

PodcastEpisodePtr
UmsPodcastProvider::addEpisode( PodcastEpisodePtr episode )
{
    QUrl localFilePath = episode->playableUrl();
    if( !localFilePath.isLocalFile() )
        return PodcastEpisodePtr();

    QUrl destination = m_scanDirectory;
    destination = destination.adjusted(QUrl::StripTrailingSlash);
    destination.setPath(destination.path() + QLatin1Char('/') + ( Amarok::vfatPath( episode->channel()->prettyName() ) ));
    KIO::mkdir( destination );
    destination = destination.adjusted(QUrl::StripTrailingSlash);
    destination.setPath(destination.path() + QLatin1Char('/') + ( Amarok::vfatPath( localFilePath.fileName() ) ));

    debug() << QString( "Copy episode \"%1\" to %2" ).arg( localFilePath.path(),
            destination.path() );
    KIO::FileCopyJob *copyJob = KIO::file_copy( localFilePath, destination );
    connect( copyJob, &KJob::result, this, &UmsPodcastProvider::slotCopyComplete );
    copyJob->start();
    //we have not copied the data over yet so we can't return an episode yet
    //TODO: return a proxy for the episode we are still copying.
    return PodcastEpisodePtr();
}

void
UmsPodcastProvider::slotCopyComplete( KJob *job )
{
    KIO::FileCopyJob *copyJob = dynamic_cast<KIO::FileCopyJob *>( job );
    if( !copyJob )
        return;

    QUrl localFilePath = copyJob->destUrl();
    MetaFile::Track *fileTrack = new MetaFile::Track( localFilePath );

    UmsPodcastEpisodePtr umsEpisode = addFile( MetaFile::TrackPtr( fileTrack ) );
}

PodcastChannelList
UmsPodcastProvider::channels()
{
    return UmsPodcastChannel::toPodcastChannelList( m_umsChannels );
}

void
UmsPodcastProvider::removeSubscription( const PodcastChannelPtr &channel )
{
    UmsPodcastChannelPtr umsChannel = UmsPodcastChannelPtr::dynamicCast( channel );
    if( umsChannel.isNull() )
    {
        error() << "trying to remove a podcast channel of the wrong type";
        return;
    }

    if( !m_umsChannels.contains( umsChannel ) )
    {
        error() << "trying to remove a podcast channel that is not in the list";
        return;
    }

    m_umsChannels.removeAll( umsChannel );
}

void
UmsPodcastProvider::configureProvider()
{
}

void
UmsPodcastProvider::configureChannel( const PodcastChannelPtr &channel )
{
    Q_UNUSED( channel );
}

QString
UmsPodcastProvider::prettyName() const
{
    return i18nc( "Podcasts on a media device", "Podcasts on %1", QStringLiteral("TODO: replace me") );
}

QIcon
UmsPodcastProvider::icon() const
{
    return QIcon::fromTheme("drive-removable-media-usb-pendrive");
}

Playlists::PlaylistList
UmsPodcastProvider::playlists()
{
    Playlists::PlaylistList playlists;
    for( UmsPodcastChannelPtr channel : m_umsChannels )
        playlists << Playlists::PlaylistPtr::dynamicCast( channel );
    return playlists;
}

QActionList
UmsPodcastProvider::episodeActions( const PodcastEpisodeList &episodes )
{
    QActionList actions;
    if( episodes.isEmpty() )
        return actions;

    if( m_deleteEpisodeAction == nullptr )
    {
        m_deleteEpisodeAction = new QAction( QIcon::fromTheme( "edit-delete" ), i18n( "&Delete Episode" ), this );
        m_deleteEpisodeAction->setProperty( "popupdropper_svg_id", "delete" );
        connect( m_deleteEpisodeAction, &QAction::triggered, this, &UmsPodcastProvider::slotDeleteEpisodes );
    }
    // set the episode list as data that we'll retrieve in the slot
    m_deleteEpisodeAction->setData( QVariant::fromValue( episodes ) );
    actions << m_deleteEpisodeAction;

    return actions;
}

void
UmsPodcastProvider::slotDeleteEpisodes()
{
    DEBUG_BLOCK
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == nullptr )
        return;

    //get the list of episodes to apply to, then clear that data.
    PodcastEpisodeList episodes =
            action->data().value<PodcastEpisodeList>();
    action->setData( QVariant() );

    UmsPodcastEpisodeList umsEpisodes;
    for( PodcastEpisodePtr episode : episodes )
    {
        UmsPodcastEpisodePtr umsEpisode =
                UmsPodcastEpisode::fromPodcastEpisodePtr( episode );
        if( !umsEpisode )
        {
            error() << "Could not cast to UmsPodcastEpisode";
            continue;
        }

        PodcastChannelPtr channel = umsEpisode->channel();
        if( !channel )
        {
            error() << "episode did not have a valid channel";
            continue;
        }

        UmsPodcastChannelPtr umsChannel =
                UmsPodcastChannel::fromPodcastChannelPtr( channel );
        if( !umsChannel )
        {
            error() << "Could not cast to UmsPodcastChannel";
            continue;
        }

        umsEpisodes << umsEpisode;
    }

    deleteEpisodes( umsEpisodes );
}

void
UmsPodcastProvider::deleteEpisodes( UmsPodcastEpisodeList umsEpisodes )
{
    QList<QUrl> urlsToDelete;
    for( UmsPodcastEpisodePtr umsEpisode : umsEpisodes )
        urlsToDelete << umsEpisode->playableUrl();

    QDialog dialog;
    dialog.setWindowTitle( i18n( "Confirm Delete" ) );

    QLabel *label = new QLabel( i18np( "Are you sure you want to delete this episode?",
                                       "Are you sure you want to delete these %1 episodes?",
                                       urlsToDelete.count() ),
                                &dialog );
    QListWidget *listWidget = new QListWidget( &dialog );
    listWidget->setSelectionMode( QAbstractItemView::NoSelection );
    for( const QUrl &url : urlsToDelete )
    {
        new QListWidgetItem( url.toLocalFile(), listWidget );
    }

    QWidget *widget = new QWidget( &dialog );
    QDialogButtonBox *buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
    QVBoxLayout *layout = new QVBoxLayout( widget );
    layout->addWidget( label );
    layout->addWidget( listWidget );
    layout->addWidget( buttonBox );

    buttonBox->button( QDialogButtonBox::Ok )->setText( i18n( "Yes, delete from %1.",
                                                        QStringLiteral("TODO: replace me") ) );

    if( dialog.exec() != QDialog::Accepted )
        return;

    KIO::DeleteJob *deleteJob = KIO::del( urlsToDelete, KIO::HideProgressInfo );

    //keep track of these episodes until the job is done
    m_deleteJobMap.insert( deleteJob, umsEpisodes );

    connect( deleteJob, &KJob::result, this, &UmsPodcastProvider::deleteJobComplete );
}

void
UmsPodcastProvider::deleteJobComplete( KJob *job )
{
    DEBUG_BLOCK
    if( job->error() )
    {
        error() << "problem deleting episode(s): " << job->errorString();
        return;
    }

    UmsPodcastEpisodeList deletedEpisodes = m_deleteJobMap.take( job );
    for( UmsPodcastEpisodePtr deletedEpisode : deletedEpisodes )
    {
        PodcastChannelPtr channel = deletedEpisode->channel();
        UmsPodcastChannelPtr umsChannel =
                UmsPodcastChannel::fromPodcastChannelPtr( channel );
        if( !umsChannel )
        {
            error() << "Could not cast to UmsPodcastChannel";
            continue;
        }

        umsChannel->removeEpisode( deletedEpisode );
        if( umsChannel->m_umsEpisodes.isEmpty() )
        {
            debug() << "channel is empty now, remove it";
            m_umsChannels.removeAll( umsChannel );
            Q_EMIT( playlistRemoved( Playlists::PlaylistPtr::dynamicCast( umsChannel ) ) );
        }
    }
}

QActionList
UmsPodcastProvider::channelActions( const PodcastChannelList &channels )
{
    QActionList actions;
    if( channels.isEmpty() )
        return actions;

    if( m_deleteChannelAction == nullptr )
    {
        m_deleteChannelAction = new QAction( QIcon::fromTheme( "edit-delete" ), i18n( "&Delete "
                "Channel and Episodes" ), this );
        m_deleteChannelAction->setProperty( "popupdropper_svg_id", "delete" );
        connect( m_deleteChannelAction, &QAction::triggered, this, &UmsPodcastProvider::slotDeleteChannels );
    }
    // set the episode list as data that we'll retrieve in the slot
    m_deleteChannelAction->setData( QVariant::fromValue( channels ) );
    actions << m_deleteChannelAction;

    return actions;
}

void
UmsPodcastProvider::slotDeleteChannels()
{
    DEBUG_BLOCK
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == nullptr )
        return;

    //get the list of episodes to apply to, then clear that data.
    PodcastChannelList channels =
            action->data().value<PodcastChannelList>();
    action->setData( QVariant() );

    for( PodcastChannelPtr channel : channels )
    {
        UmsPodcastChannelPtr umsChannel =
                UmsPodcastChannel::fromPodcastChannelPtr( channel );
        if( !umsChannel )
        {
            error() << "Could not cast to UmsPodcastChannel";
            continue;
        }

        deleteEpisodes( umsChannel->m_umsEpisodes );
        //slot deleteJobComplete() will Q_EMIT signal once all tracks are gone.
    }
}

QActionList
UmsPodcastProvider::playlistActions( const Playlists::PlaylistList &playlists )
{
    PodcastChannelList channels;
    for( const Playlists::PlaylistPtr &playlist : playlists )
    {
        PodcastChannelPtr channel = PodcastChannelPtr::dynamicCast( playlist );
        if( channel )
            channels << channel;
    }

    return channelActions( channels );
}

QActionList
UmsPodcastProvider::trackActions( const QMultiHash<Playlists::PlaylistPtr, int> &playlistTracks )
{
    PodcastEpisodeList episodes;
    for( const Playlists::PlaylistPtr &playlist : playlistTracks.uniqueKeys() )
    {
        PodcastChannelPtr channel = PodcastChannelPtr::dynamicCast( playlist );
        if( !channel )
            continue;

        PodcastEpisodeList channelEpisodes = channel->episodes();
        QList<int> trackPositions = playlistTracks.values( playlist );
        std::sort( trackPositions.begin(), trackPositions.end() );
        for( int trackPosition : trackPositions )
        {
            if( trackPosition >= 0 && trackPosition < channelEpisodes.count() )
                episodes << channelEpisodes.at( trackPosition );
        }
    }

    return episodeActions( episodes );
}

void
UmsPodcastProvider::completePodcastDownloads()
{

}

void
UmsPodcastProvider::updateAll() //slot
{
}

void
UmsPodcastProvider::update( const Podcasts::PodcastChannelPtr &channel ) //slot
{
    Q_UNUSED( channel );
}

void
UmsPodcastProvider::downloadEpisode( const Podcasts::PodcastEpisodePtr &episode ) //slot
{
    Q_UNUSED( episode );
}

void
UmsPodcastProvider::deleteDownloadedEpisode( const Podcasts::PodcastEpisodePtr &episode ) //slot
{
    Q_UNUSED( episode );
}

void
UmsPodcastProvider::slotUpdated() //slot
{

}

void
UmsPodcastProvider::scan()
{
    if( m_scanDirectory.isEmpty() )
        return;
    m_dirList.clear();
    debug() << "scan directory for podcasts: " <<
            m_scanDirectory.toLocalFile();
    QDirIterator it( m_scanDirectory.toLocalFile(), QDirIterator::Subdirectories );
    while( it.hasNext() )
        addPath( it.next() );
}

int
UmsPodcastProvider::addPath( const QString &path )
{
    DEBUG_BLOCK
    int acc = 0;
    QMimeDatabase db;
    debug() << path;
    QMimeType mime = db.mimeTypeForFile( path, QMimeDatabase::MatchContent );
    if( !mime.isValid() || mime.isDefault() )
    {
        debug() << "Trying again with findByPath:" ;
        mime = db.mimeTypeForFile( path, QMimeDatabase::MatchExtension);
        if( mime.isDefault() )
            return 0;
    }
    debug() << "Got type: " << mime.name() << ", with accuracy: " << acc;

    QFileInfo info( path );
    if( info.isDir() )
    {
        if( m_dirList.contains( path ) )
            return 0;
        m_dirList << info.canonicalPath();
        return 1;
    }
    else if( info.isFile() )
    {
//        for( const QString &mimetype : m_handler->mimetypes() )
//        {
//            if( mime.inherits( mimetype ) )
//            {
                addFile( MetaFile::TrackPtr( new MetaFile::Track(
                    QUrl::fromLocalFile( info.canonicalFilePath() ) ) ) );
                return 2;
//            }
//        }
    }

    return 0;
}

UmsPodcastEpisodePtr
UmsPodcastProvider::addFile( MetaFile::TrackPtr metafileTrack )
{
    DEBUG_BLOCK
    debug() << metafileTrack->playableUrl().url();
    debug() << "album: " << metafileTrack->album()->name();
    debug() << "title: " << metafileTrack->name();
    if( metafileTrack->album()->name().isEmpty() )
    {
        debug() << "Can't figure out channel without album tag.";
        return UmsPodcastEpisodePtr();
    }

    if( metafileTrack->name().isEmpty() )
    {
        debug() << "Can not use a track without a title.";
        return UmsPodcastEpisodePtr();
    }

    //see if there is already a UmsPodcastEpisode for this track
    UmsPodcastChannelPtr channel;
    UmsPodcastEpisodePtr episode;

    for( UmsPodcastChannelPtr c : m_umsChannels )
    {
        if( c->name() == metafileTrack->album()->name() )
        {
            channel = c;
            break;
        }
    }

    if( channel )
    {
        for( UmsPodcastEpisodePtr e : channel->umsEpisodes() )
        {
            if( e->title() == metafileTrack->name() )
            {
                episode = e;
                break;
            }
        }
    }
    else
    {
        debug() << "there is no channel for this episode yet";
        channel = UmsPodcastChannelPtr( new UmsPodcastChannel( this ) );
        channel->setTitle( metafileTrack->album()->name() );
        m_umsChannels << channel;
        Q_EMIT playlistAdded( Playlists::PlaylistPtr( channel.data() ) );
    }

    if( episode.isNull() )
    {
        debug() << "this episode was not found in an existing channel";
        episode = UmsPodcastEpisodePtr( new UmsPodcastEpisode( channel ) );
        episode->setLocalFile( metafileTrack );

        channel->addUmsEpisode( episode );
    }

    episode->setLocalFile( metafileTrack );

    return episode;
}

/******************************************************************************
 * Copyright (C) 2004 Christian Muehlhaeuser <chris@chris.de>                 *
 *           (C) 2005-2006 Martin Aumueller <aumuell@reserv.at>               *
 *           (C) 2005 Seb Ruiz <ruiz@kde.org>                                 *
 *           (C) 2006 T.R.Shashwath <trshash84@gmail.com>                     *
 *           (c) 2007 Jeff Mitchell <kde-dev@emailgoeshere.com>               *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/


#define DEBUG_PREFIX "MediaDevice"

#include "MediaDevice.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "App.h"
#include "Debug.h"
#include "mediabrowser.h"
#include "MediaItem.h"
#include "meta/file/File.h"
#include "meta/Playlist.h"
#include "meta/PlaylistFileSupport.h"
#include "PluginManager.h"
#include "AmarokProcess.h"
#include "ScriptManager.h"
#include "statusbar/StatusBar.h"

#include <KIO/Job>
#include <KMessageBox>

#include <QTimer>

#include <unistd.h>

MediaDevice::MediaDevice()
    : Amarok::Plugin()
    , m_name( QString() )
    , m_hasMountPoint( true )
    , m_autoDeletePodcasts( false )
    , m_syncStats( false )
    , m_transcode( false )
    , m_transcodeAlways( false )
    , m_transcodeRemove( false )
    , sysProc ( 0 )
    , m_parent( 0 )
    , m_view( 0 )
    , m_udi( QString() )
    , m_deviceNode( QString() )
    , m_mountPoint( QString() )
    , m_fsType( QString() )
    , m_wait( false )
    , m_requireMount( false )
    , m_canceled( false )
    , m_transferring( false )
    , m_deleting( false )
    , m_deferredDisconnect( false )
    , m_scheduledDisconnect( false )
    , m_transfer( true )
    , m_configure( true )
    , m_customButton( false )
    , m_playlistItem( 0 )
    , m_podcastItem( 0 )
    , m_invisibleItem( 0 )
    , m_staleItem( 0 )
    , m_orphanedItem( 0 )
{
    sysProc = new AmarokShellProcess(); Q_CHECK_PTR(sysProc);
}

void MediaDevice::init( MediaBrowser* parent )
{
    m_parent = parent;
    if( !m_view )
       m_view = new MediaView( m_parent->m_views, this );
    m_view->hide();
}

MediaDevice::~MediaDevice()
{
    delete m_view;
    delete sysProc;
}

bool
MediaDevice::isSpecialItem( MediaItem *item )
{
    return (item == m_playlistItem) ||
        (item == m_podcastItem) ||
        (item == m_invisibleItem) ||
        (item == m_staleItem) ||
        (item == m_orphanedItem);
}

void
MediaDevice::loadConfig()
{
    m_transcode = configBool( "Transcode" );
    m_transcodeAlways = configBool( "TranscodeAlways" );
    m_transcodeRemove = configBool( "TranscodeRemove" );
    m_preconnectcmd = configString( "PreConnectCommand" );
    if( m_preconnectcmd.isEmpty() )
        m_preconnectcmd = configString( "MountCommand" );
    m_postdisconnectcmd = configString( "PostDisconnectCommand" );
    if( m_postdisconnectcmd.isEmpty() )
        m_postdisconnectcmd = configString( "UmountCommand" );
    if( m_requireMount && m_postdisconnectcmd.isEmpty() )
        m_postdisconnectcmd = "kdeeject -q %d";
}

QString
MediaDevice::configString( const QString &name, const QString &defValue )
{
    QString configName = "MediaDevice";
    if( !udi().isEmpty() )
        configName += '_' + udi();
    KConfigGroup config = Amarok::config( configName );
    return config.readEntry( name, defValue );
}

void
MediaDevice::setConfigString( const QString &name, const QString &value )
{
    QString configName = "MediaDevice";
    if( !udi().isEmpty() )
        configName += '_' + udi();
    KConfigGroup config = Amarok::config( configName );
    config.writeEntry( name, value );
}

bool
MediaDevice::configBool( const QString &name, bool defValue )
{
    QString configName = "MediaDevice";
    if( !udi().isEmpty() )
        configName += '_' + udi();
    KConfigGroup config = Amarok::config( configName );
    return config.readEntry( name, defValue );
}

void
MediaDevice::setConfigBool( const QString &name, bool value )
{
    QString configName = "MediaDevice";
    if( !udi().isEmpty() )
        configName += '_' + udi();
    KConfigGroup config = Amarok::config( configName );
    config.writeEntry( name, value );
}

MediaView *
MediaDevice::view()
{
    return m_view;
}

void
MediaDevice::hideProgress()
{
    m_parent->m_progressBox->hide();
}

void
MediaDevice::updateRootItems()
{
    if(m_podcastItem)
        m_podcastItem->setVisible(m_podcastItem->childCount() > 0);
    if(m_invisibleItem)
        m_invisibleItem->setVisible(m_invisibleItem->childCount() > 0);
    if(m_staleItem)
        m_staleItem->setVisible(m_staleItem->childCount() > 0);
    if(m_orphanedItem)
        m_orphanedItem->setVisible(m_orphanedItem->childCount() > 0);
}

Meta::TrackList
MediaDevice::tracksToSync( const QString &name, const KUrl &url )
{
    Meta::TrackList tracks = Meta::loadPlaylist( url )->tracks();

    preparePlaylistForSync( name, tracks );

    return tracks;
}

//TODO port to meta & new collection interface
//BundleList
//MediaDevice::bundlesToSync( const QString &name, const QString &query )
//{
//    const QStringList values = CollectionDB::instance()->query( query );
//
//    BundleList bundles;
//    for( QStringList::const_iterator it = values.begin(); it != values.end(); ++it )
//        bundles += CollectionDB::instance()->bundleFromQuery( &it );
//    preparePlaylistForSync( name, bundles );
//    return bundles;
//}

void
MediaDevice::preparePlaylistForSync( const QString &name, const Meta::TrackList &tracks )
{
    if( ! m_playlistItem ) // might be syncing a new playlist from the playlist browser
        return;
    MediaItem *pl = m_playlistItem->findItem( name );
    if( pl )
    {
        MediaItem *next = 0;
        for( MediaItem *it = static_cast<MediaItem *>(pl->firstChild());
                it;
                it = next )
        {
            next = static_cast<MediaItem *>(it->nextSibling());
            const Meta::TrackPtr track = Meta::TrackPtr::dynamicCast( (*it).meta() );
            if( !track )
                continue;
            if( isOnOtherPlaylist( name, track ) )
                continue;
            if( isInTrackList( tracks, track ) )
                continue;
            deleteItemFromDevice( it );
        }
        deleteItemFromDevice( pl, None );
    }
    purgeEmptyItems();
}

bool
MediaDevice::trackMatch( Meta::TrackPtr track1, Meta::TrackPtr track2 )
{
    if (!track1 || !track2)
        return false;

    if (track1 == track2)
        return true;

    if ( track1->name() != track2->name() )
        return false;

    if ( track1->artist()->name() != track2->artist()->name() )
        return false;

    return true;
}

bool
MediaDevice::isInTrackList( const Meta::TrackList &list, const Meta::TrackPtr track )
{
    for( int i = 0, size = list.count(); i < size; i++ )
    {
        if( trackMatch( list[i], track ) )
            return true;
    }

    return false;
}

bool
MediaDevice::isOnOtherPlaylist( const QString &playlistToAvoid, const Meta::TrackPtr track )
{
    if (!track)
        return false;

    for( MediaItem *it = static_cast<MediaItem *>(m_playlistItem->firstChild());
            it;
            it = static_cast<MediaItem *>(it->nextSibling()) )
    {
        if( it->text( 0 )  == playlistToAvoid )
            continue;
        if( isOnPlaylist( *it, track ) )
            return true;
    }

    return false;
}


bool
MediaDevice::isOnPlaylist( const MediaItem &playlist, const Meta::TrackPtr track )
{
    for( MediaItem *it = static_cast<MediaItem *>(playlist.firstChild());
            it;
            it = static_cast<MediaItem *>(it->nextSibling()) )
    {
        Meta::TrackPtr playlist_track = Meta::TrackPtr::dynamicCast(it->meta());
        if( !playlist_track )
            continue;
        if( trackMatch( playlist_track, track ) )
            return true;
    }

    return false;
}


void
MediaDevice::copyTrackFromDevice( MediaItem *item )
{
    debug() << "copyTrackFromDevice: not copying " << item->url() << ": not implemented";
}

QString
MediaDevice::replaceVariables( const QString &cmd )
{
    QString result = cmd;
    result.replace( "%d", deviceNode() );
    result.replace( "%m", mountPoint() );
    return result;
}

int MediaDevice::runPreConnectCommand()
{
    if( m_preconnectcmd.isEmpty() )
        return 0;

    QString cmd = replaceVariables( m_preconnectcmd );

    debug() << "running pre-connect command: [" << cmd << "]";
    int e=sysCall(cmd);
    debug() << "pre-connect: e=" << e;
    return e;
}

int MediaDevice::runPostDisconnectCommand()
{
    if( m_postdisconnectcmd.isEmpty() )
        return 0;

    QString cmd = replaceVariables( m_postdisconnectcmd );
    debug() << "running post-disconnect command: [" << cmd << "]";
    int e=sysCall(cmd);
    debug() << "post-disconnect: e=" << e;

    return e;
}

int MediaDevice::sysCall( const QString &command )
{
    if( sysProc->state() != AmarokShellProcess::NotRunning )  return -1;

    sysProc->clearProgram();
    (*sysProc) << command;
    sysProc->setOutputChannelMode( AmarokShellProcess::MergedChannels );
    if( sysProc->execute( ) != 0 )
        kFatal() << i18n("could not execute %1", command.toLocal8Bit().data());

    return (sysProc->exitStatus());
}

void
MediaDevice::abortTransfer()
{
    setCanceled( true );
    cancelTransfer();
}

bool
MediaDevice::kioCopyTrack( const KUrl &src, const KUrl &dst )
{
    m_wait = true;

    KIO::FileCopyJob *job = KIO::file_copy( src, dst,
            -1 /* permissions */,
            KIO::HideProgressInfo );
    connect( job, SIGNAL( result( KIO::Job * ) ),
            this,  SLOT( fileTransferred( KIO::Job * ) ) );

    bool tryToRemove = false;
    while ( m_wait )
    {
        if( isCanceled() )
        {
            job->kill( KJob::EmitResult );
            tryToRemove = true;
            m_wait = false;
        }
        else
        {
            usleep(10000);
            kapp->processEvents( QEventLoop::ExcludeUserInputEvents );
        }
    }

    if( !tryToRemove )
    {
        if(m_copyFailed)
        {
            tryToRemove = true;
            The::statusBar()->longMessage(
                    i18n( "Media Device: Copying %1 to %2 failed",
                          src.prettyUrl(),
                          dst.prettyUrl()
                        ),
                    StatusBar::Error );
        }
        else
        {
            MetaFile::Track track2(dst);
            if( !track2.isPlayable() && track2.filesize()==0 )
            {
                tryToRemove = true;
                // probably s.th. went wrong
                The::statusBar()->longMessage(
                        i18n( "Media Device: Reading tags from %1 failed", dst.prettyUrl() ),
                        StatusBar::Error );
            }
        }
    }

    if( tryToRemove )
    {
        QFile::remove( dst.path() );
        return false;
    }

    return true;
}

void
MediaDevice::fileTransferred( KIO::Job *job )  //SLOT
{
    if(job->error())
    {
        m_copyFailed = true;
        debug() << "file transfer failed: " << job->errorText();
    }
    else
    {
        m_copyFailed = false;
    }

    m_wait = false;
}

bool
MediaDevice::connectDevice( bool silent )
{
    if( !lockDevice( true ) )
        return false;

    runPreConnectCommand();
    openDevice( silent );

    if( isConnected()
            && MediaBrowser::instance()->currentDevice() != this
            && MediaBrowser::instance()->currentDevice()
            && !MediaBrowser::instance()->currentDevice()->isConnected() )
    {
        MediaBrowser::instance()->activateDevice( this );
    }
    m_parent->updateStats();
    m_parent->updateButtons();

    if( !isConnected() )
    {
        unlockDevice();
        return false;
    }

    if( m_syncStats )
    {
        syncStatsFromDevice( 0 );
        //Scrobbler::instance()->m_submitter->syncComplete();
    }

    // delete podcasts already played
    if( m_autoDeletePodcasts && m_podcastItem )
    {
        QList<MediaItem*> list;
        //NOTE we assume that currentItem is the main target
        int numFiles  = m_view->getSelectedLeaves( m_podcastItem, &list, MediaView::OnlyPlayed );

        if(numFiles > 0)
        {
            m_parent->m_stats->setText( i18np( "1 track to be deleted", "%1 tracks to be deleted", numFiles ) );

            setProgress( 0, numFiles );

            int numDeleted = deleteItemFromDevice( m_podcastItem, true );
            purgeEmptyItems();
            if( numDeleted < 0 )
            {
                The::statusBar()->longMessage(
                        i18n( "Failed to purge podcasts already played" ),
                        StatusBar::Sorry );
            }
            else if( numDeleted > 0 )
            {
                The::statusBar()->shortMessage(
                        i18np( "Purged 1 podcasts already played",
                            "Purged %1 podcasts already played",
                            numDeleted ) );
            }

            synchronizeDevice();

            QTimer::singleShot( 1500, m_parent->m_progressBox, SLOT(hide()) );
            m_parent->queue()->computeSize();
            m_parent->updateStats();
        }
    }
    unlockDevice();

    updateRootItems();

    if( m_deferredDisconnect )
    {
        m_deferredDisconnect = false;
        disconnectDevice( m_runDisconnectHook );
    }

    The::statusBar()->shortMessage( i18n( "Device successfully connected" ) );

    m_parent->updateDevices();

    return true;
}

bool
MediaDevice::disconnectDevice( bool postDisconnectHook )
{
    DEBUG_BLOCK

    abortTransfer();

    debug() << "disconnecting: hook=" << postDisconnectHook;

    if( !lockDevice( true ) )
    {
        m_runDisconnectHook = postDisconnectHook;
        m_deferredDisconnect = true;
        debug() << "disconnecting: locked";
        return false;
    }
    debug() << "disconnecting: ok";

    if( m_syncStats )
    {
        syncStatsToDevice();
    }

    closeDevice();
    unlockDevice();

    m_parent->updateStats();

    bool result = true;
    if( postDisconnectHook && runPostDisconnectCommand() != 0 )
    {
        The::statusBar()->longMessage(
                i18n( "Post-disconnect command failed, before removing device, please make sure that it is safe to do so." ),
                StatusBar::Information );
        result = false;
    }
    else
        The::statusBar()->shortMessage( i18n( "Device successfully disconnected" ) );

    m_parent->updateDevices();

    return result;
}

void
MediaDevice::syncStatsFromDevice( MediaItem *root )
{
    MediaItem *it = static_cast<MediaItem *>( m_view->firstChild() );
    if( root )
    {
        it = static_cast<MediaItem *>( root->firstChild() );
    }

    kapp->processEvents( QEventLoop::ExcludeUserInputEvents );

    for( ; it; it = static_cast<MediaItem *>( it->nextSibling() ) )
    {
        switch( it->type() )
        {
        case MediaItem::TRACK:
            if( !it->parent() || static_cast<MediaItem *>( it->parent() )->type() != MediaItem::PLAYLIST )
            {
/* TODO port to meta
                const MetaBundle *bundle = it->bundle();
                for( int i=0; i<it->recentlyPlayed(); i++ )
                {
                    // submit to last.fm
                    if( bundle->length() > 30
                            && !bundle->artist().isEmpty() && bundle->artist() != i18n( "Unknown" )
                            && !bundle->title().isEmpty() && bundle->title() != i18n( "Unknown" ) )
                    {
                        // don't submit tracks shorter than 30 sec or w/o artist/title
                        debug() << "scrobbling " << bundle->artist() << " - " << bundle->title();
                        SubmitItem *sit = new SubmitItem( bundle->artist(), bundle->album(), bundle->title(), bundle->length(), false * fake time  );
                        Scrobbler::instance()->m_submitter->submitItem( sit );
                    }

                    // increase Amarok playcount
                    QString url = CollectionDB::instance()->getURL( *bundle );
                    if( !url.isEmpty() )
                    {
                        QDateTime t = it->playTime();
                        CollectionDB::instance()->addSongPercentage( url, 100, "mediadevice", t.isValid() ? &t : 0 );
                        debug() << "played " << url;
                    }
                }

                if( it->ratingChanged() )
                {
                    // copy rating from media device to Amarok
                    QString url = CollectionDB::instance()->getURL( *bundle );
                    debug() << "rating changed " << url << ": " << it->rating()/10;
                    if( !url.isEmpty() )
                    {
                        CollectionDB::instance()->setSongRating( url, it->rating()/10 );
                        it->setRating( it->rating() ); // prevent setting it again next time
                    }
                }*/
            }
            break;
        case MediaItem::PODCASTITEM:
//TODO port to meta
//            if( !it->parent() || static_cast<MediaItem *>( it->parent() )->type() != MediaItem::PLAYLIST )
//            {
//                const MetaBundle *bundle = it->bundle();
//                if( it->played() || it->recentlyPlayed() )
//                {
//                    if( PodcastEpisodeBundle *peb = bundle->podcastBundle() )
//                    {
//                        debug() << "marking podcast episode as played: " << peb->url();
//PORT 2.0
//                         if( PlaylistBrowser::instance() )
//                         {
//                             PodcastEpisode *p = PlaylistBrowser::instance()->findPodcastEpisode( peb->url(), peb->parent() );
//                             if ( p )
//                                 p->setListened();
//                             else
//                                 debug() << "did not find podcast episode: " << peb->url() << " from " << peb->parent();
//                         }
//                    }
//                }
//            }
            break;

        default:
            syncStatsFromDevice( it );
            break;
        }
    }
}

void
MediaDevice::syncStatsToDevice( MediaItem *root )
{
    MediaItem *it = static_cast<MediaItem *>( m_view->firstChild() );
    if( root )
    {
        it = static_cast<MediaItem *>( root->firstChild() );
    }

    kapp->processEvents( QEventLoop::ExcludeUserInputEvents );

    for( ; it; it = static_cast<MediaItem *>( it->nextSibling() ) )
    {
        switch( it->type() )
        {
        case MediaItem::TRACK:
            if( !it->parent() || static_cast<MediaItem *>( it->parent() )->type() != MediaItem::PLAYLIST )
            {
                //TODO port to meta
                //const MetaBundle *bundle = it->bundle();
                //QString url = CollectionDB::instance()->getURL( *bundle );
                //it->syncStatsFromPath( url );
            }
            break;

        case MediaItem::PODCASTITEM:
            if( !it->parent() || static_cast<MediaItem *>( it->parent() )->type() != MediaItem::PLAYLIST )
            {
                //TODO port to meta
                //const MetaBundle *bundle = it->bundle();
                //if( PodcastEpisodeBundle *peb = bundle->podcastBundle() )
                //{
// //PORT 2.0
//                     if( PlaylistBrowser::instance() )
//                     {
//                         PodcastEpisode *p = PlaylistBrowser::instance()->findPodcastEpisode( peb->url(), peb->parent() );
//                         if( p )
//                             it->setListened( !p->isNew() );
//                     }
                //}
            }
            break;

        default:
            syncStatsToDevice( it );
            break;
        }
    }
}

void
MediaDevice::transferFiles()
{
    if( !lockDevice( true ) )
    {
        return;
    }

    setCanceled( false );

    m_transferring = true;
    m_parent->transferAction()->setEnabled( false );

    setProgress( 0, m_parent->m_queue->childCount() );

    // ok, let's copy the stuff to the device

    KUrl::List existing, unplayable;
    unsigned transcodeFail = 0;
    // iterate through items
    MediaItem *next = static_cast<MediaItem *>(m_parent->m_queue->firstChild());
    while( next )
    {
        MediaItem *transferredItem = next;
        transferredItem->setFailed( false );
        transferredItem->m_flags |= MediaItem::Transferring;
        next = static_cast<MediaItem *>( transferredItem->nextSibling() );

        if( transferredItem->device() )
        {
            transferredItem->device()->copyTrackFromDevice( transferredItem );
            m_parent->m_queue->subtractItemFromSize( transferredItem, true );
            delete transferredItem;
            setProgress( progress() + 1 );
            m_parent->m_queue->itemCountChanged();
            kapp->processEvents( QEventLoop::ExcludeUserInputEvents );
            continue;
        }

        Meta::TrackList tracks;
        if( transferredItem->type() == MediaItem::PLAYLIST )
        {
            if( transferredItem->flags() & MediaItem::SmartPlaylist )
                tracks = tracksToSync( transferredItem->text( 0 ), transferredItem->data() );
            else
                tracks = tracksToSync( transferredItem->text( 0 ), KUrl( transferredItem->data() ) );
        }
        else if( Meta::TrackPtr::dynamicCast(transferredItem->meta()) )
            tracks.append( Meta::TrackPtr::dynamicCast(transferredItem->meta()) );
        else
        {
            // this should not happen
            debug() << "invalid item in transfer queue";
            m_parent->m_queue->subtractItemFromSize( transferredItem, true );
            delete transferredItem;
            m_parent->m_queue->itemCountChanged();
            continue;
        }

        if( tracks.count() > 1 )
            setProgress( progress(), MediaBrowser::instance()->m_progress->maximum() + tracks.count() - 1 );

        QString playlist = transferredItem->m_playlistName;
        for( Meta::TrackList::const_iterator it = tracks.constBegin(), end = tracks.constEnd();
                it != end;
                ++it )
        {
            if( isCanceled() )
                break;

            Meta::TrackPtr track = *it;

            bool transcoding = false;
            MediaItem *item = trackExists( track );
            if( item && playlist.isEmpty() )
            {
                The::statusBar()->shortMessage( i18n( "Track already on media device: %1",
                                                                           track->prettyUrl() ),
                        StatusBar::Sorry );
                existing += track->url();
                setProgress( progress() + 1 );
                continue;
            }
            else if( !item ) // the item does not yet exist on the media device
            {
                if( m_transcode && ( !isPlayable( track ) || m_transcodeAlways ) )
                {
                    QString preferred = supportedFiletypes().isEmpty() ? "mp3" : supportedFiletypes().first();
                    debug() << "transcoding " << track->url() << " to " << preferred;
                    KUrl transcoded = MediaBrowser::instance()->transcode( track->url(), preferred );
                    if( isCanceled() )
                        break;
                    if( transcoded.isEmpty() )
                    {
                        debug() << "transcoding failed";
                        transcodeFail++;
                    }
                    else
                    {
                        transcoding = true;
                        MetaFile::Track *transcodedTrack = new MetaFile::Track( transcoded );
                        transcodedTrack->setTitle( track->name() );
                        transcodedTrack->setArtist( track->artist()->name() );
                        transcodedTrack->setComposer( track->composer()->name() );
                        transcodedTrack->setAlbum( track->album()->name() );
                        transcodedTrack->setGenre( track->genre()->name() );
                        transcodedTrack->setComment( track->comment() );
                        transcodedTrack->setYear( track->year()->name() );
                        transcodedTrack->setDiscNumber( track->discNumber() );
                        transcodedTrack->setTrackNumber( track->trackNumber() );
                        //TODO port to meta
                        //if( bundle->podcastBundle() )
                        //{
                        //    transcodedBundle->setPodcastBundle( *bundle->podcastBundle() );
                        //    transcodedBundle->copyFrom( *bundle->podcastBundle() );
                        //}
                        track = Meta::TrackPtr( transcodedTrack );
                    }
                }

                if( !isPlayable( track ) )
                {
                    The::statusBar()->shortMessage( i18n( "Track not playable on media device: %1", track->prettyUrl() ),
                            StatusBar::Sorry );
                    unplayable += (*it)->url();
                    transferredItem->setFailed();
                    setProgress( progress() + 1 );
                    continue;
                }
                item = copyTrackToDevice( track );
            }

            if( !item ) // copyTrackToDevice() failed
            {
                if( !isCanceled() )
                {
                    The::statusBar()->longMessage(
                            i18n( "Failed to copy track to media device: %1", track->url() ),
                            StatusBar::Sorry );
                    transferredItem->setFailed();
                }
            }

            if( transcoding )
            {
                if( m_transcodeRemove )
                    QFile( track->url() ).remove();
            }

            if( isCanceled() )
                break;

            if( !item )
            {
                setProgress( progress() + 1 );
                continue;
            }

            item->syncStatsFromPath( (*it)->url() );

            if( m_playlistItem && !playlist.isEmpty() )
            {
                MediaItem *pl = m_playlistItem->findItem( playlist );
                if( !pl )
                {
                    QList<MediaItem*> items;
                    pl = newPlaylist( playlist, m_playlistItem, items );
                }
                if( pl )
                {
                    QList<MediaItem*> items;
                    items.append( item );
                    addToPlaylist( pl, pl->lastChild(), items );
                }
            }

            setProgress( progress() + 1 );
        }

        transferredItem->m_flags &= ~MediaItem::Transferring;

        if( isCanceled() )
        {
            m_parent->updateStats();
            break;
        }

        if( !(transferredItem->flags() & MediaItem::Failed) )
        {
            m_parent->m_queue->subtractItemFromSize( transferredItem, true );
            delete transferredItem;
            m_parent->m_queue->itemCountChanged();
        }
        m_parent->updateStats();

        kapp->processEvents( QEventLoop::ExcludeUserInputEvents );
    }
    synchronizeDevice();
    unlockDevice();
    fileTransferFinished();

    QString msg;
    if( unplayable.count() > 0 )
    {
        msg = i18np( "One track not playable on media device",
                "%1 tracks not playable on media device", unplayable.count() );
    }
    if( existing.count() > 0 )
    {
        if( msg.isEmpty() )
            msg = i18np( "One track already on media device",
                    "%1 tracks already on media device", existing.count() );
        else
            msg += i18np( ", one track already on media device",
                    ", %1 tracks already on media device", existing.count() );
    }
    if( transcodeFail > 0 )
    {
        if( msg.isEmpty() )
            msg = i18np( "One track was not transcoded",
                    "%1 tracks were not transcoded", transcodeFail );
        else
            msg += i18np( ", one track was not transcoded",
                    ", %1 tracks were not transcoded", transcodeFail );

        const ScriptManager* const sm = ScriptManager::instance();
        if( !sm->transcodeScriptRunning().isEmpty() )
            msg += i18n( " (no transcode script running)" );
    }

    if( unplayable.count() + existing.count() > 0 )
    {
        QString longMsg = i18n( "The following tracks were not transferred: ");
        for( KUrl::List::ConstIterator it = existing.constBegin(), end = existing.constEnd();
                it != end;
                ++it )
        {
            longMsg += "<br>" + (*it).prettyUrl();
        }
        for( KUrl::List::ConstIterator it = unplayable.constBegin(), end = unplayable.constEnd();
                it != end;
                it++ )
        {
            longMsg += "<br>" + (*it).prettyUrl();
        }
        The::statusBar()->longMessage( longMsg, StatusBar::Sorry );
    }
    else if( !msg.isEmpty() )
    {
        The::statusBar()->shortMessage( msg, StatusBar::Sorry );
    }

    m_parent->updateButtons();
    m_parent->queue()->save( Amarok::saveLocation() + "transferlist.xml" );
    m_transferring = false;

    if( m_deferredDisconnect )
    {
        m_deferredDisconnect = false;
        disconnectDevice( m_runDisconnectHook );
    }
    else if( m_scheduledDisconnect )
    {
        disconnectDevice( true );
    }
    m_scheduledDisconnect = false;
}

int
MediaDevice::progress() const
{
    return m_parent->m_progress->value();
}

void
MediaDevice::setProgress( const int progress, const int total )
{
    if( total != -1 )
        m_parent->m_progress->setRange( 0, total );
    m_parent->m_progress->setValue( progress );
    m_parent->m_progressBox->show();
}

void
MediaDevice::fileTransferFinished()  //SLOT
{
    m_parent->updateStats();
    m_parent->m_progressBox->hide();
    m_parent->transferAction()->setEnabled( isConnected() && m_parent->queue()->childCount() > 0 );
    m_wait = false;
}


int
MediaDevice::deleteFromDevice(MediaItem *item, int flags )
{
    MediaItem* fi = item;
    int count = 0;

    if ( !(flags & Recursing) )
    {
        if( !lockDevice( true ) )
            return 0;

        setCanceled( false );

        m_deleting = true;

        QList<MediaItem*> list;
        //NOTE we assume that currentItem is the main target
        int numFiles  = m_view->getSelectedLeaves(item, &list, MediaView::OnlySelected | ((flags & OnlyPlayed) ? MediaView::OnlyPlayed : MediaView::None) );

        m_parent->m_stats->setText( i18np( "1 track to be deleted", "%1 tracks to be deleted", numFiles ) );
        if( numFiles > 0 && (flags & DeleteTrack) )
        {
            int button = KMessageBox::warningContinueCancel( m_parent,
                    i18np( "<p>You have selected 1 track to be <b>irreversibly</b> deleted.</p>",
                        "<p>You have selected %1 tracks to be <b>irreversibly</b> deleted.</p>",
                        numFiles
                        ),
                    QString(),
                    KGuiItem(i18n("&Delete"),"edit-delete") );

            if ( button != KMessageBox::Continue )
            {
                m_parent->queue()->computeSize();
                m_parent->updateStats();
                m_deleting = false;
                unlockDevice();
                return 0;
            }

            if(!isTransferring())
            {
                setProgress( 0, numFiles );
            }

        }
        // don't return if numFiles==0: playlist items might be to delete

        if( !fi )
            fi = static_cast<MediaItem*>(m_view->firstChild());
    }

    while( fi )
    {
        MediaItem *next = static_cast<MediaItem*>(fi->nextSibling());

        if( isCanceled() )
        {
            break;
        }

        if( !fi->isVisible() )
        {
            fi = next;
            continue;
        }

        if( fi->isSelected() )
        {
            int ret = deleteItemFromDevice(fi, flags);
            if( ret >= 0 && count >= 0 )
                count += ret;
            else
                count = -1;
        }
        else
        {
            if( fi->childCount() )
            {
                int ret = deleteFromDevice( static_cast<MediaItem*>(fi->firstChild()), flags | Recursing );
                if( ret >= 0 && count >= 0 )
                    count += ret;
                else
                    count = -1;
            }
        }
        m_parent->updateStats();

        fi = next;
    }

    if(!(flags & Recursing))
    {
        purgeEmptyItems();
        synchronizeDevice();
        m_deleting = false;
        unlockDevice();

        if(!isTransferring())
        {
            QTimer::singleShot( 1500, m_parent->m_progressBox, SLOT(hide()) );
        }

        if( m_deferredDisconnect )
        {
            m_deferredDisconnect = false;
            disconnectDevice( m_runDisconnectHook );
        }
    }
    m_parent->queue()->computeSize();
    m_parent->updateStats();

    return count;
}

void
MediaDevice::purgeEmptyItems( MediaItem *root )
{
    MediaItem *it = 0;
    if( root )
    {
        it = static_cast<MediaItem *>(root->firstChild());
    }
    else
    {
        it = static_cast<MediaItem *>(m_view->firstChild());
    }

    MediaItem *next = 0;
    for( ; it; it=next )
    {
        next = static_cast<MediaItem *>(it->nextSibling());
        purgeEmptyItems( it );
        if( it->childCount() == 0 &&
                (it->type() == MediaItem::ARTIST ||
                 it->type() == MediaItem::ALBUM ||
                 it->type() == MediaItem::PODCASTCHANNEL) )
            delete it;
    }
}

bool
MediaDevice::isPlayable( const Meta::TrackPtr track )
{
    if ( track.isNull() )
        return false;

    if( supportedFiletypes().isEmpty() )
        return true;

    QString type = track->url().section( ".", -1 ).toLower();
    return supportedFiletypes().contains( type );
}

bool
MediaDevice::isPreferredFormat( const Meta::TrackPtr track )
{
    if( track.isNull() )
        return true;

    if( supportedFiletypes().isEmpty() )
        return true;

    QString type = track->url().section( ".", -1 ).toLower();
    return ( type == supportedFiletypes().first() );
}

#include "MediaDevice.moc"


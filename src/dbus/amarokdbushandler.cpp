/******************************************************************************
 * Copyright (C) 2003 Stanislav Karchebny <berk@inbox.ru>                     *
 *           (C) 2004 Christian Muehlhaeuser <chris@chris.de>                 *
 *           (C) 2005 Ian Monroe <ian@monroe.nu>                              *
 *           (C) 2005 Seb Ruiz <ruiz@kde.org>                                 *
 *           (C) 2006 Alexandre Pereira de Oliveira <aleprj@gmail.com>        *
 *           (C) 2006 2007 Leonardo Franchi <lfranchi@gmail.com>              *
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

#include "amarokdbushandler.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "App.h" //transferCliArgs
#include "Debug.h"
#include "collection/CollectionManager.h"
#include "collection/SqlStorage.h"
#include "collection/sqlcollection/SqlCollection.h"
#include "context/LyricsManager.h"
#include "EngineController.h"
#include "equalizersetup.h"
#include "MainWindow.h"
#include "mediabrowser.h"
#include "meta/Meta.h"
#include "meta/MetaUtility.h"
#include "meta/StreamInfoCapability.h"
#include "meta/SourceInfoCapability.h"
#include "Osd.h"
#include "playlist/PlaylistModel.h"
#include "ProgressSlider.h"
#include "scriptmanager.h"
#include "StatusBar.h"
#include "SvgHandler.h"
#include "TheInstances.h"

#include <QFile>
#include <QByteArray>

#include <Phonon/MediaObject>

#include <kactioncollection.h>
#include <kstartupinfo.h>

#include <collectionadaptor.h>
#include <contextadaptor.h>
#include <playeradaptor.h>
#include <playlistadaptor.h>
#include <playlistbrowseradaptor.h>
#include <scriptadaptor.h>


namespace Amarok
{
/////////////////////////////////////////////////////////////////////////////////////
// class DbusPlayerHandler
/////////////////////////////////////////////////////////////////////////////////////

    DbusPlayerHandler::DbusPlayerHandler()
        : QObject( kapp )
    {
        (void)new PlayerAdaptor(this);
        QDBusConnection::sessionBus().registerObject("/Player", this);
    }

    QString DbusPlayerHandler::version()
    {
        return APP_VERSION;
    }

    bool DbusPlayerHandler::dynamicModeStatus()
    {
        AMAROK_NOTIMPLEMENTED
        return false;
        //TODO: PORT 2.0
//         return Amarok::dynamicMode();
    }

    bool DbusPlayerHandler::equalizerEnabled()
    {
        AMAROK_NOTIMPLEMENTED
        if( false )
            return AmarokConfig::equalizerEnabled();
        else
            return false;
    }

    bool DbusPlayerHandler::osdEnabled()
    {
        return AmarokConfig::osdEnabled();
    }

    bool DbusPlayerHandler::isPlaying()
    {
        return The::engineController()->state() == Phonon::PlayingState;
    }

    bool DbusPlayerHandler::randomModeStatus()
    {
        return AmarokConfig::randomMode();
    }

    bool DbusPlayerHandler::repeatPlaylistStatus()
    {
        return Amarok::repeatPlaylist();
    }

    bool DbusPlayerHandler::repeatTrackStatus()
    {
        return Amarok::repeatTrack();
    }

    int DbusPlayerHandler::getVolume()
    {
        return The::engineController()->volume();
    }

    int DbusPlayerHandler::sampleRate()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->sampleRate() : 0;
    }

    double DbusPlayerHandler::score()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->score() : 0.0;
    }

    int DbusPlayerHandler::rating()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->rating() : 0;
    }

    int  DbusPlayerHandler::status()
    {
        // <0 - error, 0 - stopped, 1 - paused, 2 - playing
        switch( The::engineController()->state() )
        {
        case Phonon::PlayingState:
        case Phonon::BufferingState:
            return 2;
        case Phonon::PausedState:
            return 1;
        case Phonon::LoadingState:
        case Phonon::StoppedState:
            return 0;
        case Phonon::ErrorState:
            return -1;
        }
        return -1;
    }

    int DbusPlayerHandler::trackCurrentTime()
    {
        return The::engineController()->trackPosition();
    }

    int DbusPlayerHandler::trackCurrentTimeMs()
    {
        return The::engineController()->trackPosition() * 1000;
    }

    int DbusPlayerHandler::trackPlayCounter()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->playCount() : 0;
    }

    int DbusPlayerHandler::trackTotalTime()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->length() : 0;
    }

    QStringList DbusPlayerHandler::labels()
    {
        AMAROK_NOTIMPLEMENTED
        //TODO: fix me
        return QStringList();
    }

    QString DbusPlayerHandler::album()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->album()->prettyName() : QString();
    }

    QString DbusPlayerHandler::artist()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->artist()->prettyName() : QString();
    }

    QString DbusPlayerHandler::bitrate()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? QString::number( track->bitrate() ) : QString();
    }

    QString DbusPlayerHandler::comment()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->comment() : QString();
    }

    QString DbusPlayerHandler::coverImage()
    {
        //TODO: fix me. oups, Meta:.Album can't actually do this yet:(
        return QString();
    }

    QString DbusPlayerHandler::currentTime()
    {
        return Meta::secToPrettyTime( The::engineController()->trackPosition() );
    }

    QString DbusPlayerHandler::encodedURL()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->playableUrl().url() : QString();
    }

    QString DbusPlayerHandler::genre()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->genre()->prettyName() : QString();
    }

    QString DbusPlayerHandler::lyrics()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->cachedLyrics() : QString();
    }

    QString DbusPlayerHandler::lyricsByPath( QString path )
    {
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( KUrl(path) );
        return track ? track->cachedLyrics() : QString();
    }

    QString DbusPlayerHandler::streamName()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        if( !track )
            return QString();
        Meta::StreamInfoCapability *streamInfo = track->as<Meta::StreamInfoCapability>();
        QString streamText;
        if( streamInfo )
        {
            streamText = streamInfo->streamName();
            if( !streamInfo->streamSource().isEmpty() )
                streamText += " (" + streamInfo->streamSource() + ')';
            delete streamInfo;
        }
        else
        {
            Meta::SourceInfoCapability *sourceInfo = track->as<Meta::SourceInfoCapability>();

            if( sourceInfo )
            {
                streamText = sourceInfo->sourceName();
                delete sourceInfo;
            }
        }

        return streamText;
    }

    QString DbusPlayerHandler::nowPlaying()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        if( !track )
            return QString();
        QString playingText;
        if( track->artist() )
        {
            playingText = track->artist()->prettyName();
            playingText += " - ";
        }
        playingText += track->prettyName();
        if( track->album() )
        {
            playingText += '(';
            playingText += track->album()->prettyName();
            playingText += ')';
        }
        return playingText;
    }

    QString DbusPlayerHandler::path()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->playableUrl().path() : QString();
    }

    QString DbusPlayerHandler::title()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->prettyName() : QString();
    }

    QString DbusPlayerHandler::totalTime()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? Meta::secToPrettyTime( track->length() ) : QString();
    }

    QString DbusPlayerHandler::track()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? QString::number( track->trackNumber() ) : QString();
    }

    QString DbusPlayerHandler::type()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        QString type = track ? track->type() : QString();
        if( type == "stream/lastfm" )
            return "LastFm Stream";
        else
            return type;
    }

    QString DbusPlayerHandler::year()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->year()->prettyName() : QString();
    }

    void DbusPlayerHandler::configEqualizer()
    {
        AMAROK_NOTIMPLEMENTED
        if( false ) //TODO phonon
        {
            EqualizerSetup::instance()->show();
            EqualizerSetup::instance()->raise();
        }
    }

    void DbusPlayerHandler::enableOSD(bool enable)
    {
        Amarok::OSD::instance()->setEnabled( enable );
        AmarokConfig::setOsdEnabled( enable );
    }

    void DbusPlayerHandler::enableRandomMode( bool enable )
    {
        static_cast<KSelectAction*>(Amarok::actionCollection()->action( "random_mode" ))
            ->setCurrentItem( enable ? AmarokConfig::EnumRandomMode::Tracks : AmarokConfig::EnumRandomMode::Off );
    }

    void DbusPlayerHandler::enableRepeatPlaylist( bool enable )
    {
        static_cast<KSelectAction*>( Amarok::actionCollection()->action( "repeat" ) )
               ->setCurrentItem( enable ? AmarokConfig::EnumRepeat::Playlist : AmarokConfig::EnumRepeat::Off );
    }

     void DbusPlayerHandler::enableRepeatTrack( bool enable)
    {
        static_cast<KSelectAction*>( Amarok::actionCollection()->action( "repeat" ) )
               ->setCurrentItem( enable ? AmarokConfig::EnumRepeat::Track : AmarokConfig::EnumRepeat::Off );
    }

    void DbusPlayerHandler::loveTrack()
    {
        MainWindow::self()->loveTrack();
    }

    void DbusPlayerHandler::mediaDeviceMount()
    {
        AMAROK_NOTIMPLEMENTED
        //if ( MediaBrowser::instance()->currentDevice() )
        //   MediaBrowser::instance()->currentDevice()->connectDevice();
    }

    void DbusPlayerHandler::mediaDeviceUmount()
    {
        AMAROK_NOTIMPLEMENTED
        //if ( MediaBrowser::instance()->currentDevice() )
        //    MediaBrowser::instance()->currentDevice()->disconnectDevice();
    }

    void DbusPlayerHandler::mute()
    {
        The::engineController()->mute();
    }

    void DbusPlayerHandler::next()
    {
        The::playlistModel()->next();
    }

    void DbusPlayerHandler::pause()
    {
        The::engineController()->pause();
    }

    void DbusPlayerHandler::play()
    {
        The::engineController() ->play();
    }

    void DbusPlayerHandler::playPause()
    {
        The::engineController() ->playPause();
    }

    void DbusPlayerHandler::prev()
    {
        The::playlistModel()->back();
    }

    void DbusPlayerHandler::queueForTransfer( KUrl url )
    {
        AMAROK_NOTIMPLEMENTED
        Q_UNUSED( url );
        //MediaBrowser::queue()->addUrl( url );
        //MediaBrowser::queue()->URLsAdded();
    }

    void DbusPlayerHandler::seek(int s)
    {
        if ( s > 0 && The::engineController()->state() != Phonon::StoppedState )
            The::engineController()->seek( s * 1000 );
    }

    void DbusPlayerHandler::seekRelative(int s)
    {
        The::engineController() ->seekRelative( s * 1000 );
    }

    void DbusPlayerHandler::setEqualizer(int preamp, int band60, int band170, int band310,
        int band600, int band1k, int band3k, int band6k, int band12k, int band14k, int band16k)
    {
        AMAROK_NOTIMPLEMENTED
        if( false )
        {
            bool instantiated = EqualizerSetup::isInstantiated();
            EqualizerSetup* eq = EqualizerSetup::instance();

            QList<int> gains;
            gains << band60 << band170 << band310 << band600 << band1k
                  << band3k << band6k << band12k << band14k << band16k;

            eq->setBands( preamp, gains );
            if( !instantiated )
                delete eq;
        }
    }

    void DbusPlayerHandler::setEqualizerEnabled( bool active )
    {
        AMAROK_NOTIMPLEMENTED
//TODO PhononEqualizer        EngineController::engine()->setEqualizerEnabled( active );
        AmarokConfig::setEqualizerEnabled( active );

        if( EqualizerSetup::isInstantiated() )
            EqualizerSetup::instance()->setActive( active );
    }

    void DbusPlayerHandler::setEqualizerPreset( QString name )
    {
        AMAROK_NOTIMPLEMENTED
        bool instantiated = EqualizerSetup::isInstantiated();
        EqualizerSetup* eq = EqualizerSetup::instance();
        eq->setPreset( name );
        if ( !instantiated )
            delete eq;
    }

    void DbusPlayerHandler::setLyricsByPath( const QString& url, const QString& lyrics )
    {
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( KUrl(url) );
        if( track )
            track->setCachedLyrics( lyrics );
    }

    void DbusPlayerHandler::setScore( float score )
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        if( track )
            track->setScore( score );
    }

    void DbusPlayerHandler::setScoreByPath( const QString &url, double score )
    {
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( KUrl(url) );
        if( track )
            track->setScore( score );
    }

    void DbusPlayerHandler::setBpm( float bpm )
    {
        AMAROK_NOTIMPLEMENTED
        Q_UNUSED( bpm )
        //Meta::Track does not provide a setBpm method
        //is it necessary?
    }

    void DbusPlayerHandler::setBpmByPath( const QString &url, float bpm )
    {
        AMAROK_NOTIMPLEMENTED
        Q_UNUSED( url )
        Q_UNUSED( bpm )
        /*MetaBundle bundle( url );
        bundle.setBpm(bpm);
        bundle.save();
        CollectionDB::instance()->updateTags( bundle.url().path(), bundle, true );*/
    }

    void DbusPlayerHandler::setRating( int rating )
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        if( track )
            track->setRating( rating );
    }

    void DbusPlayerHandler::setRatingByPath( const QString &url, int rating )
    {
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( KUrl(url) );
        if( track )
            track->setRating( rating );
    }

    void DbusPlayerHandler::setVolume(int volume)
    {
        The::engineController()->setVolume(volume);
    }

    void DbusPlayerHandler::setVolumeRelative(int ticks)
    {
        The::engineController()->increaseVolume(ticks);
    }

    void DbusPlayerHandler::showBrowser( QString browser )
    {
        if ( browser == "collection" )
            MainWindow::self()->showBrowser( "CollectionBrowser" );
        if ( browser == "playlist" )
            MainWindow::self()->showBrowser( "PlaylistBrowser" );
        //if ( browser == "media" )
        //    MainWindow::self()->showBrowser( "MediaBrowser" );
        if ( browser == "file" )
            MainWindow::self()->showBrowser( "FileBrowser" );
    }

    void DbusPlayerHandler::showOSD()
    {
        Amarok::OSD::instance()->forceToggleOSD();
    }

    void DbusPlayerHandler::stop()
    {
        The::engineController() ->stop();
    }

    void DbusPlayerHandler::transferDeviceFiles()
    {
        AMAROK_NOTIMPLEMENTED
        //if ( MediaBrowser::instance()->currentDevice() )
        //    MediaBrowser::instance()->currentDevice()->transferFiles();
    }

    void DbusPlayerHandler::volumeDown()
    {
        The::engineController()->decreaseVolume();
    }

    void DbusPlayerHandler::volumeUp()
    {
        The::engineController()->increaseVolume();
    }

    void Amarok::DbusPlayerHandler::setThemeFile( const QString & url )
    {
        The::svgHandler()->setThemeFile( url );
    }

/////////////////////////////////////////////////////////////////////////////////////
// class DbusPlaylistHandler
/////////////////////////////////////////////////////////////////////////////////////

    DbusPlaylistHandler::DbusPlaylistHandler()
        : QObject( kapp )
    {
         (void)new PlaylistAdaptor(this);
         QDBusConnection::sessionBus().registerObject("/Playlist", this);
    }

    int  DbusPlaylistHandler::getActiveIndex()
    {
        return The::playlistModel()->activeRow();
    }

    int  DbusPlaylistHandler::getTotalTrackCount()
    {
        return The::playlistModel()->rowCount();
    }

    QString DbusPlaylistHandler::saveCurrentPlaylist()
    {
        QString savePath = The::playlistModel()->defaultPlaylistPath();
        The::playlistModel()->savePlaylist( savePath );
        return savePath;
    }

    void DbusPlaylistHandler::addMedia(const KUrl &url)
    {
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );
        The::playlistModel()->insertOptioned( track, Playlist::Append );
    }

    void DbusPlaylistHandler::addMediaList(const KUrl::List &urls)
    {
        Meta::TrackList tracks = CollectionManager::instance()->tracksForUrls( urls );
        The::playlistModel()->insertOptioned( tracks, Playlist::Append );
    }

    void DbusPlaylistHandler::clearPlaylist()
    {
        The::playlistModel()->clear();
    }

    void DbusPlaylistHandler::playByIndex(int index)
    {
        The::playlistModel()->play( index );
    }

    void DbusPlaylistHandler::playMedia( const KUrl &url )
    {
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );
        The::playlistModel()->insertOptioned( track, Playlist::DirectPlay | Playlist::Unique );
    }

    void DbusPlaylistHandler::popupMessage( const QString& msg )
    {
        StatusBar::instance()->longMessageThreadSafe( msg );
    }

    void DbusPlaylistHandler::removeCurrentTrack()
    {
        The::playlistModel()->removeRows( getActiveIndex(), 1 );
    }

    void DbusPlaylistHandler::removeByIndex( int index )
    {
        if( index < getTotalTrackCount() )
            The::playlistModel()->removeRows( index, 1 );
    }

    void DbusPlaylistHandler::repopulate()
    {
        AMAROK_NOTIMPLEMENTED
        //PORT 2.0
//         Playlist::instance()->repopulate();
    }

    void DbusPlaylistHandler::savePlaylist( const QString& path, bool relativePaths )
    {
        Q_UNUSED( relativePaths );
        The::playlistModel()->savePlaylist( path );
    }

    void DbusPlaylistHandler::setStopAfterCurrent( bool on )
    {
        The::playlistModel()->setStopAfterMode( on ? Playlist::StopAfterCurrent : Playlist::StopNever );
    }

    void DbusPlaylistHandler::shortStatusMessage(const QString& msg)
    {
        StatusBar::instance()->shortMessage( msg );
    }

    void DbusPlaylistHandler::shufflePlaylist()
    {
        AMAROK_NOTIMPLEMENTED
        //PORT 2.0
//         Playlist::instance()->shuffle();
    }

    void DbusPlaylistHandler::togglePlaylist()
    {
        MainWindow::self()->showHide();
    }

    QStringList DbusPlaylistHandler::filenames()
    {
        QStringList fileNames;
        foreach( Playlist::Item* item, The::playlistModel()->itemList() )
            fileNames << item->track()->prettyUrl();
        return fileNames;
    }
//     QString DbusPlaylistHandler::currentTrackUniqueId()
//     {
//         if( Playlist::instance()->currentItem() )
//             return Playlist::instance()->currentItem()->uniqueId();
//         return QString();
//     }



/////////////////////////////////////////////////////////////////////////////////////
// class DbusPlaylistBrowserHandler
/////////////////////////////////////////////////////////////////////////////////////

    DbusPlaylistBrowserHandler::DbusPlaylistBrowserHandler()
        :QObject( kapp )
    {
         (void)new PlaylistBrowserAdaptor(this);
         QDBusConnection::sessionBus().registerObject("/PlaylistBrowser", this);
    }

    void DbusPlaylistBrowserHandler::addPodcast( const QString &url )
    {
        AMAROK_NOTIMPLEMENTED
        Q_UNUSED( url );
        //PORT 2.0
//         PlaylistBrowser::instance()->addPodcast( url );
    }

    void DbusPlaylistBrowserHandler::scanPodcasts()
    {
        AMAROK_NOTIMPLEMENTED
        //PORT 2.0
//         PlaylistBrowser::instance()->scanPodcasts();
    }

    void DbusPlaylistBrowserHandler::addPlaylist( const QString &url )
    {
        AMAROK_NOTIMPLEMENTED
        Q_UNUSED( url );
        //PORT 2.0
//         PlaylistBrowser::instance()->addPlaylist( url );
    }

    int DbusPlaylistBrowserHandler::loadPlaylist( const QString &playlist )
    {
        AMAROK_NOTIMPLEMENTED
        Q_UNUSED( playlist ); return -1;
        //PORT 2.0
//         return PlaylistBrowser::instance()->loadPlaylist( playlist );
    }

/////////////////////////////////////////////////////////////////////////////////////
//  class DbusContextHandler
/////////////////////////////////////////////////////////////////////////////////////

DbusContextHandler::DbusContextHandler()
    : QObject( kapp )
{
    (void)new ContextAdaptor(this);
    QDBusConnection::sessionBus().registerObject("/Context", this);
}

void DbusContextHandler::showLyrics( const QByteArray& lyrics )
{
    LyricsManager::self()->lyricsResult( lyrics );
}
/////////////////////////////////////////////////////////////////////////////////////
// class DbusCollectionHandler
/////////////////////////////////////////////////////////////////////////////////////

    DbusCollectionHandler::DbusCollectionHandler()
        : QObject( kapp )
    {
         (void)new CollectionAdaptor(this);
         QDBusConnection::sessionBus().registerObject("/Collection", this);
    }

    int DbusCollectionHandler::totalAlbums()
    {
        SqlStorage *s = CollectionManager::instance()->sqlStorage();
        Q_ASSERT(s);
        QStringList albums = CollectionManager::instance()->sqlStorage()->query( "SELECT COUNT( id ) FROM albums;" );
        if( albums.size() < 1 )
            return 0;
        QString total = albums[0];
        return total.toInt();
    }

    int DbusCollectionHandler::totalArtists()
    {
        QStringList artists = CollectionManager::instance()->sqlStorage()->query( "SELECT COUNT( id ) FROM artists;" );
        if( artists.size() < 1 )
            return 0;
        QString total = artists[0];
        return total.toInt();
    }

    int DbusCollectionHandler::totalComposers()
    {
        QStringList composers = CollectionManager::instance()->sqlStorage()->query( "SELECT COUNT( id ) FROM composers;" );
        if( composers.size() < 1 )
            return 0;
        QString total = composers[0];
        return total.toInt();
    }

    int DbusCollectionHandler::totalCompilations()
    {
        AMAROK_NOTIMPLEMENTED
        //Todo port 2.0
        //QStringList comps = CollectionManager::instance()->sqlStorage()->query( "SELECT COUNT( DISTINCT album ) FROM tags WHERE sampler = 1;" );
        //QString total = comps[0];
        //return total.toInt();
        return -1;
    }

    int DbusCollectionHandler::totalGenres()
    {
        //This should really work across multiple collections, but theres no interface for it currently..
        QStringList genres = CollectionManager::instance()->sqlStorage()->query( "SELECT COUNT( id ) FROM genres;" );
        if( genres.size() < 1 )
            return 0;
        QString total = genres[0];
        return total.toInt();
    }

    int DbusCollectionHandler::totalTracks()
    {
        //This should really work across multiple collections, but theres no interface for it currently..
        QStringList tracks = CollectionManager::instance()->sqlStorage()->query( "SELECT COUNT( url ) FROM tracks;" );
        if( tracks.size() < 0 )
            return 0;
        QString total = tracks[0];
        int final = total.toInt();
        return final;
    }

    bool DbusCollectionHandler::isDirInCollection( const QString& path )
    {
        AMAROK_NOTIMPLEMENTED
        Q_UNUSED( path );
        return false;
    }

    bool DbusCollectionHandler::moveFile( const QString &oldURL, const QString &newURL, bool overwrite )
    {
        Q_UNUSED( oldURL ); Q_UNUSED( newURL ); Q_UNUSED( overwrite );
        AMAROK_NOTIMPLEMENTED
        return false;
//         return CollectionDB::instance()->moveFile( oldURL, newURL, overwrite );
    }

    QStringList DbusCollectionHandler::query( const QString& sql )
    {
        return CollectionManager::instance()->sqlStorage()->query( sql );
    }

    QStringList DbusCollectionHandler::similarArtists( int artists )
    {
        AMAROK_NOTIMPLEMENTED
        Q_UNUSED( artists );
        //TODO: implement
        return QStringList();
    }

    void DbusCollectionHandler::migrateFile( const QString &oldURL, const QString &newURL )
    {
        Q_UNUSED( oldURL ); Q_UNUSED( newURL );
        AMAROK_NOTIMPLEMENTED
        return;
//         CollectionDB::instance()->migrateFile( oldURL, newURL );
    }

    void DbusCollectionHandler::scanCollection()
    {
        CollectionManager::instance()->startFullScan();
    }

    void DbusCollectionHandler::scanCollectionChanges()
    {
        CollectionManager::instance()->checkCollectionChanges();
    }

    int DbusCollectionHandler::addLabels( const QString &url, const QStringList &labels )
    {
        Q_UNUSED( url ); Q_UNUSED( labels );
        AMAROK_NOTIMPLEMENTED
        return -1;
//         CollectionDB *db = CollectionDB::instance();
//         QString uid = db->getUniqueId( url );
//         int count = 0;
//         oldForeach( labels )
//         {
//             if( db->addLabel( url, *it, uid , CollectionDB::typeUser ) )
//                 count++;
//         }
//         return count;
    }

    void DbusCollectionHandler::removeLabels( const QString &url, const QStringList &oldLabels )
    {
        Q_UNUSED( url ); Q_UNUSED( oldLabels );
        AMAROK_NOTIMPLEMENTED
        return;
    }

    void DbusCollectionHandler::disableAutoScoring( bool disable )
    {
        Q_UNUSED( disable );
        AMAROK_NOTIMPLEMENTED
        return;
    }

/////////////////////////////////////////////////////////////////////////////////////
// class DbusScriptHandler
/////////////////////////////////////////////////////////////////////////////////////

    DbusScriptHandler::DbusScriptHandler()
        : QObject( kapp )
    {
         (void)new ScriptAdaptor(this);
         QDBusConnection::sessionBus().registerObject("/Script", this);
    }

    bool DbusScriptHandler::runScript(const QString& name)
    {
        return ScriptManager::instance()->runScript(name);
    }

    bool DbusScriptHandler::stopScript(const QString& name)
    {
        return ScriptManager::instance()->stopScript(name);
    }

    QStringList DbusScriptHandler::listRunningScripts()
    {
        return ScriptManager::instance()->listRunningScripts();
    }

    void DbusScriptHandler::addCustomMenuItem(QString submenu, QString itemTitle )
    {
        Q_UNUSED( submenu ); Q_UNUSED( itemTitle );
        //PORT 2.0
//         Playlist::instance()->addCustomMenuItem( submenu, itemTitle );
    }

    void DbusScriptHandler::removeCustomMenuItem(QString submenu, QString itemTitle )
    {
        Q_UNUSED( submenu ); Q_UNUSED( itemTitle );
        //PORT 2.0
//         Playlist::instance()->removeCustomMenuItem( submenu, itemTitle );
    }

    QString DbusScriptHandler::readConfig(const QString& key)
    {
        QString cleanKey = key;
        KConfigSkeletonItem* configItem = AmarokConfig::self()->findItem(cleanKey.remove(' '));
        if (configItem)
            return configItem->property().toString();
        else
            return QString();
    }

    QStringList DbusScriptHandler::readListConfig(const QString& key)
    {
        QString cleanKey = key;
        KConfigSkeletonItem* configItem = AmarokConfig::self()->findItem(cleanKey.remove(' '));
        QStringList stringList;
        if(configItem)
        {
            QVariantList variantList = configItem->property().toList();
            QVariantList::Iterator it = variantList.begin();
            while(it != variantList.end())
            {
                stringList << (*it).toString();
                ++it;
            }
        }
        return stringList;
    }

    QString DbusScriptHandler::proxyForUrl(const QString& url)
    {
        return Amarok::proxyForUrl( url );
    }

    QString DbusScriptHandler::proxyForProtocol(const QString& protocol)
    {
        return Amarok::proxyForProtocol( protocol );
    }

/////////////////////////////////////////////////////////////////////////////////////
// class DbusMediaBrowserHandler
/////////////////////////////////////////////////////////////////////////////////////

//     DbusMediaBrowserHandler::DbusMediaBrowserHandler()
//         : QObject( kapp )
//     {
//          (void)new MediaBrowserAdaptor(this);
//          QDBusConnection::sessionBus().registerObject("/MediaBrowser", this);
//     }
//
//     void DbusMediaBrowserHandler::deviceConnect()
//     {
//         if ( MediaBrowser::instance()->currentDevice() )
//             MediaBrowser::instance()->currentDevice()->connectDevice();
//     }
//
//     void DbusMediaBrowserHandler::deviceDisconnect()
//     {
//         if ( MediaBrowser::instance()->currentDevice() )
//             MediaBrowser::instance()->currentDevice()->disconnectDevice();
//     }
//
//     QStringList DbusMediaBrowserHandler::deviceList()
//     {
//         return MediaBrowser::instance()->deviceNames();
//     }
//
//     void DbusMediaBrowserHandler::deviceSwitch( QString name )
//     {
//         MediaBrowser::instance()->deviceSwitch( name );
//     }
//
//     void DbusMediaBrowserHandler::queue( KUrl url )
//     {
//         MediaBrowser::queue()->addUrl( url );
//         MediaBrowser::queue()->URLsAdded();
//     }
//
//     void DbusMediaBrowserHandler::queueList( KUrl::List urls )
//     {
//         MediaBrowser::queue()->addUrls( urls );
//     }
//
//     void DbusMediaBrowserHandler::transfer()
//     {
//         if ( MediaBrowser::instance()->currentDevice() )
//             MediaBrowser::instance()->currentDevice()->transferFiles();
//     }
//
//     void DbusMediaBrowserHandler::transcodingFinished( QString src, QString dest )
//     {
//         MediaBrowser::instance()->transcodingFinished( src, dest );
//     }

} //namespace Amarok



#include "amarokdbushandler.moc"

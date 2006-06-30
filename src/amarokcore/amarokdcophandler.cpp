/***************************************************************************
                         amarokdcophandler.cpp  -  DCOP Implementation
                            -------------------
   begin                : Sat Oct 11 2003
   copyright            : (C) 2003 by Stanislav Karchebny
                          (C) 2004 Christian Muehlhaeuser
                          (C) 2005 Ian Monroe
                          (C) 2005 Seb Ruiz
   email                : berkus@users.sf.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarok.h"
#include "amarokconfig.h"
#include "amarokdcophandler.h"
#include "app.h" //transferCliArgs
#include "debug.h"
#include "collectiondb.h"
#include "contextbrowser.h"
#include "devicemanager.h"
#include "enginebase.h"
#include "enginecontroller.h"
#include "equalizersetup.h"
#include "htmlview.h"
#include "mediabrowser.h"
#include "osd.h"
#include "playlist.h"
#include "playlistbrowser.h"
#include "playlistitem.h"
#include "playlistwindow.h"
#include "scriptmanager.h"
#include "statusbar.h"
#include "lastfm.h"

#include <qfile.h>

#include <dcopclient.h>
#include <kactioncollection.h>
#include <kstartupinfo.h>


namespace amaroK
{
/////////////////////////////////////////////////////////////////////////////////////
// class DcopPlayerHandler
/////////////////////////////////////////////////////////////////////////////////////

    DcopPlayerHandler::DcopPlayerHandler()
        : DCOPObject( "player" )
        , QObject( kapp )
    {
        // Register with DCOP
        if ( !kapp->dcopClient()->isRegistered() ) {
            kapp->dcopClient()->registerAs( "amarok", false );
            kapp->dcopClient()->setDefaultObject( objId() );
        }
    }

    QString DcopPlayerHandler::version()
    {
        return APP_VERSION;
    }

    bool DcopPlayerHandler::dynamicModeStatus()
    {
        return amaroK::dynamicMode();
    }

    bool DcopPlayerHandler::equalizerEnabled()
    {
        if(EngineController::hasEngineProperty( "HasEqualizer" ))
            return AmarokConfig::equalizerEnabled();
        else
            return false;
    }

    bool DcopPlayerHandler::osdEnabled()
    {
        return AmarokConfig::osdEnabled();
    }

    bool DcopPlayerHandler::isPlaying()
    {
        return EngineController::engine()->state() == Engine::Playing;
    }

    bool DcopPlayerHandler::randomModeStatus()
    {
        return AmarokConfig::randomMode();
    }

    bool DcopPlayerHandler::repeatPlaylistStatus()
    {
        return amaroK::repeatPlaylist();
    }

    bool DcopPlayerHandler::repeatTrackStatus()
    {
        return amaroK::repeatTrack();
    }

    int DcopPlayerHandler::getVolume()
    {
        return EngineController::engine() ->volume();
    }

    int DcopPlayerHandler::sampleRate()
    {
        return EngineController::instance()->bundle().sampleRate();
    }

    int DcopPlayerHandler::score()
    {
        const MetaBundle &bundle = EngineController::instance()->bundle();
        int score = CollectionDB::instance()->getSongPercentage( bundle.url().path() );
        return score;
    }

    int DcopPlayerHandler::rating()
    {
        const MetaBundle &bundle = EngineController::instance()->bundle();
        int rating = CollectionDB::instance()->getSongRating( bundle.url().path() );
        return rating;
    }

    int  DcopPlayerHandler::status()
    {
        // <0 - error, 0 - stopped, 1 - paused, 2 - playing
        switch( EngineController::engine()->state() )
        {
        case Engine::Playing:
            return 2;
        case Engine::Paused:
            return 1;
        case Engine::Empty:
        case Engine::Idle:
            return 0;
        }
        return -1;
    }

    int DcopPlayerHandler::trackCurrentTime()
    {
        return EngineController::engine()->position() / 1000;
    }

    int DcopPlayerHandler::trackPlayCounter()
    {
        const MetaBundle &bundle = EngineController::instance()->bundle();
        int count = CollectionDB::instance()->getPlayCount( bundle.url().path() );
        return count;
    }

    int DcopPlayerHandler::trackTotalTime()
    {
        return EngineController::instance()->bundle().length();
    }

    QString DcopPlayerHandler::album()
    {
        return EngineController::instance()->bundle().album();
    }

    QString DcopPlayerHandler::artist()
    {
        return EngineController::instance()->bundle().artist();
    }

    QString DcopPlayerHandler::bitrate()
    {
        return EngineController::instance()->bundle().prettyBitrate();
    }

    QString DcopPlayerHandler::comment()
    {
        return EngineController::instance()->bundle().comment();
    }

    QString DcopPlayerHandler::coverImage()
    {
        const MetaBundle &bundle = EngineController::instance()->bundle();
        QString image = CollectionDB::instance()->albumImage( bundle, 0 );
        return image;
    }

    QString DcopPlayerHandler::currentTime()
    {
        return MetaBundle::prettyLength( EngineController::engine() ->position() / 1000 ,true );
    }

    QString DcopPlayerHandler::encodedURL()
    {
        return EngineController::instance()->bundle().url().url();
    }

    QString DcopPlayerHandler::engine()
    {
        return AmarokConfig::soundSystem();
    }

    QString DcopPlayerHandler::genre()
    {
        return EngineController::instance()->bundle().genre();
    }

    QString DcopPlayerHandler::lyrics()
    {
        return CollectionDB::instance()->getLyrics( EngineController::instance()->bundle().url().path() );
    }

    QString DcopPlayerHandler::lyricsByPath( QString path )
    {
        return CollectionDB::instance()->getLyrics( path );
    }

    QString DcopPlayerHandler::nowPlaying()
    {
        return EngineController::instance()->bundle().prettyTitle();
    }

    QString DcopPlayerHandler::path()
    {
        return EngineController::instance()->bundle().url().path();
    }

    QString DcopPlayerHandler::setContextStyle(const QString& msg)
    {
        AmarokConfig::setContextBrowserStyleSheet( msg );
        ContextBrowser::instance()->reloadStyleSheet();

        if ( QFile::exists( amaroK::saveLocation( "themes/" + msg + '/' ) + "stylesheet.css" ) )
            return "Context browser theme '"+msg+"' applied.";
        else
            return "No such theme '"+msg+"' exists, default theme applied.";
    }

    QString DcopPlayerHandler::title()
    {
        return EngineController::instance()->bundle().title();
    }

    QString DcopPlayerHandler::totalTime()
    {
        return EngineController::instance()->bundle().prettyLength();
    }

    QString DcopPlayerHandler::track()
    {
        if ( EngineController::instance()->bundle().track() != 0 )
            return QString::number( EngineController::instance()->bundle().track() );
        else
            return QString();
    }

    QString DcopPlayerHandler::type()
    {
       if (EngineController::instance()->bundle().url().protocol() == "lastfm")
          return QString("LastFm Stream");
       else
          return EngineController::instance()->bundle().type();
    }

    QString DcopPlayerHandler::year()
    {
        return QString::number( EngineController::instance()->bundle().year() );
    }

    void DcopPlayerHandler::configEqualizer()
    {
        if(EngineController::hasEngineProperty( "HasEqualizer" ))
            EqualizerSetup::instance()->show();
            EqualizerSetup::instance()->raise();
    }

    void DcopPlayerHandler::enableOSD(bool enable)
    {
        amaroK::OSD::instance()->setEnabled( enable );
        AmarokConfig::setOsdEnabled( enable );
    }

    void DcopPlayerHandler::enableRandomMode( bool enable )
    {
        static_cast<KSelectAction*>(amaroK::actionCollection()->action( "random_mode" ))
            ->setCurrentItem( enable ? AmarokConfig::EnumRandomMode::Tracks : AmarokConfig::EnumRandomMode::Off );
    }

    void DcopPlayerHandler::enableRepeatPlaylist( bool enable )
    {
        static_cast<KSelectAction*>( amaroK::actionCollection()->action( "repeat" ) )
               ->setCurrentItem( enable ? AmarokConfig::EnumRepeat::Playlist : AmarokConfig::EnumRepeat::Off );
    }

     void DcopPlayerHandler::enableRepeatTrack( bool enable)
    {
        static_cast<KSelectAction*>( amaroK::actionCollection()->action( "repeat" ) )
               ->setCurrentItem( enable ? AmarokConfig::EnumRepeat::Track : AmarokConfig::EnumRepeat::Off );
    }

    void DcopPlayerHandler::mediaDeviceMount()
    {
        MediaBrowser::instance()->currentDevice()->connectDevice();
    }

    void DcopPlayerHandler::mediaDeviceUmount()
    {
        MediaBrowser::instance()->currentDevice()->disconnectDevice();
    }

    void DcopPlayerHandler::mute()
    {
        EngineController::instance()->mute();
    }

    void DcopPlayerHandler::next()
    {
        EngineController::instance() ->next();
    }

    void DcopPlayerHandler::pause()
    {
        EngineController::instance()->pause();
    }

    void DcopPlayerHandler::play()
    {
        EngineController::instance() ->play();
    }

    void DcopPlayerHandler::playPause()
    {
        EngineController::instance() ->playPause();
    }

    void DcopPlayerHandler::prev()
    {
        EngineController::instance() ->previous();
    }

    void DcopPlayerHandler::queueForTransfer( KURL url )
    {
        MediaBrowser::queue()->addURL( url );
        MediaBrowser::queue()->URLsAdded();
    }

    void DcopPlayerHandler::seek(int s)
    {
        if ( s > 0 && EngineController::engine()->state() != Engine::Empty )
            EngineController::instance()->seek( s * 1000 );
    }

    void DcopPlayerHandler::seekRelative(int s)
    {
        EngineController::instance() ->seekRelative( s * 1000 );
    }

    void DcopPlayerHandler::setEqualizer(int preamp, int band60, int band170, int band310,
        int band600, int band1k, int band3k, int band6k, int band12k, int band14k, int band16k)
    {
        if( EngineController::hasEngineProperty( "HasEqualizer" ) ) {
            bool instantiated = EqualizerSetup::isInstantiated();
            EqualizerSetup* eq = EqualizerSetup::instance();

            QValueList<int> gains;
            gains << band60 << band170 << band310 << band600 << band1k
                  << band3k << band6k << band12k << band14k << band16k;

            eq->setBands( preamp, gains );
            if( !instantiated )
                delete eq;
        }
    }

    void DcopPlayerHandler::setEqualizerEnabled( bool active )
    {
        EngineController::engine()->setEqualizerEnabled( active );
        AmarokConfig::setEqualizerEnabled( active );

        if( EqualizerSetup::isInstantiated() )
            EqualizerSetup::instance()->setActive( active );
    }

    void DcopPlayerHandler::setEqualizerPreset( QString name )
    {
        if( EngineController::hasEngineProperty( "HasEqualizer" ) ) {
            bool instantiated = EqualizerSetup::isInstantiated();
            EqualizerSetup* eq = EqualizerSetup::instance();
            eq->setPreset( name );
            if ( !instantiated )
                delete eq;
        }
    }

    void DcopPlayerHandler::setLyricsByPath( const QString& url, const QString& lyrics )
    {
        CollectionDB::instance()->setLyrics( url, lyrics );
    }

    void DcopPlayerHandler::setScore( int score )
    {
        const QString &url = EngineController::instance()->bundle().url().path();
        CollectionDB::instance()->setSongPercentage(url, score);
    }

    void DcopPlayerHandler::setScoreByPath( const QString &url, int score )
    {
        CollectionDB::instance()->setSongPercentage(url, score);
    }

    void DcopPlayerHandler::setRating( int rating )
    {
        const QString &url = EngineController::instance()->bundle().url().path();
        CollectionDB::instance()->setSongRating(url, rating);
    }

    void DcopPlayerHandler::setRatingByPath( const QString &url, int rating )
    {
        CollectionDB::instance()->setSongRating(url, rating);
    }

    void DcopPlayerHandler::setVolume(int volume)
    {
        EngineController::instance()->setVolume(volume);
    }

    void DcopPlayerHandler::setVolumeRelative(int ticks)
    {
        EngineController::instance()->increaseVolume(ticks);
    }

    void DcopPlayerHandler::showBrowser( QString browser )
    {
        if ( browser == "context" )
            PlaylistWindow::self()->showBrowser( "ContextBrowser" );
        if ( browser == "collection" )
            PlaylistWindow::self()->showBrowser( "CollectionBrowser" );
        if ( browser == "playlist" )
            PlaylistWindow::self()->showBrowser( "PlaylistBrowser" );
        if ( browser == "media" )
            PlaylistWindow::self()->showBrowser( "MediaBrowser" );
        if ( browser == "file" )
            PlaylistWindow::self()->showBrowser( "FileBrowser" );
    }

    void DcopPlayerHandler::showOSD()
    {
        amaroK::OSD::instance()->forceToggleOSD();
    }

    void DcopPlayerHandler::stop()
    {
        EngineController::instance() ->stop();
    }

    void DcopPlayerHandler::transferDeviceFiles()
    {
        MediaBrowser::instance()->currentDevice()->transferFiles();
    }

    void DcopPlayerHandler::volumeDown()
    {
        EngineController::instance()->decreaseVolume();
    }

    void DcopPlayerHandler::volumeUp()
    {
        EngineController::instance()->increaseVolume();
    }

    void DcopPlayerHandler::transferCliArgs( QStringList args )
    {
        DEBUG_BLOCK

        //stop startup cursor animation - do not mess with this, it's carefully crafted
        //NOTE I have no idea why we need to do this, I never get startup notification from
        //the amarok binary anyway --mxcl
        debug() << "Startup ID: " << args.first() << endl;
        kapp->setStartupId( args.first().local8Bit() );
#ifdef Q_WS_X11
        // currently X11 only
        KStartupInfo::appStarted();
#endif
        args.pop_front();

        const int argc = args.count() + 1;
        char **argv = new char*[argc];

        QStringList::ConstIterator it = args.constBegin();
        for( int i = 1; i < argc; ++i, ++it ) {
            argv[i] = qstrdup( (*it).local8Bit() );
            debug() << "Extracted: " << argv[i] << endl;
        }

        // required, loader doesn't add it
        argv[0] = qstrdup( "amarokapp" );

        // re-initialize KCmdLineArgs with the new arguments
        App::initCliArgs( argc, argv );
        App::handleCliArgs();

        //FIXME are we meant to leave this around?
        //FIXME are we meant to allocate it all on the heap?
        //NOTE we allow the memory leak because I think there are
        // some very mysterious crashes due to deleting this
        //delete[] argv;
    }

/////////////////////////////////////////////////////////////////////////////////////
// class DcopPlaylistHandler
/////////////////////////////////////////////////////////////////////////////////////

    DcopPlaylistHandler::DcopPlaylistHandler()
        : DCOPObject( "playlist" )
        , QObject( kapp )
    {}

    int  DcopPlaylistHandler::getActiveIndex()
    {
        return Playlist::instance()->currentTrackIndex( false );
    }

    int  DcopPlaylistHandler::getTotalTrackCount()
    {
        return Playlist::instance()->totalTrackCount();
    }

    QString DcopPlaylistHandler::saveCurrentPlaylist()
    {
        Playlist::instance()->saveXML( Playlist::defaultPlaylistPath() );
        return Playlist::defaultPlaylistPath();
    }

    void DcopPlaylistHandler::addMedia(const KURL &url)
    {
        Playlist::instance()->appendMedia(url);
    }

    void DcopPlaylistHandler::addMediaList(const KURL::List &urls)
    {
        Playlist::instance()->insertMedia(urls);
    }

    void DcopPlaylistHandler::clearPlaylist()
    {
        Playlist::instance()->clear();
    }

    void DcopPlaylistHandler::playByIndex(int index)
    {
        Playlist::instance()->activateByIndex( index );
    }

    void DcopPlaylistHandler::playMedia( const KURL &url )
    {
        Playlist::instance()->insertMedia( url, Playlist::DirectPlay | Playlist::Unique);
    }

    void DcopPlaylistHandler::popupMessage( const QString& msg )
    {
        StatusBar::instance()->longMessageThreadSafe( msg );
    }

    void DcopPlaylistHandler::removeCurrentTrack()
    {
        PlaylistItem* const item = Playlist::instance()->currentTrack();
        if ( item ) {
            if( item->isBeingRenamed() )
                item->setDeleteAfterEditing( true );
            else
            {
                Playlist::instance()->removeItem( item );
                delete item;
            }
        }
    }

    void DcopPlaylistHandler::removeByIndex( int index )
    {
        PlaylistItem* const item =
            static_cast<PlaylistItem*>( Playlist::instance()->itemAtIndex( index ) );

        if ( item ) {
            Playlist::instance()->removeItem( item );
            delete item;
        }
    }

    void DcopPlaylistHandler::repopulate()
    {
        Playlist::instance()->repopulate();
    }

    void DcopPlaylistHandler::saveM3u( const QString& path, bool relativePaths )
    {
        Playlist::instance()->saveM3U( path, relativePaths );
    }

    void DcopPlaylistHandler::setStopAfterCurrent( bool on )
    {
        Playlist::instance()->setStopAfterCurrent( on );
    }

    void DcopPlaylistHandler::shortStatusMessage(const QString& msg)
    {
        StatusBar::instance()->shortMessage( msg );
    }

    void DcopPlaylistHandler::shufflePlaylist()
    {
        Playlist::instance()->shuffle();
    }

    void DcopPlaylistHandler::togglePlaylist()
    {
        PlaylistWindow::self()->showHide();
    }

/////////////////////////////////////////////////////////////////////////////////////
// class DcopPlaylistBrowserHandler
/////////////////////////////////////////////////////////////////////////////////////

    DcopPlaylistBrowserHandler::DcopPlaylistBrowserHandler()
        : DCOPObject( "playlistbrowser" )
        , QObject( kapp )
    {}

    void DcopPlaylistBrowserHandler::addPodcast( const QString &url )
    {
        PlaylistBrowser::instance()->addPodcast( url );
    }

    void DcopPlaylistBrowserHandler::scanPodcasts()
    {
        PlaylistBrowser::instance()->scanPodcasts();
    }

    void DcopPlaylistBrowserHandler::addPlaylist( const QString &url )
    {
        PlaylistBrowser::instance()->addPlaylist( url );
    }

    int DcopPlaylistBrowserHandler::loadPlaylist( const QString &playlist )
    {
        return PlaylistBrowser::instance()->loadPlaylist( playlist );
    }

/////////////////////////////////////////////////////////////////////////////////////
// class DcopContextBrowserHandler
/////////////////////////////////////////////////////////////////////////////////////

    DcopContextBrowserHandler::DcopContextBrowserHandler()
        : DCOPObject( "contextbrowser" )
        , QObject( kapp )
    {}

    void DcopContextBrowserHandler::showCurrentTrack()
    {
        ContextBrowser::instance()->showCurrentTrack();
    }

    void DcopContextBrowserHandler::showLyrics()
    {
        ContextBrowser::instance()->showLyrics();
    }

    void DcopContextBrowserHandler::showWiki()
    {
        ContextBrowser::instance()->showWikipedia();
    }

    void DcopContextBrowserHandler::showLyrics( const QCString& lyrics )
    {
        ContextBrowser::instance()->lyricsResult( lyrics );
    }

/////////////////////////////////////////////////////////////////////////////////////
// class DcopCollectionHandler
/////////////////////////////////////////////////////////////////////////////////////

    DcopCollectionHandler::DcopCollectionHandler()
        : DCOPObject( "collection" )
        , QObject( kapp )
    {}

    int DcopCollectionHandler::totalAlbums()
    {
        QStringList albums = CollectionDB::instance()->query( "SELECT COUNT( id ) FROM album;" );
        QString total = albums[0];
        return total.toInt();
    }

    int DcopCollectionHandler::totalArtists()
    {
        QStringList artists = CollectionDB::instance()->query( "SELECT COUNT( id ) FROM artist;" );
        QString total = artists[0];
        return total.toInt();
    }

    int DcopCollectionHandler::totalCompilations()
    {
        QStringList comps = CollectionDB::instance()->query( "SELECT COUNT( DISTINCT album ) FROM tags WHERE sampler = 1;" );
        QString total = comps[0];
        return total.toInt();
    }

    int DcopCollectionHandler::totalGenres()
    {
        QStringList genres = CollectionDB::instance()->query( "SELECT COUNT( id ) FROM genre;" );
        QString total = genres[0];
        return total.toInt();
    }

    int DcopCollectionHandler::totalTracks()
    {
        QStringList tracks = CollectionDB::instance()->query( "SELECT COUNT( url ) FROM tags;" );
        QString total = tracks[0];
        int final = total.toInt();
        return final;
    }

    bool DcopCollectionHandler::isDirInCollection( const QString& path )
    {
        return CollectionDB::instance()->isDirInCollection( path );
    }

    bool DcopCollectionHandler::moveFile( const QString &oldURL, const QString &newURL, bool overwrite )
    {
        return CollectionDB::instance()->moveFile( oldURL, newURL, overwrite );
    }

    QStringList DcopCollectionHandler::query( const QString& sql )
    {
        return CollectionDB::instance()->query( sql );
    }

    QStringList DcopCollectionHandler::similarArtists( int artists )
    {
        return CollectionDB::instance()->similarArtists( EngineController::instance()->bundle().artist(), artists );
    }

    void DcopCollectionHandler::migrateFile( const QString &oldURL, const QString &newURL )
    {
        CollectionDB::instance()->migrateFile( oldURL, newURL );
    }

    void DcopCollectionHandler::scanCollection()
    {
        CollectionDB::instance()->startScan();
    }

    void DcopCollectionHandler::scanCollectionChanges()
    {
        CollectionDB::instance()->scanMonitor();
    }

    void DcopCollectionHandler::disableAutoScoring( bool disable )
    {
        CollectionDB::instance()->disableAutoScoring( disable );
    }

    void DcopCollectionHandler::newUniqueIdForFile( const QString &path )
    {
        CollectionDB::instance()->newUniqueIdForFile( path );
    }

    void DcopCollectionHandler::newUniqueIdForFiles( const QStringList &list )
    {
        foreach( list )
            CollectionDB::instance()->newUniqueIdForFile( *it );
    }

/////////////////////////////////////////////////////////////////////////////////////
// class DcopScriptHandler
/////////////////////////////////////////////////////////////////////////////////////

    DcopScriptHandler::DcopScriptHandler()
        : DCOPObject( "script" )
        , QObject( kapp )
    {}

    bool DcopScriptHandler::runScript(const QString& name)
    {
        return ScriptManager::instance()->runScript(name);
    }

    bool DcopScriptHandler::stopScript(const QString& name)
    {
        return ScriptManager::instance()->stopScript(name);
    }

    QStringList DcopScriptHandler::listRunningScripts()
    {
        return ScriptManager::instance()->listRunningScripts();
    }

    void DcopScriptHandler::addCustomMenuItem(QString submenu, QString itemTitle )
    {
        Playlist::instance()->addCustomMenuItem( submenu, itemTitle );
    }

    void DcopScriptHandler::removeCustomMenuItem(QString submenu, QString itemTitle )
    {
        Playlist::instance()->removeCustomMenuItem( submenu, itemTitle );
    }

    QString DcopScriptHandler::readConfig(const QString& key)
    {
        QString cleanKey = key;
        KConfigSkeletonItem* configItem = AmarokConfig::self()->findItem(cleanKey.remove(' '));
        if (configItem)
            return configItem->property().toString();
        else
            return QString::null;
    }

    QStringList DcopScriptHandler::readListConfig(const QString& key)
    {
        QString cleanKey = key;
        KConfigSkeletonItem* configItem = AmarokConfig::self()->findItem(cleanKey.remove(' '));
        QStringList stringList;
        if(configItem)
        {
            QValueList<QVariant> variantList = configItem->property().toList();
            QValueList<QVariant>::Iterator it = variantList.begin();
            while(it != variantList.end())
            {
                stringList << (*it).toString();
                ++it;
            }
        }
        return stringList;
    }

/////////////////////////////////////////////////////////////////////////////////////
// class DcopDevicesHandler
/////////////////////////////////////////////////////////////////////////////////////

    DcopDevicesHandler::DcopDevicesHandler()
        : DCOPObject( "devices" )
        , QObject( kapp )
    {}

    void DcopDevicesHandler::mediumAdded(QString name)
    {
        DeviceManager::instance()->mediumAdded(name);
    }

    void DcopDevicesHandler::mediumRemoved(QString name)
    {
        DeviceManager::instance()->mediumRemoved(name);
    }

    void DcopDevicesHandler::mediumChanged(QString name)
    {
        DeviceManager::instance()->mediumChanged(name);
    }

    QStringList DcopDevicesHandler::showDeviceList()
    {
        return DeviceManager::instance()->getDeviceStringList();
    }

/////////////////////////////////////////////////////////////////////////////////////
// class DcopDevicesHandler
/////////////////////////////////////////////////////////////////////////////////////

    DcopMediaBrowserHandler::DcopMediaBrowserHandler()
        : DCOPObject( "mediabrowser" )
        , QObject( kapp )
    {}

    void DcopMediaBrowserHandler::deviceConnect()
    {
        MediaBrowser::instance()->currentDevice()->connectDevice();
    }

    void DcopMediaBrowserHandler::deviceDisconnect()
    {
        MediaBrowser::instance()->currentDevice()->disconnectDevice();
    }

    QStringList DcopMediaBrowserHandler::deviceList()
    {
        return MediaBrowser::instance()->deviceNames();
    }

    void DcopMediaBrowserHandler::deviceSwitch( QString name )
    {
        MediaBrowser::instance()->deviceSwitch( name );
    }

    void DcopMediaBrowserHandler::queue( KURL url )
    {
        MediaBrowser::queue()->addURL( url );
        MediaBrowser::queue()->URLsAdded();
    }

    void DcopMediaBrowserHandler::queueList( KURL::List urls )
    {
        MediaBrowser::queue()->addURLs( urls );
    }

    void DcopMediaBrowserHandler::transfer()
    {
        MediaBrowser::instance()->currentDevice()->transferFiles();
    }

    void DcopMediaBrowserHandler::transcodingFinished( QString src, QString dest )
    {
        MediaBrowser::instance()->transcodingFinished( src, dest );
    }

} //namespace amaroK

#include "amarokdcophandler.moc"

/***************************************************************************
                         amarokdcophandler.cpp  -  DCOP Implementation
                            -------------------
   begin                : Sat Oct 11 2003
   copyright            : (C) 2003 by Stanislav Karchebny
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
#include "engine/enginebase.h"
#include "enginecontroller.h"
#include "equalizersetup.h"
#include "mediabrowser.h"
#include "osd.h"
#include "playlist.h"
#include "playlistbrowser.h"
#include "playlistitem.h"
#include "playlistwindow.h"
#include "scriptmanager.h"
#include "statusbar.h"

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

    bool DcopPlayerHandler::dynamicModeStatus()
    {
        return AmarokConfig::dynamicMode();
    }

    bool DcopPlayerHandler::equalizerEnabled()
    {
        if(EngineController::hasEngineProperty( "HasEqualizer" ))
            return AmarokConfig::equalizerEnabled();
        else
            return false;
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
        return AmarokConfig::repeatPlaylist();
    }

    bool DcopPlayerHandler::repeatTrackStatus()
    {
        return AmarokConfig::repeatTrack();
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
        return MetaBundle::prettyLength( EngineController::engine() ->position() / 1000 );
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
        ContextBrowser::instance()->setStyleSheet();

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
        return EngineController::instance()->bundle().track();
    }

    QString DcopPlayerHandler::type()
    {
        return EngineController::instance()->bundle().type();
    }

    QString DcopPlayerHandler::year()
    {
        return EngineController::instance()->bundle().year();
    }

    void DcopPlayerHandler::configEqualizer()
    {
        if(EngineController::hasEngineProperty( "HasEqualizer" ))
            EqualizerSetup::instance()->show();
            EqualizerSetup::instance()->raise();
    }

    void DcopPlayerHandler::enableDynamicMode( bool enable )
    {
        static_cast<KToggleAction*>(amaroK::actionCollection()->action( "dynamic_mode" ))->setChecked( enable );
    }

    void DcopPlayerHandler::enableOSD(bool enable)
    {
        amaroK::OSD::instance()->setEnabled( enable );
        AmarokConfig::setOsdEnabled( enable );
    }

    void DcopPlayerHandler::enableRandomMode( bool enable )
    {
        static_cast<KToggleAction*>(amaroK::actionCollection()->action( "random_mode" ))->setChecked( enable );
    }

    void DcopPlayerHandler::enableRepeatPlaylist( bool enable )
    {
        static_cast<KToggleAction*>(amaroK::actionCollection()->action( "repeat_playlist" ))->setChecked( enable );
    }

     void DcopPlayerHandler::enableRepeatTrack( bool enable)
    {
        static_cast<KToggleAction*>(amaroK::actionCollection()->action( "repeat_track" ))->setChecked( enable );
    }

    void DcopPlayerHandler::mediaDeviceMount()
    {
        MediaDevice::instance()->mount();
    }

    void DcopPlayerHandler::mediaDeviceUmount()
    {
        MediaDevice::instance()->umount();
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
        MediaDevice::instance()->addURL( url );
    }

    void DcopPlayerHandler::seek(int s)
    {
        EngineBase* const engine = EngineController::engine();
        if ( s > 0 && engine->state() != Engine::Empty )
            engine ->seek( s * 1000 );
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

    void DcopPlayerHandler::setVolume(int volume)
    {
        EngineController::instance()->setVolume(volume);
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
        MediaDevice::instance()->transferFiles();
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
        KStartupInfo::appStarted();
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
        return Playlist::instance()->currentTrackIndex();
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

    void DcopPlaylistHandler::popupMessage(const QString& msg)
    {
        StatusBar::instance()->longMessageThreadSafe( msg );
    }

    void DcopPlaylistHandler::removeCurrentTrack()
    {
        PlaylistItem* const item = Playlist::instance()->currentTrack();
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

/////////////////////////////////////////////////////////////////////////////////////
// class DcopContextBrowserHandler
/////////////////////////////////////////////////////////////////////////////////////

    DcopContextBrowserHandler::DcopContextBrowserHandler()
        : DCOPObject( "contextbrowser" )
        , QObject( kapp )
    {}

    void DcopContextBrowserHandler::showHome()
    {
        ContextBrowser::instance()->showHome();
    }

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
} //namespace amaroK

#include "amarokdcophandler.moc"

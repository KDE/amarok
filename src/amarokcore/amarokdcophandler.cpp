/***************************************************************************
                         amarokdcophandler.cpp  -  DCOP Implementation
                            -------------------
   begin                : Sat Oct 11 2003
   copyright            : (C) 2003 by Stanislav Karchebny
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
#include "osd.h"
#include "playlist.h"
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

    void DcopPlayerHandler::play()
    {
        EngineController::instance() ->play();
    }

    void DcopPlayerHandler::playPause()
    {
        EngineController::instance() ->playPause();
    }

    void DcopPlayerHandler::stop()
    {
        EngineController::instance() ->stop();
    }

    void DcopPlayerHandler::next()
    {
        EngineController::instance() ->next();
    }

    void DcopPlayerHandler::prev()
    {
        EngineController::instance() ->previous();
    }

    void DcopPlayerHandler::pause()
    {
        EngineController::instance()->pause();
    }

    bool DcopPlayerHandler::isPlaying()
    {
        return EngineController::engine()->state() == Engine::Playing;
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

    bool DcopPlayerHandler::repeatTrackStatus()
    {
        return AmarokConfig::repeatTrack();
    }

    bool DcopPlayerHandler::repeatPlaylistStatus()
    {
        return AmarokConfig::repeatPlaylist();
    }

    bool DcopPlayerHandler::randomModeStatus()
    {
        return AmarokConfig::randomMode();
    }

    bool DcopPlayerHandler::dynamicModeStatus()
    {
        return AmarokConfig::partyMode();
    }

    QString DcopPlayerHandler::nowPlaying()
    {
        return EngineController::instance()->bundle().prettyTitle();
    }

    QString DcopPlayerHandler::artist()
    {
        return EngineController::instance()->bundle().artist();
    }

    QString DcopPlayerHandler::title()
    {
        return EngineController::instance()->bundle().title();
    }

    QString DcopPlayerHandler::track()
    {
        return EngineController::instance()->bundle().track();
    }

    QString DcopPlayerHandler::album()
    {
        return EngineController::instance()->bundle().album();
    }

    QString DcopPlayerHandler::totalTime()
    {
        return EngineController::instance()->bundle().prettyLength();
    }

    QString DcopPlayerHandler::currentTime()
    {
        return MetaBundle::prettyLength( EngineController::engine() ->position() / 1000 );
    }

    QString DcopPlayerHandler::genre()
    {
        return EngineController::instance()->bundle().genre();
    }

    QString DcopPlayerHandler::year()
    {
        return EngineController::instance()->bundle().year();
    }

    QString DcopPlayerHandler::comment()
    {
        return EngineController::instance()->bundle().comment();
    }

    QString DcopPlayerHandler::bitrate()
    {
        return EngineController::instance()->bundle().prettyBitrate();
    }

    int DcopPlayerHandler::sampleRate()
    {
        return EngineController::instance()->bundle().sampleRate();
    }

    QString DcopPlayerHandler::encodedURL()
    {
        return EngineController::instance()->bundle().url().url();
    }

    QString DcopPlayerHandler::path()
    {
        return EngineController::instance()->bundle().url().path();
    }

    QString DcopPlayerHandler::coverImage()
    {
        const MetaBundle &bundle = EngineController::instance()->bundle();
        QString image = CollectionDB::instance()->albumImage( bundle, 0 );
        return image;
    }

    int DcopPlayerHandler::score()
    {
        const MetaBundle &bundle = EngineController::instance()->bundle();
        int score = CollectionDB::instance()->getSongPercentage( bundle.url().path() );
        return score;
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

    int DcopPlayerHandler::trackTotalTime()
    {
        return EngineController::instance()->bundle().length();
    }

    int DcopPlayerHandler::trackCurrentTime()
    {
        return EngineController::engine()->position() / 1000;
    }

    int DcopPlayerHandler::trackPlayCounter()
    {
        const MetaBundle &bundle = EngineController::instance()->bundle();
        int count = CollectionDB::instance()->getPlayCount( bundle.url().path() );
        return count;    }

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

    void DcopPlayerHandler::enableRandomMode(bool enable)
    {
        static_cast<KToggleAction*>(amaroK::actionCollection()->action( "random_mode" ))->setChecked( enable );
    }

    void DcopPlayerHandler::enableRepeatPlaylist(bool enable)
    {
        static_cast<KToggleAction*>(amaroK::actionCollection()->action( "repeat_playlist" ))->setChecked( enable );
    }

     void DcopPlayerHandler::enableRepeatTrack(bool enable)
    {
        static_cast<KToggleAction*>(amaroK::actionCollection()->action( "repeat_track" ))->setChecked( enable );
    }

    void DcopPlayerHandler::enableDynamicMode(bool enable)
    {
        static_cast<KToggleAction*>(amaroK::actionCollection()->action( "dynamic_mode" ))->setChecked( enable );
    }

    void DcopPlayerHandler::setVolume(int volume)
    {
        EngineController::instance()->setVolume(volume);
    }

    int DcopPlayerHandler::getVolume()
    {
        return EngineController::engine() ->volume();
    }

    void DcopPlayerHandler::volumeUp()
    {
        EngineController::instance()->increaseVolume();
    }

    void DcopPlayerHandler::volumeDown()
    {
        EngineController::instance()->decreaseVolume();
    }

    void DcopPlayerHandler::mute()
    {
        EngineController::instance()->mute();
    }

    void DcopPlayerHandler::setEqualizerEnabled( bool active )
    {
        EngineController::engine()->setEqualizerEnabled( active );
        AmarokConfig::setEqualizerEnabled( active );
    }
    bool DcopPlayerHandler::equalizerEnabled()
    {
        if(EngineController::hasEngineProperty( "HasEqualizer" ))
            return AmarokConfig::equalizerEnabled();
        else
            return false;
    }
    void DcopPlayerHandler::configEqualizer()
    {
        if(EngineController::hasEngineProperty( "HasEqualizer" ))
            EqualizerSetup::instance()->raise();
    }

    void DcopPlayerHandler::enableOSD(bool enable)
    {
        amaroK::OSD::instance()->setEnabled( enable );
        AmarokConfig::setOsdEnabled( enable );
    }

    void DcopPlayerHandler::showOSD()
    {
        amaroK::OSD::instance()->forceToggleOSD();
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

    QString DcopPlayerHandler::setContextStyle(const QString& msg)
    {
        AmarokConfig::setContextBrowserStyleSheet( msg );
        ContextBrowser::instance()->setStyleSheet();

        if ( QFile::exists( amaroK::saveLocation( "themes/" + msg + '/' ) + "stylesheet.css" ) )
            return "Context browser theme '"+msg+"' applied.";
        else
            return "No such theme '"+msg+"' exists, default theme applied.";
    }

    void DcopPlayerHandler::setEqualizer(int preamp, int band60, int band170, int band310,
        int band600, int band1k, int band3k, int band6k, int band12k, int band14k, int band16k)
    {
        QValueList<int> gains;
        gains << band60 << band170 << band310 << band600 << band1k
            << band3k << band6k << band12k << band14k << band16k;
        AmarokConfig::setEqualizerGains( gains );
        AmarokConfig::setEqualizerPreamp( preamp );
        EngineController::engine()->setEqualizerParameters(preamp, gains);
        if (EqualizerSetup::isInstantiated())
            EqualizerSetup::instance()->updateSliders(preamp,gains);
    }


/////////////////////////////////////////////////////////////////////////////////////
// class DcopPlaylistHandler
/////////////////////////////////////////////////////////////////////////////////////

    DcopPlaylistHandler::DcopPlaylistHandler()
        : DCOPObject( "playlist" )
        , QObject( kapp )
    {}

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

    void DcopPlaylistHandler::shufflePlaylist()
    {
        Playlist::instance()->shuffle();
    }

    QString DcopPlaylistHandler::saveCurrentPlaylist()
    {
        Playlist::instance()->saveXML( Playlist::defaultPlaylistPath() );
        return Playlist::defaultPlaylistPath();
    }

    void DcopPlaylistHandler::saveM3u( const QString& path, bool relativePaths )
    {
        Playlist::instance()->saveM3U( path, relativePaths );
    }

    void DcopPlaylistHandler::removeCurrentTrack()
    {
        PlaylistItem* const item = Playlist::instance()->currentTrack();
        if ( item ) {
            Playlist::instance()->removeItem( item );
            delete item;
        }
    }

    void DcopPlaylistHandler::playByIndex(int index)
    {
        Playlist::instance()->activateByIndex( index );
    }

    int  DcopPlaylistHandler::getActiveIndex()
    {
        return Playlist::instance()->currentTrackIndex();
    }

    int  DcopPlaylistHandler::getTotalTrackCount()
    {
        return Playlist::instance()->totalTrackCount();
    }

    void DcopPlaylistHandler::setStopAfterCurrent( bool on )
    {
        Playlist::instance()->setStopAfterCurrent( on );
    }

    void DcopPlaylistHandler::togglePlaylist()
    {
        PlaylistWindow::self()->showHide();
    }

    void DcopPlaylistHandler::playMedia( const KURL &url )
    {
        /* ContextBrowser::openURLRequest is necessary for konquisidebar */
        ContextBrowser::instance()->openURLRequest( url );
    }

    void DcopPlaylistHandler::shortStatusMessage(const QString& msg)
    {
        StatusBar::instance()->shortMessage( msg );
    }

    void DcopPlaylistHandler::popupMessage(const QString& msg)
    {
        StatusBar::instance()->longMessageThreadSafe( msg );
    }

/////////////////////////////////////////////////////////////////////////////////////
// class DcopCollectionHandler
/////////////////////////////////////////////////////////////////////////////////////

    DcopCollectionHandler::DcopCollectionHandler()
        : DCOPObject( "collection" )
        , QObject( kapp )
    {}

    QStringList DcopCollectionHandler::query( const QString& sql )
    {
        return CollectionDB::instance()->query( sql );
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

} //namespace amaroK

#include "amarokdcophandler.moc"

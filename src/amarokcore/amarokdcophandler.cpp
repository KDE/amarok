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
#include "browserbar.h"
#include "collectiondb.h"
#include "contextbrowser.h"
#include "engine/enginebase.h"
#include "enginecontroller.h"
#include "equalizersetup.h"
#include "osd.h"
#include "playlist.h"
#include "playlistwindow.h"
#include "statusbar.h"

#include <dcopclient.h>
#include <kactioncollection.h>


namespace amaroK
{
/////////////////////////////////////////////////////////////////////////////////////
// class DcopPlayerHandler
/////////////////////////////////////////////////////////////////////////////////////

    DcopPlayerHandler::DcopPlayerHandler()
        : DCOPObject( "player" )
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

    QString DcopPlayerHandler::nowPlaying()
    {
        return EngineController::instance()->bundle().prettyTitle();
    }

    int DcopPlayerHandler::trackTotalTime()
    {
        return EngineController::instance()->bundle().length();
    }

    int DcopPlayerHandler::trackCurrentTime()
    {
        return EngineController::engine()->position() / 1000;
    }

    void DcopPlayerHandler::seek(int s)
    {
        EngineBase* const engine = EngineController::engine();
        if ( s > 0 && engine->state() != Engine::Empty )
            engine ->seek( s * 1000 );
    }

    void DcopPlayerHandler::seekRelative(int s)
    {
        EngineBase* const engine = EngineController::engine();
        if ( engine->state() != Engine::Empty ) {
            int newpos = s * 1000 + engine->position();
            if ( newpos < 0 )
                newpos = 0;
            engine->seek ( newpos );
        }
    }

    void DcopPlayerHandler::enableRandomMode(bool enable)
    {
        static_cast<KToggleAction*>(amaroK::actionCollection()->action( "random_mode" ))->setChecked( enable );
    }

    void DcopPlayerHandler::scanCollection()
    {
        CollectionDB::instance()->startScan();
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

    void DcopPlayerHandler::configEqualizer()
    {
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
//         debug() << "Startup ID: " << args.first() << endl;
//         kapp->setStartupId( args.first().local8Bit() );
//         KStartupInfo::appStarted();
//         args.pop_front();

        const int argc = args.count() + 1;
        char **argv = new char*[argc];

        QStringList::ConstIterator it = args.constBegin();
        for( int i = 1; i < argc; ++i, ++it ) {
            argv[i] = const_cast<char*>((*it).latin1());
            debug() << "Extracted: " << argv[i] << endl;
        }

        // required, loader doesn't add it
        argv[0] = (char*)"amarokapp";

        // re-initialize KCmdLineArgs with the new arguments
        App::initCliArgs( argc, argv );
        App::handleCliArgs();

        //FIXME are we meant to leave this around?
        //FIXME are we meant to allocate it all on the heap?
        delete[] argv;
    }



/////////////////////////////////////////////////////////////////////////////////////
// class DcopPlaylistHandler
/////////////////////////////////////////////////////////////////////////////////////

    DcopPlaylistHandler::DcopPlaylistHandler()
        : DCOPObject( "playlist" )
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

    void DcopPlaylistHandler::saveCurrentPlaylist()
    {
        Playlist::instance()->saveXML( Playlist::defaultPlaylistPath() );
    }

    QString DcopPlaylistHandler::artist()
    {
        return EngineController::instance()->bundle().artist();
    }

    QString DcopPlaylistHandler::title()
    {
        return EngineController::instance()->bundle().title();
    }

    QString DcopPlaylistHandler::track()
    {
        return EngineController::instance()->bundle().track();
    }

    QString DcopPlaylistHandler::album()
    {
        return EngineController::instance()->bundle().album();
    }

    QString DcopPlaylistHandler::totalTime()
    {
        return EngineController::instance()->bundle().prettyLength();
    }

    QString DcopPlaylistHandler::currentTime()
    {
        return MetaBundle::prettyLength( EngineController::engine() ->position() / 1000 );
    }

    QString DcopPlaylistHandler::genre()
    {
        return EngineController::instance()->bundle().genre();
    }

    QString DcopPlaylistHandler::year()
    {
        return EngineController::instance()->bundle().year();
    }

    QString DcopPlaylistHandler::comment()
    {
        return EngineController::instance()->bundle().comment();
    }

    QString DcopPlaylistHandler::bitrate()
    {
        return EngineController::instance()->bundle().prettyBitrate();
    }

    int DcopPlaylistHandler::sampleRate()
    {
        return EngineController::instance()->bundle().sampleRate();
    }

    QString DcopPlaylistHandler::encodedURL()
    {
        return EngineController::instance()->bundle().url().url();
    }

    QString DcopPlaylistHandler::coverImage()
    {
        const MetaBundle &bundle = EngineController::instance()->bundle();
        QString image = CollectionDB::instance()->albumImage( bundle, 0 );
        return image;
    }

    void DcopPlaylistHandler::togglePlaylist()
    {
        PlaylistWindow::self()->showHide();
    }

    int DcopPlaylistHandler::score()
    {
        const MetaBundle &bundle = EngineController::instance()->bundle();
        int score = CollectionDB::instance()->getSongPercentage( bundle.url().path() );
        return score;
    }

    void DcopPlaylistHandler::playMedia( const KURL &url )
    {
        ContextBrowser* m_contextBrowser=(ContextBrowser*) (PlaylistWindow::self()->browserBar()->browser( "ContextBrowser" ) );
        m_contextBrowser->openURLRequest( url );
    }

    void DcopPlaylistHandler::shortStatusMessage(const QString& msg)
    {
        StatusBar::instance()->shortMessage( msg );
    }

} //namespace amaroK

#include "amarokdcophandler.moc"

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

// #include "amarokconfig.h"
#include "amarokdcophandler.h"
#include "engine/enginebase.h"
#include "enginecontroller.h"
#include "playlist.h"
#include "osd.h"

#include <dcopclient.h>

#include <kapplication.h> //kapp pointer
#include <kdebug.h> // for kdWarning()

namespace amaroK
{

    DcopHandler::DcopHandler()
        : DCOPObject( "player" )
    {
        // Register with DCOP
        if ( !kapp->dcopClient() ->isRegistered() ) {
            kapp->dcopClient() ->registerAs( "amarok", false );
            kapp->dcopClient() ->setDefaultObject( objId() );
        }
    }

    void DcopHandler::play()
    {
        EngineController::instance() ->play();
    }

    void DcopHandler::playPause()
    {
        EngineController::instance() ->playPause();
    }

    void DcopHandler::stop()
    {
        EngineController::instance() ->stop();
    }


    void DcopHandler::next()
    {
        EngineController::instance() ->next();
    }


    void DcopHandler::prev()
    {
        EngineController::instance() ->previous();
    }


    void DcopHandler::pause()
    {
        EngineController::instance()->pause();
    }

    bool DcopHandler::isPlaying()
    {
	kdWarning() << k_funcinfo << " is DEPRECATED!" << endl;
        return EngineController::engine()->state() == Engine::Playing;
    }

    int  DcopHandler::status()
    {
	// <0 - error, 0 - stopped, 1 - paused, 2 - playing
	int ret = -1;
	switch( EngineController::engine()->state() )
	{
	    case Engine::Playing:
		ret = 2;
		break;
	    case Engine::Paused:
		ret = 1;
		break;
	    case Engine::Empty:
	    case Engine::Idle:
		ret = 0;
		break;
	}
	return ret;
    }

// Now for the DCOP id3 output stuff

    QString DcopHandler::nowPlaying()
    {
        return EngineController::instance()->bundle().prettyTitle();
    }

// Added own calls for Artist/Album/Title for flexibility reasons
    QString DcopHandler::artist()
    {
        return EngineController::instance()->bundle().artist();
    }

    QString DcopHandler::title()
    {
        return EngineController::instance()->bundle().title();
    }

    QString DcopHandler::album()
    {
        return EngineController::instance()->bundle().album();
    }

// Changed DCOP time output to mm:ss, by using MetaBundle::prettyLength ;)
// prettyLength also adds an "0" when sec < 10

    QString DcopHandler::totalTime()
    {
        return EngineController::instance()->bundle().prettyLength();
    }

    QString DcopHandler::currentTime()
    {
        return MetaBundle::prettyLength( EngineController::engine() ->position() / 1000 );
    }

// Some additional DCOP id3 tag output, very useful e.g. for annoying IRC-scripts ;)

    QString DcopHandler::genre()
    {
        return EngineController::instance()->bundle().genre();
    }

    QString DcopHandler::year()
    {
        return EngineController::instance()->bundle().year();
    }

    QString DcopHandler::comment()
    {
        return EngineController::instance()->bundle().comment();
    }

    QString DcopHandler::bitrate()
    {
        return EngineController::instance()->bundle().prettyBitrate();
    }

// Ok, that should be enough, have fun :-)

    QString DcopHandler::encodedURL()
    {
        return EngineController::instance()->bundle().url().url();
    }

    int DcopHandler::trackTotalTime()
    {
        return EngineController::instance()->bundle().length();
    }

    int DcopHandler::trackCurrentTime()
    {
        return EngineController::engine() ->position() / 1000;
    }

    void DcopHandler::seek(int s)
    {
        EngineBase* const engine = EngineController::engine();
        if ( s > 0 && engine->state() != Engine::Empty )
        {
            engine ->seek( s * 1000 );
        }
    }

    void DcopHandler::addMedia(const KURL &url)
    {
        Playlist::instance()->appendMedia(url);
    }

    void DcopHandler::addMediaList(const KURL::List &urls)
    {
        Playlist::instance()->appendMedia(urls);
    }

    void DcopHandler::setVolume(int volume)
    {
        EngineController::instance()->setVolume(volume);
    }

    int DcopHandler::getVolume()
    {
        return EngineController::engine() ->volume();
    }

    void DcopHandler::volumeUp()
    {
        EngineController::instance()->increaseVolume();
    }

    void DcopHandler::volumeDown()
    {
        EngineController::instance()->decreaseVolume();
    }

    void DcopHandler::mute()
    {
        EngineController::instance()->mute();
    }

    void DcopHandler::enableOSD(bool enable)
    {
        amaroK::OSD::instance()->setEnabled(enable);
        
        //FIXME deactivated for causing compile troubles
//         AmarokConfig::setOsdEnabled(enable);
    }

} //namespace amaroK

#include "amarokdcophandler.moc"

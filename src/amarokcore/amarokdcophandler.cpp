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

#include "amarokconfig.h"
#include "amarokdcophandler.h"
#include "app.h"
#include "engine/enginebase.h"
#include "enginecontroller.h"
#include "playlist.h"
#include "osd.h"

#include <dcopclient.h>

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
        return EngineController::engine()->state() == EngineBase::Playing;
    }

// Now for the DCOP id3 output stuff
   
    QString DcopHandler::nowPlaying()
    {
        return EngineController::instance()->bundle().prettyTitle();
    }

// Added own calls for Artist/Album/Title for flexibility reasons
    QString DcopHandler::Artist()
    {
        return EngineController::instance()->bundle().artist();
    }
    
    QString DcopHandler::Title()
    {
        return EngineController::instance()->bundle().title();
    }
    
    QString DcopHandler::Album()
    {
        return EngineController::instance()->bundle().album();
    }
    
// Changed DCOP time output to mm:ss, by using MetaBundle::prettyLength ;)
// prettyLength also adds an "0" when sec < 10
    
    QString DcopHandler::TotalTime()
    {
        return MetaBundle::prettyLength( EngineController::instance()->bundle().length() );
    }

    QString DcopHandler::CurrentTime()
    {
        return MetaBundle::prettyLength( EngineController::engine() ->position() / 1000 );
    }

// Some additional DCOP id3 tag output, very useful e.g. for annoying IRC-scripts ;)

    QString DcopHandler::Genre()
    {
        return EngineController::instance()->bundle().genre();
    }

    QString DcopHandler::Year()
    {
        return EngineController::instance()->bundle().year();
    }
        
    QString DcopHandler::Comment()
    {
        return EngineController::instance()->bundle().comment();
    }

    QString DcopHandler::Bitrate()
    {
        return EngineController::instance()->bundle().prettyBitrate();
    }

// Ok, that should be enough, have fun :-)
    
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
        if ( s > 0 && engine->state() != EngineBase::Empty )
        {
            engine ->seek( s * 1000 );
        }
    }

    void DcopHandler::addMedia(const KURL &url)
    {
        pApp->playlist()->insertMedia(url);
    }

    void DcopHandler::addMediaList(const KURL::List &urls)
    {
        pApp->playlist()->insertMedia(urls);
    }

    void DcopHandler::setVolume(int volume)
    {
        EngineController::instance()->setVolume(volume);
    }

    void DcopHandler::volumeUp()
    {
        EngineController::instance()->increaseVolume();
    }

    void DcopHandler::volumeDown()
    {
        EngineController::instance()->decreaseVolume();
    }

    void DcopHandler::enableOSD(bool enable)
    {
        pApp->osd()->setEnabled(enable);
        AmarokConfig::setOsdEnabled(enable);
    }

} //namespace amaroK

#include "amarokdcophandler.moc"

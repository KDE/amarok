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

    DcopHandler::DcopHandler( QObject *parent )
        : DCOPObject( parent )
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

    QString DcopHandler::nowPlaying()
    {
        return EngineController::instance()->bundle().prettyTitle();
    }

    bool DcopHandler::isPlaying()
    {
        return EngineController::engine()->state() == EngineBase::Playing;
    }

    int DcopHandler::trackTotalTime()
    {
        return EngineController::instance()->bundle().length();
    }

    void DcopHandler::seek(int s)
    {
        EngineBase* const engine = EngineController::engine();
        if ( s > 0 && engine->state() != EngineBase::Empty )
        {
            engine ->seek( s * 1000 );
        }
    }

    int DcopHandler::trackCurrentTime()
    {
        //return time in seconds
        return EngineController::engine() ->position() / 1000;
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

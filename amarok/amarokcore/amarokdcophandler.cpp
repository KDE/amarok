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
#include "engine/enginebase.h"
#include "playerapp.h"
#include "enginecontroller.h"

#include <dcopclient.h>

AmarokDcopHandler::AmarokDcopHandler()
   : DCOPObject( "player" )
   , m_nowPlaying( "" )
{
    // Register with DCOP
    if ( !kapp->dcopClient()->isRegistered() )
    {
        kapp->dcopClient()->registerAs( "amarok", false );
        kapp->dcopClient()->setDefaultObject( objId() );
    }
}

void AmarokDcopHandler::play()
{
    EngineController::instance()->play();
}

void AmarokDcopHandler::playPause()
{
    if (isPlaying())
   pause();
    else
   play();
}

void AmarokDcopHandler::stop()
{
    EngineController::instance()->stop();
}


void AmarokDcopHandler::next()
{
    EngineController::instance()->next();
}


void AmarokDcopHandler::prev()
{
    EngineController::instance()->previous();
}


void AmarokDcopHandler::pause()
{
    EngineController::instance()->pause();
}

void AmarokDcopHandler::setNowPlaying( const QString &s )
{
   m_nowPlaying = s;
}

QString AmarokDcopHandler::nowPlaying()
{
    return m_nowPlaying;
}

bool AmarokDcopHandler::isPlaying()
{
    return EngineController::instance()->engine() ? EngineController::instance()->engine()->loaded() : false;
}

int AmarokDcopHandler::trackTotalTime()
{
    return EngineController::instance()->trackLength();
}

void AmarokDcopHandler::seek(int s)
{
    EngineBase *engine = EngineController::instance()->engine();
    if ( (s > 0) && ( engine->state() != EngineBase::Empty ) )
    {
        engine->seek( s * 1000 );
    }
}

int AmarokDcopHandler::trackCurrentTime()
{
   //return time in seconds
   return EngineController::instance()->engine()->position() / 1000;
}

void AmarokDcopHandler::addMedia(const KURL &url)
{
   pApp->insertMedia(url);
}

void AmarokDcopHandler::addMediaList(const KURL::List &urls)
{
   pApp->insertMedia(urls);
}

void AmarokDcopHandler::setVolume(int volume)
{
    EngineController::instance()->setVolume( volume );
}

void AmarokDcopHandler::volumeUp()
{
    pApp->slotIncreaseVolume(); // show OSD
}

void AmarokDcopHandler::volumeDown()
{
    pApp->slotDecreaseVolume(); // show OSD
}

void AmarokDcopHandler::enableOSD(bool enable)
{
   pApp->setOsdEnabled(enable);
}

#include "amarokdcophandler.moc"

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

namespace amaroK
{

DcopHandler::DcopHandler()
   : DCOPObject( "player" )
   , m_nowPlaying( QString::null )
{
   // Register with DCOP
   if ( !kapp->dcopClient()->isRegistered() )
   {
      kapp->dcopClient()->registerAs( "amarok", false );
      kapp->dcopClient()->setDefaultObject( objId() );
   }
}

void DcopHandler::play()
{
   EngineController::instance()->play();
}

void DcopHandler::playPause()
{
   if (isPlaying())  pause();
   else              play();
}

void DcopHandler::stop()
{
   EngineController::instance()->stop();
}


void DcopHandler::next()
{
   EngineController::instance()->next();
}


void DcopHandler::prev()
{
   EngineController::instance()->previous();
}


void DcopHandler::pause()
{
   EngineController::instance()->pause();
}

void DcopHandler::setNowPlaying( const QString &s )
{
   m_nowPlaying = s;
}

QString DcopHandler::nowPlaying()
{
   return m_nowPlaying;
}

bool DcopHandler::isPlaying()
{
   return EngineController::engine() ? EngineController::engine()->loaded() : false;
}

int DcopHandler::trackTotalTime()
{
   return EngineController::instance()->trackLength() / 1000;
}

void DcopHandler::seek(int s)
{
   EngineBase *engine = EngineController::instance()->engine();
   if ( (s > 0) && ( engine->state() != EngineBase::Empty ) )
   {
      engine->seek( s * 1000 );
   }
}

int DcopHandler::trackCurrentTime()
{
   //return time in seconds
   return EngineController::engine()->position() / 1000;
}

void DcopHandler::addMedia(const KURL &url)
{
   pApp->insertMedia(url);
}

void DcopHandler::addMediaList(const KURL::List &urls)
{
   pApp->insertMedia(urls);
}

void DcopHandler::setVolume(int volume)
{
   EngineController::instance()->setVolume( volume );
}

void DcopHandler::volumeUp()
{
   pApp->slotIncreaseVolume(); // show OSD
}

void DcopHandler::volumeDown()
{
   pApp->slotDecreaseVolume(); // show OSD
}

void DcopHandler::enableOSD(bool enable)
{
   pApp->setOsdEnabled(enable);
}

}

#include "amarokdcophandler.moc"

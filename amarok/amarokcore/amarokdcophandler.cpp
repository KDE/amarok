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

#include <dcopclient.h>

AmarokDcopHandler::AmarokDcopHandler()
   : DCOPObject( "player" )
   , m_nowPlaying( "" )
{
    // Register with DCOP
    if ( !kapp->dcopClient()->isRegistered() )
    {
        kapp->dcopClient()->registerAs( "amarok" );
        kapp->dcopClient()->setDefaultObject( objId() );
    }
}

void AmarokDcopHandler::play()
{
    pApp->slotPlay();
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
    pApp->slotStop();
}


void AmarokDcopHandler::next()
{
    pApp->slotNext();
}


void AmarokDcopHandler::prev()
{
    pApp->slotPrev();
}


void AmarokDcopHandler::pause()
{
    pApp->slotPause();
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
    return pApp->isPlaying();
}

int AmarokDcopHandler::trackTotalTime()
{
   return pApp->trackLength();
}

void AmarokDcopHandler::seek(int s)
{
   if ( (s > 0) && ( pApp->m_pEngine->state() != EngineBase::Empty ) )
   {
      pApp->m_pEngine->seek( s * 1000 );
   }
}

int AmarokDcopHandler::trackCurrentTime()
{
   //return time in seconds
   return pApp->m_pEngine->position() / 1000;
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
    pApp->slotVolumeChanged(volume);
}

void AmarokDcopHandler::volumeUp()
{
    pApp->slotVolumeChanged( pApp->m_pEngine->volume() + 100 / 25 );
}

void AmarokDcopHandler::volumeDown()
{
    pApp->slotVolumeChanged( pApp->m_pEngine->volume() - 100 / 25 );
}

void AmarokDcopHandler::enableOSD(bool enable)
{
   pApp->setOsdEnabled(enable);
}

#include "amarokdcophandler.moc"

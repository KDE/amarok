/***************************************************************************
 * copyright            : (C) 2007 Shane King <kde@dontletsstart.com>      *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "AudioController.h"
#include "core/Radio.h"
#include "enginecontroller.h"

#include <KLocale>


AudioController::AudioController( QObject *parent ) 
    : QObject( parent )
{
}


AudioController::~AudioController() 
{
}


void 
AudioController::setVolume( int vol ) 
{
    EngineController::instance()->setVolume( vol );
}


void 
AudioController::play() 
{
    loadNext();
}


void 
AudioController::play( RadioPlaylist &playlist ) 
{
    m_playlist = &playlist;
    loadNext();
}


void 
AudioController::play( const QUrl &trackUrl ) 
{
    TrackInfo ti;
    ti.setPath( trackUrl.toString() );
    play( ti );
}


void 
AudioController::play( const TrackInfo &track ) 
{
    m_playlist = 0;
    playTrack( track );
}


void 
AudioController::stop() 
{
    EngineController::instance()->stop();
}


void 
AudioController::loadNext() 
{
    if ( m_playlist == 0 )
    {
        stop();
    }
    else if ( m_playlist->hasMore() )
    {
        TrackInfo track = m_playlist->nextTrack();
        playTrack( track );
    }
    else
    {
        emit error( Radio_OutOfPlaylist, i18n( "Radio playlist has ended." ) );

        stop();
    }
}


void
AudioController::playTrack( const TrackInfo &track )
{
}

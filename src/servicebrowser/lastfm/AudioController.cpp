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

// TODO: implement
AudioController::AudioController( QObject *parent ) {}
AudioController::~AudioController() {}
void AudioController::setVolume( int vol ) {}
void AudioController::play() {}
void AudioController::play( RadioPlaylist& playlist ) {}
void AudioController::play( const QUrl& trackUrl ) {}
void AudioController::play( const TrackInfo& track ) {}
void AudioController::stop() {}
void AudioController::loadNext() {}

namespace The
{
    Radio &radio()
    {
        static Radio radio( 0 );
        return radio;
    }
}

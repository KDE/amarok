/***************************************************************************
 *   Copyright (C) 2007 Shane King <kde@dontletsstart.com>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "phonon-directshow"

#include "DirectShowAudioOutput.h"


DirectShowAudioOutput::DirectShowAudioOutput(QObject *parent)
    : QObject( parent ), 
      m_device( 0 ),
      m_volume( 0 )
{
}


DirectShowAudioOutput::~DirectShowAudioOutput()
{
}


qreal 
DirectShowAudioOutput::volume() const
{
    return m_volume;
}


void 
DirectShowAudioOutput::setVolume(qreal volume)
{
    m_volume = volume;
    emit volumeChanged(m_volume);
}


int 
DirectShowAudioOutput::outputDevice() const
{
    return m_device;
}


bool 
DirectShowAudioOutput::setOutputDevice( int device )
{
    m_device = device;
    emit outputDeviceChanged( device );
    return true;
}

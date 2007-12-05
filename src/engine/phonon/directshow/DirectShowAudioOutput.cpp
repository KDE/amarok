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
    : QObject(parent)
{
}


DirectShowAudioOutput::~DirectShowAudioOutput()
{
}


qreal 
DirectShowAudioOutput::volume() const
{
    return 0;
}


void 
DirectShowAudioOutput::setVolume(qreal)
{
}


int 
DirectShowAudioOutput::outputDevice() const
{
    return 0;
}


bool 
DirectShowAudioOutput::setOutputDevice(int)
{
    return false;
}

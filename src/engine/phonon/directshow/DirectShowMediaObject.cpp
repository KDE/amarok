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

#include "DirectShowMediaObject.h"

#include <kpluginfactory.h>


DirectShowMediaObject::DirectShowMediaObject(QObject *parent)
    : QObject(parent)
{
}


DirectShowMediaObject::~DirectShowMediaObject()
{
}


void 
DirectShowMediaObject::play()
{
}


void 
DirectShowMediaObject::pause()
{
}


void
DirectShowMediaObject::stop()
{
}


void 
DirectShowMediaObject::seek(qint64 milliseconds)
{
}


qint32 
DirectShowMediaObject::tickInterval() const
{
    return 0;
}


void 
DirectShowMediaObject::setTickInterval(qint32 interval)
{
}

bool 
DirectShowMediaObject::hasVideo() const
{
    return false;
}


bool
DirectShowMediaObject::isSeekable() const
{
    return false;
}


qint64 
DirectShowMediaObject::currentTime() const
{
    return 0;
}


Phonon::State 
DirectShowMediaObject::state() const
{
    return Phonon::StoppedState;
}


qint64 
DirectShowMediaObject::totalTime() const
{
    return 0;
}


QString 
DirectShowMediaObject::errorString() const
{
    return "";
}


Phonon::ErrorType 
DirectShowMediaObject::errorType() const
{
    return Phonon::NoError;
}


Phonon::MediaSource 
DirectShowMediaObject::source() const
{
    return Phonon::MediaSource();
}


void 
DirectShowMediaObject::setSource(const Phonon::MediaSource &)
{
}


void 
DirectShowMediaObject::setNextSource(const Phonon::MediaSource &source)
{
}

qint32 
DirectShowMediaObject::prefinishMark() const
{
    return 0;
}


void 
DirectShowMediaObject::setPrefinishMark(qint32)
{
}


qint32 
DirectShowMediaObject::transitionTime() const
{
    return 0;
}


void 
DirectShowMediaObject::setTransitionTime(qint32)
{
}

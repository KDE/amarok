/***************************************************************************
 *   Copyright (C) 2007 Shane King <kde@dontletsstart.com>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_DIRECTSHOWAUDIOOUTPUT_H
#define AMAROK_DIRECTSHOWAUDIOOUTPUT_H

#include <phonon/audiooutputinterface.h>

class DirectShowAudioOutput : public QObject, public Phonon::AudioOutputInterface
{
    Q_OBJECT
    Q_INTERFACES(Phonon::AudioOutputInterface)

    public:
        DirectShowAudioOutput(QObject *parent);
        ~DirectShowAudioOutput();

        qreal volume() const;
        void setVolume(qreal);

        int outputDevice() const;
        bool setOutputDevice(int);
};

#endif // AMAROK_DIRECTSHOWAUDIOOUTPUT_H

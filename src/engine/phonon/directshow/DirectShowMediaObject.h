/***************************************************************************
 *   Copyright (C) 2007 Shane King <kde@dontletsstart.com>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_DIRECTSHOWMEDIAOBJECT_H
#define AMAROK_DIRECTSHOWMEDIAOBJECT_H

#include <phonon/mediaobjectinterface.h>

class DirectShowMediaObject : public QObject, public Phonon::MediaObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(Phonon::MediaObjectInterface)

    public:
        DirectShowMediaObject(QObject *parent);
        ~DirectShowMediaObject();

        void play();
        void pause();
        void stop();
        void seek(qint64 milliseconds);

        qint32 tickInterval() const;
        void setTickInterval(qint32 interval);

        bool hasVideo() const;
        bool isSeekable() const;

        qint64 currentTime() const;
        Phonon::State state() const;
        qint64 totalTime() const;

        QString errorString() const;
        Phonon::ErrorType errorType() const;

        Phonon::MediaSource source() const;
        void setSource(const Phonon::MediaSource &);
        void setNextSource(const Phonon::MediaSource &source);

        qint32 prefinishMark() const;
        void setPrefinishMark(qint32);

        qint32 transitionTime() const;
        void setTransitionTime(qint32);
};

#endif // AMAROK_DIRECTSHOWMEDIAOBJECT_H

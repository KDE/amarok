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

class DirectShowGraph;

// Phonon MediaObject implementation.
// Mostly just forwards everything on to its associated graph object.
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

        void setGraph( DirectShowGraph *graph );
        DirectShowGraph *getGraph() { return m_graph; }

    signals:
        void aboutToFinish();
        void finished();
        void prefinishMarkReached(qint32 msec);
        void totalTimeChanged(qint64 length);
        void currentSourceChanged(const Phonon::MediaSource &);

        void stateChanged(Phonon::State newstate, Phonon::State oldstate);
        void tick(qint64 time);
        void metaDataChanged(const QMultiMap<QString, QString> &);
        void seekableChanged(bool);
        void hasVideoChanged(bool);
        void bufferStatus(int);

    private:
        DirectShowGraph *m_graph;
};

#endif // AMAROK_DIRECTSHOWMEDIAOBJECT_H

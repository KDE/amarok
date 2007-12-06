/***************************************************************************
 *   Copyright (C) 2007 Shane King <kde@dontletsstart.com>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_DIRECTSHOWGRAPH_H
#define AMAROK_DIRECTSHOWGRAPH_H

#include "ComPtr.h"

#include <phonon/mediasource.h>

#include <QObject>

#include <dshow.h>

class DirectShowBackend;
class DirectShowMediaObject;
class DirectShowAudioOutput;

// Implements a single DirectShow graph.
// Most of the real work is done here.
class DirectShowGraph : public QObject
{
    Q_OBJECT

    public:
        DirectShowGraph( DirectShowBackend *backend, DirectShowMediaObject *mediaObject, DirectShowAudioOutput *audioOutput );
        ~DirectShowGraph();

        bool initialized() const { return m_initialized; }

        bool onEvent();

        void stop();
        void play();
        void pause();
        void seek( qint64 milliseconds );

        bool isSeekable() const;

        qint64 currentTime() const;
        qint64 totalTime() const;

        Phonon::State state() const;

        QString errorString() const;
        Phonon::ErrorType errorType() const;

        Phonon::MediaSource source() const;
        void setSource( const Phonon::MediaSource &source );

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

    private slots:
        void volumeChanged( qreal volume );
        void outputDeviceChanged( int device );

    private:      
        void error( Phonon::ErrorType type, const QString &string );
        void setState( Phonon::State state );
        void newSource();

        bool createGraph();
        void render();

        bool m_initialized;

        DirectShowBackend     *m_backend;
        DirectShowMediaObject *m_mediaObject;
        DirectShowAudioOutput *m_audioOutput;

        Phonon::MediaSource m_source;
        Phonon::State       m_state;
        Phonon::ErrorType   m_errorType;
        QString             m_errorString;

        mutable ComPtr<IGraphBuilder> m_graph;
        mutable ComPtr<IMediaControl> m_control;
        mutable ComPtr<IMediaEventEx> m_event;
        mutable ComPtr<IMediaSeeking> m_seek;
        mutable ComPtr<IBasicAudio>   m_audio;
        mutable ComPtr<IBaseFilter>   m_input;
        mutable ComPtr<IBaseFilter>   m_output;
};

#endif // AMAROK_DIRECTSHOWGRAPH_H

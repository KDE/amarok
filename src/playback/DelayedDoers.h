/****************************************************************************************
 * Copyright (c) 2013 Matěj Laitl <matej@laitl.cz>                                      *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 * **************************************************************************************/

#ifndef DELAYEDDOERS_H
#define DELAYEDDOERS_H

#include "amarok_export.h"

#include <Phonon/Global>

#include <QObject>
#include <QSet>

namespace Phonon {
    class MediaObject;
    class MediaController;
}

/**
 * Abstract base helper class for helpers that do something with Phonon objects once
 * MediaObject transitions to appropriate state. Then it auto-deletes itself.
 */
class AMAROK_EXPORT DelayedDoer : public QObject
{
    Q_OBJECT

    public:
        explicit DelayedDoer( Phonon::MediaObject *mediaObject,
                              const QSet<Phonon::State> &applicableStates );

    protected:
        /**
         * This method gets called when MediaObject transitions to the right state.
         */
        virtual void performAction() = 0;

    private Q_SLOTS:
        void slotStateChanged( Phonon::State newState );

    protected:
        Phonon::MediaObject *m_mediaObject;

    private:
        QSet<Phonon::State> m_applicableStates;
};

/**
 * Helper class that calls seek() followed by play() on Phonon::MediaObject after it
 * emits stateChanged() with suitable new state and then auto-destructs itself.
 */
class AMAROK_EXPORT DelayedSeeker : public DelayedDoer
{
    Q_OBJECT

    public:
        DelayedSeeker( Phonon::MediaObject *mediaObject, qint64 seekTo, bool startPaused );

    Q_SIGNALS:
        void trackPositionChanged( qint64 position, bool userSeek );

    protected:
        virtual void performAction();

        qint64 m_seekTo;
        bool m_startPaused;
};

class AMAROK_EXPORT DelayedTrackChanger : public DelayedSeeker
{
    Q_OBJECT

    public:
        explicit DelayedTrackChanger( Phonon::MediaObject *mediaObject,
                                      Phonon::MediaController *mediaController,
                                      int trackNumber, qint64 seekTo, bool startPaused );

    protected:
        virtual void performAction();

    private:
        Phonon::MediaController *m_mediaController;
        int m_trackNumber;
};

#endif // DELAYEDDOERS_H

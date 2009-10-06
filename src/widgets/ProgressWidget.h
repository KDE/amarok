/****************************************************************************************
 * Copyright (c) 2007 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
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
 ****************************************************************************************/

#ifndef AMAROK_PROGRESSWIDGET_H
#define AMAROK_PROGRESSWIDGET_H

#include "EngineObserver.h"

#include <unistd.h>

#include <QHash>
#include <QPainter>
#include <QPolygon>
#include <QWidget>

class TimeLabel;
namespace Amarok { class TimeSlider; }

class ProgressWidget : public QWidget, public EngineObserver
{
    Q_OBJECT
    public:
        ProgressWidget( QWidget* );
        ~ProgressWidget();

        virtual QSize sizeHint() const;

        void addBookmark( const QString &name, int milliSeconds );
        void redrawBookmarks();

    public slots:
        void drawTimeDisplay( int position );

    protected:
        virtual void engineTrackPositionChanged( qint64 position, bool /*userSeek*/ );
        virtual void engineStateChanged( Phonon::State state, Phonon::State oldState = Phonon::StoppedState );
        virtual void engineTrackLengthChanged( qint64 milliseconds );
        virtual void engineNewTrackPlaying();
        virtual void enginePlaybackEnded( qint64 finalPosition, qint64 trackLength, PlaybackEndedReason reason );

    private:
        TimeLabel *m_timeLabelLeft;
        TimeLabel *m_timeLabelRight;
        int m_timeLength;
        Amarok::TimeSlider *m_slider;
        QString m_currentUrlId;
};

#endif


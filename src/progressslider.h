/***************************************************************************
 * copyright     : (C) 2007 Dan Meltzer <hydrogen@notyetimplemented.com>   *
 **************************************************************************/

 /***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PROGRESSWIDGET_H
#define PROGRESSWIDGET_H

#include "engineobserver.h"
#include "sliderwidget.h"

#include <QWidget>

class QLabel;

class TimeSlider : public QWidget, public EngineObserver
{
    Q_OBJECT

    AMAROK_EXPORT static TimeSlider *s_instance;

    public:
        TimeSlider( QWidget* );
        static TimeSlider *instance() { return s_instance; }

    public slots:
        void drawTimeDisplay( int position );

    protected:
        virtual void engineTrackPositionChanged( long position, bool /*userSeek*/ );
        virtual void engineStateChanged( Engine::State state, Engine::State oldState = Engine::Empty );
        virtual void engineTrackLengthChanged( long length );
        virtual void engineNewMetaData( const MetaBundle &bundle, bool trackChanged );

    private:
        QLabel *m_timeLabel;
        QLabel *m_timeLabel2;
        QWidget *m_positionBox;
        int m_timeLength;
        Amarok::PrettySlider *m_slider;
};
#endif

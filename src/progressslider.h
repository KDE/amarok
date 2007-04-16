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

#include <kpassivepopup.h>

#include <unistd.h>
#include <QHash>
#include <QPainter>
#include <QPolygon>

class QLabel;

class Polygon : public QPolygon
{
    public:
        Polygon( int size, int seconds ) : QPolygon( size ),
                 m_seconds( seconds)
        {}
        QString time() { return prettyTime( m_seconds ); }
        inline int seconds() { return m_seconds; }
        void setTime( int seconds ) { m_seconds = seconds; }

    private:
        QString prettyTime( int seconds )
        {
            int hours = 0;
            int minutes = 0;
            while( seconds >= 3600 )
            {
                hours++;
                seconds -= 3600;
            }
            while( seconds >= 60 )
            {
                minutes++;
                seconds -= 60;
            }
            QString sHours, sMinutes, sSeconds;
            sHours = sMinutes = sSeconds = QString();
            if( hours > 0 )
                sHours = QString("%1 Hours").arg( hours );
            if( minutes > 0)
                sMinutes = QString("%1 Minutes").arg( minutes );
            if( seconds > 0 )
                sSeconds = QString("%1 Seconds").arg( seconds );
            return QString( "%1\n%2\n%3" ).arg( sHours, sMinutes, sSeconds );
        }
        int m_seconds;
};

class ProgressSlider : public Amarok::PrettySlider
{
    Q_OBJECT
    static ProgressSlider *s_instance;

    public:
        ProgressSlider( QWidget *parent );
        AMAROK_EXPORT static ProgressSlider *instance() { return s_instance; }
        void addBookmark( uint second );
        void addBookmarks( QList<uint> seconds );
        AMAROK_EXPORT QList<uint> bookmarks() { return m_bookmarks; }

    protected:
        virtual void paintEvent( QPaintEvent *e );
        virtual void mouseMoveEvent( QMouseEvent *e );
        virtual void mousePressEvent( QMouseEvent *e );

    private:
        QList<uint> m_bookmarks;
        QList<Polygon> m_polygons;
        QPoint oldpoint;
        KPassivePopup *m_popup;
};

class ProgressWidget : public QWidget, public EngineObserver
{
    Q_OBJECT

    AMAROK_EXPORT static ProgressWidget *s_instance;

    public:
        ProgressWidget( QWidget* );
        static ProgressWidget *instance() { return s_instance; }

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
        int m_timeLength;
        ProgressSlider *m_slider;
};
#endif

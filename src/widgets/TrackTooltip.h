/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  tracktooltip.h  -  Provides an interface to a plain QWidget, which is independent of KDE (bypassed to X11)
  begin:     Tue 10 Feb 2004
  copyright: (C) 2004 by Christian Muehlhaeuser
  email:     chris@chris.de
*/

#ifndef TRACKTOOLTIP_H
#define TRACKTOOLTIP_H

#include "meta/Meta.h"

#include <QWidget>
#include <QSystemTrayIcon>

class QLabel;
class QWidget;

// class TrackToolTip: public QObject, public Amarok::ToolTipClient, public Meta::Observer
class TrackToolTip : public QWidget, public Meta::Observer
{
    Q_OBJECT

    public:
        TrackToolTip();
        ~TrackToolTip();
        static TrackToolTip* instance();
    
        void setTrack( const Meta::TrackPtr track, bool force = false );
        void setTrackPosition( int pos );
        void clear();
        void show( const QPoint &bottomRight );
    
        //Reimplemented from Meta::Observer
        virtual void metadataChanged( Meta::Track *track );
        virtual void metadataChanged( Meta::Album *album );
        virtual void metadataChanged( Meta::Artist *artist );
    
        virtual void metadataChanged( Meta::Genre * ) {}; //prevent compiler warning
        virtual void metadataChanged( Meta::Composer * ) {}; //prevent compiler warning
        virtual void metadataChanged( Meta::Year * ) {}; //prevent compiler warning

    protected:
        bool eventFilter( QObject* obj, QEvent* event );
        virtual void mousePressEvent( QMouseEvent* );

    private:
        QString tooltip() const;
        void updateWidgets();
    
        static TrackToolTip *s_instance;

        QList<QWidget> m_widgets;
        Meta::TrackPtr m_track;
        int            m_trackPosition;
        QString        m_tooltip;
        bool           m_haspos;
        QString        m_moodbarURL;
        QString        m_title;
    
        QLabel *m_imageLabel;
        QPixmap m_image;
        QLabel *m_titleLabel;
        QLabel *m_otherInfoLabel;
};

#endif


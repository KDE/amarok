/******************************************************************************
 * Copyright (C) 2004 Christian Muehlhaeuser <chris@chris.de>                 *
 * Copyright (C) 2005 Gábor Lehel <illissius@gmail.com>                       *
 * Copyright (C) 2008 Mark Kretschmann <kretschmann@kde.org>                  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

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
        TrackToolTip( QWidget* parent );
        ~TrackToolTip();
        static TrackToolTip* instance();
    
        void setTrack( const Meta::TrackPtr track );
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

    public slots:
       void hide(); 

    protected:
        bool eventFilter( QObject* obj, QEvent* event );
        virtual void mousePressEvent( QMouseEvent* );

    private slots:
       void slotTimer();

    private:
        QString tooltip() const;
        void updateWidgets();
    
        static TrackToolTip *s_instance;

        QList<QWidget> m_widgets;
        Meta::TrackPtr m_track;
        int            m_trackPosition;
        QString        m_tooltip;
        bool           m_haspos;
        QString        m_title;
        QTimer*        m_timer;
            
        QLabel *m_imageLabel;
        QPixmap m_image;
        QLabel *m_titleLabel;
        QLabel *m_otherInfoLabel;
};

#endif


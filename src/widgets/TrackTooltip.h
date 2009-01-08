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

#include "EngineObserver.h"
#include "meta/Meta.h"

#include <QWidget>
#include <QSystemTrayIcon>

class QLabel;
class QWidget;

class TrackToolTip : public QWidget, public Meta::Observer, public EngineObserver
{
    Q_OBJECT

    public:
        TrackToolTip( QWidget* parent );
        ~TrackToolTip();
        static TrackToolTip* instance();
    
        void show( const QPoint &bottomRight );

        //Reimplemented from Meta::Observer
        using Observer::metadataChanged;
        void metadataChanged( Meta::TrackPtr track );
        void metadataChanged( Meta::AlbumPtr album );
    
    public slots:
       void hide(); 


    protected:
        //Reimplemented from EngineObserver
        virtual void engineNewTrackPlaying();
        virtual void enginePlaybackEnded( int finalPosition, int trackLength, const QString &reason );
        virtual void engineTrackPositionChanged( long position, bool userSeek );
        virtual void engineVolumeChanged( int percent );

        bool eventFilter( QObject* obj, QEvent* event );
        virtual void mousePressEvent( QMouseEvent* );

    private slots:
       void slotTimer();

    private:
        void setTrack();
        void clear();
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


/***************************************************************************
 *   Copyright (C) 2005 by Gábor Lehel <illissius@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#ifndef AMAROK_QUEUELABEL_H
#define AMAROK_QUEUELABEL_H

#include "playlistitem.h"
#include "popupMessage.h"

#include <qlabel.h>
#include <qtimer.h>

class QueueLabel: public QLabel //homonym, heh heh
{
    Q_OBJECT

    public:
        QueueLabel( QWidget *parent, const char *name = 0 );

    signals:
        void queueChanged( const PLItemList &, const PLItemList & );

    public slots:
        virtual void update();
        virtual void setNum( int num );
        void enterEvent( QEvent * );
        void leaveEvent( QEvent * );

    private slots:
        void aboutToShow();
        void slotCoverChanged( const QString &artist, const QString &album );

    protected:
        virtual void mousePressEvent( QMouseEvent* e );

    private:
        void    showToolTip();
        void    hideToolTip();
        QString veryNiceTitle( PlaylistItem * item, bool bold = false ) const;
        void    getCover( const QString &artist, const QString &album );

        inline int mapXToGlobal( int x ) { return mapToGlobal( QPoint( x, 0 ) ).x(); }
        inline int mapYToGlobal( int y ) { return mapToGlobal( QPoint( 0, y ) ).y(); }

        QTimer  m_timer;

        QString m_cover;

        KDE::PopupMessage *m_tooltip;
        bool               m_tooltipShowing;
        bool               m_tooltipHidden;
};

#endif

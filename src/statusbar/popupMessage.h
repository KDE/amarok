/***************************************************************************
 *   Copyright (C) 2005 by Max Howell <max.howell@methylblue.com>          *
 *                 2005 by Seb Ruiz <me@sebruiz.net>                       *
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


//WARNING this is not meant for use outside this unit!


#ifndef KDE_POPUPMESSAGE_H
#define KDE_POPUPMESSAGE_H

#include "overlayWidget.h"

#include <qbitmap.h>
#include <qlayout.h>
#include <qpixmap.h>

namespace KDE
{
    /**
     * @class PopupMessage
     * @short Widget that animates itself into a position relative to an anchor widget
     */
    class PopupMessage : public OverlayWidget
    {
        Q_OBJECT

    public:
        /**
         * @param anchor  : which widget to tie the popup widget to.
         * @param timeout : how long to wait before auto closing. A value of 0 means close
         *                  only on pressing the closeButton or close() is called.
         */
        PopupMessage( QWidget *parent, QWidget *anchor, int timeout = 5000 /*milliseconds*/, const char* name = 0 );

        enum MaskEffect { Plain, Slide, Dissolve };

        void addWidget( QWidget *widget );
        void setShowCloseButton( const bool show );
        void setShowCounter( const bool show );
        void setImage( const QString &location );
        void setImage( const QPixmap &pix );
        void setMaskEffect( const MaskEffect type ) { m_maskEffect = type; }
        void setText( const QString &text );
        void setTimeout( const int time ) { m_timeout = time; }

    public slots:
        void close();
        void display();

    protected:
        void timerEvent( QTimerEvent* );
        void countDown();

        /**
        * @short Gradually show widget by dissolving from background
        */
        void dissolveMask();

        /**
        * @short instantly display widget
        */
        void plainMask();

        /**
        * @short animation to slide the widget into view
        */
        void slideMask();

    private:
        QVBoxLayout *m_layout;
        QFrame      *m_countdownFrame;
        QWidget     *m_anchor;
        QWidget     *m_parent;
        QBitmap      m_mask;
        MaskEffect   m_maskEffect;

        int      m_dissolveSize;
        int      m_dissolveDelta;

        int      m_offset;
        int      m_counter;
        int      m_stage;
        int      m_timeout;
        int      m_timerId;

        bool     m_showCounter;
    };
}

#endif

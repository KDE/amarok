/***************************************************************************
 *   Copyright (C) 2005 Max Howell <max.howell@methylblue.com>             *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef AMAROK_STATUSBAR_H
#define AMAROK_STATUSBAR_H

#include "engineobserver.h" //baseclass
#include "statusBarBase.h"  //baseclass

class QTimer;

namespace amaroK
{
    class Slider;

    class StatusBar : public KDE::StatusBar, public EngineObserver
    {
        Q_OBJECT

        static StatusBar* s_instance;

    public:
        StatusBar( QWidget *parent, const char *name = 0 );

        static StatusBar* instance() { return s_instance; }

    public slots:
        /** update total song count */
        void slotItemCountChanged( int newCount, int newLength, int, int ); //TODO improve

    protected:  /* reimpl from engineobserver */
        virtual void engineStateChanged( Engine::State state );
        virtual void engineTrackPositionChanged( long position );
        virtual void engineNewMetaData( const MetaBundle &bundle, bool trackChanged );

    private slots:
        void slotPauseTimer();
        void drawTimeDisplay( int position );

    private:
        QLabel *m_timeLabel;
        QLabel *m_itemCountLabel;
        amaroK::Slider *m_slider;
        QTimer *m_pauseTimer;
    };
} //namespace amaroK

#endif

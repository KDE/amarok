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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef AMAROK_STATUSBAR_H
#define AMAROK_STATUSBAR_H

#include "engineobserver.h" //baseclass
#include "statusBarBase.h"  //baseclass
#include "queueLabel.h"

#include <qvaluestack.h>

class QTimer;

namespace Amarok
{
    class Slider;
    class PrettySlider;

    class StatusBar : public KDE::StatusBar, public EngineObserver
    {
        Q_OBJECT

        LIBAMAROK_EXPORT static StatusBar* s_instance;

    public:
        StatusBar( QWidget *parent, const char *name = 0 );

        static StatusBar* instance() { return s_instance; }

        PrettySlider *slider() { return m_slider; }

    public slots:
        /** update total song count */
        void slotItemCountChanged( int newCount, int newLength, int, int, int, int ); //TODO improve
        void updateQueueLabel() { m_queueLabel->update(); }
        void drawTimeDisplay( int position );
    protected:  /* reimpl from engineobserver */
        virtual void engineStateChanged( Engine::State state, Engine::State oldState = Engine::Empty );
        virtual void engineTrackPositionChanged( long position, bool /*userSeek*/ );
        virtual void engineTrackLengthChanged( long length );
        virtual void engineNewMetaData( const MetaBundle &bundle, bool trackChanged );

    private slots:
        void slotPauseTimer();

    private:
        QLabel *m_timeLabel;
        QLabel *m_timeLabel2;
        int m_timeLength;
        QLabel *m_itemCountLabel;
        QueueLabel *m_queueLabel;
        Amarok::PrettySlider *m_slider;
        QTimer *m_pauseTimer;
    };
    /**
     * Is used to queue up longMessages for the StatusBar before the StatusBar
     * is created.
     */
    class MessageQueue
    {
    public:
        static MessageQueue* instance();
        void addMessage ( const QString & );
        void sendMessages();
    private:
        MessageQueue();
        QValueStack<QString> m_messages;
        bool m_queueMessages;
    };

} //namespace Amarok

namespace The
{
    inline Amarok::StatusBar *statusbar() { return Amarok::StatusBar::instance(); }
}

#endif

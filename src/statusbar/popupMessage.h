/***************************************************************************
 *   Copyright (C) 2005 by Max Howell <max.howell@methylblue.com>          *
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


//WARNING this is not meant for use outside this unit!


#ifndef KDE_POPUPMESSAGE_H
#define KDE_POPUPMESSAGE_H

#include <kpushbutton.h>
#include "overlayWidget.h"

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
        PopupMessage( StatusBar *statusbar, QWidget *anchor, const char *name = 0 )
                : OverlayWidget( statusbar, anchor )
                , m_statusBar( statusbar )
                , m_offset( 0 )
        {
            setPalette( QToolTip::palette() );
            setFrameStyle( QFrame::Panel | QFrame::Sunken );

            QVBoxLayout *vbox;
            QHBoxLayout *hbox;
            QLabel *label;

            vbox  = new QVBoxLayout( this, 9 /*margin*/, 6 /*spacing*/ );

            hbox  = new QHBoxLayout( vbox, 12 );

            label = new QLabel( this );
            label->setPixmap( QMessageBox::standardIcon( QMessageBox::Information ) );
            hbox->add( label );

            label = new QLabel( this, "label" );
            label->setTextFormat( Qt::RichText );
            label->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
            hbox->add( label );

            hbox = new QHBoxLayout( vbox );
            hbox->addItem( new QSpacerItem( 4, 4, QSizePolicy::Expanding, QSizePolicy::Preferred ) );
            hbox->add( new KPushButton( KStdGuiItem::close(), this, "closeButton" ) );

            connect( child( "closeButton" ), SIGNAL(clicked()), SLOT(animate()) );
        }

        void setText( const QString &text )
        {
            static_cast<QLabel*>(child( "label" ))->setText( text );
            adjustSize();
        }

    public slots:
        void animate()
        {
            m_timer.disconnect( this );

            if( isHidden() ) {
                show();
                connect( &m_timer, SIGNAL(timeout()), SLOT(showing()) );
                m_timer.start( 10 );
            }
            else {
                m_offset = height();
                connect( &m_timer, SIGNAL(timeout()), SLOT(hiding()) );
                m_timer.start( 5 );
            }
        }

    private slots:
        void showing()
        {
            m_offset +=2;

            move( 0, m_statusBar->y() - m_offset );

            if( m_offset >= height() )
                m_timer.stop();
        }

        void hiding()
        {
            m_offset -= 2;

            move( 0, m_statusBar->y() - m_offset );

            if( m_offset <= 0 )
                delete this;
        }

    private:
        QWidget *m_statusBar;
        QTimer   m_timer;
        int      m_offset;
    };
}

#endif

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

#include "popupMessage.h"

#include <kactivelabel.h>
#include <kpushbutton.h>
#include <kstdguiitem.h>

#include <qfont.h>
#include <qframe.h>
#include <qlabel.h>
#include <qmessagebox.h>
#include <qpainter.h>
#include <qtimer.h>

namespace KDE
{

PopupMessage::PopupMessage( QWidget *parent, QWidget *anchor, int timeout )
                : OverlayWidget( parent, anchor )
                , m_anchor( anchor )
                , m_parent( parent )
                , m_offset( 0 )
                , m_counter( 0 )
                , m_stage( 1 )
                , m_timeout( timeout )
                , m_showCounter( true )
{
    setFrameStyle( QFrame::Panel | QFrame::Raised );
    setFrameShape( QFrame::StyledPanel );

    QHBoxLayout *hbox;
    QLabel *label;
    KActiveLabel *alabel;

    m_layout = new QVBoxLayout( this, 9 /*margin*/, 6 /*spacing*/ );

    hbox = new QHBoxLayout( m_layout, 12 );

    hbox->addWidget( m_countdownFrame = new QFrame( this, "counterVisual" ) );
    m_countdownFrame->setFixedWidth( fontMetrics().width( "X" ) );
    m_countdownFrame->setFrameStyle( QFrame::Plain | QFrame::Box );
    m_countdownFrame->setPaletteForegroundColor( paletteBackgroundColor().dark() );

    label = new QLabel( this, "image" );
    hbox->add( label );

    alabel = new KActiveLabel( this, "label" );
    alabel->setTextFormat( Qt::RichText );
    alabel->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Preferred );

    hbox->add( alabel );

    hbox = new QHBoxLayout( m_layout );

    hbox->addItem( new QSpacerItem( 4, 4, QSizePolicy::Expanding, QSizePolicy::Preferred ) );
    hbox->add( new KPushButton( KStdGuiItem::close(), this, "closeButton" ) );

    connect( child( "closeButton" ), SIGNAL(clicked()), SLOT(close()) );

    m_timerId = startTimer( 6 );
}

void PopupMessage::addWidget( QWidget *widget )
{
    m_layout->add( widget );
}

void PopupMessage::showCloseButton( const bool show )
{
    static_cast<KPushButton*>(child( "closeButton" ))->setShown( show );
}

void PopupMessage::showCounter( const bool show )
{
    m_showCounter = show;
    static_cast<QFrame*>(child( "counterVisual" ))->setShown( show );
}

void PopupMessage::setText( const QString &text )
{
    static_cast<KActiveLabel*>(child( "label" ))->setText( text );
    adjustSize();
}

void PopupMessage::setImage( const QString &location )
{
    static_cast<QLabel*>(child( "image" ))->setPixmap( QPixmap( location ) );
}


void PopupMessage::close() //SLOT
{
    m_stage = 3;
    killTimer( m_timerId );
    m_timerId = startTimer( 6 );
}

void PopupMessage::timerEvent( QTimerEvent* )
{
    QFrame *&h = m_countdownFrame;

    switch( m_stage )
    {
        case 1: //raise
            move( 0, m_parent->y() - m_offset );

            m_offset++;
            if( m_offset > height() )
            {
                killTimer( m_timerId );

                if( m_timeout )
                {
                    m_timerId = startTimer( 40 );
                    m_stage = 2;
                }
            }

            break;

        case 2: //fill in pause timer bar
            if( m_counter < h->height() - 3 )
                QPainter( h ).fillRect( 2, 2, h->width() - 4, m_counter, palette().active().highlight() );

            if( !hasMouse() )
                m_counter++;

            if( m_counter > h->height() )
            {
                m_stage = 3;
                killTimer( m_timerId );
                m_timerId = startTimer( 6 );
            }
            else
            {
                killTimer( m_timerId );
                m_timerId = startTimer( m_timeout / h->height() );
            }

            break;

        case 3: //lower
            m_offset--;
            move( 0, m_parent->y() - m_offset);

            if( m_offset < 0 )
                deleteLater();
    }
}


}

#include "popupMessage.moc"

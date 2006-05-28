/***************************************************************************
 *   Copyright (C) 2005 by Max Howell <max.howell@methylblue.com>          *
 *                 2005 by Seb Ruiz <me@sebruiz.net>                       *
 *                                                                         *
 *   Dissolve Mask (c) Kicker Authors kickertip.cpp, 2005/08/17            *
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
#include "debug.h"

#include <kactivelabel.h>
#include <kpushbutton.h>
#include <kstdguiitem.h>

#include <qfont.h>
#include <qframe.h>
#include <qlabel.h>
#include <qmessagebox.h>
#include <qpainter.h>
#include <qtimer.h>
#include <qtooltip.h>

namespace KDE
{

PopupMessage::PopupMessage( QWidget *parent, QWidget *anchor, int timeout, const char *name )
                : OverlayWidget( parent, anchor, name )
                , m_anchor( anchor )
                , m_parent( parent )
                , m_maskEffect( Slide )
                , m_dissolveSize( 0 )
                , m_dissolveDelta( -1 )
                , m_offset( 0 )
                , m_counter( 0 )
                , m_stage( 1 )
                , m_timeout( timeout )
                , m_showCounter( true )
{
    setFrameStyle( QFrame::Panel | QFrame::Raised );
    setFrameShape( QFrame::StyledPanel );
    setWFlags( Qt::WX11BypassWM );

    QPalette p = QToolTip::palette();
    setPalette( p );

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
    alabel->setPalette( p );

    hbox->add( alabel );

    hbox = new QHBoxLayout( m_layout );

    hbox->addItem( new QSpacerItem( 4, 4, QSizePolicy::Expanding, QSizePolicy::Preferred ) );
    hbox->add( new KPushButton( KStdGuiItem::close(), this, "closeButton" ) );

    connect( child( "closeButton" ), SIGNAL(clicked()), SLOT(close()) );
}

void PopupMessage::addWidget( QWidget *widget )
{
    m_layout->add( widget );
    adjustSize();
}

void PopupMessage::setShowCloseButton( const bool show )
{
    static_cast<KPushButton*>(child( "closeButton" ))->setShown( show );
    adjustSize();
}

void PopupMessage::setShowCounter( const bool show )
{
    m_showCounter = show;
    static_cast<QFrame*>(child( "counterVisual" ))->setShown( show );
    adjustSize();
}

void PopupMessage::setText( const QString &text )
{
    static_cast<KActiveLabel*>(child( "label" ))->setText( text );
    adjustSize();
}

void PopupMessage::setImage( const QString &location )
{
    static_cast<QLabel*>(child( "image" ))->setPixmap( QPixmap( location ) );
    adjustSize();
}

void PopupMessage::setImage( const QPixmap &pix )
{
    static_cast<QLabel*>(child( "image" ))->setPixmap( pix );
    adjustSize();
}


////////////////////////////////////////////////////////////////////////
//     Public Slots
////////////////////////////////////////////////////////////////////////

void PopupMessage::close() //SLOT
{
    m_stage = 3;
    killTimer( m_timerId );
    m_timerId = startTimer( 6 );
}

void PopupMessage::display() //SLOT
{
    m_dissolveSize = 24;
    m_dissolveDelta = -1;

    if( m_maskEffect == Dissolve )
    {
        // necessary to create the mask
        m_mask.resize( width(), height() );
        // make the mask empty and hence will not show widget with show() called below
        dissolveMask();
        m_timerId = startTimer( 1000 / 30 );
    }
    else
    {
        m_timerId = startTimer( 6 );
    }
    show();
}

////////////////////////////////////////////////////////////////////////
//     Protected
////////////////////////////////////////////////////////////////////////

void PopupMessage::timerEvent( QTimerEvent* )
{
    switch( m_maskEffect )
    {
        case Plain:
            plainMask();
            break;

        case Slide:
            slideMask();
            break;

        case Dissolve:
            dissolveMask();
            break;
    }
}

void PopupMessage::countDown()
{
    if( !m_timeout )
    {
        killTimer( m_timerId );
        return;
    }

    QFrame *&h = m_countdownFrame;

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
}

void PopupMessage::dissolveMask()
{
    if( m_stage == 1 )
    {
        repaint( false );
        QPainter maskPainter(&m_mask);

        m_mask.fill(Qt::black);

        maskPainter.setBrush(Qt::white);
        maskPainter.setPen(Qt::white);
        maskPainter.drawRect( m_mask.rect() );

        m_dissolveSize += m_dissolveDelta;

        if( m_dissolveSize > 0 )
        {
            maskPainter.setRasterOp( Qt::EraseROP );

            int x, y, s;
            const int size = 16;

            for (y = 0; y < height() + size; y += size)
            {
                x = width();
                s = m_dissolveSize * x / 128;

                for ( ; x > size; x -= size, s -= 2 )
                {
                    if (s < 0)
                        break;

                    maskPainter.drawEllipse(x - s / 2, y - s / 2, s, s);
                }
            }
        }
        else if( m_dissolveSize < 0 )
        {
            m_dissolveDelta = 1;
            killTimer( m_timerId );

            if( m_timeout )
            {
                m_timerId = startTimer( 40 );
                m_stage = 2;
            }
        }

        setMask(m_mask);
    }
    else if ( m_stage == 2 )
    {
        countDown();
    }
    else
    {
        deleteLater();
    }
}


void PopupMessage::plainMask()
{
    switch( m_stage )
    {
        case 1: // Raise
            killTimer( m_timerId );
            if( m_timeout )
            {
                m_timerId = startTimer( 40 );
                m_stage = 2;
            }

            break;

        case 2: // Counter
            countDown();
            break;

        case 3: // Lower/Remove
            deleteLater();
    }
}


void PopupMessage::slideMask()
{
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
            countDown();
            break;

        case 3: //lower
            m_offset--;
            move( 0, m_parent->y() - m_offset );

            if( m_offset < 0 )
                deleteLater();
    }
}

}

#include "popupMessage.moc"

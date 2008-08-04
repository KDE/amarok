/***************************************************************************
 *   Copyright (C) 2005 by Max Howell <max.howell@methylblue.com>          *
 *                 2005 by Seb Ruiz <ruiz@kde.org>                         *
 *                 2008 by Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>  *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "popupMessage.h"
#include "Debug.h"

#include <kpushbutton.h>
#include <kstandardguiitem.h>

#include <qfont.h>
#include <QFrame>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QTimer>
#include <QToolTip>
#include <kguiitem.h>
//Added by qt3to4:
#include <QTimerEvent>
#include <QPixmap>

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
    setFrameStyle( QFrame::Panel | QFrame::Sunken );
    setFrameShape( QFrame::StyledPanel );
    setWindowFlags( Qt::X11BypassWindowManagerHint | Qt::Tool | Qt::FramelessWindowHint );

    setMinimumWidth( 26 );
    setMinimumHeight( 26 );
    setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );

    QPalette p = QToolTip::palette();
    setPalette( p );

    KHBox *hbox;
//     QLabel *label;
    QLabel *alabel;

    m_layout = new QVBoxLayout( this );
    //m_layout->setMargin( 9 );
    //m_layout->setSpacing( 6 );
    

    hbox = new KHBox( this );
    //hbox->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );
    //hbox->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_layout->addWidget( hbox );
    
    hbox->setSpacing( 12 );
    
    m_countdownFrame = new CountdownFrame( hbox );
    m_countdownFrame->setObjectName( "counterVisual" );
    //hbox->addWidget( m_countdownFrame );
    m_countdownFrame->setFixedWidth( fontMetrics().width( "X" ) );
    m_countdownFrame->setFrameStyle( QFrame::Plain | QFrame::Box );
    QPalette pal;
    pal.setColor( m_countdownFrame->foregroundRole(), p.dark().color() );
    m_countdownFrame->setPalette( pal );

/*  label = new QLabel( this );
    label->setObjectName( "image" );
    hbox->addWidget( label );
*/
    //QLabel *alabel;
    alabel = new QLabel( "hello, world", hbox );
    //alabel->setBackgroundRole( QPalette::Highlight );
    //alabel->setParent( this );
    alabel->setWordWrap( true );
    alabel->setObjectName( "label" );
    alabel->setTextFormat( Qt::RichText );
    alabel->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Preferred );
    alabel->setPalette( p );

    //m_layout->addWidget( alabel );


    hbox = new KHBox( this );
    m_layout->addWidget( hbox );

    //hbox->addItem( new QSpacerItem( hbox, 4, 4, QSizePolicy::Expanding, QSizePolicy::Preferred ) );
    KPushButton *button = new KPushButton( KStandardGuiItem::close(), hbox );
    button->setObjectName( "closeButton" );
    connect( button, SIGNAL(clicked()), SLOT(close()) );
    
}

void PopupMessage::addWidget( QWidget *widget )
{
    m_layout->addWidget( widget );
    adjustSize();
}

void PopupMessage::setShowCloseButton( const bool show )
{
    findChild<KPushButton*>( "closeButton" )->setVisible( show );
    adjustSize();
}

void PopupMessage::setShowCounter( const bool show )
{
    m_showCounter = show;
    findChild<QFrame*>( "counterVisual" )->setVisible( show );
    adjustSize();
}

void PopupMessage::setText( const QString &text )
{
    findChild<QLabel*>( "label" )->setText( text );
    findChild<QLabel*>( "label" )->adjustSize();
    adjustSize();
}

void PopupMessage::setImage( const QString &location )
{
    findChild<QLabel*>( "image" )->setPixmap( QPixmap( location ) );
    adjustSize();
}

void PopupMessage::setImage( const QPixmap &pix )
{
    findChild<QLabel*>( "image" )->setPixmap( pix );
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
    setGeometry ( 0, 0, 0, 0 );
    adjustSize();
    reposition();
    m_dissolveSize = 24;
    m_dissolveDelta = -1;

    if( m_maskEffect == Dissolve )
    {
        // create the mask
        m_mask = QPixmap( width(), height() );
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

    CountdownFrame *&h = m_countdownFrame;

    if( m_counter < h->height() - 3 )
    {
        h->setFilledRatio( (float) m_counter / ( float ) h->height() );
        h->repaint();
    }

    if( !testAttribute(Qt::WA_UnderMouse))
        m_counter++;

    if( m_counter > h->height() )
    {
        m_stage = 3;
        killTimer( m_timerId );

        h->setFilledRatio( 1 );
        h->repaint();
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
        repaint();
        QPainter maskPainter(&m_mask);

        m_mask.fill(Qt::black);

        maskPainter.setBrush(Qt::white);
        maskPainter.setPen(Qt::white);
        maskPainter.drawRect( m_mask.rect() );

        m_dissolveSize += m_dissolveDelta;

        if( m_dissolveSize > 0 )
        {
            maskPainter.setCompositionMode( QPainter::CompositionMode_SourceOut );

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
            emit( deleted() );
            deleteLater();
    }
}


void PopupMessage::slideMask()
{

    int anchorY = m_anchor->mapToGlobal( m_anchor->pos() ).y();
    // This is no longer used, saving it in case we need again..
//     int ourY = m_anchor->mapToGlobal( pos() ).y();
    
    switch( m_stage )
    {
        case 1: //raise
            move( 0, anchorY - m_offset );

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
            move( 0, anchorY - m_offset );

            if( m_offset < 0 )
                deleteLater();
    }

    //now, make sure the part that is not supposed to be visible is hidden:

    QPainter maskPainter(&m_mask);
    //m_mask.fill(Qt::black);
    m_mask.fill(Qt::white);
    /*maskPainter.setBrush(Qt::white);
    maskPainter.setPen(Qt::white);

    maskPainter.setCompositionMode( QPainter::CompositionMode_SourceOut );


    QRectF visibleRect( m_mask.rect().x(), m_mask.rect().y(), m_mask.rect().width(), anchorY - ourY );
    
    maskPainter.drawRect( visibleRect );
*/
    
    setMask(m_mask);

    


}

}

KDE::CountdownFrame::CountdownFrame(QWidget * parent)
    : QFrame( parent )
{
}

void KDE::CountdownFrame::setFilledRatio(float filled)
{
    m_filled = filled;
}

void KDE::CountdownFrame::paintEvent( QPaintEvent * e )
{
    QFrame::paintEvent( e );

    QPalette p = palette();
    p.setCurrentColorGroup( QPalette::Active );
    QPainter( this ).fillRect( 2, (int)m_filled * height(), width() - 4, static_cast<int>(height() - m_filled * height()), p.highlight() );
}

#include "popupMessage.moc"

/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2005 Max Howell <max.howell@methylblue.com>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#include "LongMessageWidget.h"

#include "Debug.h"

#include <KPushButton>

#include <QLabel>
#include <QLayout>
#include <QPainter>
#include <QToolTip>


LongMessageWidget::LongMessageWidget( QWidget * anchor, const QString & message, StatusBar::MessageType type )
        : PopupWidget( anchor )
        , m_counter( 0 )
        , m_timeout( 6000 )
{
    DEBUG_BLOCK
    Q_UNUSED( type )

    setFrameStyle( QFrame::Panel | QFrame::Sunken );
    setFrameShape( QFrame::StyledPanel );

    setContentsMargins( 4, 4, 4, 4 );

    setMinimumWidth( 26 );
    setMinimumHeight( 26 );
    setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );

    QPalette p = QToolTip::palette();
    setPalette( p );

    KHBox *hbox;
    QLabel *alabel;

    hbox = new KHBox( this );
    layout()->addWidget( hbox );

    hbox->setSpacing( 12 );

    m_countdownFrame = new CountdownFrame( hbox );
    m_countdownFrame->setObjectName( "counterVisual" );
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
    alabel = new QLabel( message, hbox );
    //alabel->setBackgroundRole( QPalette::Highlight );
    //alabel->setParent( this );
    alabel->setWordWrap( true );
    alabel->setObjectName( "label" );
    alabel->setTextFormat( Qt::RichText );
    alabel->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Preferred );
    alabel->setPalette( p );

    //m_layout->addWidget( alabel );

    hbox = new KHBox( this );
    layout()->addWidget( hbox );

    //hbox->addItem( new QSpacerItem( hbox, 4, 4, QSizePolicy::Expanding, QSizePolicy::Preferred ) );
    KPushButton *button = new KPushButton( KStandardGuiItem::close(), hbox );
    button->setObjectName( "closeButton" );
    connect( button, SIGNAL( clicked() ), SLOT( close() ) );

    reposition();

    show();
    m_timerId = startTimer( m_timeout / m_countdownFrame->height() );
}

LongMessageWidget::~LongMessageWidget()
{}

void LongMessageWidget::close()
{
    hide();
    emit( closed() );
}

void LongMessageWidget::timerEvent( QTimerEvent* )
{
    if ( !m_timeout )
    {
        killTimer( m_timerId );
        return;
    }

    CountdownFrame *&h = m_countdownFrame;

    if ( m_counter < h->height() - 3 )
    {
        h->setFilledRatio(( float ) m_counter / ( float ) h->height() );
        h->repaint();
    }

    if ( !testAttribute( Qt::WA_UnderMouse ) )
        m_counter++;

    if ( m_counter > h->height() )
    {
        killTimer( m_timerId );
        h->setFilledRatio( 1 );
        h->repaint();
        close();
    }
    else
    {
        killTimer( m_timerId );
        m_timerId = startTimer( m_timeout / h->height() );
    }
}


//////////////////////////////////////////////////////////////////////////////
// class CountdownFrame 
//////////////////////////////////////////////////////////////////////////////

CountdownFrame::CountdownFrame( QWidget * parent )
        : QFrame( parent )
        , m_filled(0.0)
{}

void CountdownFrame::setFilledRatio( float filled )
{
    m_filled = filled;
}

void CountdownFrame::paintEvent( QPaintEvent * e )
{
    QFrame::paintEvent( e );

    QPalette p = palette();
    p.setCurrentColorGroup( QPalette::Active );
    //QPainter( this ).fillRect( 2, (int)m_filled * height(), width() - 4, static_cast<int>(height() - m_filled * height()), p.highlight() );

    QPainter( this ).fillRect( 2, m_filled * height() , width() - 4, height() - ( m_filled * height() ) , p.highlight() );
}


#include "LongMessageWidget.moc"


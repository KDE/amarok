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

#include "core/support/Debug.h"

#include <KStandardGuiItem>

#include <QPushButton>

#include <QLabel>
#include <QLayout>
#include <QPainter>
#include <QToolTip>


LongMessageWidget::LongMessageWidget( const QString &message )
        : m_counter( 0 )
        , m_timeout( 6000 )
{
    DEBUG_BLOCK

    setFrameStyle( QFrame::StyledPanel | QFrame::Raised );

    setContentsMargins( 4, 4, 4, 4 );

    setMinimumWidth( 26 );
    setMinimumHeight( 26 );
    setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );

    QPalette p = QToolTip::palette();
    setPalette( p );

    BoxWidget *hbox = new BoxWidget( false, this );
    hbox->layout()->setSpacing( 12 );

    m_countdownFrame = new CountdownFrame( hbox );
    m_countdownFrame->setObjectName( QStringLiteral("counterVisual") );
    m_countdownFrame->setFixedWidth( fontMetrics().horizontalAdvance( QStringLiteral("X") ) );
    m_countdownFrame->setFrameStyle( QFrame::Plain | QFrame::Box );
    QPalette pal;
    pal.setColor( m_countdownFrame->foregroundRole(), p.dark().color() );
    m_countdownFrame->setPalette( pal );

    QLabel *alabel = new QLabel( message, hbox );
    alabel->setWordWrap( true );
    alabel->setOpenExternalLinks( true );
    alabel->setObjectName( QStringLiteral("label") );
    alabel->setTextFormat( Qt::RichText );
    alabel->setTextInteractionFlags( Qt::TextBrowserInteraction );
    alabel->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Preferred );
    alabel->setPalette( p );

    hbox = new BoxWidget( false, this );

    QPushButton *button = new QPushButton( hbox );
    KStandardGuiItem::assign( button, KStandardGuiItem::Close );
    connect( button, &QAbstractButton::clicked, this, &LongMessageWidget::close );

    reposition();

    show();
    m_timerId = startTimer( m_timeout / m_countdownFrame->height() );
}

LongMessageWidget::~LongMessageWidget()
{}

void LongMessageWidget::close()
{
    hide();
    Q_EMIT( closed() );
}

void LongMessageWidget::timerEvent( QTimerEvent* )
{
    if( !m_timeout )
    {
        killTimer( m_timerId );
        return;
    }

    CountdownFrame *&h = m_countdownFrame;

    if( m_counter < h->height() - 3 )
    {
        h->setFilledRatio(( float ) m_counter / ( float ) h->height() );
        h->repaint();
    }

    if( !testAttribute( Qt::WA_UnderMouse ) )
        m_counter++;

    if( m_counter > h->height() )
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

CountdownFrame::CountdownFrame( QWidget *parent )
        : QFrame( parent )
        , m_filled( 0.0 )
{}

void CountdownFrame::setFilledRatio( float filled )
{
    m_filled = filled;
}

void CountdownFrame::paintEvent( QPaintEvent *e )
{
    QFrame::paintEvent( e );

    QPalette p = palette();
    p.setCurrentColorGroup( QPalette::Active );

    QPainter( this ).fillRect( 2, m_filled * height() , width() - 4, height()
                               - ( m_filled * height() ) , p.highlight()
                             );
}


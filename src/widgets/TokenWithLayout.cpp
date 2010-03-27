/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Roman Jarosz <kedgedev@gmail.com>                                 *
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

#include "TokenWithLayout.h"

#include "core/support/Debug.h"
#include "TokenDropTarget.h"
#include "playlist/layouts/LayoutEditDialog.h"

#include <KColorScheme>
#include <KIcon>
#include <KLocale>

#include <QContextMenuEvent>
#include <QLayout>
#include <QPainter>
#include <QPushButton>
#include <QTimerEvent>

Wrench::Wrench( QWidget *parent ) : QLabel( parent )
{
    setCursor( Qt::ArrowCursor );
    setPixmap( KIcon( "configure" ).pixmap( 64 ) );
    setScaledContents( true );
    setMargin( 4 );
}

void Wrench::enterEvent( QEvent * )
{
    setMargin( 1 );
    update();
}

void Wrench::leaveEvent( QEvent * )
{
    setMargin( 4 );
    update();
}

void Wrench::mousePressEvent( QMouseEvent * )
{
    setMargin( 4 );
    update();
    emit clicked();
}

void Wrench::mouseReleaseEvent( QMouseEvent * )
{
    setMargin( 1 );
    update();
    emit clicked();
}

void Wrench::paintEvent( QPaintEvent *pe )
{
    QPainter p( this );
    QColor c = palette().color( backgroundRole() );
    p.setPen( Qt::NoPen );
    c = palette().color( backgroundRole() );
    c.setAlpha( 212 );
    p.setBrush( c );
    p.setRenderHint( QPainter::Antialiasing );
    p.drawEllipse( rect() );
    p.end();
    QLabel::paintEvent( pe );
}


const QString ActionBoldName = QLatin1String( "ActionBold" );
const QString ActionItalicName = QLatin1String( "ActionItalic" );
const QString ActionAlignLeftName = QLatin1String( "ActionAlignLeft" );
const QString ActionAlignCenterName = QLatin1String( "ActionAlignCenter" );
const QString ActionAlignRightName = QLatin1String( "ActionAlignRight" );

Token * TokenWithLayoutFactory::createToken( const QString &text, const QString &iconName, int value, QWidget *parent )
{
    return new TokenWithLayout( text, iconName, value, parent );
}

QPointer<LayoutEditDialog> TokenWithLayout::m_dialog = 0L;

TokenWithLayout::TokenWithLayout( const QString &text, const QString &iconName, int value, QWidget *parent )
    : Token( text, iconName, value, parent  )
    , m_width( 0.0 ), m_wrenchTimer( 0 )
{
    m_widthForced = m_width > 0.0;
    m_alignment = Qt::AlignCenter;
    m_bold = false;
    m_italic = false;
    m_underline = false;
    m_wrench = new Wrench( this );
    m_wrench->installEventFilter( this );
    m_wrench->hide();
    connect ( m_wrench, SIGNAL( clicked() ), this, SLOT( showConfig() ) );
    setFocusPolicy( Qt::ClickFocus );
}


TokenWithLayout::~TokenWithLayout()
{
    delete m_wrench;
}

void TokenWithLayout::enterEvent( QEvent *e )
{
    QWidget *win = window();
    const int sz = 2*height();
    QPoint pt = mapTo( win, rect().topLeft() );

    m_wrench->setParent( win );
    m_wrench->setFixedSize( sz, sz );
    m_wrench->move( pt - QPoint( m_wrench->width()/3, m_wrench->height()/3 ) );
    m_wrench->setCursor( Qt::PointingHandCursor );
    m_wrench->raise();
    m_wrench->show();

    Token::enterEvent( e );
}

bool TokenWithLayout::eventFilter( QObject *o, QEvent *e )
{
    if ( e->type() == QEvent::Leave && o == m_wrench )
    {
        if ( m_wrenchTimer )
            killTimer( m_wrenchTimer );
        m_wrenchTimer = startTimer( 40 );
    }
    return false;
}

void TokenWithLayout::leaveEvent( QEvent *e )
{
    Token::leaveEvent( e );
    if ( m_wrenchTimer )
        killTimer( m_wrenchTimer );
    m_wrenchTimer = startTimer( 40 );
}

void TokenWithLayout::showConfig()
{
    if ( !m_dialog )
        m_dialog = new LayoutEditDialog( window() );
    m_dialog->setToken( this );
    if ( !m_dialog->isVisible() )
    {
        m_dialog->adjustSize();
        QPoint pt = mapToGlobal( rect().bottomLeft() );
        pt.setY( pt.y() + 9 );
        if ( parentWidget() )
            pt.setX( parentWidget()->mapToGlobal( QPoint( 0, 0 ) ).x() + ( parentWidget()->width() - m_dialog->QDialog::width() ) / 2 );
        m_dialog->move( pt );
    }
    m_dialog->show(); // ensures raise in doubt
    QTimerEvent te( m_wrenchTimer );
    timerEvent( &te ); // it's not like we'd get a leave event when the child dialog pops in between...
}

void TokenWithLayout::timerEvent( QTimerEvent *te )
{
    if ( te->timerId() == m_wrenchTimer )
    {
        killTimer( m_wrenchTimer );
        m_wrenchTimer = 0;

        QRegion rgn;
        rgn |= QRect( mapToGlobal( QPoint( 0, 0 ) ), QWidget::size() );
        rgn |= QRect( m_wrench->mapToGlobal( QPoint( 0, 0 ) ), m_wrench->size() );
        if ( !rgn.contains( QCursor::pos() ) )
            m_wrench->hide();
    }
    Token::timerEvent( te );
}

Qt::Alignment TokenWithLayout::alignment()
{
    return m_alignment;
}

void TokenWithLayout::setAlignment( Qt::Alignment alignment )
{
    if ( m_alignment == alignment )
        return;

    m_alignment = alignment;
    m_label->setAlignment( alignment );
    emit changed();
}

void TokenWithLayout::setAlignLeft( bool b )
{
    if (b)
        setAlignment( Qt::AlignLeft );
}

void TokenWithLayout::setAlignCenter( bool b )
{
    if (b)
        setAlignment( Qt::AlignCenter );
}

void TokenWithLayout::setAlignRight( bool b )
{
    if (b)
        setAlignment( Qt::AlignRight );
}

bool TokenWithLayout::bold() const
{
    return m_bold;
}

void TokenWithLayout::setBold( bool bold )
{
    if ( m_bold == bold )
        return;

    m_bold = bold;
    QFont font = m_label->font();
    font.setBold( bold );
    m_label->setFont( font );
    emit changed();
}

void TokenWithLayout::setPrefix( const QString& string )
{
    if ( m_prefix == string )
        return;
    if ( string == i18n( "[prefix]" ) )
        m_prefix.clear();
    else
        m_prefix = string;
    emit changed();
}

void TokenWithLayout::setSuffix( const QString& string )
{
    if ( m_suffix == string )
        return;
    if ( string == i18n( "[suffix]" ) )
        m_suffix.clear();
    else
        m_suffix = string;
    emit changed();
}

void TokenWithLayout::setWidth( int size )
{
    m_width = qMax( qMin( 1.0, size/100.0 ), 0.0 ) ;
    if ( m_width > 0.0 )
        m_widthForced = true;

    emit changed();
}

void TokenWithLayout::setWidthForced( bool on )
{
    m_widthForced = on;
}

qreal TokenWithLayout::width() const
{
    return m_width;
}

bool TokenWithLayout::italic() const
{
    return m_italic;
}

bool TokenWithLayout::underline() const
{
    return m_underline;
}

void TokenWithLayout::setItalic( bool italic )
{
    if ( m_italic == italic )
        return;

    m_italic = italic;
    QFont font = m_label->font();
    font.setItalic( italic );
    m_label->setFont( font );

    emit changed();
}

void TokenWithLayout::setUnderline( bool underline )
{
    if( m_underline == underline )
        return;

    m_underline = underline;
    QFont font  = m_label->font();
    font.setUnderline( underline );
    m_label->setFont( font );

    emit changed();
}


#include "TokenWithLayout.moc"




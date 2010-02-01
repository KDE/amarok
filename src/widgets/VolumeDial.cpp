/****************************************************************************************
* Copyright (c) 2009 Thomas Luebking <thomas.luebking@web.de>                          *
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

#include "VolumeDial.h"

#include <QCoreApplication>
// #include <QPaintEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QToolBar>
#include <QToolTip>

#include <KLocale>

#include "SvgHandler.h"


VolumeDial::VolumeDial( QWidget *parent ) : QDial( parent )
    , m_muted( false )
{
    toolTipTimer.setSingleShot( true );
    connect ( &toolTipTimer, SIGNAL( timeout() ), this, SLOT( hideToolTip() ) );
    connect ( this, SIGNAL( valueChanged(int) ), SLOT( valueChangedSlot(int) ) );
}


// NOTICE: we intercept wheelEvents for ourself to prevent the tooltip hiding on them,
// see ::wheelEvent()
bool VolumeDial::eventFilter( QObject *o, QEvent *e )
{
    if ( e->type() == QEvent::Wheel && o == this )
    {
        wheelEvent( static_cast<QWheelEvent*>(e) );
        return true;
    }
    return false;
}

void VolumeDial::mousePressEvent( QMouseEvent *me )
{
    if ( me->button() == Qt::LeftButton )
    {
        setCursor( Qt::PointingHandCursor );
        const int dx = width()/4;
        const int dy = height()/4;
        m_isClick = rect().adjusted(dx, dy, -dx, -dy).contains( me->pos() );
    }
//     if ( !m_isClick )
    QDial::mousePressEvent( me );
}

void VolumeDial::mouseReleaseEvent( QMouseEvent *me )
{
    if ( me->button() != Qt::LeftButton )
        return;
    
//     QToolTip::hideText();
    setCursor( Qt::ArrowCursor );
    if ( !m_isClick )
    {
        QDial::mouseReleaseEvent( me );
        return;
    }
    const int dx = width()/4;
    const int dy = height()/4;
    m_isClick = rect().adjusted(dx, dy, -dx, -dy).contains( me->pos() );
    if ( !m_isClick )
    {
        QDial::mouseReleaseEvent( me );
        return;
    }

    m_isClick = false;

    emit muteToggled( !m_muted );
}

static QColor mix( const QColor &c1, const QColor &c2 )
{
    QColor c;
    c.setRgb( ( c1.red() + c2.red() ) / 2, ( c1.green() + c2.green() ) / 2,
              ( c1.blue() + c2.blue() ) / 2, ( c1.alpha() + c2.alpha() ) / 2 );
    return c;
}


void VolumeDial::paintEvent( QPaintEvent * )
{
    QPainter p( this );
    int icon = m_muted ? 0 : 3;
    if (icon && value() < 66)
        icon = value() < 33 ? 1 : 2;
    p.drawPixmap(0,0, m_icon[ icon ]);
    QColor c = mix( palette().color( foregroundRole() ), palette().color( QPalette::Highlight ) );
    c.setAlpha( 160 );
    p.setPen( QPen( c, 3, Qt::SolidLine, Qt::RoundCap ) );
    p.setRenderHint(QPainter::Antialiasing);
    p.drawArc( rect().adjusted(4,4,-4,-4), -110*16, - value()*320*16 / (maximum() - minimum()) );
    p.end();
}

void VolumeDial::resizeEvent( QResizeEvent *re )
{
    if ( width() != height() )
        resize( height(), height() );
    else
        QDial::resizeEvent( re );

    m_icon[0] = The::svgHandler()->renderSvg( "Muted",      width(), height(), "Muted" );
    m_icon[1] = The::svgHandler()->renderSvg( "Volume_low", width(), height(), "Volume_low" );
    m_icon[2] = The::svgHandler()->renderSvg( "Volume_mid", width(), height(), "Volume_mid" );
    m_icon[3] = The::svgHandler()->renderSvg( "Volume",     width(), height(), "Volume" );
    
    update();
}

void VolumeDial::wheelEvent( QWheelEvent *wev )
{
    QDial::wheelEvent( wev );
    wev->accept();
    if ( wev->pos() == QPoint( 0, 0 ) )
        return; // this is probably our synthetic event from the toolbar and there's really
                // no simple way to keep the tooltip alive this way. "simple" as the eventfilter
                // hack - see below
        
    toolTipTimer.start( 1000 );
    showToolTip();
    
    // NOTICE: this is a bit tricky.
    // the ToolTip "QTipLabel" just installed a global eventfilter that intercepts various
    // events and hides itself on them. Therefore every odd wheelevent will close the tip
    // ("works - works not - works - works not - ...")
    // so we post-install our own global eventfilter to handle wheel events meant for us bypassing
    // the ToolTip eventfilter

    // first remove to prevent multiple installations but ensure we're on top of the ToolTip filter
    qApp->removeEventFilter( this );
    // it's ultimately removed in the timer triggered ::hideToolTip() slot
    qApp->installEventFilter( this );
}

void VolumeDial::setMuted( bool mute )
{
    if ( mute == m_muted )
        return;

    if ( mute )
    {
        m_unmutedValue = value();
        setValue( minimum() );
    }
    else
        setValue( m_unmutedValue );
}

void VolumeDial::showToolTip() const
{
    const QPoint pos = mapToGlobal( rect().bottomLeft() - QPoint( 0, 12 ) );
    QToolTip::showText( pos, QString( "Volume: %1 %" ).arg( value() ) );
}

QSize VolumeDial::sizeHint() const
{
    if ( QToolBar *toolBar = qobject_cast<QToolBar*>( parentWidget() ) )
        return toolBar->iconSize();

    return QDial::sizeHint();
}

void VolumeDial::hideToolTip()
{
    QToolTip::hideText();
    // ultimately remove wheelevent hack-a-round (global eventfilters can be expensive)
    qApp->removeEventFilter( this );
}

void VolumeDial::valueChangedSlot( int v )
{
    if ( isSliderDown() )
        showToolTip();

    m_isClick = false;

    if ( m_muted == ( v == minimum() ) )
        return;
    m_muted = ( v == minimum() );

    update();
}

#include "VolumeDial.moc"

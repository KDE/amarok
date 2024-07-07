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

#include "PaletteHandler.h"
#include "SvgHandler.h"

#include <QConicalGradient>
#include <QCoreApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QToolBar>
#include <QToolTip>

#include <KColorUtils>
#include <KLocalizedString>

#include <cmath>

VolumeDial::VolumeDial( QWidget *parent ) : QDial( parent )
    , m_isClick( false )
    , m_isDown( false )
    , m_muted( false )
{
    m_anim.step = 0;
    m_anim.timer = 0;
    setMouseTracking( true );

    connect( this, &VolumeDial::valueChanged, this, &VolumeDial::valueChangedSlot );
    connect( The::paletteHandler(), &PaletteHandler::newPalette, this, &VolumeDial::paletteChanged );
}

void VolumeDial::addWheelProxies(const QList<QWidget *> &proxies )
{
    for ( QWidget *proxy : proxies )
    {
        if ( !m_wheelProxies.contains( proxy ) )
        {
            proxy->installEventFilter( this );
            connect ( proxy, &QWidget::destroyed, this, &VolumeDial::removeWheelProxy );
            m_wheelProxies << proxy;
        }
    }
}

void VolumeDial::paletteChanged( const QPalette &palette )
{
    const QColor &fg = palette.color( foregroundRole() );
    const QColor &hg = palette.color( QPalette::Highlight );
    const qreal contrast = KColorUtils::contrastRatio( hg, palette.color( backgroundRole() ) );
    m_highlightColor = KColorUtils::mix( hg, fg, 1.0 - contrast/3.0 );
    renderIcons();
}


#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void VolumeDial::enterEvent ( QEvent * )
#else
void VolumeDial::enterEvent ( QEnterEvent * )
#endif
{
    startFade();
}

// NOTICE: we intercept wheelEvents for ourself to prevent the tooltip hiding on them,
// see ::wheelEvent()
// this is _NOT_ redundant to the code in MainToolbar.cpp
bool VolumeDial::eventFilter( QObject *o, QEvent *e )
{
    if ( e->type() == QEvent::Wheel && !static_cast<QWheelEvent*>(e)->modifiers() )
    {
        if ( o == this || m_wheelProxies.contains( static_cast<QWidget*>( o ) ) )
        {
            QWheelEvent *wev = static_cast<QWheelEvent*>(e);
            if ( o != this )
            {
                QPoint pos( 0, 0 ); // the event needs to be on us or nothing will happen
                QWheelEvent nwev( pos, mapToGlobal( pos ), wev->pixelDelta(), wev->angleDelta(), wev->buttons(), wev->modifiers(), wev->phase(), wev->inverted(), wev->source() );
                wheelEvent( &nwev );
            }
            else
                wheelEvent( wev );
            return true;
        }
        else // we're not needed globally anymore
            qApp->removeEventFilter( this );
    }
    return false;
}

void VolumeDial::leaveEvent( QEvent * )
{
    startFade();
}

static bool onRing( const QRect &r, const QPoint &p )
{
    const QPoint c = r.center();
    const int dx = p.x() - c.x();
    const int dy = p.y() - c.y();
    return sqrt(dx*dx + dy*dy) > r.width()/4;
}

void VolumeDial::mouseMoveEvent( QMouseEvent *me )
{
    if ( me->buttons() == Qt::NoButton )
        setCursor( onRing( rect(), me->pos() ) ? Qt::PointingHandCursor : Qt::ArrowCursor );
    else if ( m_isClick )
        me->accept();
    else
        QDial::mouseMoveEvent( me );
}

void VolumeDial::mousePressEvent( QMouseEvent *me )
{
    if ( me->button() != Qt::LeftButton )
    {
        QDial::mousePressEvent( me );
        return;
    }

    m_isClick = !onRing( rect(), me->pos() );

    if ( m_isClick )
        update(); // hide the ring
    else
    {
        setCursor( Qt::PointingHandCursor ); // hint dragging
        QDial::mousePressEvent( me ); // this will directly jump to the proper position
    }

    // for value changes caused by mouseevent we'll only let our adjusted value changes be emitted
    // see ::sliderChange()
    m_formerValue = value();
    blockSignals( true );
}

void VolumeDial::mouseReleaseEvent( QMouseEvent *me )
{
    if ( me->button() != Qt::LeftButton )
        return;

    blockSignals( false ); // free signals
    setCursor( Qt::ArrowCursor );
    setSliderDown( false );

    if ( m_isClick )
    {
        m_isClick = !onRing( rect(), me->pos() );
        if ( m_isClick )
            Q_EMIT muteToggled( !m_muted );
    }

    m_isClick = false;
}

void VolumeDial::paintEvent( QPaintEvent * )
{
    QPainter p( this );
    int icon = m_muted ? 0 : 3;
    if ( icon && value() < 66 )
        icon = value() < 33 ? 1 : 2;
    p.setRenderHint( QPainter::SmoothPixmapTransform );
    p.drawPixmap( 0,0, m_icon[ icon ].width()/2,  m_icon[ icon ].height()/2,  m_icon[ icon ] );
    if ( !m_isClick )
    {
        p.setPen( QPen( m_sliderGradient, 3, Qt::SolidLine, Qt::RoundCap ) );
        p.setRenderHint( QPainter::Antialiasing );
        p.drawArc( rect().adjusted(4,4,-4,-4), -110*16, - value()*320*16 / (maximum() - minimum()) );
    }
    p.end();
}

void VolumeDial::removeWheelProxy( QObject *w )
{
    m_wheelProxies.removeOne( static_cast<QWidget*>(w) );
}

void VolumeDial::resizeEvent( QResizeEvent *re )
{
    if( width() != height() )
        resize( height(), height() );
    else
        QDial::resizeEvent( re );

    if( re->size() != re->oldSize() )
    {
        renderIcons();
        m_sliderGradient = QPixmap( size() );
        updateSliderGradient();
        update();
    }
}

void VolumeDial::renderIcons()
{
    //double size svg render to have better looking high-dpi toolbar
    m_icon[0] = The::svgHandler()->renderSvg( QStringLiteral("Muted"),      width()*2, height()*2, QStringLiteral("Muted"),      true );
    m_icon[1] = The::svgHandler()->renderSvg( QStringLiteral("Volume_low"), width()*2, height()*2, QStringLiteral("Volume_low"), true );
    m_icon[2] = The::svgHandler()->renderSvg( QStringLiteral("Volume_mid"), width()*2, height()*2, QStringLiteral("Volume_mid"), true );
    m_icon[3] = The::svgHandler()->renderSvg( QStringLiteral("Volume"),     width()*2, height()*2, QStringLiteral("Volume"),     true );
    if( layoutDirection() == Qt::RightToLeft )
    {
        for ( int i = 0; i < 4; ++i )
            m_icon[i] = QPixmap::fromImage( m_icon[i].toImage().mirrored( true, false ) );
    }
}

void VolumeDial::startFade()
{
    if ( m_anim.timer )
        killTimer( m_anim.timer );
    m_anim.timer = startTimer( 40 );
}

void VolumeDial::stopFade()
{
    killTimer( m_anim.timer );
    m_anim.timer = 0;
    if ( m_anim.step < 0 )
        m_anim.step = 0;
    else if ( m_anim.step > 6 )
        m_anim.step = 6;
}

void VolumeDial::timerEvent( QTimerEvent *te )
{
    if ( te->timerId() != m_anim.timer )
        return;
    if ( underMouse() ) // fade in
    {
        m_anim.step += 2;
        if ( m_anim.step > 5 )
            stopFade();
    }
    else // fade out
    {
        --m_anim.step;
        if ( m_anim.step < 1 )
            stopFade();
    }
    updateSliderGradient();
    repaint();
}

void VolumeDial::updateSliderGradient()
{
    m_sliderGradient.fill( Qt::transparent );
    QColor c = m_highlightColor;
    if ( !m_anim.step )
    {
        c.setAlpha( 99 );
        m_sliderGradient.fill( c );
        return;
    }

    QConicalGradient cg( m_sliderGradient.rect().center(), -90 );

    c.setAlpha( 99 + m_anim.step*156/6 );
    cg.setColorAt( 0, c );
    c.setAlpha( 99 + m_anim.step*42/6 );
    cg.setColorAt( 1, c );

    QPainter p( &m_sliderGradient );
    p.fillRect( m_sliderGradient.rect(), cg );
    p.end();
}

void VolumeDial::wheelEvent( QWheelEvent *wev )
{
    QDial::wheelEvent( wev );
    wev->accept();

    const QPoint tooltipPosition = mapToGlobal( rect().translated( 7, -22 ).bottomLeft() );
    QToolTip::showText( tooltipPosition, toolTip() );

    // NOTICE: this is a bit tricky.
    // the ToolTip "QTipLabel" just installed a global eventfilter that intercepts various
    // events and hides itself on them. Therefore every even wheelevent will close the tip
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
    m_muted = mute;

    setToolTip( m_muted ? i18n( "Muted" ) : i18n( "Volume: %1%", value() ) );
    update();
}

QSize VolumeDial::sizeHint() const
{
    if ( QToolBar *toolBar = qobject_cast<QToolBar*>( parentWidget() ) )
        return toolBar->iconSize();

    return QDial::sizeHint();
}

void VolumeDial::sliderChange( SliderChange change )
{
    if ( change == SliderValueChange && isSliderDown() && signalsBlocked() )
    {
        int d = value() - m_formerValue;
        if ( d && d < 33 && d > -33 ) // don't allow real "jumps" > 1/3
        {
            if ( d > 5 ) // ease movement
                d = 5;
            else if ( d < -5 )
                d = -5;
            m_formerValue += d;
            blockSignals( false );
            Q_EMIT sliderMoved( m_formerValue );
            Q_EMIT valueChanged( m_formerValue );
            blockSignals( true );
        }
        if ( d )
            setValue( m_formerValue );
    }
    QDial::sliderChange(change);
}

void VolumeDial::valueChangedSlot( int v )
{
    m_isClick = false;

    setToolTip( m_muted ? i18n( "Muted" ) : i18n( "Volume: %1%", v ) );
    update();
}


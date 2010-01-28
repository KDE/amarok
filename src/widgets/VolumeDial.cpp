#include "VolumeDial.h"

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
    connect ( this, SIGNAL( valueChanged(int) ), SLOT( valueChangedSlot(int) ) );
    connect ( &toolTipTimer, SIGNAL( timeout() ), this, SLOT( hideToolTip() ) );
}

void VolumeDial::mousePressEvent( QMouseEvent *me )
{
    setCursor( Qt::PointingHandCursor );
    const int dx = width()/4;
    const int dy = height()/4;
    m_isClick = rect().adjusted(dx, dy, -dx, -dy).contains( me->pos() );
//     if ( !m_isClick )
    QDial::mousePressEvent( me );
}

void VolumeDial::mouseReleaseEvent( QMouseEvent *me )
{
    if ( me->button() != Qt::LeftButton )
        return;
    
    QToolTip::hideText();
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

    m_icon[0] = The::svgHandler()->renderSvg( "Muted", width(), height(), "Muted" );
    m_icon[1] =  The::svgHandler()->renderSvg( "Volume_low", width(), height(), "Volume_low" );
    m_icon[2] =  The::svgHandler()->renderSvg( "Volume_mid", width(), height(), "Volume_mid" );
    m_icon[3] =  The::svgHandler()->renderSvg( "Volume", width(), height(), "Volume" );
    
    update();
}

void VolumeDial::wheelEvent( QWheelEvent *wev )
{
    QDial::wheelEvent( wev );
    toolTipTimer.start( 1000 );
    showToolTip();
    wev->accept();
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
    toolTipTimer.stop();
    QToolTip::hideText();
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

#include "VolumeDial.h"

// #include <QPaintEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QToolBar>

#include "SvgHandler.h"

#include <QtDebug>

VolumeDial::VolumeDial( QWidget *parent ) : QDial( parent )
, m_muted( false )
{
    connect ( this, SIGNAL( valueChanged(int) ), SLOT( valueChangedSlot(int) ) );
}

void VolumeDial::mousePressEvent( QMouseEvent *me )
{
    const int dx = width()/4;
    const int dy = height()/4;
    m_isClick = rect().adjusted(dx, dy, -dx, -dy).contains( me->pos() );
    if (!m_isClick)
        QDial::mousePressEvent( me );
}

void VolumeDial::mouseReleaseEvent( QMouseEvent *me )
{
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

//     setMute( !m_muted );
    emit muteToggled( !m_muted );
}

void VolumeDial::paintEvent( QPaintEvent * )
{
    QPainter p( this );
    p.drawPixmap(0,0, m_icon[ m_muted ]);
    QColor c = palette().color( QPalette::Highlight );
    c.setAlpha( 196 );
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

    m_icon[0] =  The::svgHandler()->renderSvg( "Volume", width(), height(), "Volume" );
    m_icon[1] = The::svgHandler()->renderSvg( "Muted", width(), height(), "Muted" );
    update();
}

void VolumeDial::setMute( bool mute )
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

QSize VolumeDial::sizeHint() const
{
    if ( QToolBar *toolBar = qobject_cast<QToolBar*>( parentWidget() ) )
        return toolBar->iconSize();

    return QDial::sizeHint();
}

void VolumeDial::valueChangedSlot( int v )
{
    m_isClick = false;
    if ( m_muted == ( v == minimum() ) )
        return;
    m_muted = ( v == minimum() );
    update();
}

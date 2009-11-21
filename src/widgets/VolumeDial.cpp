#include "VolumeDial.h"

// #include <QPaintEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QToolBar>

#include <QtDebug>

VolumeDial::VolumeDial( QWidget *parent ) : QDial( parent )
, m_muted( false )
{
    m_icon = QImage("volume.png");
    m_mutedIcon = QImage("muted.png");
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

    emit muteToggled( !m_muted );
    
    if ( m_muted )
        setValue( m_unmutedValue );
    else
    {
        m_unmutedValue = value();
        setValue( minimum() );
    }
}

void VolumeDial::paintEvent( QPaintEvent * )
{
    QPainter p( this );
    p.drawPixmap(0,0, m_iconBuffer);
    QColor c = palette().color( QPalette::Highlight );
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
    if (size() != m_iconBuffer.size())
        updateIconBuffer();
}

QSize VolumeDial::sizeHint() const
{
    if ( QToolBar *toolBar = qobject_cast<QToolBar*>( parentWidget() ) )
        return toolBar->iconSize();

    return QDial::sizeHint();
}

void VolumeDial::updateIconBuffer()
{
    if ( m_muted )
        m_iconBuffer = QPixmap::fromImage( m_mutedIcon.scaled( size(), Qt::KeepAspectRatio, Qt::SmoothTransformation ) );
    else
        m_iconBuffer = QPixmap::fromImage( m_icon.scaled( size(), Qt::KeepAspectRatio, Qt::SmoothTransformation ) );
}

void VolumeDial::valueChangedSlot( int v )
{
    m_isClick = false;
    if ( m_muted == ( v == minimum() ) )
        return;
    m_muted = ( v == minimum() );
    updateIconBuffer();
    update();
}

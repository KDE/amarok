
#include "PlayPauseButton.h"

#include "SvgHandler.h"

#include <QMouseEvent>
#include <QPainter>
#include <QSizePolicy>
#include <QTimerEvent>
#include <QToolBar>

#include <QtDebug>

PlayPauseButton::PlayPauseButton( QWidget *parent ) : QWidget( parent )
, m_isPlaying( false )
, m_isClick( false )
, m_animStep( 0 )
, m_animTimer( 0 )
{
    QResizeEvent re( size(), QSize() );
    resizeEvent( &re );
}

void PlayPauseButton::enterEvent( QEvent * )
{
    startFade();
}

void PlayPauseButton::leaveEvent( QEvent * )
{
    startFade();
}

void PlayPauseButton::mousePressEvent( QMouseEvent *me )
{
    me->accept();
    m_isClick = true;
    update();
}

void PlayPauseButton::mouseReleaseEvent( QMouseEvent *me )
{
    me->accept();
    if ( m_isClick && underMouse() )
    {
        m_isClick = false;
        emit toggled( !m_isPlaying );
    }
}


void PlayPauseButton::paintEvent( QPaintEvent * )
{
    QPainter p(this);
    p.drawPixmap( 0,0, m_iconBuffer );
    p.end();
}

void PlayPauseButton::resizeEvent( QResizeEvent *re )
{
    if ( width() != height() )
        resize( height(), height() );
    else
        QWidget::resizeEvent( re );
    //NOTICE this is a bit cumbersome, as Qt renders faster to images than to pixmaps
    // However we need the Image and generate the pixmap ourself - maybe extend the SvgHandler API
    m_iconPlay[0] =  The::svgHandler()->renderSvg( "PLAYpause", width(), height(), "PLAYpause" ).toImage();
    m_iconPlay[1] =  The::svgHandler()->renderSvg( "PLAYpause_active", width(), height(), "PLAYpause_active" ).toImage();
    m_iconPause[0] =  The::svgHandler()->renderSvg( "playPAUSE", width(), height(), "playPAUSE" ).toImage();
    m_iconPause[1] =  The::svgHandler()->renderSvg( "playPAUSE_active", width(), height(), "playPAUSE_active" ).toImage();
    updateIconBuffer();
}

void PlayPauseButton::setPlaying( bool b )
{
    if ( m_isPlaying == b )
        return;
    m_isPlaying = b;
    updateIconBuffer();
    update();
}

QSize PlayPauseButton::sizeHint() const
{
    if ( QToolBar *toolBar = qobject_cast<QToolBar*>( parentWidget() ) )
        return toolBar->iconSize();

    return QSize( 32, 32 );
}

void PlayPauseButton::startFade()
{
    if ( m_animTimer )
        killTimer( m_animTimer );
    m_animTimer = startTimer( 40 );
}

void PlayPauseButton::stopFade()
{
    killTimer( m_animTimer );
    m_animTimer = 0;
    if ( m_animStep < 0 )
        m_animStep = 0;
    else if ( m_animStep > 6 )
        m_animStep = 6;
}

void PlayPauseButton::timerEvent( QTimerEvent *te )
{
    if ( te->timerId() != m_animTimer )
        return;
    if ( underMouse() ) // fade in
    {
        m_animStep += 2;
        if ( m_animStep > 5 )
            stopFade();
        updateIconBuffer();
    }
    else // fade out
    {
        --m_animStep;
        if ( m_animStep < 1 )
            stopFade();
        updateIconBuffer();
    }
    repaint();
}

static QImage
interpolated( const QImage &img1, const QImage &img2, int a1, int a2 )
{
    const int a = a1 + a2;
    if (!a)
        return img1.copy();
    
//     QImage *wider, *higher;
//     QImage img( qMax( img1.width(), img2.width() ), qMax( img1.height(), img2.height() ), QImage::Format_RGB32 );
    QImage img( img1.size(), img1.format() );

    const uchar *src[2] = { img1.bits(), img2.bits() };
    uchar *dst = img.bits();
    const int n = img.width()*img.height()*4;
    for ( int i = 0; i < n; ++i )
    {
        *dst = ((*src[0]*a1 + *src[1]*a2)/a) & 0xff;
        ++dst; ++src[0]; ++src[1];
    }
    return img;
}

void PlayPauseButton::updateIconBuffer()
{
    QImage img;
    QImage (*base)[2] = m_isPlaying ? &m_iconPause : &m_iconPlay;

    if (m_animStep < 1)
        img = (*base)[0];
    else if (m_animStep > 5)
        img = (*base)[1];
    else
        img = interpolated( (*base)[0], (*base)[1], 6 - m_animStep, m_animStep );

    m_iconBuffer = QPixmap::fromImage( img );
//     m_iconBuffer = QPixmap::fromImage( img.scaled( size(), Qt::KeepAspectRatio, Qt::SmoothTransformation ) );
}

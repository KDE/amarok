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

#include "IconButton.h"
#include "SvgHandler.h"

#include <QMouseEvent>
#include <QPainter>
#include <QSizePolicy>
#include <QTimerEvent>
#include <QToolBar>

IconButton::IconButton( QWidget *parent ) : QWidget( parent )
    , m_isClick( false )
{
    m_anim.step = 0;
    m_anim.timer = 0;
    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

    // cannot use paletteChanged() from the palette handler directly since the
    // svg handler also watches it for retinting. So upon palette change, we are
    // called first and the old svg icons will be used.
    connect( The::svgHandler(), &SvgHandler::retinted, this, &IconButton::svgRetinted );
}

void IconButton::setIcon( const QImage &img, int steps )
{
    m_anim.step = 0;
    m_anim.steps = steps;
    
    m_icon = img;
    m_oldIcon = steps ? m_buffer.image : QImage();

    if ( m_anim.timer )
        killTimer( m_anim.timer );
    if ( steps )
        m_anim.timer = startTimer( 40 );
    else
        updateIconBuffer();
    repaint();
}


void IconButton::mousePressEvent( QMouseEvent *me )
{
    me->accept();
    m_isClick = true;
}

void IconButton::mouseReleaseEvent( QMouseEvent *me )
{
    me->accept();
    if ( m_isClick && rect().contains( me->pos() ) )
    {
        m_isClick = false;
        Q_EMIT clicked();
    }
}

void IconButton::paintEvent( QPaintEvent * )
{
    QPainter p(this);
    p.setRenderHint( QPainter::SmoothPixmapTransform );
    p.drawPixmap( 0, 0, width(), height(), m_buffer.pixmap );
    p.end();
}

void IconButton::svgRetinted()
{
    reloadContent( size() );
}

void IconButton::resizeEvent( QResizeEvent *re )
{
    if( width() != height() )
        resize( height(), height() );
    else
    {
        reloadContent( re->size() );
        QWidget::resizeEvent( re );
    }
}

QSize IconButton::sizeHint() const
{
    if ( QToolBar *toolBar = qobject_cast<QToolBar*>( parentWidget() ) )
        return toolBar->iconSize();

    return QSize( 32, 32 );
}

void IconButton::timerEvent( QTimerEvent *te )
{
    if ( te->timerId() != m_anim.timer )
        return;

    ++m_anim.step;
    updateIconBuffer();
    if ( m_anim.step >= m_anim.steps )
    {
        killTimer( m_anim.timer );
        m_anim.timer = 0;
        m_oldIcon = QImage();
    }
    repaint();
}

static QImage adjusted( QImage img, const QSize &sz )
{
    QSize doublesz( sz.width()*2, sz.height()*2 );
    if ( img.size() == doublesz ) //double size render to have better looking high-dpi toolbar
        return img;
    QImage ret( doublesz, QImage::Format_ARGB32_Premultiplied );
    ret.fill( Qt::transparent );
    QPainter p( &ret );
    int xcentershift=(ret.width() - img.width()) /2;
    int ycentershift=(ret.height() - img.height()) /2;
    if(xcentershift<0)
        xcentershift=0;
    if(ycentershift<0)
        ycentershift=0;
    p.drawImage( xcentershift, ycentershift, img );
    p.end();
    return ret;
}

static inline QImage interpolated( const QImage &img1, const QImage &img2, int a1, int a2 )
{
    const int a = a1 + a2;
    if (!a)
        return img1.copy();

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

void IconButton::updateIconBuffer()
{
    if ( m_anim.step >= m_anim.steps )
        m_buffer.image = adjusted( m_icon, size() );
    else
        m_buffer.image = interpolated( adjusted( m_oldIcon, size() ), adjusted( m_icon, size() ),
                                       m_anim.steps - m_anim.step, m_anim.step );

    m_buffer.pixmap = QPixmap::fromImage( m_buffer.image );
}



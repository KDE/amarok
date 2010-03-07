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

#include "TrackActionButton.h"

#include <QAction>
#include <QEvent>
#include <QTimer>

TrackActionButton::TrackActionButton( QWidget *parent, const QAction *act ) : IconButton( parent )
{
    if ( act )
        setAction( act );
    if ( parent )
        parent->installEventFilter( this );
    // this is during the labelslide - so we wait a short time with image processing ;-)
    QTimer::singleShot( 1200, this, SLOT( init() ) );
}

bool TrackActionButton::eventFilter( QObject *o, QEvent *e )
{
    if ( o == parentWidget() )
    {
        if ( e->type() == QEvent::Enter )
            setIcon( m_icon.image[1], 3 );
        else if ( e->type() == QEvent::Leave )
            setIcon( m_icon.image[0], 6 );
    }
    return false;
}

void TrackActionButton::enterEvent( QEvent *e )
{
    setIcon( m_icon.image[2], 3 );
    IconButton::enterEvent( e );
}

void TrackActionButton::init()
{
    reloadContent( size() );
}

void TrackActionButton::leaveEvent( QEvent *e )
{
    setIcon( m_icon.image[1], 6 );
    IconButton::leaveEvent( e );
}

void TrackActionButton::reloadContent( const QSize &sz )
{
    if ( sz.isNull() )
        return;
    int r,g,b;
    palette().color(foregroundRole()).getRgb(&r,&g,&b);
    
    m_icon.image[2] = m_icon.icon.pixmap( sz ).toImage();
    QImage img = m_icon.image[2].convertToFormat(QImage::Format_ARGB32);
    int n = img.width() * img.height();
    
    const uchar *bits = img.bits();
    QRgb *pixel = (QRgb*)(const_cast<uchar*>(bits));

    // this creates a (slightly) translucent monochromactic version of the
    // image using the foreground color
    // the gray value is turned into the opacity
#define ALPHA qAlpha(pixel[i])
#define GRAY qGray(pixel[i])
    if ( qMax( qMax(r,g), b ) > 128 ) // value > 50%, bright foreground
        for (int i = 0; i < n; ++i)
            pixel[i] = qRgba( r,g,b, ( ALPHA * ( (160*GRAY) / 255 ) ) / 255 );
    else // inverse
        for (int i = 0; i < n; ++i)
            pixel[i] = qRgba( r,g,b, ( ALPHA * ( (160*(255-GRAY)) / 255 ) ) / 255 );

    // premultiplied is much faster on painting / alphablending
    m_icon.image[1] = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);

    // and a very translucent variant
    for (int i = 0; i < n; ++i)
        pixel[i] = qRgba(r,g,b, ALPHA/6);

#undef ALPHA
#undef GRAY

    m_icon.image[0] = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);

    int i = 0;
    if ( underMouse() )
        i = 2;
    else if ( !parentWidget() || parentWidget()->underMouse() )
        i = 1;

    setIcon( m_icon.image[i] );
}

void TrackActionButton::setAction( const QAction *act )
{
    disconnect( SIGNAL( clicked() ) );
    m_action = act;
    if ( act )
    {
        m_icon.icon = act->icon();
        setToolTip( act->toolTip() );
        connect ( this, SIGNAL( clicked() ), act, SLOT( trigger() ) );
        connect ( act, SIGNAL( changed() ), SLOT( updateAction() ) );
    }
    else
    {
        m_icon.icon = QIcon();
        setToolTip( QString() );
    }
}

QSize TrackActionButton::sizeHint() const
{
    return QSize( 16, 16 );
}

void TrackActionButton::updateAction()
{
    if ( QAction *act = qobject_cast<QAction*>(sender()) )
    {
        if ( act == m_action )
        {
            m_icon.icon = act->icon();
            setToolTip( act->toolTip() );
        }
        else // old action, stop listening
            disconnect ( act, SIGNAL( changed() ), this, SLOT( updateAction() ) );
    }
}

#include "TrackActionButton.moc"

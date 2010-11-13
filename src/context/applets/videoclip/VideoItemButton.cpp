/****************************************************************************************
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
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

#define DEBUG_PREFIX "VideoItemButton"

#include "VideoItemButton.h"

#include "SvgHandler.h"
#include "core/support/Debug.h"

#include <KAction>
#include <KIcon>
#include <KMenu>

#include <QPixmapCache>

VideoItemButton::VideoItemButton()
    : QToolButton()
    , m_videoInfo ( 0 )
{}

VideoItemButton::~VideoItemButton()
{
}

void VideoItemButton::setVideoInfo( VideoInfo *info )
{
    // save the video Info
    m_videoInfo = info;
    
    // Create a pixmap with nice border    
    QPixmap pix;
    QString key = QString( "%1_%2" ).arg( info->url, QString::number(100) );
    if( !QPixmapCache::find( key, &pix ) )
    {
        pix = info->cover.scaledToHeight( 100, Qt::SmoothTransformation );
        pix = The::svgHandler()->addBordersToPixmap( pix, 5, QString(), true );
        QPixmapCache::insert( key, pix );
    }

    // then add info
    setToolButtonStyle( Qt::ToolButtonIconOnly );
    setAutoRaise( true );
    setIcon( QIcon( pix ) );
    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    setIconSize( pix.size() ) ;
    setToolTip( info->title );
    setContextMenuPolicy( Qt::CustomContextMenu );
    connect(this, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( myMenu( QPoint ) ) );
}

VideoInfo * VideoItemButton::getVideoInfo()
{
    return m_videoInfo;
}

void VideoItemButton::mousePressEvent(QMouseEvent* event )
{
    if( event->button() == Qt::LeftButton )
        appendPlay();

    if ( event->button() == Qt::MidButton )
        queue();
}

void VideoItemButton::mouseDoubleClickEvent(QMouseEvent* )
{
    DEBUG_BLOCK
    appendPlay();
}

void VideoItemButton::enterEvent( QEvent* )
{
    setCursor( Qt::PointingHandCursor );
}

void VideoItemButton::leaveEvent( QEvent* )
{
    setCursor( Qt::ArrowCursor );
}

void VideoItemButton::myMenu(QPoint point)
{
    DEBUG_BLOCK
    KAction *appendAction = new KAction( KIcon( "media-track-add-amarok" ), i18n( "&Add to playlist" ), this );
    KAction *queueAction  = new KAction( KIcon( "media-track-queue-amarok" ), i18n( "&Queue" ), this );
    KAction *appendPlayAction   = new KAction( KIcon( "music-amarok" ), i18n( "Append and &Play" ), this );
    
    KMenu * men = new KMenu(this);

    men->addAction( appendAction );
    men->addAction( queueAction );
    men->addAction( appendPlayAction );

    connect( appendAction, SIGNAL( triggered(bool) ), this, SLOT( append() ) );
    connect( queueAction, SIGNAL( triggered(bool) ), this, SLOT( queue() ) );
    connect( appendPlayAction, SIGNAL( triggered(bool) ), this, SLOT( appendPlay() ) );
    
    men->exec( mapToGlobal( point ) );
    
}

void VideoItemButton::append()
{
    emit appendRequested( m_videoInfo );
}

void VideoItemButton::queue()
{
    emit queueRequested( m_videoInfo );
}

void VideoItemButton::appendPlay()
{
    emit appendPlayRequested( m_videoInfo );
}


#include "VideoItemButton.moc"

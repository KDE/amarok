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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

//Plamsa applet for showing videoclip in the context view

#include "VideoItemButton.h"

#include "SvgHandler.h"
#include "Debug.h"

#include <QPixmap>

#include <KAction>
#include <KIcon>
#include <KMenu>

#define DEBUG_PREFIX "VideoItemButton"

VideoItemButton::VideoItemButton( QWidget *parent )
    : QToolButton( parent )
    , m_videoInfo ( 0 )
{
    
}

void VideoItemButton::setVideoInfo( VideoInfo *info )
{
    // save the video Info
    m_videoInfo = info;
    
    // Create a pixmap with nice border    
    QPixmap pix( The::svgHandler()->addBordersToPixmap( *info->cover, 3, "Thumbnail", true ).scaledToHeight( 85 ) ) ;

    // then add info
    setText( "" );
    setToolButtonStyle( Qt::ToolButtonIconOnly );
    setAutoRaise( true );
    setIcon( QIcon( pix ) );
    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    resize( pix.size() );
    setIconSize( pix.size() ) ;
    setToolTip( QString( "<html><body>" ) + info->desc + QString( "</body></html>" ) );
    setContextMenuPolicy( Qt::CustomContextMenu );
    connect(this, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( myMenu( QPoint ) ) );
}

VideoInfo * VideoItemButton::getVideoInfo()
{
    return m_videoInfo;
}

void VideoItemButton::mousePressEvent(QMouseEvent* event )
{
    // TODO what should we do on simple click ?
    if ( event->button() == Qt::MidButton )
        queue();
}

void VideoItemButton::mouseDoubleClickEvent(QMouseEvent* )
{
    DEBUG_BLOCK
    appendPlay();
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

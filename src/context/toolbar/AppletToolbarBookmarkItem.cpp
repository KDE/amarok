/****************************************************************************************
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include "AppletToolbarBookmarkItem.h"

#include "App.h"
#include "Debug.h"
#include "PaletteHandler.h"

#include <plasma/widgets/iconwidget.h>

#include <KIcon>

#include <QAction>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>
#include <QSizePolicy>
#include <QStyleOptionGraphicsItem>

Context::AppletToolbarBookmarkItem::AppletToolbarBookmarkItem( QGraphicsItem* parent )
    : AppletToolbarBase( parent )
    , m_icon( 0 )
    , m_iconPadding( 2 )
{

    m_icon = new Plasma::IconWidget( this );

    m_icon->setIcon( KIcon( "bookmark-new-list" ) );
    m_icon->setText( QString() );
    m_icon->setToolTip( i18n( "Context Bookmarks" ) );
    m_icon->setDrawBackground( false );
    m_icon->setOrientation( Qt::Horizontal );
    QSizeF iconSize = m_icon->sizeFromIconSize( 22 );
    m_icon->setMinimumSize( iconSize );
    m_icon->setMaximumSize( iconSize );
    m_icon->resize( iconSize );
    m_icon->setZValue( zValue() + 1 );


    connect( m_icon, SIGNAL( pressed( bool ) ), this, SLOT( showMenu() ) );

    //HACK! keep this around to generate the menu.
    //TODO: factor out menu generation code to AmarokUrlHandler or something
    m_bookmarkButton = new BreadcrumbUrlMenuButton( "context", 0 );
    m_bookmarkButton->setAutoFillBackground( false );

    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Preferred );
    
}

Context::AppletToolbarBookmarkItem::~AppletToolbarBookmarkItem()
{
    delete m_bookmarkButton;
}
   
void 
Context::AppletToolbarBookmarkItem::resizeEvent( QGraphicsSceneResizeEvent * event )
{
    Q_UNUSED( event )
    // center horizontally and vertically
}

QSizeF 
Context::AppletToolbarBookmarkItem::sizeHint( Qt::SizeHint which, const QSizeF & constraint ) const
{
    return QSizeF( m_icon->size().width() + 2 * m_iconPadding , QGraphicsWidget::sizeHint( which, constraint ).height() );
}

void Context::AppletToolbarBookmarkItem::showMenu()
{
  
    DEBUG_BLOCK
    QGraphicsView * gView = scene()->views().at( 0 );
    if( gView )
        m_bookmarkButton->generateMenu( gView->mapToGlobal( pos().toPoint() ) + QPoint( 0, gView->contentsRect().height() + contentsRect().height() ) );
    
}

#include "AppletToolbarBookmarkItem.moc"

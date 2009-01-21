/***************************************************************************
* copyright            : (C) 2009 Leo Franchi <lfranchi@kde.org>           *
****************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "BookmarkManagerWidgetProxy.h"

#include "BookmarkManagerWidget.h"
#include "Debug.h"

#include <KMenu>

#include <QGraphicsSceneContextMenuEvent>

BookmarkManagerWidgetProxy::BookmarkManagerWidgetProxy( QGraphicsWidget *parent )
    : QGraphicsProxyWidget( parent )
    , m_manager( 0 )
{
    m_manager = new BookmarkManagerWidget( );
    setWidget( m_manager );
    
    connect( m_manager, SIGNAL( showMenu( KMenu*, const QPointF& ) ), this, SLOT( showMenu( KMenu*, const QPointF& ) ) );
}

void 
BookmarkManagerWidgetProxy::showMenu(KMenu* menu, const QPointF& point) // slot
{
    DEBUG_BLOCK
    menu->exec( point.toPoint() );
}

#include "BookmarkManagerWidgetProxy.h"

/**************************************************************************
* copyright            : (C) 2008 Leo Franchi <lfranchi@kde.org>          *
**************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "ToolbarView.h"

#include "Debug.h"

#include <QGraphicsScene>
#include <QSizePolicy>
#include <QWidget>

#define TOOLBAR_X_OFFSET 10000

Context::ToolbarView::ToolbarView( QGraphicsScene* scene, QWidget* parent )
    : QGraphicsView( scene, parent )
    , m_height( 30 )
{
    setSceneRect( TOOLBAR_X_OFFSET, 0, size().width(), m_height );
    QSizePolicy policy( QSizePolicy::Preferred, QSizePolicy::Fixed );
    policy.setHeightForWidth( true );
    setSizePolicy( policy );
    setAutoFillBackground( true );
    
}

Context::ToolbarView::~ToolbarView()
{
    
}

QSize
Context::ToolbarView::sizeHint() const
{
    return QSize( size().width(), m_height );
}

int 
Context::ToolbarView::heightForWidth ( int w ) const
{
    return m_height;
} 


void  
Context::ToolbarView::resizeEvent( QResizeEvent * event )
{
    setSceneRect( TOOLBAR_X_OFFSET, 0, size().width(), m_height );
}

#include "ToolbarView.moc"

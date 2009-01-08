/***************************************************************************
* copyright            : (C) 2008 Leo Franchi <lfranchi@kde.org>         *
****************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "AppletToolbar.h"

#include "AppletToolbarAddItem.h"
#include "AppletToolbarAppletItem.h"
#include "AppletToolbarConfigItem.h"
#include "Containment.h"
#include "Debug.h"
#include "plasma/applet.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneResizeEvent>
#include <QGraphicsLinearLayout>
#include <QSizePolicy>

Context::AppletToolbar::AppletToolbar( QGraphicsItem* parent )
    : QGraphicsWidget( parent )
    , m_appletLayout( 0 )
    , m_cont( 0 )
    , m_addItem( 0 )
    , m_configItem( 0 )
{    
    Context::Containment* cont = dynamic_cast<Context::Containment*>( parent );
    if( cont )
    {    
        m_cont = cont;
        debug() << "applettoolbar created with a real containment";
    }
        
    m_appletLayout = new QGraphicsLinearLayout( Qt::Horizontal, this );
    
    m_addItem = new AppletToolbarAddItem( this, m_cont, false );
    connect( cont, SIGNAL( updatedContainment( Containment* ) ), m_addItem, SLOT( updatedContainment( Containment* ) ) );
    
    m_configItem = new AppletToolbarConfigItem( this );
    connect( m_configItem, SIGNAL( triggered() ), this, SLOT( toggleConfigMode() ) );
    
    m_appletLayout->addItem( m_addItem );
    m_appletLayout->setAlignment( m_addIt2em, Qt::AlignRight );
    m_appletLayout->addItem( m_configItem );
    m_appletLayout->setAlignment( m_configItem, Qt::AlignRight );
}

Context::AppletToolbar::~AppletToolbar()
{
    
}

void 
Context::AppletToolbar::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget )
{
    //DEBUG_BLOCK
    
   // debug() << "drawing rect:" << boundingRect();
    
    // draw translucent curved background
    painter->save();
    QColor fillColor( 176, 176, 176, 225 );
    QPainterPath path;
    path.addRoundedRect( boundingRect(), 5, 5 );
    painter->fillPath( path ,fillColor );
    painter->restore();
    
    
}

void
Context::AppletToolbar::resizeEvent( QGraphicsSceneResizeEvent * event )
{
    debug() << "setting layout to" << QRectF( QPointF( 0, 0 ), event->newSize() );
    m_appletLayout->setGeometry( QRectF( QPointF( 0, 0 ), event->newSize() ) );
}

QSizePolicy 
Context::AppletToolbar::sizePolicy () const
{
    return QSizePolicy( QSizePolicy::Expanding,  QSizePolicy::Fixed );
}


void 
Context::AppletToolbar::appletRemoved( Plasma::Applet* applet )
{
    DEBUG_BLOCK
    for( int i = 0; i < m_appletLayout->count(); i++ )
    {
        AppletToolbarAppletItem* app = dynamic_cast< AppletToolbarAppletItem* >( m_appletLayout->itemAt( i ) );
        if( app && app->applet() == applet )
        {
            m_appletLayout->removeItem( app );
            app->deleteLater();
        }
    }
}

QSizeF
Context::AppletToolbar::sizeHint( Qt::SizeHint which, const QSizeF & constraint ) const
{
    return QSizeF( constraint.width(), 40 );
}


void
Context::AppletToolbar::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
    DEBUG_BLOCK
    
    QGraphicsWidget::mousePressEvent( event );
}


void 
Context::AppletToolbar::appletAdded( Plasma::Applet* applet, int loc ) // SLOT
{
    DEBUG_BLOCK
    
    debug() << "inserting applet icon in position" << loc;
    Context::AppletToolbarAppletItem* item = new Context::AppletToolbarAppletItem( this, applet );
    connect( item, SIGNAL( appletChosen( Plasma::Applet* ) ), this, SIGNAL( showApplet( Plasma::Applet* ) ) );
    m_appletLayout->insertItem( loc, item );
    m_addItem->setMaximized( false );
    m_addItem->hideMenu();
}
    
void 
Context::AppletToolbar::appletRemoved( Plasma::Applet* applet, int loc) // SLOT
{
    DEBUG_BLOCK
}
  
#include "AppletToolbar.moc"

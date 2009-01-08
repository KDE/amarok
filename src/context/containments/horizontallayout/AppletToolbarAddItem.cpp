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

#include "AppletToolbarAddItem.h"

#include "widgets/ToolboxMenu.h"
#include "Debug.h"

#include <KIcon>

#include <QAction>
#include <QGraphicsSceneResizeEvent>
#include <QGraphicsSimpleTextItem>
#include <QSizeF>
#include <QStyleOptionGraphicsItem>
#include <QSizeF>
#include <QPainter>

Context::AppletToolbarAddItem::AppletToolbarAddItem( QGraphicsItem* parent )
    : QGraphicsWidget( parent )
    , m_icon( 0 )
    , m_label( 0 )
    , m_addMenu( 0 )
{
    QAction* listAdd = new QAction( i18n( "Add Widgets..." ), this );
    listAdd->setIcon( KIcon( "list-add" ) );
    listAdd->setVisible( true );
    listAdd->setEnabled( true );
    
    connect( listAdd, SIGNAL( triggered() ), this, SLOT( SOMETHING ) );
    
    m_icon = new Plasma::IconWidget( this );

    m_icon->setAction( listAdd );
    m_icon->setText( QString() );
    m_icon->setToolTip( listAdd->text() );
    m_icon->setDrawBackground( false );
    m_icon->setOrientation( Qt::Horizontal );
    QSizeF iconSize = m_icon->sizeFromIconSize( 7 );
    m_icon->setMinimumSize( iconSize );
    m_icon->setMaximumSize( iconSize );
    m_icon->resize( iconSize );
    m_icon->setZValue( zValue() + 1 );
    
    m_label = new QGraphicsSimpleTextItem( "Add Applet", this );
}

Context::AppletToolbarAddItem::~AppletToolbarAddItem()
{
    
}

void 
Context::AppletToolbarAddItem::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget )
{
  //  DEBUG_BLOCK
    
    painter->save();
    QColor fillColor( 88, 88, 88, 225 );
    QPainterPath fillPath;
    fillPath.addRoundedRect( boundingRect(), 5, 5 );
    painter->fillPath( fillPath ,fillColor );
    painter->restore();
}


QSizePolicy 
Context::AppletToolbarAddItem::sizePolicy () const
{
    return QSizePolicy( QSizePolicy::Expanding,  QSizePolicy::Expanding );
}

void 
Context::AppletToolbarAddItem::resizeEvent( QGraphicsSceneResizeEvent * event )
{
    m_icon->setPos( 0, ( boundingRect().height() / 2 ) - ( m_icon->size().height() / 2 ) );
    m_label->setPos( ( boundingRect().width() / 2 ) - ( m_label->boundingRect().width() / 2 ),  ( boundingRect().height() / 2 ) - ( m_label->boundingRect().height() / 2 ) );
}


QSizeF
Context::AppletToolbarAddItem::sizeHint( Qt::SizeHint which, const QSizeF & constraint ) const
{
    // return QSizeF( m_icon->size().width(), QGraphicsWidget::sizeHint( which, constraint ).height() );
    return QGraphicsWidget::sizeHint( which, constraint );
}

void 
Context::AppletToolbarAddItem::mouseReleaseEvent( QGraphicsSceneMouseEvent * event )
{
    DEBUG_BLOCK
    
    showAddAppletsMenu( event->pos() );
    event->accept();
}

void
Context::AppletToolbarAddItem::showAddAppletsMenu( QPointF pos )
{
    if( m_addMenu->showing() )
    {   // hide again on double-click
        m_addMenu->hide();
        return;
    }
    
    const qreal xpos = pos.x();
    const qreal ypos = contentsRect().height() - m_addMenu->boundingRect().height() + 40;

    m_addMenu->setPos( xpos, ypos );
    m_addMenu->show();
}

#include "AppletToolbarAddItem.moc"
